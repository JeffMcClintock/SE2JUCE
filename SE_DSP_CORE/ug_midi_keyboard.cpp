#include "pch.h"
#include "ug_midi_keyboard.h"
#include "midi_defs.h"
#include "module_register.h"
#include "conversion.h"
#include "my_input_stream.h"

namespace
{
REGISTER_MODULE_1(L"KeyBoard", IDS_MN_KEYBOARD,IDS_MG_OLD,ug_midi_keyboard ,CF_STRUCTURE_VIEW|CF_PANEL_VIEW,L"On-Screen MIDI keyboard.  Can be played with mouse, or from PC keyboard ( L'Q' is Middle-C, L'Z' two Octaves lower)  The 'T' symbol sets 'Toggle Mode' where each key stays held untill you click it a second time.");
}

#define PLG_MIDI_OUT 1

// Fill an array of InterfaceObjects with plugs and parameters
void ug_midi_keyboard::ListInterface2(InterfaceObjectArray& PList)
{
	// IO Var, Direction, Datatype, MIDI In, Default, defid (index into ug_base::PlugFormats)
	//                                                   extra unnesc entry makes it's compatible with other Chan pins.
	LIST_VAR3( L"Channel", channel, DR_IN, DT_ENUM , L"0", MIDI_CHAN_LIST, IO_IGNORE_PATCH_CHANGE|IO_POLYPHONIC_ACTIVE, L"MIDI Channel");
	LIST_VAR3( L"MIDI Out", midi_out, DR_OUT, DT_MIDI2 , L"", L"", 0, L"");
}

ug_midi_keyboard::ug_midi_keyboard()
{
	SET_CUR_FUNC( &ug_midi_keyboard::sub_process );
}

void ug_midi_keyboard::OnMidiData(unsigned int midi_data, timestamp_t timeStamp)
{
	buffer.push_back( midi_data );
}

// Compatibility with "SE Keyboard (MIDI)" which may be substituted in DX GUIs.
// MIDI data is sent via standard "sdk" message.
void ug_midi_keyboard::OnUiNotify2(int p_msg_id, my_input_stream& p_stream)
{
	ug_base::OnUiNotify2(p_msg_id, p_stream);

	if (p_msg_id == id_to_long("sdk"))
	{
		int size, user_msg_id;
		p_stream >> user_msg_id;
		p_stream >> size;

		unsigned char data[16];
		assert(size < sizeof(data));
		p_stream.Read(data, size);

		int chan = (std::max)(0, (int)channel); // chan -1 -> chan 0
		int midi_data = chan + data[0] + data[1] * 0x100 + data[2] * 0x10000; // some note on
		buffer.push_back(midi_data);
	}
}


void ug_midi_keyboard::OnNoteOn(int p_note_num)
{
	assert( p_note_num >= 0 && p_note_num < 128 );
	Wake();
	SET_CUR_FUNC( &ug_midi_keyboard::sub_process );
	int vel = 90;
	int chan = std::max( 0, (int)channel ); // chan -1 -> chan 0
	int midi_data = chan + NOTE_ON + (p_note_num & 0x7f) * 0x100 + vel * 0x10000; // some note on
	buffer.push_back( midi_data );
	/*
		if( current_note > -1 )
		{
			int vel = 64;
			DWORD midi_data = chan + NOTE_OFF + (p_note_num & 0x7f) * 0x100 + vel * 0x10000; // some note on
			buffer.AddTail( midi_data );
		}*/
	//	output_change( midi_out, ST_ONE_OFF );
	//	current_note = midi_note;
}

void ug_midi_keyboard::OnNoteOff(int p_note_num)
{
	assert( p_note_num >= 0 && p_note_num < 128 );
	Wake();
	SET_CUR_FUNC( &ug_midi_keyboard::sub_process );
	int vel = 64;
	int chan = std::max( 0, (int)channel ); // chan -1 -> chan 0
	int midi_data = chan + NOTE_OFF + (p_note_num & 0x7f) * 0x100 + vel * 0x10000; // some note on
	buffer.push_back( midi_data );
	//	current_note = -1;
}

void ug_midi_keyboard::sub_process(int start_pos, int sampleframes)
{
	MidiEvent me;

	while( !buffer.empty() )
	{
		me.dwEvent = buffer.front();
		buffer.pop_front();
		//me.dwDeltaTime = SampleClock();
		//midi_out.AddCircular( me );
		midi_out.Send( SampleClock(), (unsigned char*) &(me.dwEvent), 3 );
		//		GetPlug(PLG_MIDI_OUT)->WakeDownstream();
	}

	SleepMode();
}
