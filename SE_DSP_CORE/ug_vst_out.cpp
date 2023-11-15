#include "pch.h"
#include <algorithm>
#include "ug_vst_out.h"
#include "SeAudioMaster.h"
#include "module_register.h"

SE_DECLARE_INIT_STATIC_FILE(ug_vst_out)

using namespace std;

namespace
{
REGISTER_MODULE_1(L"VST Output", IDS_MN_VST_OUTPUT,IDS_MG_DEBUG,ug_vst_out ,0,"");
}

// Fill an array of InterfaceObjects with plugs and parameters
void ug_vst_out::ListInterface2(InterfaceObjectArray& PList)
{
	// IO Var, Direction, Datatype, Name, Default, defid (index into unit_gen::PlugFormats)
	// defid used to name a enum list or range of values
	LIST_VAR3N(L"MIDI In", DR_IN, DT_MIDI2 ,L"0",L"", 0,L"");
//	LIST_VAR3(L"Processor/ClearTails", m_clear_tails, DR_IN, DT_INT, L"", L"", IO_HOST_CONTROL|IO_HIDE_PIN, L"");
	LIST_PIN(L"Audio",DR_IN, L"",L"", IO_AUTODUPLICATE|IO_LINEAR_INPUT|IO_CUSTOMISABLE,L"");
}

ug_vst_out::ug_vst_out() :
	m_outputs(0)
	,fadeLevel_(1.0f)
    ,targetLevel(1.0f)
    ,bufferIncrement_(1)
{
	SetFlag( UGF_POLYPHONIC_AGREGATOR );
}

ug_vst_out::~ug_vst_out()
{
	delete [] m_outputs;
	assert(0 == (flags & UGF_OPEN));
}

void ug_vst_out::onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state)
{
	switch (p_to_plug->getPlugIndex())
	{
/* not needed at present
		case PIN_CLEARTAILS:
			// DAW has resumed, but plugin may emit spikes due to discontinuities. Fade output up gradually to mask them. (fix for Presonus One "invalid output" crap).
			_RPTN(0, "ug_vst_out CLEARTAILS. fade start at %f\n", fadeLevel_);
			startFade(false);
			break;
*/
		default: // audio pins?
		{
			const int outputIndex = p_to_plug->getPlugIndex() - PIN_AUDIOOUT0;
			if (outputIndex >= 0)
			{
				pinIsSilent[outputIndex] = p_to_plug->getState() != ST_RUN && p_to_plug->getValue() == 0.0f;
				pinChanged[outputIndex] = true;
			}
		}
		break;
	}
}

uint64_t ug_vst_out::updateSilenceFlags(int output, int count)
{
	uint64_t ret{};

	uint64_t flag = 1;
	for (int i = 0 ; i < count; ++i)
	{
		if (pinIsSilent[output + i] && !pinChanged[output + i])
		{
			ret &= flag; // set one bit
		}

		pinChanged[output + i] = false; // reset for next buffer
		flag = flag << 1;
	}

	return ret;
}

void ug_vst_out::setIoBuffers(float* const* p_outputs, int numChannels, int stride)
{
	bufferIncrement_ = stride;
	dawChannels_ = numChannels;

    const int myChannelCount = static_cast<int>(plugs.size()) - PIN_AUDIOOUT0;
    const int channelCount = (std::min)(myChannelCount, numChannels); // Ableton will connect two channels to a 1 channel effect. Ignore last.

    int i = 0;
    for (; i < channelCount; ++i)
    {
        m_outputs[i] = p_outputs[i];
    }

    // plugin has more outputs than DAW wants?
    for (; i < myChannelCount; ++i)
    {
        m_outputs[i] = nullptr;
    }
}

int ug_vst_out::Open()
{
	ug_base::Open();

    ChooseProcess();

	size_t out_count = plugs.size() - PIN_AUDIOOUT0;
	m_outputs = new float *[out_count];

	pinIsSilent.assign(out_count, false);
	pinChanged.assign(out_count, false);

	return 0;
}

void ug_vst_out::ChooseProcess()
{
    // We can't know if we are in replacing mode or not till host called process, defer decision till then.
	SET_CUR_FUNC( &ug_vst_out::sub_process_check_mode );
}

void ug_vst_out::HandleEvent(SynthEditEvent* e)
{
	// send event to ug
	switch( e->eventType )
	{
	case UET_EVENT_MIDI:
	{
		MidiBuffer.Add( e->timeStamp, (unsigned char*) e->Data(), e->parm2 );
	}
	break;

	default:
		ug_midi_device::HandleEvent( e );
	};
}


// on first block, need to check if host requested process replacing mode (or process add mode)
void ug_vst_out::sub_process_check_mode(int start_pos, int sampleframes)
{
	typedef void (ug_vst_out::* process_func_ptr_vstout)(int start_pos, int sampleframes); // Pointer to sound processing member function
	process_func_ptr_vstout sb = 0;

	if( fadeLevel_ != targetLevel )
	{
         sb = ( &ug_vst_out::subProcessTemplate<ProcessFading>);
	}
    else
    {
        sb = ( &ug_vst_out::subProcessTemplate<ProcessNotFading>);
    }
    
    SET_CUR_FUNC( sb );

	(this->*(process_function))( start_pos, sampleframes );
}

void ug_vst_out::startFade(bool isDucked )
{
	const float fadeTimeMs = 50.0f;
	fadeIncrement_ = 1000.f / ( fadeTimeMs * getSampleRate());

    if( isDucked )
    {
        targetLevel = 0.0f;
        fadeIncrement_ = -fadeIncrement_;
    }
    else
    {
        targetLevel = 1.0f;
    }

    SET_CUR_FUNC( &ug_vst_out::sub_process_check_mode );
}
/* not needed at present

// DAW has resumed, but plugin may 'spike' due to discontinuities, mute output until we recieve the 'clear tails' signal (then fade-up).
void ug_vst_out::MuteUntilTailReset()
{
	_RPT0(0, "ug_vst_out::MuteUntilTailReset()\n");

	fadeLevel_ = targetLevel = 0.0f;
	SET_CUR_FUNC(&ug_vst_out::sub_process_check_mode);
}
*/
