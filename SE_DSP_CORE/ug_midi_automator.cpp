#include "pch.h"
#include "ug_midi_automator.h"
#include "module_register.h"
#include "midi_defs.h"
#include "ug_patch_automator.h"

SE_DECLARE_INIT_STATIC_FILE(ug_midi_automator)

using namespace std;
// SEE ug_patch_automator.

namespace
{
REGISTER_MODULE_1(L"MIDI Automator", IDS_MN_MIDI_AUTOMATOR,IDS_MG_DEBUG,ug_midi_automator ,CF_DONT_EXPAND_CONTAINER|/*CF_CONTROL_AUTOMATOR|*/CF_STRUCTURE_VIEW,L"Enables MIDI automation of all a container's controls. See <a href=midi_control.htm>Patches & MIDI Control</a> for more.");
REGISTER_MODULE_1(L"MIDI Automator Output", IDS_MN_MIDI_AUTOMATOR_OUTPUT,IDS_MG_DEBUG,ug_patch_automator_out ,CF_HIDDEN,L"Hidden module, provides MIDI output for control panel automation.");
}

#define PLG_MIDI_OUT 2 //0

// Fill an array of InterfaceObjects with plugs and parameters
void ug_midi_automator::ListInterface2(InterfaceObjectArray& PList)
{
	// IO Var, Direction, Datatype, Name, Default, defid (index into unit_gen::PlugFormats)
	LIST_VAR3( L"Channel", midi_channel/*midi_in.channel()*/, DR_IN, DT_ENUM , L"-1", MIDI_CHAN_LIST,IO_IGNORE_PATCH_CHANGE|IO_POLYPHONIC_ACTIVE, L"MIDI Channel selection");
	LIST_VAR3N( L"MIDI In", DR_IN, DT_MIDI2 , L"0", L"", 0, L"Use this to automate the controls from a MIDI sequence");
	LIST_VAR3( L"MIDI Out", midi_out, DR_OUT, DT_MIDI2 , L"", L"", 0, L"Sends MIDI controller messages when control is changed");
	LIST_VAR3( L"Hidden Output", midi_out_hidden, DR_OUT, DT_MIDI2 , L"0", L"", IO_DISABLE_IF_POS, L"Allows routing setup to position this ug correctly in sortorder");
	LIST_VAR3( L"Send All On Patch Change", trash_bool_ptr, DR_IN, DT_BOOL, L"0", L"", IO_MINIMISED, L"Sends all MIDI Controllers when patch changes, usefull when using SE as a front-end for imbedded VSTs");
}

ug_midi_automator::ug_midi_automator()
{
	SetFlag(UGF_PATCH_AUTO);
}


// Fill an array of InterfaceObjects with plugs and parameters
void ug_patch_automator_out::ListInterface2(InterfaceObjectArray& PList)
{
	// IO Var, Direction, Datatype, Name, Default, defid (index into unit_gen::PlugFormats)
	// defid used to name a enum list or range of values
	/*
		LIST_VAR3( L"MIDI Out", midi_out, DR_OUT, DT_MIDI2 , L"", L"", 0, L"Sends MIDI controller messages when control is changed");
		LIST_VAR3( L"Hidden Input", midi_in_hidden, DR_IN, DT_MIDI2 , L"0", L"", IO_DISABLE_IF_POS, L"Allows routing setup to position this ug correctly in sortorder");
	*/
	// Changed to match ug_patch_automator.
	// because ug_base::Setup() expects this to have same plugs as CUG
	LIST_VAR3( L"Channel", trash_sample_ptr, DR_IN, DT_ENUM , L"0", MIDI_CHAN_LIST,IO_IGNORE_PATCH_CHANGE|IO_POLYPHONIC_ACTIVE, L"MIDI Channel selection");
	LIST_VAR3N( L"Hidden Input", DR_IN, DT_MIDI2 , L"0", L"", 0, L"");
	LIST_VAR3( L"MIDI Out", midi_out, DR_OUT, DT_MIDI2 , L"", L"", 0, L"");
	LIST_VAR3( L"Hidden Output", midi_out_hidden, DR_OUT, DT_MIDI2 , L"0", L"", IO_DISABLE_IF_POS, L"");
	LIST_VAR3( L"Send All On Patch Change", trash_bool_ptr, DR_IN, DT_BOOL, L"0", L"", IO_MINIMISED, L"");
}

ug_patch_automator_out::ug_patch_automator_out() :
	last_out_timestamp(0)
	,lastRpnHeader_(INT_MAX)
{
	SET_CUR_FUNC( &ug_base::process_sleep );

	for( int i = 0 ; i < 128 ; ++i )
	{
		voicePitch[i] = (float) i / 127.0f;
		voiceVelocityOn[i] = voiceVelocityOff[i] = 0.5f;
	}
}

void ug_patch_automator_out::setMidiChannel( int channel )
{
	midi_channel = channel;
}

void ug_patch_automator_out::SendAutomation2( float p_normalised, int voiceId, int p_unified_controller_id, const wchar_t* sysexFormat, bool caused_by_patch_change, int* lastMidiValue)
{
	if( !automator_in->m_send_automation_on_patch_change && caused_by_patch_change )
		return;

	// todo fix, currently garbage value.!!!!!!!!
	int chan = max( (short)0, midi_channel ); // chan -1 -> chan 0
	// decode controller_id
	int controller_id = -1;
	int rpn = 0;

	switch ( p_unified_controller_id >> 24 )
	{
	case ControllerType::None:
		return;
		break;

	case ControllerType::CC:
		controller_id = p_unified_controller_id & 0x7f;
		break;

	case ControllerType::RPN:
		controller_id = RPN_MSB;
		rpn = p_unified_controller_id & 0x3fff;
		break;

	case ControllerType::NRPN:
		controller_id = NRPN_MSB;
		rpn = p_unified_controller_id & 0x3fff;
		break;

	case ControllerType::PolyAftertouch:
	{
		int quantizedControllerValue = (int) (0.5f + p_normalised * 127.0f);
		if( quantizedControllerValue != *lastMidiValue )
		{
			*lastMidiValue = quantizedControllerValue;
			int noteNum = voiceId; // p_unified_controller_id & 0x7f;
			int msg = ( quantizedControllerValue << 8 ) + noteNum;
			int chan = max( (short)0, midi_channel ); // chan -1 -> chan 0
			msg = ( msg << 8 ) + POLY_AFTERTOUCH + chan;
			Wake(); // ensure SampleClock() correct
			last_out_timestamp = SampleClock();
			midi_out.Send( last_out_timestamp, &msg, 3 );
		}
		return;
	}
	break;

	case ControllerType::ChannelPressure:
	{
		int quantizedControllerValue = (int) ( 0.5f + p_normalised * 127.0f );
		if( quantizedControllerValue != *lastMidiValue )
		{
			*lastMidiValue = quantizedControllerValue;
			int chan = max( (short)0, midi_channel ); // chan -1 -> chan 0
			int msg = ( ( quantizedControllerValue & 0x7f ) << 8 ) + CHANNEL_PRESSURE + chan;
			Wake(); // ensure SampleClock() correct
			last_out_timestamp = SampleClock();
			midi_out.Send( last_out_timestamp, &msg, 3 );
		}
		return;
	}
	break;

	case ControllerType::Bender:
	{
		int quantizedControllerValue = (int) ( 0.5f + p_normalised * 16383.0f );
		if( quantizedControllerValue != *lastMidiValue )
		{
			*lastMidiValue = quantizedControllerValue;

			int chan = max( (short)0, midi_channel ); // chan -1 -> chan 0
			int msg = (( quantizedControllerValue << 9 ) & 0x7f0000 ) + ( ( quantizedControllerValue & 0x7f ) << 8 ) + PITCHBEND + chan;
			Wake(); // ensure SampleClock() correct
			last_out_timestamp = SampleClock();
			midi_out.Send( last_out_timestamp, &msg, 3 );
		}
		return;
	}
	break;

	case ControllerType::SYSEX:
	{
		unsigned char outBuffer[MAX_SYSEX_SIZE];
		int outBufferIdx = 0;
		int inBufferIdx = 0;
		bool highNybble = true;
		int quantizedControllerValue = INT_MAX;

		while( sysexFormat[inBufferIdx] != 0 && outBufferIdx < MAX_SYSEX_SIZE )
		{
			wchar_t c = sysexFormat[inBufferIdx];
			int nybble = -1;

			if( c >= L'0' && c <= L'9' )
			{
				nybble = c - L'0';
			}

			if( c >= L'A' && c <= L'F' )
			{
				nybble = 10 + c - L'A';
			}

			// CC Value - 7-bit.
			if( c == L'v' )
			{
				// no. 7-bit only					int val = (int) ( 0.5f + p_normalised * 255.0f );
				quantizedControllerValue = (int) ( 0.5f + p_normalised * 127.0f );

				//	_RPT1(_CRT_WARN, "%d\n", val );
				if( highNybble )
				{
					nybble = (quantizedControllerValue >> 4) & 0x07;
				}
				else
				{
					nybble = quantizedControllerValue & 0x0f;
				}
			}

			// Value - 14-bit MSB.
			if( c == L'M' )
			{
				const float conv = (float) (1 << 14) - 1;
				quantizedControllerValue = (int) ( 0.5f + p_normalised * conv );
				int MSB = (quantizedControllerValue >> 7);

				if( highNybble )
				{
					nybble = (MSB >> 4) & 0x07;
				}
				else
				{
					nybble = MSB & 0x0f;
				}
			}

			// Value - 14-bit LSB.
			if( c == L'L' )
			{
				const float conv = (float) (1 << 14) - 1;
				quantizedControllerValue = (int) ( 0.5f + p_normalised * conv );
				int LSB = quantizedControllerValue;

				if( highNybble )
				{
					nybble = (LSB >> 4) & 0x07;
				}
				else
				{
					nybble = LSB & 0x0f;
				}
			}

			/* ??
							// Voice (MIDI Key Num)
							if( c == L'k' )
							{
								int val = (int) ( 0.5f + p_normalised * 255.0f );
								if( highNybble )
								{
									nybble = val >> 4;
								}
								else
								{
									nybble = val & 0x0f;
								}
							}
			*/
			// MIDI Channel
			if( c == L'm' )
			{
				nybble = chan & 0x0f;
			}

			// Roland Checksum.
			if( c == L'S' )
			{
				int val = 0;

				for( int i = 5 ; i < outBufferIdx; ++i )
				{
					val += outBuffer[i];
				}

				val = 0x80 - (val & 0x7f);

				//	_RPT1(_CRT_WARN, "%d\n", val );
				if( highNybble )
				{
					nybble = (val >> 4) & 0x07;
				}
				else
				{
					nybble = val & 0x0f;
				}
			}

			if( nybble >= 0 )
			{
				if( highNybble )
				{
					outBuffer[outBufferIdx] = nybble << 4;
				}
				else
				{
					outBuffer[outBufferIdx] = outBuffer[outBufferIdx] | nybble;
					++outBufferIdx;
				}

				highNybble = !highNybble;
			}

			++inBufferIdx;
		}

		if( quantizedControllerValue != *lastMidiValue || quantizedControllerValue == INT_MAX ) // INT_MAX = controller value not used.
		{
			*lastMidiValue = quantizedControllerValue;
			Wake(); // ensure SampleClock() correct
			last_out_timestamp = SampleClock();
			midi_out.Send( last_out_timestamp, outBuffer, outBufferIdx );
		}
		return;
	}
	break;

	case ControllerType::VelocityOn:
	{
		voiceVelocityOn[voiceId] = p_normalised;
		//_RPTW2(_CRT_WARN, L"voiceVelocityOn[%d] %f\n", voiceId, p_normalised );
		return;
	}
	break;

	case ControllerType::VelocityOff:
	{
		voiceVelocityOff[voiceId] = p_normalised;
		return;
	}
	break;

	case ControllerType::Pitch:
	{
		// TODO: send MIDI SYSEX tuning commands.
		voicePitch[voiceId] = p_normalised;
		//_RPTW2(_CRT_WARN, L"voicePitch[%d] %f\n", voiceId, p_normalised );
		return;
	}
	break;

	case ControllerType::Gate:
	{
		int MidiNoteNumber = (int) (0.5f + voicePitch[voiceId] * 127.0f);
		// 7-bit velocity.
		float Velocityf;

		int quantizedControllerValue;

		if( p_normalised > 0.0f ) // Note-on.
		{
			quantizedControllerValue = 127;
			Velocityf = voiceVelocityOn[MidiNoteNumber];
			//_RPTW1(_CRT_WARN, L"Note-on  V%f", Velocityf );
		}
		else
		{
			quantizedControllerValue = 0;
			Velocityf = voiceVelocityOff[MidiNoteNumber];
			//_RPTW1(_CRT_WARN, L"Note-off V%f", Velocityf );
		}

		// Ensure we don't send repeated note-ons or repeated note-offs.
		if (quantizedControllerValue != *lastMidiValue)
		{
			*lastMidiValue = quantizedControllerValue;

			int Velocity = (int) (0.5f + Velocityf * 127.0f);
			//			_RPTW1(_CRT_WARN, L" NN %d\n", MidiNoteNumber );
			int msg = ( Velocity << 8 ) + MidiNoteNumber;
			int chan = max( (short)0, midi_channel ); // chan -1 -> chan 0
			msg = ( msg << 8 ) + chan;

			if( p_normalised > 0.0f )
			{
				msg |= NOTE_ON;
			}
			else
			{
				msg |= NOTE_OFF;
			}

			Wake(); // ensure SampleClock() correct
			last_out_timestamp = SampleClock();
			midi_out.Send( last_out_timestamp, &msg, 3 );
		}

		return;
	}
	break;

	default:
		//todo		assert(false);
		return;
	};

	assert(controller_id >= 0);

	if( !GetPlug(PLG_MIDI_OUT)->InUse() || (flags & UGF_OPEN) == 0 )// can't send a prog change before all open()
		return;

	Wake(); // ensure SampleClock() correct.
	timestamp_t p_clock = SampleClock();

	//	_RPT3(_CRT_WARN, "SendAutomation CTR %d, Val %f  N/RPN %d \n", controller_id, value, nrpn );
	// although theoretically posible for several sliders to send unsorted controller
	// messages all at once, it is not likely (user would need to be very fast with mouse)
	// so to avoid having to sort messages, simply ignore any out of order ones.
	// (can't use as pass though though, as a sequencer could generate a lot of controllers)
	if( last_out_timestamp > p_clock )
		return;

	last_out_timestamp = p_clock;

	int quantizedControllerValue;

	if( NRPN_MSB == controller_id || RPN_MSB == controller_id )
	{
		// Avoid flooding MIDI with unness RPN headers.
		if( lastRpnHeader_ != p_unified_controller_id )
		{
			// send registered parameter header first
			SendController(p_clock, controller_id, rpn );
			lastRpnHeader_ = p_unified_controller_id ;
		}

		// Now actual value.
		controller_id = RPN_CONTROLLER;
		// RPN use full 14 bit range.
		quantizedControllerValue = (int) ( 0.5f + p_normalised * (float) MAX_FULL_CNTRL_VAL );
	}
	else
	{
		// CC use 7 bit range.
		quantizedControllerValue = (int) ( 0.5f + p_normalised * 127.0f );
		// remove 2-byte support, zero low 7 bits.
		quantizedControllerValue = quantizedControllerValue << 7;
	}

	// Avoid sending same controller value twice (when parameter changed only a fraction).
	if( quantizedControllerValue != *lastMidiValue )
	{
		*lastMidiValue = quantizedControllerValue;
		SendController( p_clock, controller_id, quantizedControllerValue );
	}
}

void ug_patch_automator_out::SendController(timestamp_t p_clock, short controller_id, short val)
{
	// removed 2-byte support, except for RPN
	short low_bits_controller_id = -1;

	// transmit NPRN number if nesc
	if( 99 == controller_id || 101 == controller_id )
	{
		low_bits_controller_id = controller_id - 1;
	}

	// some controllers use 2 bytes
	// Correct, but unliked. if( controller_id < 32 ) // ALl official 2-byte controllers.
	if( 6 == controller_id ) // RPN DATA ENTRY SLIDER.
	{
		low_bits_controller_id = controller_id + 32;
	}

	short hi_bits = ( val >> 7 ) & 0x7f;	// hi  7 bits
	short low_bits = val & 0x7f;			// low 7 bits
	int msg = ( hi_bits << 8 ) + controller_id;
	int chan = max( (short)0, midi_channel/*midi_in.channel()*/ ); // chan -1 -> chan 0
	msg = ( msg << 8 ) + CONTROL_CHANGE + chan;
	midi_out.Send( p_clock, &msg, 3 );

	if( low_bits_controller_id > 0 )
	{
		msg = ( low_bits << 8 ) + low_bits_controller_id;
		msg = ( msg << 8 ) + CONTROL_CHANGE + chan;
		midi_out.Send( p_clock, &msg, 3 );
	}
}

void ug_patch_automator_out::sendProgramChange( int program )
{
	Wake(); // very nesc to update sample clock.
	assert(program >=0 && program < 128 );
	int msg = ( program << 8 ) + PROGRAM_CHANGE;
	midi_out.Send( SampleClock(), &msg, 3 );
}

int ug_patch_automator_out::Open()
{
	//Debug Identify();
	//_RPT1(_CRT_WARN, "(Out) SortOrder=%d\n", GetSortOrder() );
	midi_out.SetUp( GetPlug(PLG_MIDI_OUT) );
	return ug_base::Open();
}
#if defined( _DEBUG )

struct MidiMessage
{
	int channel;
	int voice;
	int controller;
	int value;
};

void test()
{
	MidiMessage msg;
	// Message 1.
	msg.channel     = 0;
	msg.voice       = 0x3C;       // Key#60
	msg.controller  = 0x21000000; // On Velocity.
	msg.value       = 0x64000000; // 100
	// Message 2.
	msg.channel     = 0;
	msg.voice       = 0x3C;       // Key#60
	msg.controller  = 0x20000000; // Key On/Off.
	msg.value       = 0xFFFFFFFF; // 100%
	msg.channel     = 0;
	msg.voice       = 0x3C;       // Key#60
	msg.controller  = 0x20000000; // Pitch.
	msg.value       = 0x80000000; // Middle-A 440Hz
}

#endif

