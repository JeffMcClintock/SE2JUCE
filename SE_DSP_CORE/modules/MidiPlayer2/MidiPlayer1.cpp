#include "./MidiPlayer1.h"

REGISTER_PLUGIN ( MidiPlayer1, L"MIDI Player" );

MidiPlayer1::MidiPlayer1( IMpUnknown* host ) : MidiPlayer2( host, false )
{
	// Register pins.
	initializePin( 0, pinFileName );
	initializePin( 1, pinTempo );
	initializePin( 2, pinMIDIOut );
	initializePin( 3, pinTempoFrom ); // in this case means IgnoreMIDItempoChanges.
	initializePin( 4, pinLoopMode );

	gate_state = trigger_state = true;// Older version don't have these pins, just pretent they're 'on'.
}

void MidiPlayer1::onSetPins()
{
	if( pinFileName.isUpdated() )
	{
		if( loadMidiFile() == 0 )
		{
			MidiClockStart(-1);
		}
	}

	// Setting tempo depends on PPQN value from MIDI file, so this MUST be done after pinFileName.
	if( pinTempo.isUpdated() || pinFileName.isUpdated())
	{
		Setbpm( pinTempo * 10.0f );
	}

	// Set sleep mode.
	setSleep(false);
}
