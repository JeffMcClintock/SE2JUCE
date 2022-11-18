#include "pch.h"
#include "ug_midi_filter.h"
#include "midi_defs.h"
#include "module_register.h"

SE_DECLARE_INIT_STATIC_FILE(ug_midi_filter)

namespace
{
REGISTER_MODULE_1_BC(92,L"MIDI Filter", IDS_MN_MIDI_FILTER,IDS_MG_MIDI,ug_midi_filter ,CF_STRUCTURE_VIEW,L"Filters MIDI data based on Channel, Velocity and Note information.  Use to create Keyboard Splits and Velocity Switching between Synths");
}

#define PN_MIDI_OUT 1

ug_midi_filter::ug_midi_filter() :
	// init the midi converter
	midiConverter(
		// provide a lambda to accept converted MIDI 2.0 messages
		[this](const gmpi::midi::message_view& msg, int /*offset*/)
		{
			onMidi2Message(msg);
		}
	)
{
}

// Fill an array of InterfaceObjects with plugs and parameters
void ug_midi_filter::ListInterface2(InterfaceObjectArray& PList)
{
	// IO Var, Direction, Datatype, Template, ppactive, Default,Range/enum list
	LIST_VAR3N( L"MIDI In",  DR_IN, DT_MIDI2 , L"", L"", 0, L"MIDI Input Plug");
	LIST_VAR3( L"MIDI Out",midi_out, DR_OUT, DT_MIDI2 , L"", L"", 0, L"MIDI Output Plug");
	LIST_VAR3( L"Channel Lo", channel_lo, DR_IN, DT_ENUM , L"1", L"range 1,16",IO_POLYPHONIC_ACTIVE, L"MIDI Channel Low");
	LIST_VAR3( L"Channel Hi", channel_hi, DR_IN, DT_ENUM , L"16", L"range 1,16",IO_POLYPHONIC_ACTIVE, L"MIDI Channel Hi");
	LIST_VAR3( L"Note Lo", note_num_lo, DR_IN, DT_ENUM , L"0", L"range 0,127",IO_POLYPHONIC_ACTIVE, L"Note Low");
	LIST_VAR3( L"Note Hi", note_num_hi, DR_IN, DT_ENUM , L"127", L"range 0,127",IO_POLYPHONIC_ACTIVE, L"Note Hi");
	LIST_VAR3( L"Velocity Lo", velocity_lo, DR_IN, DT_ENUM , L"0", L"range 0,127",IO_POLYPHONIC_ACTIVE, L"Velocity Low");
	LIST_VAR3(L"Velocity Hi", velocity_hi, DR_IN, DT_ENUM, L"127", L"range 0,127", IO_POLYPHONIC_ACTIVE, L"Velocity Hi");
	LIST_VAR3(L"Program Change", pinProgramChange, DR_IN, DT_ENUM, L"1", L"Off,On", 0, L"Program Change");
	LIST_VAR3(L"MIDI Excluded", midi_out2, DR_OUT, DT_MIDI2, L"", L"", 0, L"MIDI Output Plug");
}

// put your midi handling code in here.
void ug_midi_filter::onMidi2Message(const gmpi::midi::message_view& msg)
{
	const auto header = gmpi::midi_2_0::decodeHeader(msg);

	// only 8-byte messages supported. only 16 channels supported
	if (header.messageType != gmpi::midi_2_0::ChannelVoice64)
		goto fail_filter;

	if (header.channel < (channel_lo - 1) || header.channel  > (channel_hi - 1)) // -1 is to convert user range 1-16 to midi range 0-15
	{
		goto fail_filter;
	}

	switch (header.status)
	{
	case gmpi::midi_2_0::NoteOn:
	{
		const auto note = gmpi::midi_2_0::decodeNote(msg);
		const auto Midi1Velocity = (std::max)((uint8_t)1, gmpi::midi::utils::floatToU7(note.velocity));

		noteOnVelocities[header.channel][note.noteNumber] = Midi1Velocity;

		if (
			note.noteNumber < note_num_lo || note.noteNumber > note_num_hi ||
			Midi1Velocity < velocity_lo || Midi1Velocity > velocity_hi
			)
		{
			goto fail_filter;
		}
	}

	// fall through case
	case gmpi::midi_2_0::NoteOff:
	{
		const auto note = gmpi::midi_2_0::decodeNote(msg);
		const auto Midi1Velocity = noteOnVelocities[header.channel][note.noteNumber];

		if (
			note.noteNumber < note_num_lo || note.noteNumber > note_num_hi ||
			Midi1Velocity < velocity_lo || Midi1Velocity > velocity_hi
			)
		{
			goto fail_filter;
		}
	}
	break;

	case gmpi::midi_2_0::ProgramChange:
		if (pinProgramChange == 0)
		{
			goto fail_filter;
		}
		break;
	};

	// pass_filter:
	midi_out.Send(SampleClock(), msg.begin(), msg.size());
	return;

fail_filter:
	midi_out2.Send(SampleClock(), msg.begin(), msg.size());
}

void ug_midi_filter::OnMidiData( int size, unsigned char* midi_bytes )
{
	gmpi::midi::message_view msg((const uint8_t*)midi_bytes, size);

	// convert everything to MIDI 2.0
	midiConverter.processMidi(msg, -1);
}

