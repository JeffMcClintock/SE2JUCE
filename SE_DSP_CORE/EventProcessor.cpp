#include "pch.h"
#define _USE_MATH_DEFINES // for C++  
#include <cmath>  
#include "EventProcessor.h"
#include "SeAudioMaster.h"
#include "ug_base.h"
#include "ug_event.h"
#include <assert.h>
#include "UgDebugInfo.h"

#if defined( LOG_ALL_MODULES_CPU )
#include <fstream>
#include <iomanip>

std::ofstream EventProcessor::logFileCsv;
#endif


#ifdef _DEBUG
void dumpList( EventList& events, SynthEditEvent* current )
{
	int i = 0;

	for( EventIterator it(events.begin()) ; it != events.end() ; ++it )
	{
		SynthEditEvent* e = *it;

		if( e == current )
		{
			_RPTW0(_CRT_WARN, L"*" );
		}
		else
		{
			_RPTW0(_CRT_WARN, L" " );
		}

		_RPTW3(_CRT_WARN, L"%2d:%x, ts=%6d\n", i, e, e->timeStamp );
		i++;
	}

	_RPTW0(_CRT_WARN, L"  END\n" );
}
#endif

EventProcessor::EventProcessor() :
	m_audio_master(0)
	,private_sample_clock(0)
	,flags(static_cast<UgFlags>( 0 ))
	,sleeping(false)

	// editor only
	,cpuRunningAverage(0.0f)
	,cpuRunningMedian(0.0f)
	,cpuMeasuedCycles(0.0f)
{
}

EventProcessor::~EventProcessor()
{
#if !defined( SE_FIXED_POOL_MEMORY_ALLOCATION )
    // With fixed-pool managed by Audiomaster, it's too late to call into Audiomaster to deallocate,  in it's destructor already.
    // Shouldn't matter as there's no individual memory to free anyhow.
	DeleteEvents();
#endif
}

float EventProcessor::getSampleRate()
{
	return m_audio_master->SampleRate();
}

// sub position within current block
int EventProcessor::getBlockPosition()
{
	return (int) (private_sample_clock - AudioMaster()->BlockStartClock());
}

void EventProcessor::Wake()
{
	// bring sampleclock back up to speed
	// note don't check sleeping flag b4 this, it may only be PENDING sleep
	AudioMaster()->Wake(this);
	sleeping = false; // done after WantProcess() so it can check flag
}

void EventProcessor::AddEvent( SynthEditEvent* p_event )
{
	// added sleeping test to prevent resumeing plugin on every single event
	if( sleeping && /*p_wake && */(flags & UGF_SUSPENDED) == 0 )
	{
		Wake();
		assert( !sleeping );
	}

	// Ensure modules downstream from sender. Unhandled case: [MIDI-CV]->[feedback Delay]->[ADSR2]->[Voice Combiner]->[MIDI-CV] (<-same MIDI-CV).
	assert( sleeping || (timestamp_t) p_event->timeStamp >= SampleClock() );

	// note: private_sample_clock used directly, because may be asleep (SampleClock() would assert)
	timestamp_t timeStamp = p_event->timeStamp;
	// new, search from tail, stat change event's go in pin order
	// insert sorted into list
	EventIterator it( events.rbegin() );

	while( it != events.rend() )
	{
		SynthEditEvent* event2 = *it;

		if( (timestamp_t)event2->timeStamp <= timeStamp ) // events with same time must be added in same order received (for stat changes to work properly)
		{
			// most events can be inserted now, sorted only on timestamp.
			if( p_event->eventType < UET_EVENT_SETPIN || p_event->eventType > UET_EVENT_STREAMING_STOP )
			{
				events.insertAfter( it, p_event );
				return;
			}

			// stat change (and streaming change) requires additional sort on pin number
			int pin_idx = p_event->parm1;
			goto short_cut; // jump into loop

			while( it != events.rend() ) //pos != NULL )
			{
				event2 = *it;
short_cut:
				// If previous event old, insert here. If prev event = Graph-Start, insert after it. Graph-start MUST be first, else oversampler-in overrides initial input plug values.
				if( (timestamp_t)event2->timeStamp < timeStamp || event2->eventType == UET_GRAPH_START)
				{
					events.insertAfter( it, p_event );
					return;
				}

				if( event2->parm1 <= pin_idx && event2->eventType >= UET_EVENT_SETPIN && event2->eventType <= UET_EVENT_STREAMING_STOP )
				{
					events.insertAfter( it, p_event );

					// remove any previous identical stat change
					if( event2->parm1 == pin_idx )
					{
						events.erase( it );
						// delete event2;
						delete_SynthEditEvent(event2);
					}

					return;
				}

				--it;
			}

			goto done;
		}

		--it;
	}

done:
	// list empty
	events.push_front( p_event );
}

void EventProcessor::DeleteEvents()
{
	while( ! events.empty() )
	{
		SynthEditEvent* e = events.front();
		events.pop_front();
		delete_SynthEditEvent(e);
	}
}

// needed when objects has been sleeping
void EventProcessor::SetClockRescheduleEvents(timestamp_t newClock)
{
	// check for old events, set due time to now
	for( EventIterator it( events.begin() ) ; it != events.end() ; ++it )
	{
		SynthEditEvent* next_event = *it;

		if( (timestamp_t)next_event->timeStamp < newClock )
		{
			next_event->timeStamp = newClock;
		}
		else
		{
			// done
			break;
		}
	}

	SetSampleClock( newClock );
}

#if defined( _DEBUG )

void EventProcessor::SetSampleClock(timestamp_t p_sample_clock)
{
	assert( p_sample_clock >= AudioMaster()->SampleClock() && p_sample_clock < AudioMaster()->SampleClock() + 2000 );

	if( !events.empty() )
	{
		assert( events.front()->timeStamp >= p_sample_clock );
	}

	private_sample_clock = p_sample_clock;
}

timestamp_t EventProcessor::SampleClock() const
{
	assert( !sleeping ); // can't get sampleclock if asleep( not up to date )
	/* no, sampleclock used to output initial plug stat change b4 unit opened (fixable?, send stat change on open?)
	#ifdef _DEBUG
		assert( ( ub->flags & UGF_OPEN ) != 0 ); // can't get sampleclock unless open( not up to date )
	#endif
	*/
	return private_sample_clock;
}
#endif

// discard expired stat changes on pin
void EventProcessor::DiscardOldPinEvents( int pin_index )
{
	const timestamp_t bsc = AudioMaster()->BlockStartClock();
	// find most recent event for this pin, at or before block start. leave it in place.
	EventIterator it( events.rbegin() );

	while( it != events.rend() )
	{
		SynthEditEvent* e2 = *it;
		--it;

		if( e2->parm1 == pin_index && e2->eventType >= UET_EVENT_SETPIN && e2->eventType <= UET_EVENT_STREAMING_STOP && (timestamp_t)e2->timeStamp <= bsc )
		{
			break;
		}
	}

	// delete any earlier events on same pin.
	for( ; it != events.rend() ; --it )
	{
		SynthEditEvent* e2 = *it;

		if( e2->parm1 == pin_index && e2->eventType >= UET_EVENT_SETPIN && e2->eventType <= UET_EVENT_STREAMING_STOP  )
		{
			it = events.erase( it );
			delete_SynthEditEvent(e2);
		}
	}
}

// only ever used for discarding old program changes.
void EventProcessor::DiscardOldEvents( int eventType )
{
	timestamp_t bsc = AudioMaster()->BlockStartClock();
	// skip over events in current block.
	EventIterator it( events.rbegin() );

	for( ; it != events.rend() ; --it )
	{
		if( (*it)->timeStamp <= bsc )
		{
			break;
		}
	}

	// skip over first event before or equal to block start (so module gets initial value correct)
	for( ; it != events.rend() ; --it )
	{
		SynthEditEvent* e2 = *it;

		if( e2->eventType == eventType && (timestamp_t)e2->timeStamp <= bsc )
		{
			break;
		}
	}

	// erase any earlier events.
	for( ; it != events.rend() ; --it )
	{
		SynthEditEvent* e2 = *it;

		if( e2->eventType == eventType )
		{
			it = events.erase( it );
			delete_SynthEditEvent(e2);
		}
	}
}

void EventProcessor::RunDelayed(timestamp_t p_timestamp, ug_func p_function)
{
	ug_func temp[2] = { p_function, nullptr }; // prevent reading off end on 32-bit systems.
	int* i = (int*) temp;
	AddEvent( new_SynthEditEvent( p_timestamp, UET_RUN_FUNCTION2 , 0, i[0], i[1],0 ));
}

#if defined( LOG_ALL_MODULES_CPU )

void EventProcessor::UpdateCpu(int64_t nanosecondsElapsed)
{
	// New way.
	float fcpu = (float)nanosecondsElapsed;

	cpuRunningAverage += (fcpu - cpuRunningAverage) * 0.1f; // rough running average.
	cpuRunningMedian += copysignf(cpuRunningAverage * 0.005f, fcpu - cpuRunningMedian);

	cpuMeasuedCycles += cpuRunningMedian;
	cpuPeakCycles = (std::max)(cpuPeakCycles, fcpu);

	if (fcpu > 100 && private_sample_clock > 300 )
	{
		logFileCsv << private_sample_clock << "," << fcpu << "," << Handle() << ",";
//		if (fcpu > 400)
		{
			std::string debugName = WStringToUtf8(dynamic_cast<ug_base*>(this)->DebugModuleName());
			logFileCsv << debugName;
		}
		logFileCsv << "\n";
	}
}

#endif // LOG_ALL_MODULES_CPU

#if defined( _DEBUG )

void EventProcessor::DebugTimestamp()
{
#ifdef _WIN32
	timestamp_t seconds = SampleClock() / (timestamp_t)getSampleRate();
	timestamp_t samples = SampleClock() % (timestamp_t)getSampleRate();
	double millisecs = 1000 * (double) samples / getSampleRate();
	_RPT3(_CRT_WARN, "%3d.%03d sec (%05d samps) ",seconds, (int) millisecs, SampleClock() );
#endif
}

#endif

#ifdef _DEBUG
void EventProcessor::DebugOnSetFlag(int flag, bool /*setit*/)
{
	if(SeAudioMaster::GetDebugFlag(DBF_TRACE_POLY))
	{
		switch(flag)
		{
		case UGF_POLYPHONIC_DOWNSTREAM:
		{
			DebugPrintName(true);
			DebugPrintContainer();
			_RPT0(_CRT_WARN, "-SetDownstream\n" );
		}
		break;

		case UGF_PP_TERMINATED_PATH:
		{
			DebugPrintName(true);
			DebugPrintContainer();
			_RPT0(_CRT_WARN, "-SetTerminatedPath\n" );
		}
		break;

		case UGF_POLYPHONIC:
		{
			DebugPrintName(true);
			DebugPrintContainer();
			_RPT0(_CRT_WARN, "-PPSetUpstream [Polyphonic=true]\n" );
		}
		break;
		
		};
	}
}
#endif

#if defined(LOG_PIN_EVENTS )
void EventProcessor::LogEvents(std::ofstream& eventLogFile)
{
	auto current_sample_clock = SampleClock();
	if (current_sample_clock > 44000) // skip startup
	{
		auto module = dynamic_cast<ug_base*>(this);

		bool printedModule = false;
		for (EventIterator it(events.begin()); it != events.end(); ++it)
		{
			SynthEditEvent* e2 = *it;
			//				if (e2->timeStamp < end_time) // events with same time must be added in same order received (for stat changes to work properly)
			{
				if (!printedModule)
				{
					switch (e2->eventType)
					{
					case UET_EVENT_STREAMING_STOP:
					case UET_EVENT_SETPIN:
						eventLogFile << "module " << Handle() << "\n";
						printedModule = true;
						break;
					}
				}

				switch (e2->eventType)
				{
				case UET_UI_NOTIFY:
				case UET_DEACTIVATE_VOICE:
				case UET_SUSPEND:
				{
				}
				break;

				case UET_EVENT_STREAMING_STOP:
				{
					UPlug* p = module->GetPlug(e2->parm1);
					eventLogFile << "Stop " << e2->parm1 << " :";

					int deltat = (int)(e2->timeStamp - SampleClock());
					float v = p->GetSamplePtr()[deltat];
					eventLogFile << v << "\n";
				}
				break;

				case UET_EVENT_SETPIN:

				{
					UPlug* p = module->GetPlug(e2->parm1);
					eventLogFile << "setpin " << e2->parm1 << " :";
					int cast_new_value = (int)e2->parm3; // new value forced into an int

					switch (p->DataType)
					{
					case DT_FLOAT:
					{
						eventLogFile << (float) *((float*)&cast_new_value);
					}
					break;

					case DT_ENUM:
					case DT_INT:
					case DT_BOOL:
					{
						eventLogFile << (int)cast_new_value;
					}
					break;

					case DT_STRING_UTF8:
					case DT_TEXT:
					{
						eventLogFile << "{some text}";
						/*
						if (e->parm2 > sizeof(int))
						{
						std::wstring temp((wchar_t*)e->extraData, e->parm2 / sizeof(wchar_t));
						*((std::wstring*)p->io_variable) = (temp);
						}
						else // less than 4 bytes.
						{
						std::wstring temp((wchar_t*)&(e->parm3), e->parm2 / sizeof(wchar_t));
						*((std::wstring*)p->io_variable) = (temp);
						}
						*/
					}
					break;

					case DT_DOUBLE: // phase out this type. float easier
					{
						eventLogFile << *((double*)e2->extraData);
					}
					break;

					case DT_MIDI2:
					case DT_FSAMPLE:
						break;

					case DT_BLOB:
					{
						eventLogFile << "{BLOB}";
						/*
						MpBlob* blob = (MpBlob*)p->io_variable;

						if (e->parm2 > sizeof(int))
						{
						blob->setValueRaw(e->parm2, e->extraData);
						}
						else // less than 4 bytes.
						{
						blob->setValueRaw(e->parm2, &(e->parm3));
						}
						*/
					}
					break;

					default:
						assert(false); // new datatype
						break;
					}

					eventLogFile << "\n";
				}
				break;
				};

				// transform to block-relative timing
				int32_t block_relative_clock = (int32_t)(e2->timeStamp - current_sample_clock);
				e2->timeDelta = block_relative_clock;
			}
		}
	}
}
#endif

