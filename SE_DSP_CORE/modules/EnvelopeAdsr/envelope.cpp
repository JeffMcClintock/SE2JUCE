#include <assert.h>
#include <algorithm>
#include "Envelope.h"
#include "math.h"
#include <float.h>

using namespace std;

// tc = 1200 * log2( s )
//    = 1200 * ln(s)/ln(2)
//    = 1731 * ln(s)
//#define TimecentToSecond(t) pow( 2., (double)(t) / 1200.0 )
// 0 Volts = -8000 timecents = approx 10ms
//#define VoltageToTimecent(v) ( ((v) * 12000.f) / 10.f - 8000.f )

// New. 0v = 0.001s, 10V = 10s
#define VoltageToTime(v) ( powf( 10.0f,((v) * 0.4f ) - 3.0f ) )

/*
A linear ENV to a expo VCA will give a 'unnatural' attack response.
This results in a sound with lack off 'punch' as the attack starts to slow
and ends to fast.
In general a log envelope works better with VCAs of both the expo 
and linear type.
Odd as it may seem, it is quite common in analogue synths to use 
log envelopes in combination with expo VCAs and VCFs.
*/

Envelope::Envelope( IMpUnknown* host ) : EnvelopeBase(host)
{
	current_segment_func = static_cast <SubProcess_ptr> ( &EnvelopeBase::sub_process7 );
}

void Envelope::ChooseSubProcess( int blockPosition )
{
	bool outputIsActive = true;

	bool segment_end_level_run;// = level_plugs[cur _segment] && level_plugs[cur _segment]->getState() == ST_RUN;

	// special case, end segment clamped to zero
	if( cur_segment == end_segment )
	{
		fixed_segment_level = 0.f;
		segment_end_level_run = false;
	}
	else
	{
		if( level_plugs[cur_segment] != NULL )
		{
			fixed_segment_level = level_plugs[cur_segment]->getValue( blockPosition );
			segment_end_level_run = level_plugs[cur_segment]->isStreaming(); //getState() == ST_RUN;
		}
		else
		{
			fixed_segment_level = fixed_levels[cur_segment];
			segment_end_level_run = false;
		}
	}

	// what main process to use
	if( pinGate.isStreaming() )
	{
		SET_PROCESS(&EnvelopeBase::process_with_gate);
	}
	else
	{
		SET_PROCESS(&EnvelopeBase::sub_process);
	}

	if( fixed_increment == 0.f ) // sustain or end
	{
		if( segment_end_level_run )
		{
			assert(level_plugs[cur_segment] != 0 );
			current_segment_func = static_cast <SubProcess_ptr> (&Envelope::sub_process9);
		}
		else
		{
			if( pinOverallLevel.isStreaming() && cur_segment == sustain_segment && fixed_segment_level != 0.0f )
			{
				current_segment_func = static_cast <SubProcess_ptr> (&EnvelopeBase::sub_process4); // level changing, but rate fixed, seg level fixed
//				_RPT0(_CRT_WARN, "sub_process4\n" );
			}
			else // special case optimised.  Flat Sustain or end (no change in level)
			{
				/* already done up above
					if( level_plugs[cur_segment] != NULL )
					{
						fixed_segment_level = level_plugs[cur_segment]->getValue();
					}
					else
					{
						fixed_segment_level = fixed_levels[cur_segment];
					}
					*/
				fixed_level = pinOverallLevel.getValue( blockPosition );
				
				outputIsActive = false;	
				output_val = fixed_segment_level;
				current_segment_func = static_cast <SubProcess_ptr> (&EnvelopeBase::sub_process3); // level flat
//				_RPT0(_CRT_WARN, "sub_process3 OPT\n" );
				// now automatic. setSleep(false); //ResetStaticOutput(); // need to reset nesc?
			}
		}
	}
	else // attack/decay
	{
		if( segment_end_level_run )
		{
			current_segment_func = static_cast <SubProcess_ptr> (&Envelope::sub_process8); // level changing, but rate fixed, seg level fixed
//			_RPT0(_CRT_WARN, "sub_process8\n" );
		}
		else
		{
			bool segment_time_fixed = true; // we can pre-calc the segment length
/*
current_segment_func = static_cast <SubProcess_ptr> (sub_process5); // seg level fixed. rate, master_level RUN
segment_time_fixed = false;
_RPT0(_CRT_WARN, "sub_process5\n" );
*/	
			if( pinOverallLevel.isStreaming() || rate_plugs[cur_segment]->isStreaming() )
			{
				if( rate_plugs[cur_segment]->isStreaming() )
				{
					current_segment_func = static_cast <SubProcess_ptr> (&Envelope::sub_process5); // seg level fixed. rate, master_level RUN
					segment_time_fixed = false;
//					_RPT0(_CRT_WARN, "sub_process5\n" );
				}
				else // special case optimised. fixed rate to fixed end level, master level changing
				{
					current_segment_func = static_cast <SubProcess_ptr> (&EnvelopeBase::sub_process5b); // seg level fixed. rate, master_level RUN
//					_RPT0(_CRT_WARN, "sub_process5b OPT\n" );
				}
			}
			else // special case optimised. fixed rate to fixed end level
			{
		//		assert( pinGate.getValue(blockPosition) <= 0.0f || fixed_segment_level > 0.0f ); // or sustain = 0
				current_segment_func = static_cast <SubProcess_ptr> (&EnvelopeBase::sub_process6); // level changing, but rate fixed, seg level fixed
//				_RPT0(_CRT_WARN, "sub_process6 OPT\n" );
			}

			// pre-calculate some constants to accelerate main loops
			if(segment_time_fixed)
			{
				fixed_increment = CalcIncrement( rate_plugs[cur_segment]->getValue(blockPosition) );
			}

			// pre-calculate some constants to accelerate main loops
			if(segment_time_fixed)
			{
				//fixed_increment = CalcIncrement( rate_plugs[cur_segment]->getValue() );
				float delta_level = fixed_segment_level - output_val;
				if( delta_level == 0.0f )
				{
					// Segment is zero-length. Skip it.
					// Typical when Sustain == 0.
					next_segment(blockPosition);
					return;
				}

				fixed_increment = (float) copysign(fixed_increment, delta_level);
				float remaining = ( fixed_segment_level - output_val ) / fixed_increment;

				if( ! isfinite( remaining ) ) // divide by zero?
				{
					remaining = 0.0f;
				}

				// acumulating small errors result in undershoot.
				// partial fix is to add 1 to time.  However on slow segments,
				// overshoot might be several samples. !! fix.
				fixed_segment_time_remain = 1 + (int) remaining; // implied floor() important
				assert( remaining >= 0.f);
				fixed_segment_time_remain = max(fixed_segment_time_remain,1); // minimum segment length 1 sample (else overshoot fix don't kick in)
				// store pre-calculated values (sub_process6 only)
				fixed_level = pinOverallLevel.getValue( blockPosition );
				m_scaled_increment = fixed_increment * fixed_level;
				m_scaled_output_val = output_val * fixed_level;
			}
		}
	}

	pinSignalOut.setStreaming( outputIsActive, blockPosition );

#ifdef _DEBUG
	// sub_process9 needs a level plug.
	Envelope *e = dynamic_cast<Envelope *>( this );
	assert( e->level_plugs[cur_segment] != 0 || current_segment_func != static_cast < SubProcess_ptr> (&Envelope::sub_process9));
#endif
}

void Envelope::onSetPins()
{
	bool rechooseProcess = false;

	if( pinOverallLevel.isUpdated() )
	{
//		const char* state = pinOverallLevel.isStreaming() ? "Streaming" : "Static";
//		_RPTN(_CRT_WARN, "pinOverallLevel %f %s\n", (float) pinOverallLevel, state);
		rechooseProcess = true;
	}

	bool forcedReset = pinVoiceReset.isUpdated() && pinVoiceReset == 1.0f; // > 0.0f;
	if( forcedReset )
	{
//		_RPT1(_CRT_WARN, "Envelope:: Hard Reset. %d\n", (int) pinVoiceReset  );
		// envelope must reset to zero.
		output_val = 0.0f;
		trigger_state = false;
		gate_state = false;
		cur_segment = end_segment;
		assert( cur_segment < num_segments );
//		decay_mode = true;
		//next_segment( blockPosition() );
		ChooseSubProcess( blockPosition() );
	}

	if( pinGate.isUpdated() || pinTrigger.isUpdated() || forcedReset )
	{
//		_RPT2(_CRT_WARN, "Envelope::onSetPins. G%.3f  T%.3f\n", pinGate.getValue(), pinTrigger.getValue());
//		_RPTN(_CRT_WARN, "Envelope::Sus %.3f\n", level_plugs[1]->getValue());

		if(	pinGate.isStreaming() || pinTrigger.isStreaming() )
		{
			SET_PROCESS( &EnvelopeBase::process_with_gate );
		}
		else
		{
			OnGateChange( blockPosition(), pinGate.getValue(), pinTrigger.getValue() );

			// in some cases need to re-select subprocess
			if( getSubProcess() == static_cast <SubProcess_ptr> (&EnvelopeBase::process_with_gate) )
			{
				rechooseProcess = true;
			}
		}
	}
	else
	{
		if( fixed_increment != 0.f || cur_segment < end_segment ) // env not ended?
		{
//			if( to == GetPlug(pn_level) || to == rate_plugs[cur_segment] || to == level_plugs[cur_segment])
			if( pinOverallLevel.isUpdated() || rate_plugs[cur_segment]->isUpdated() || ( level_plugs[cur_segment] != 0 && level_plugs[cur_segment]->isUpdated() ) )
			{
				rechooseProcess = true;
			}
		}
	}

	if( rechooseProcess )
	{
		ChooseSubProcess( blockPosition() );
	}
}

float Envelope::CalcIncrement(float p_rate)
{
	// float i = (float) TimecentToSecond(VoltageToTimecent(p_rate * 10.f));
	float i = VoltageToTime(p_rate * 10.f);
	return 1.f / ( i * getSampleRate() );
}

// slope, run/fixed, level RUN, SL stop
void Envelope::sub_process5(int start_pos, int sampleframes)
{
	int count = sampleframes;

	float *out	= start_pos + pinSignalOut.getBuffer();
	float *level	= start_pos + pinOverallLevel.getBuffer();
	float *rate	= start_pos + rate_plugs[cur_segment]->getBuffer();

//	float delta_level = fixed_segment_level - output_val;
//	if( delta_level > 0.0 )
	if( fixed_segment_level > output_val )
	{
		while( count-- > 0 )
		{
			output_val += CalcIncrement( *rate++ );
			if( output_val >= fixed_segment_level )
			{
				break;
			}
			*out++ = output_val * *level++; // mayby output_val should be moved to temp var for speed?
		}
	}
	else
	{
		while( count-- > 0 )
		{
			output_val -= CalcIncrement( *rate++ );
			if( output_val <= fixed_segment_level )
			{
				break;
			}
			*out++ = output_val * *level++;
		}
	}

	//!!! All this could be moved into sub-process?????
	// i.e. if any samples remain, segment MUST have ended, call next segment...

//	sampleframesRemain = sampleframes - count - 1;
	sampleframesRemain = count + 1;

	if( sampleframes > 0 )
	{
		//SetSampleClock( SampleClock() + sampleframes - count - 1);
		if( count >= 0 ) // implies seg just ended
		{
			output_val = fixed_segment_level;
			*out = fixed_segment_level * *level;

			int blockPosition = start_pos + sampleframes - sampleframesRemain;
			next_segment(blockPosition);
		}
	}
}
// DO ALL. rate RUN, sustain RUN, level RUN, seg L run
void Envelope::sub_process8(int start_pos, int sampleframes)
{
	int count = sampleframes;

	float *out	= start_pos + pinSignalOut.getBuffer();
	float *level	= start_pos + pinOverallLevel.getBuffer();
	float *rate	= start_pos + rate_plugs[cur_segment]->getBuffer();
	float *seg_level = start_pos + level_plugs[cur_segment]->getBuffer();

	float delta_level = *seg_level - output_val;
	if( delta_level > 0.0 )
	{
		while( count-- > 0 )
		{
			output_val += CalcIncrement( *rate++ ); // mayby output_val should be moved to temp var for speed?
			if( output_val >= *seg_level )
			{
				break;
			}
			*out++ = output_val * *level++;
			seg_level++;
		}
	}
	else
	{
		while( count-- > 0 )
		{
			output_val -= CalcIncrement( *rate++ );
			if( output_val <= *seg_level )
			{
				break;
			}
			*out++ = output_val * *level++;
			seg_level++;
		}
	}

//	sampleframesRemain = sampleframes - count - 1;
	sampleframesRemain = count + 1;

	if( sampleframes > 0 )
	{
		// SetSampleClock( SampleClock() + sampleframes - count - 1);

		if( count >= 0 )
		{
			output_val = *seg_level;
			*out = *seg_level * *level;
			int blockPosition = start_pos + sampleframes - sampleframesRemain;
			next_segment(blockPosition);
		}
	}
}

// rate 0, sustain RUN, level RUN, seg L run
void Envelope::sub_process9(int start_pos, int sampleframes)
{
	if( sampleframes > 0 )
	{
		float *out	= start_pos + pinSignalOut.getBuffer();
		float *level	= start_pos + pinOverallLevel.getBuffer();
		float *seg_level	= start_pos + level_plugs[cur_segment]->getBuffer();

		int count = sampleframes;

		while( count-- > 0 )
		{
			*out++ = *seg_level++ * *level++;
		}

		output_val = *(seg_level-1); // hack so next segment starts right
		// SetSampleClock( SampleClock() + sampleframes );
	}
	sampleframesRemain = 0;
}

EnvelopeBase::EnvelopeBase( IMpUnknown* host ) : MpBase(host)
,num_segments(8)
,cur_segment(7)
,output_val(0.f)
,fixed_level(0.f)
,fixed_segment_level(0.f)
,gate_state(false)
,trigger_state(false)
{
	current_segment_func = static_cast <SubProcess_ptr> ( &EnvelopeBase::sub_process7 );
}

int32_t EnvelopeBase::open()
{
	cur_segment = end_segment;
	return MpBase::open();
}

// gate or trigger pin streaming...
void EnvelopeBase::process_with_gate(int start_pos, int sampleframes)
{
	float *gate	= pinGate.getBuffer() + start_pos;
	float *trigger	= pinTrigger.getBuffer() + start_pos;

	int last = start_pos + sampleframes;
	int to_pos = start_pos;
	int cur_pos = start_pos;

	while( cur_pos < last )
	{
		// Time till next gate/trigger change?
		//while( (*gate > 0.f ) == gate_state && to_pos < last )
		while( (*gate > 0.f ) == gate_state && (*trigger > 0.f ) == trigger_state && to_pos < last )
		{
			to_pos++;
			gate++;
			trigger++;
		}

		//		if( to_pos > cur_pos ) // no harm (sub_process seems to handle sampleframes = 0 ok)
		{
			sub_process( cur_pos, to_pos - cur_pos );
		}

		if( to_pos == last )
		{
			return;
		}

		cur_pos = to_pos;

		OnGateChange( cur_pos, *gate, *trigger );
	}
}


// master function, calls helpers for each segment
void EnvelopeBase::sub_process(int start_pos, int sampleframes)
{
#ifdef _DEBUG
	// sub_process9 needs a level plug.
	Envelope *e = dynamic_cast<Envelope *>( this );
	assert( e->level_plugs[cur_segment] != 0 || current_segment_func != static_cast < SubProcess_ptr> (&Envelope::sub_process9));
#endif

	do
	{
		#if defined( _DEBUG )
			sampleframesRemain = -2000;
			SubProcess_ptr last_segment_func = current_segment_func;
		#endif

		(this->*(current_segment_func))( start_pos, sampleframes );

		assert( sampleframesRemain >= 0 && "Sub Process must set 'sampleframesRemain'" );

		// int samples_remain = target_sample_clock - SampleClock();
		start_pos += sampleframes - sampleframesRemain;
		sampleframes = sampleframesRemain;
	}
	while( sampleframes > 0 );

	assert( sampleframes == 0 );
}

// rate zero, level STOP 
void EnvelopeBase::sub_process3(int start_pos, int sampleframes)
{
	int count = sampleframes;
	float *out = start_pos + pinSignalOut.getBuffer();
	float level = output_val * fixed_level;

	while( count-- > 0 )
	{
		*out++ = level;
	}

	// SetSampleClock( SampleClock() + sampleframes );
	sampleframesRemain = 0;

	/* TODO
	// Sleep IfOutputStatic(sampleframes);
	static_output_count -= sampleframes;
	if( static_output_count <= 0 )
	{
		// if gate stopped, can sleep , else must keep watching it
		if( GetPlug(pn_gate)->getState() != ST_RUN )
		{
			SET _PROCESS( &ug_base::process_sleep );
		}

		current_segment_func = static_cast <SubProcess_ptr> ( &EnvelopeBase::sub_process7 ); // do nothing sub-process
	}
	*/
}

// rate zero (sustain), level RUN, sl fixed
void EnvelopeBase::sub_process4(int start_pos, int sampleframes)
{
	int count = sampleframes;

	float *out	= start_pos + pinSignalOut.getBuffer();
	float *level	= start_pos + pinOverallLevel.getBuffer();

	while( count-- > 0 )
	{
		*out++ = output_val * *level++;
	}

	/*
	if( sampleframes > 0 )
	{
		SetSampleClock( SampleClock() + sampleframes ); //- count - 1);
	}
	*/
	sampleframesRemain = 0;
}

// slope fixed, level RUN, SL stop
void EnvelopeBase::sub_process5b(int start_pos, int sampleframes)
{
	int count = min( sampleframes, fixed_segment_time_remain );

	if( count == 0 ) // segment ended on last call
	{
		if( sampleframes != 0 ) // can happen, must do nothing if zero.
		{
			next_segment(start_pos);
		}
		sampleframesRemain = sampleframes;
		return;
	}

	fixed_segment_time_remain -= count;
	// SetSampleClock( SampleClock() + count );
	sampleframesRemain = sampleframes - count;

	float *out	= start_pos + pinSignalOut.getBuffer();
	float *level= start_pos + pinOverallLevel.getBuffer();

	float increment = fixed_increment;
	while( count-- > 0 )
	{
		output_val += increment; // mayby output_val should be moved to temp var for speed?
		*out++ = output_val * *level++;
	}

	// when incmenting output val has resulted in end of segment
	// set output value to exact end level (undoes any overshoot due to quantisation of segment length)
	// NEXT time this routine is called, it triggers next_segment().
	// because at the end of a segment, the sampleclock could 'point' to 
	// start of next block, so we delay calling next_segment untill then
	// calc last sample written and set to exact end value
	assert( fixed_segment_time_remain >= 0 );
	if( fixed_segment_time_remain == 0 )
	{
		assert( sampleframes != 0 );

		output_val = fixed_segment_level;
		out--;
		level--;
		*out = output_val * *level;
	}
}

// rate non-zero, STOP. level STOP , seg lev stop
void EnvelopeBase::sub_process6(int start_pos, int sampleframes)
{
	int count = min( sampleframes, fixed_segment_time_remain );
	fixed_segment_time_remain -= count;

	sampleframesRemain = sampleframes - count;

	float* output_ptr = start_pos + pinSignalOut.getBuffer();
	for( int i = count ; i > 0 ; --i )
	{
		m_scaled_output_val += m_scaled_increment;
		*output_ptr++ = m_scaled_output_val;
	}

	output_val += fixed_increment * (float) count; // so next routine knows where to start

// done in asm	output_val += count * fixed_increment; // so next routine knows where to start

	assert( fixed_segment_time_remain >= 0 );

	if( fixed_segment_time_remain == 0 )
	{
		if( sampleframes == 0 ) // can happen, must do nothing if zero.
		{
			return;
		}

		// when incmenting output val has resulted in end of segment
		// set output value to exact end level (undoes any overshoot due to quantisation of segment length)
		// NEXT time this routine is called, it triggers next_segment().
		// because at the end of a segment, the sampleclock could 'point' to 
		// start of next block, so we delay calling next_segment untill then
		if( count != 0 )
		{
			// calc last sample written and set to exact end value
			output_val = fixed_segment_level;
			count--;
			float *out_ptr = pinSignalOut.getBuffer() + start_pos + count;
			*out_ptr = output_val * fixed_level;
		}
		else
		{
			next_segment(start_pos + count);
		}
	}
}

// Sus/end. level STOP, output sta-tic
void EnvelopeBase::sub_process7(int start_pos, int sampleframes)
{
	// SetSampleClock( SampleClock() + sampleframes);
	sampleframesRemain = 0;
}

// switch to next segment, perhaps prematurely
void EnvelopeBase::next_segment( int blockPosition ) // Called when envelope section ends
{
	//	DEBUG_TIMESTAMP;
	//	_RPT1(_CRT_WARN, "next_segment() cur_segment = %d\n", cur_segment );
	// reached sustain or end?
	fixed_increment = 0.f; // indicated sustain, or end
	// as user can switch end segment, need >= test, not ==
	if( cur_segment < end_segment  )
	{
		if( cur_segment != sustain_segment || pinGate.getValue( blockPosition ) <= 0.f )
		{
//			 _RPT1(_CRT_WARN, "segment %d\n",cur_segment  );
			fixed_increment = 1.f; // just to flag non-hold level, will be calculated properly if needed in ChooseSubProcess()
			cur_segment++;
			assert( cur_segment < num_segments );
		}
	}

	ChooseSubProcess( blockPosition );
}


void EnvelopeBase::OnGateChange( int blockPosition, float gate, float trigger)
{
	bool new_trigger_state = ( trigger > 0.f );
	bool new_gate_state = ( gate > 0.f );

	if( new_trigger_state != trigger_state ) // ignore spurious changes
	{
		trigger_state = new_trigger_state;

		if( trigger_state == true && new_gate_state == true )
		{
			// force re-start of envelope.
			gate_state = false;
		}
	}

	if( new_gate_state != gate_state ) // prevent spurious gate changes from onplugstatechange()
	{
		gate_state = new_gate_state;
	//	DEBUG_TIMESTAMP;
	//	_RPT2(_CRT_WARN, "%d EnvelopeBase::OnGateChange. %.3f\n",SampleClock(), p_gate );

		if( gate_state )
		{
			cur_segment = -1;
			next_segment( blockPosition );
		}
		else
		{
//			_RPT1(_CRT_WARN, "Uenvelope::Release (%d)\n",(int) SampleClock() );
//				_RPT0(_CRT_WARN, "Release\n" );
			// if env is at sustain, move to next segment
			// else move to end segment
			if( cur_segment != sustain_segment || sustain_segment >= end_segment )
			{
				cur_segment = end_segment - 1;
				assert( cur_segment < num_segments );
			}
			next_segment( blockPosition );
		}
	}
}