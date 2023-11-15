#include "pch.h"
#include "modules/shared/xp_simd.h"
#include "ug_patch_automator.h"
#include "ug_container.h"
#include "midi_defs.h"
#include "module_register.h"
#include "IDspPatchManager.h"
#include "ug_event.h"
#include "midi_defs.h"
#include "iseshelldsp.h"
#include "math.h"
#include "BundleInfo.h"

using namespace std;

// #define DEBUG_TIMING

#ifdef DEBUG_TIMING
FILE* timingFile = 0;
#endif

namespace
{
	static pin_description plugs[] =
	{
		L"Channel"		, DR_IN, DT_ENUM ,L"", L"All=-1,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16",IO_IGNORE_PATCH_CHANGE|IO_POLYPHONIC_ACTIVE|IO_DISABLE_IF_POS, L"",
		L"MIDI In"		, DR_IN, DT_MIDI2 ,L"0",L"", 0, L"",
		L"MIDI Out"		, DR_OUT, DT_MIDI2 ,L"",L"", 0, L"",
		L"Hidden Output", DR_OUT, DT_MIDI2 ,L"0",L"", IO_DISABLE_IF_POS, L"",
		L"Send All On Patch Change", DR_IN, DT_BOOL,L"0",L"", IO_MINIMISED, L"",
	};

	static module_description_dsp mod =
	{
		"SE Patch Automator"				// unique id
		, IDS_MN_PATCH_SELECT
		, IDS_MG_MIDI
		, &ug_patch_automator::CreateObject	// create function
		, CF_DONT_EXPAND_CONTAINER|CF_STRUCTURE_VIEW					// flags CF_WHATEVER
		,plugs
		, sizeof(plugs)/sizeof(pin_description)
	};

	bool res = ModuleFactory()->RegisterModule( mod );
}

/////////////////////////////////////////////////////////////////////////////////////////////////

ug_patch_automator::ug_patch_automator() :
	m_send_automation_on_patch_change(false)
	,transmitTempo_(false)
{
	SetFlag(UGF_PATCH_AUTO);
	memset(&timeInfo, 0, sizeof(timeInfo));

#ifdef DEBUG_TIMING
	timingFile = fopen("HostTimingLog.txt", "w");
	char* header = "SampleClock, samplePos, sampleRate, nanoSeconds, ppqPos, tempo, barStartPos, cycleStartPos, cycleEndPos, timeSigNumerator, timeSigDenominator, smpteOffset, smpteFrameRate, samplesToNextClock, flags\n";
	fwrite(header, 1, strlen(header), timingFile);
#endif
}

ug_patch_automator::~ug_patch_automator()
{
#ifdef DEBUG_TIMING
	fclose(timingFile);
#endif
}

// note: this causes GetPLug (by name) to fail because it calls ListInterface2.
// had to use literal plug ID in various places.
void ug_patch_automator::assignPlugVariable(int p_plug_desc_id, UPlug* p_plug)
{
	switch( p_plug_desc_id )
	{
	case 0:
		// comes from GUI now
		//		p_plug->AssignVariable( &midi_channel );
		break;

	case 1:
		//p_plug->AssignVariable( &trash_sample_ptr );
		break;

	case 2: // don't change. used in CContainer.cpp
		p_plug->AssignVariable( &midi_out );
		break;

	case 3: // don't change. used in ug_container::RouteDummyPinToPatchAutomator()
		p_plug->AssignVariable( &midi_out_hidden );
		break;

	case 4:
		p_plug->AssignVariable( &m_send_automation_on_patch_change );
		break;

	case 5:
		p_plug->AssignVariable( &polyMode_ );
		break;

	case 6:
		p_plug->AssignVariable( &monoRetrigger_ );
		break;

	case 7:
		p_plug->AssignVariable( &monoNotePriority_ );
		break;

	case 8:
		p_plug->AssignVariable( &portamentoTime_ );
		break;

	default:
		break;
	};
}

void ug_patch_automator::OnMidiData( int size, unsigned char* midi_bytes )
{
	patch_control_container->get_patch_manager()->OnMidi( &voiceState_, SampleClock(), midi_bytes, size, false);
}

int ug_patch_automator::Open()
{
	AudioMaster()->RegisterPatchAutomator(this);

	voiceState_.voiceControlContainer_ = patch_control_container;

#if 0
	{
		// On reset, restore tuning table.
		auto app = AudioMaster()->getShell();
		auto tuningTable = voiceState_.getTuningTable();
		auto tunningTableRaw = app->getPersisentHostControl(voiceState_.voiceControlContainer_->Handle(), HC_VOICE_TUNING, RawView((const void*)tuningTable, sizeof(int) * 128));
		memcpy(tuningTable, tunningTableRaw.data(), sizeof(tunningTableRaw.size()));

//		_RPT2(_CRT_WARN, "ug_patch_automator[%x] TUNE %x\n", voiceState_.voiceControlContainer_->Handle(), voiceState_.GetIntKeyTune(61)); // C#4
	}
#endif

	// rather convoluted, but can't use patch-store pins i this module.
	//setMidiChannel( patch_control_container->get_patch_manager()->getMidiChannel() );
	ug_midi_device::Open();

	// if acting as panel for external MIDI device, need to send all CCs at startup.
	if( m_send_automation_on_patch_change )
	{
		// can't send controller on first block, as controls havn't registered yet
		//RUN_AT( SampleClock() + AudioMaster()->BlockSize(), &ug_patch_automator::AutomationSendAll );
		patch_control_container->get_patch_manager()->SendInitialUpdates();
	}

	/*
		DebugIdentify();
		_RPT1(_CRT_WARN, "(In) SortOrder=%d\n", GetSortOrder() );
	*/

	// Act as Proxy for Audiomaster. It can't have events.
	if( parent_container->parent_container == 0 )
	{
		SeAudioMaster* mainAudioMaster = dynamic_cast<SeAudioMaster*>(AudioMaster());

		if( mainAudioMaster ) // else we're virtualized inside oversampler)
		{
			// SynthEdit 60Hz.
			const int GuiServiceFreqHz = 60;

			const auto bs = AudioMaster()->BlockSize();
			// Num blocks, rounded down to whole block boundary. Ensure update rate equal or higher to target.
			guiQueServicePeriod_ = (int)ug_base::getSampleRate() / ( GuiServiceFreqHz * bs);

			guiQueServicePeriod_ = max( guiQueServicePeriod_, 1 ); // can't be zero blocks! (low SR, low latency).
			guiQueServicePeriod_ *= bs;
			assert( guiQueServicePeriod_ > 0 );
			EventProcessor::AddEvent( new_SynthEditEvent( SampleClock() + guiQueServicePeriod_, UET_SERVICE_GUI_QUE , 0, 0, 0, 0 ));
			SET_CUR_FUNC(&ug_base::process_nothing); // lower CPU than process_sleep
		}
	}

	return 0;
}

void ug_patch_automator::setMidiChannel( int channel )
{
	midi_channel = static_cast<short>(channel);
}

// Called at begining of host buffer. i.e. begining of every process function.
void ug_patch_automator::UpdateTempo( my_VstTimeInfo* pTimeInfo )
{
	if (!wantsTempo())
		return;

	Wake();

#ifdef DEBUG_TIMING
	{
		char buffer[300];
		sprintf(buffer, "%d,%f,%f,%f,%f,%f,%f,%f,%f,%d,%d,%d,%d,%d,%c%c%c\n",
			static_cast<int>(SampleClock()),
			pTimeInfo->samplePos,			// current location
			pTimeInfo->sampleRate,
			pTimeInfo->nanoSeconds,			// system time
			pTimeInfo->ppqPos,				// 1 ppq
			pTimeInfo->tempo,				// in bpm
			pTimeInfo->barStartPos,			// last bar start, in 1 ppq
			pTimeInfo->cycleStartPos,		// 1 ppq
			pTimeInfo->cycleEndPos,			// 1 ppq
			pTimeInfo->timeSigNumerator,		// time signature
			pTimeInfo->timeSigDenominator,
			pTimeInfo->smpteOffset,
			pTimeInfo->smpteFrameRate,			// 0:24, 1:25, 2:29.97, 3:30, 4:29.97 df, 5:30 df
			pTimeInfo->samplesToNextClock,		// midi clock resolution (24 ppq), can be negative
			(pTimeInfo->flags & kVstTransportChanged) != 0 ? 'C' : '-',
			(pTimeInfo->flags & kVstTransportPlaying) != 0 ? 'P' : '-',
			(pTimeInfo->flags & kVstTransportCycleActive) != 0 ? 'L' : '-'
			);

		fwrite(buffer, 1, strlen(buffer), timingFile);
	}
#endif


	auto patch_manager = patch_control_container->get_patch_manager();

	/*
	BSP 4.000000, PQNP 6.993605, SNC 0
	BSP 4.000000, PQNP 7.005215, SNC 0
	BSP 4.000000, PQNP 7.016826, SNC 0
	BSP 4.000000, PQNP 7.028436, SNC 0
	*/

	if(timeInfo.tempo != (float) pTimeInfo->tempo )
	{
		if(timeInfo.tempo <= 0.0f ) // indicates first-time use.
		{
			// Force update.
			timeInfo.flags = (~pTimeInfo->flags) & my_VstTimeInfo::kVstTransportPlaying;
		}

		timeInfo.tempo = pTimeInfo->tempo;

		// _RPT1(_CRT_WARN, "TEMPO %f\n", timeInfo.tempo );
		float v = static_cast<float>(timeInfo.tempo);
		patch_manager->vst_Automation2( SampleClock(), ControllerType::BPM << 24, &v, sizeof(v) );

		constexpr double seconds_per_minute = 60.0;
		incrementPerSample = timeInfo.tempo / (getSampleRate() * seconds_per_minute);
	}

	if (timeInfo.timeSigNumerator != pTimeInfo->timeSigNumerator)
	{
		timeInfo.timeSigNumerator = pTimeInfo->timeSigNumerator;
		patch_manager->vst_Automation2(SampleClock(), ControllerType::timeSignatureNumerator << 24, &timeInfo.timeSigNumerator, sizeof(timeInfo.timeSigNumerator));
	}

	if (timeInfo.timeSigDenominator != pTimeInfo->timeSigDenominator)
	{
		timeInfo.timeSigDenominator = pTimeInfo->timeSigDenominator;
		patch_manager->vst_Automation2(SampleClock(), ControllerType::timeSignatureDenominator << 24, &timeInfo.timeSigDenominator, sizeof(timeInfo.timeSigDenominator));
	}

	if (timeInfo.barStartPos != pTimeInfo->barStartPos )
	{
		timeInfo.barStartPos = pTimeInfo->barStartPos;
		float v = static_cast<float>(timeInfo.barStartPos);
		patch_manager->vst_Automation2(SampleClock(), ControllerType::barStartPosition << 24, &v, sizeof(v));
	}

	if (timeInfo.TransportRun() != ((pTimeInfo->flags & my_VstTimeInfo::kVstTransportPlaying) != 0))
	{
		timeInfo.flags = pTimeInfo->flags & my_VstTimeInfo::kVstTransportPlaying;
		patch_manager->vst_Automation(patch_control_container, SampleClock(), ControllerType::TransportPlaying << 24, static_cast<float>(timeInfo.TransportRun()));

		if (timeInfo.TransportRun())
		{
			SET_CUR_FUNC( &ug_patch_automator::process_timing );
		}
		else
		{
			// Send QNP.
			timeInfo.ppqPos = pTimeInfo->ppqPos;
			float v = static_cast<float>(timeInfo.ppqPos);
			patch_manager->vst_Automation2(SampleClock(), ControllerType::SongPosition << 24, &v, sizeof(v));

			SET_CUR_FUNC( &ug_base::process_sleep );
		}
	}

	if ((pTimeInfo->flags & (my_VstTimeInfo::kVstTransportCycleActive| my_VstTimeInfo::kVstCyclePosValid)) == (my_VstTimeInfo::kVstTransportCycleActive | my_VstTimeInfo::kVstCyclePosValid))
	{
		timeInfo.cycleStartPos = pTimeInfo->cycleStartPos;
		timeInfo.cycleEndPos = pTimeInfo->cycleEndPos;
	}
	else
	{
		timeInfo.cycleEndPos = (std::numeric_limits<double>::max)();
	}

	double error = timeInfo.ppqPos - pTimeInfo->ppqPos;
	const double errorTollerence = 0.0001;
	timeInfo.ppqPos = pTimeInfo->ppqPos;
	if( error < -errorTollerence || error > errorTollerence)
	{
        // Out of sync. Send immediate correction.
		float v = static_cast<float>(timeInfo.ppqPos);
		patch_manager->vst_Automation2( SampleClock(), ControllerType::SongPosition << 24, &v, sizeof(v) );
//        _RPT3(_CRT_WARN, "                                       RESYNC: SampleClock %d, SongPos mine:%f  host:%f\n", (int) SampleClock(), barSongPosition + error, barSongPosition );
	}
/*
	// ppqPos is same as timeInfo.barStartPos, except timeInfo.barStartPos = floor(ppqPos)
	// so quarter notes are ppqPos * 4
	_RPT4(_CRT_WARN, "BSP %f, PQNP %f, *4 %f, flags %x\n", timeInfo->timeInfo.barStartPos, timeInfo->ppqPos, timeInfo->ppqPos * 4.0, timeInfo->flags );
	_RPT2(_CRT_WARN, "QNP %f,                                       ERR %f\n", (float)ppqPos, error / samples_per_clock );
*/

#ifdef _DEBUG
	debugTimingPrint();
#endif
}

void ug_patch_automator::debugTimingPrint()
{
#if 0
	static my_VstTimeInfo prevInfo;

	if (memcmp(&prevInfo, &timeInfo, sizeof(timeInfo)) != 0)
	{
		memcpy(&prevInfo, &timeInfo, sizeof(timeInfo));

		_RPT0(_CRT_WARN, "TIMING (SYNTHEDIT)\n");
		_RPT1(_CRT_WARN, "  tempo:              %.2f\n", timeInfo.tempo);
		_RPT1(_CRT_WARN, "  ppqPos:             %6.2f\n", timeInfo.ppqPos);
		_RPT1(_CRT_WARN, "  barStartPos:        %.2f\n", timeInfo.barStartPos);
		_RPT2(_CRT_WARN, "  timeSig:            %d/%d\n", timeInfo.timeSigNumerator, timeInfo.timeSigDenominator);

		/* not provided
		_RPT1(_CRT_WARN, "  samplesToNextClock: %5d\n", timeInfo.samplesToNextClock);
		_RPT1(_CRT_WARN, "  samplePos:          %6.2f\n", timeInfo.samplePos);
		_RPT1(_CRT_WARN, "  sampleRate:         %6.2f\n", timeInfo.sampleRate);
		*/
	}
#endif
}

void ug_patch_automator::setTempoTransmit()
{
	transmitTempo_ = true;
}

// For sending 'neat' whole quarter-note updates.
void ug_patch_automator::UpdateSongPosition(double qnEventTime, double ppqPos)
{
	// Does event fall cleanly within current block?
	if (timeInfo.ppqPos > qnEventTime)
		return;

	double samplesTillEvent = (qnEventTime - timeInfo.ppqPos) / incrementPerSample;
	int samplesTillEvent_i = FastRealToIntTruncateTowardZero(ceil(samplesTillEvent));
	assert(samplesTillEvent_i >= 0);
	double fraction = static_cast<double>(samplesTillEvent_i) - samplesTillEvent;

	timestamp_t t = SampleClock() + samplesTillEvent_i;
	// Bar position may be a little more due it not falling on an exact sample.
	float barSongPosition = static_cast<float>(ppqPos) + static_cast<float>(fraction * incrementPerSample);

	auto patch_manager = patch_control_container->get_patch_manager();
	patch_manager->vst_Automation2(t, ControllerType::SongPosition << 24, &barSongPosition, sizeof(barSongPosition));

	lastWholeQuarterNoteSent = FastRealToIntFloor(ppqPos);

#ifdef _DEBUG
	debugTimingPrint();
#endif
}

void ug_patch_automator::process_timing(int /*start_pos*/, int sampleframes)
{
    double ppqPosNew = timeInfo.ppqPos + incrementPerSample * (double) sampleframes;

	// Loop?
	if (ppqPosNew > timeInfo.cycleEndPos)
	{
		UpdateSongPosition(timeInfo.cycleEndPos, timeInfo.cycleStartPos);

		// Move song position back to loop start.
		ppqPosNew -= timeInfo.cycleEndPos - timeInfo.cycleStartPos;
	}
	else
	{
		int ppqPosNewFloor = FastRealToIntFloor(ppqPosNew);

		// Does QNP increase by 1 or more quarter notes during this block?
		// If so send out update on next precise sample after quarter note.
		if(lastWholeQuarterNoteSent != ppqPosNewFloor )
		{
			UpdateSongPosition(ppqPosNewFloor, ppqPosNewFloor);
		}
	}

    timeInfo.ppqPos = ppqPosNew;
}

void ug_patch_automator::HandleEvent(SynthEditEvent* e)
{
	// send event to ug
	switch( e->eventType )
	{
		// handled on behalf of AudioMaster.
		case UET_SERVICE_GUI_QUE:
		{
			assert( dynamic_cast<SeAudioMaster*>(AudioMaster()) ); // else we're virtualized inside oversampler)

			AudioMaster()->ServiceGuiQue();
			EventProcessor::AddEvent( new_SynthEditEvent( SampleClock() + guiQueServicePeriod_, UET_SERVICE_GUI_QUE , 0, 0, 0, 0 ));
		}
		break;

	default:
		ug_midi_device::HandleEvent( e );
	};
}
