// Base class for unit gens that control note creation
#include "pch.h"
#include <algorithm>
#include "ug_cv_midi.h"
#include "midi_defs.h"
#include "mp_midi.h"

#include <math.h>
#include "module_register.h"

SE_DECLARE_INIT_STATIC_FILE(ug_cv_midi);

using namespace std;

namespace
{
REGISTER_MODULE_1(L"Trigger to MIDI", IDS_MN_TRIGGER_TO_MIDI,IDS_MG_MIDI,ug_cv_midi ,CF_STRUCTURE_VIEW,L"Sends a MIDI note-on message whenever the 'Trigger' input goes over 0 Volts.");
}

#define PN_GATE 0
#define PN_PITCH 1
#define PN_VELOCITY 2
#define PN_CHANNEL 3
#define PN_MIDI_OUT 4

// Fill an array of InterfaceObjects with plugs and parameters
void ug_cv_midi::ListInterface2(InterfaceObjectArray& PList)
{
	// IO Var, Direction, Datatype, NoteSource, ppactive, Default, defid (index into unit_gen::PlugFormats)
	LIST_PIN2( L"Gate",gate_ptr,	DR_IN, L"0", L"", IO_POLYPHONIC_ACTIVE, L"Triggers the Note on message");
	LIST_PIN( L"Pitch",		DR_IN, L"5", L"", IO_POLYPHONIC_ACTIVE, L"Sets the note pitch");
	LIST_PIN( L"Velocity",	DR_IN, L"5", L"", IO_POLYPHONIC_ACTIVE, L"Sets the note velocity");
	LIST_VAR3( L"Channel", channel, DR_IN, DT_ENUM , L"0", MIDI_CHAN_LIST,IO_IGNORE_PATCH_CHANGE|IO_POLYPHONIC_ACTIVE, L"MIDI Channel");
	LIST_VAR3( L"MIDI Out", midi_out, DR_OUT, DT_MIDI2 , L"", L"", 0, L"");
	//	LIST_VAR3( L"Freq Scale", freq_scale, DR _PARAMETER, DT_ENUM   , L"0", L"1 Volt/Octave, 1 Volt/kHz ", 0, L"Switches between 1 Volt per Octave or 1 Volt per kHz response.");
	LIST_VAR3( L"Freq Scale", freq_scale, DR_IN, DT_ENUM   , L"0", L"1 Volt/Octave, 1 Volt/kHz ", IO_MINIMISED, L"Switches between 1 Volt per Octave or 1 Volt per kHz response.");
}

ug_cv_midi::ug_cv_midi() :
	gate_state(false)
{
}

int ug_cv_midi::Open()
{
	ug_base::Open();
	return 0;
}

void ug_cv_midi::sub_process(int start_pos, int sampleframes)
{
	float* gate	= gate_ptr + start_pos;
	int count = sampleframes;
	float* last = gate + sampleframes;
	timestamp_t start_sampleclock = SampleClock();

	while( count > 0 )
	{
		// duration till next gate change?
		if( gate_state )
		{
			while( *gate > 0.f && gate < last )
			{
				gate++;
			}
		}
		else
		{
			while( *gate <= 0.f && gate < last )
			{
				gate++;
			}
		}

		count = (int) (last - gate);

		if( count > 0 )
		{
			SetSampleClock( start_sampleclock + sampleframes - count);
			gate_changed();
		}
	}
}

void ug_cv_midi::onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state)
{
	Wake();

	if( p_to_plug == GetPlug(PN_GATE) )
	{
		if( GetPlug(PN_GATE)->getState() == ST_RUN )
		{
			SET_CUR_FUNC( &ug_cv_midi::sub_process );
		}
		else
		{
			gate_changed();
			SET_CUR_FUNC( &ug_base::process_sleep );
		}
	}
}

void ug_cv_midi::gate_changed()
{
	bool new_in_state = GetPlug(PN_GATE)->getValue() > 0.f;

	if( new_in_state != gate_state )
	{
		gate_state = new_in_state;
//		unsigned int midi_data;
		int chan = max( (short)0, channel ); // chan -1 -> chan 0

		if( gate_state )
		{
			//float vel_volts = GetPlug(PN_VELOCITY)->getValue();
			//vel_volts *= 127.0; // covert to
			//int vel = (int) vel_volts;
			//vel = min(vel, 127);
			//vel = max(vel, 1);
			float pitch_volts = GetPlug(PN_PITCH)->getValue() * 10.f;
			float note_num;

			if( freq_scale == 0 )
			{
				note_num = 0.5f + MIDDLE_A + (pitch_volts - 5.f) * 12.f;
			}
			else
			{
				constexpr float constA = 1000.f / 440.f;
				const float recipLog2 = 1.f / logf(2.f);
				float octave = logf( pitch_volts * constA ) * recipLog2;

				#if defined( _DEBUG )
					const float const1 = logf(440.f)/logf(2.f);
					const float inv_log2 = 1.f / logf(2.f);
					float octave2 = logf(pitch_volts * 1000.f) * inv_log2 - const1;
					assert( fabsf(octave2 - octave) < 0.0001f );
				#endif

				note_num = octave * 12.f + MIDDLE_A;
			}

			midi_note = (int) note_num;
			midi_note = min(midi_note, 127);
			midi_note = max(midi_note,0);
//			midi_data =  chan + NOTE_ON + (midi_note & 0x7f) * 0x100 + vel * 0x10000; // some note on

			const auto velocity = std::clamp(GetPlug(PN_VELOCITY)->getValue(), 1.0f / 127.0f, 1.0f);
			const auto out = gmpi::midi_2_0::makeNoteOnMessage(
				(midi_note & 0x7f),
				velocity,
				chan
			);

			midi_out.Send(SampleClock(), (const unsigned char*)&out, sizeof(out));
		}
		else
		{
//			int vel = 64; // note off velocity
//			midi_data =  chan + NOTE_OFF + (midi_note & 0x7f) * 0x100 + vel * 0x10000;

			const auto out = gmpi::midi_2_0::makeNoteOffMessage(
				(midi_note & 0x7f),
				0.5f,
				chan
			);

			midi_out.Send(SampleClock(), (const unsigned char*)&out, sizeof(out));
		}
	}
}
