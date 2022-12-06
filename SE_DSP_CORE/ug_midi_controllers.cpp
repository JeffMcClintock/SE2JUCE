#include "pch.h"
// Fix for <sstream> on Mac (sstream uses undefined int_64t)
#include "./modules/se_sdk3/mp_api.h"
#include <sstream>
#include "ug_midi_controllers.h"
#include "midi_defs.h"
#include "SeAudioMaster.h"
#include "module_register.h"
#include "./modules/se_sdk3/mp_midi.h"

SE_DECLARE_INIT_STATIC_FILE(ug_midi_controllers);

using namespace std;

namespace
{
REGISTER_MODULE_1(L"Controllers", IDS_MN_CONTROLLERS,IDS_MG_MIDI,ug_midi_controllers ,CF_STRUCTURE_VIEW,L"Converts MIDI Controllers like Modulation Wheel etc to voltages.  Has four outputs, configurable to any MIDI controller number. See <a href=midi_control.htm>Patches & MIDI Control</a> for more.");
}

ug_midi_controllers::ug_midi_controllers() :
		midiConverter(
			// provide a lambda to accept converted MIDI 2.0 messages
			[this](const gmpi::midi::message_view& msg, int /*offset*/)
			{
				onMidi2Message(msg);
			}
		)
{
	SET_CUR_FUNC( &ug_midi_controllers::sub_process );
}

//!! plug names to reflect controller?
#define PLG_AFTERTOUCH_MC (2 + UMIDICONTROL_NUM_CONTROLS )
#define PLG_BENDER (1 + PLG_AFTERTOUCH_MC )

// Fill an array of InterfaceObjects with plugs and parameters
void ug_midi_controllers::ListInterface2(InterfaceObjectArray& PList)
{
	// bypass, different order.	ug_midi_device::ListInterface2(PList);	// Call base class
	//	ug_base::ListInterface2(PList);	// Call base class
	LIST_VAR3N( L"MIDI In", DR_IN, DT_MIDI2 , L"0", L"", 0, L"");
	LIST_VAR3( L"Channel", midi_channel/*midi_in.channel()*/, DR_IN, DT_ENUM , L"-1", MIDI_CHAN_LIST,IO_IGNORE_PATCH_CHANGE|IO_POLYPHONIC_ACTIVE, L"MIDI Channel");

	for( int i = 0 ; i < UMIDICONTROL_NUM_CONTROLS ; i ++ )
	{
		//std::wstring default_val;
		//default_val.Format((L"%d"), i + 1);
		std::wostringstream oss;
		oss << (i+1);
		LIST_VAR3( L"Type", controller_type[i], DR_IN, DT_ENUM ,oss.str().c_str(),CONTROLLER_ENUM_LIST, IO_POLYPHONIC_ACTIVE, L"Choose which MIDI controller you want converted to a Voltage output");
	}

	LIST_PIN( L"Aftertouch", DR_OUT, ( L""),( L""), 0,( L""));
	LIST_PIN( L"Bender", DR_OUT, ( L""),( L""), 0,( L""));

	for( int i = 0 ; i < UMIDICONTROL_NUM_CONTROLS ; i ++ )
	{
		LIST_PIN( L"Controller", DR_OUT, ( L""),( L""), 0,( L""));
	}
}

int ug_midi_controllers::Open()
{
	ug_midi_device::Open();
	output_info.assign( 2 + UMIDICONTROL_NUM_CONTROLS, SOT_LINEAR );
	output_info[0].SetPlug( GetPlug(PLG_AFTERTOUCH_MC) );
	output_info[1].SetPlug( GetPlug(PLG_BENDER) );

	for( int i = 0 ; i < UMIDICONTROL_NUM_CONTROLS ; i ++ )
	{
		output_info[2+i].SetPlug( GetPlug(PLG_BENDER+i+1) );
		controller_val[i] = 0;
	}

	// init smart outputs
	int smoothing_sample_count = (int) getSampleRate() / MIDI_CONTROLLER_SMOOTHING; // allow 1/32 sec for transition;

	//for( int i = output_info.GetUpperBound() ; i >= 0 ; i-- )
	for( vector<smart_output>::iterator it = output_info.begin() ; it != output_info.end() ; ++it )
	{
		(*it).curve_type = SOT_LINEAR;
		(*it).m_transition_samples = smoothing_sample_count;
	}

	/*
	#if defined( _DEBUG )
		//testing
		output_info[2].m_transition_samples = 0;

		output_info[3].m_transition_samples = (int) SampleRate() / 126; // roland mod wheel
		output_info[3].curve_type = SOT_LOW_PASS;
	#endif
	*/
	SET_CUR_FUNC( &ug_midi_controllers::sub_process );
	return 0;
}

void ug_midi_controllers::sub_process(int start_pos, int sampleframes)
{
	bool can_sleep = true;

	for( vector<smart_output>::iterator it = output_info.begin() ; it != output_info.end() ; ++it )
	{
		(*it).Process( start_pos, sampleframes, can_sleep );
	}

	if( can_sleep )
	{
		SET_CUR_FUNC( &ug_base::process_sleep );
	}
}

void ug_midi_controllers::OnMidiData( int size, unsigned char* midiMessage )
{
	gmpi::midi::message_view msg((const uint8_t*)midiMessage, size);

	// convert everything to MIDI 2.0
	midiConverter.processMidi(msg, -1);
}

void ug_midi_controllers::onMidi2Message(const gmpi::midi::message_view & msg)
{
	const auto header = gmpi::midi_2_0::decodeHeader(msg);

	// only 8-byte messages supported. only 16 channels supported
	if (header.messageType != gmpi::midi_2_0::ChannelVoice64)
		return;

	switch (header.status)
	{

	case gmpi::midi_2_0::ControlChange:
	{
		const auto controller = gmpi::midi_2_0::decodeController(msg);

		for (int i = 0; i < UMIDICONTROL_NUM_CONTROLS; i++)
		{
			if (controller.type == controller_type[i])
			{
				ChangeOutput(SampleClock(), GetPlug(PLG_BENDER + i + 1), controller.value);
			}
		}
	}
	break;

	case gmpi::midi_2_0::ChannelPressue:
	{
		const auto normalized = gmpi::midi_2_0::decodeController(msg).value;

		ChangeOutput(SampleClock(), GetPlug(PLG_AFTERTOUCH_MC), normalized);
	}
	break;

	case gmpi::midi_2_0::PitchBend:
	{
		const auto normalized = gmpi::midi_2_0::decodeController(msg).value;

		ChangeOutput(SampleClock(), GetPlug(PLG_BENDER), normalized * 2.f - 1.f); // -10V -> +10V
	}
	break;

	default:
		break;
	};
}

void ug_midi_controllers::ChangeOutput(timestamp_t p_sample_clock, UPlug* plug, float new_val)
{
	SET_CUR_FUNC( &ug_midi_controllers::sub_process );

	for( auto& info : output_info )
	{
		if( info.plug == plug )
		{
			info.Set( p_sample_clock, new_val );
			info.static_output_count = AudioMaster()->BlockSize();
			return;
		}
	}

	assert( false ); // can't find plug
}

void ug_midi_controllers::Resume()
{
	ug_base::Resume();

	for( auto& info : output_info )
	{
		info.Resume();
	}
}
