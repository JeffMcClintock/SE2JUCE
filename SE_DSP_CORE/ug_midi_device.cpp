// Base class for unit gens that have a MIDI input
#include "pch.h"
#include "ug_midi_device.h"
#include "midi_defs.h"
#include "ug_event.h"

ug_midi_device::ug_midi_device():
	midi_channel(-1)
{
}

// Fill an array of InterfaceObjects with plugs and parameters
void ug_midi_device::ListInterface2(InterfaceObjectArray& PList)
{
	ug_base::ListInterface2(PList);	// Call base class
	// IO Var, Direction, Datatype, NoteSource, ppactive, Default, defid (index into ug_base::PlugFormats)
	LIST_VAR3( L"Channel", midi_channel/*midi_in.channel()*/, DR_IN, DT_ENUM , L"-1", MIDI_CHAN_LIST,IO_IGNORE_PATCH_CHANGE|IO_POLYPHONIC_ACTIVE, L"MIDI Channel");
	LIST_VAR3N( L"MIDI In", DR_IN, DT_MIDI2 , L"0", L"", 0, L"");
}

int ug_midi_device::Open()
{
	SET_CUR_FUNC( &ug_base::process_sleep );
	return ug_base::Open();
}

void ug_midi_device::HandleEvent(SynthEditEvent* e)
{
	// send event to ug
	switch( e->eventType )
	{
	case UET_EVENT_MIDI:
	{
		// re-instated to allow patch automator MIDI-Chan to work (was accepting everything)
		if(e->parm2 < 4)
		{
			// filter out non-relevant events. Less events = less subdivision of audio block
			// 0xF0 indicates a system msg (clocks etc)
			if(midi_channel != -1) // -1 = All channels
			{
				const unsigned char* midiBytes = (const unsigned char*)e->Data();
				const short chan = (short)midiBytes[0] & 0x0f;
				const bool is_system_msg = (midiBytes[0] & SYSTEM_MSG) == SYSTEM_MSG;

				if (chan != midi_channel && !is_system_msg) // -1 = All channels
				{
					return;
				}
			}
		}

		OnMidiData( e->parm2, (unsigned char*) e->Data());
	}
	break;

	default:
		ug_base::HandleEvent( e );
	};
}

void ug_midi_device::OnMidiData( int size, unsigned char* /*midi_bytes*/ )
{
}
