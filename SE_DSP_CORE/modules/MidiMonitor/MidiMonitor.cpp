#include "./MidiMonitor.h"

REGISTER_PLUGIN2 ( MidiMonitor, L"EA MIDI Monitor" );

MidiMonitor::MidiMonitor( )
{
	// Register pins.
	initializePin( pinMIDIIn );
	initializePin( pinChannel );
}

void MidiMonitor::onMidiMessage(int pin, const unsigned char * midiMessage, int size)
{
	getHost()->sendMessageToGui(0, size, midiMessage);
}


