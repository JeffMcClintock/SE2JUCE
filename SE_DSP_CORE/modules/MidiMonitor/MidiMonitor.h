#ifndef MIDIMONITOR_H_INCLUDED
#define MIDIMONITOR_H_INCLUDED

#include "../se_sdk3/mp_sdk_audio.h"

class MidiMonitor : public MpBase2
{
public:
	MidiMonitor( );
	virtual void onMidiMessage(int pin, const unsigned char* midiMessage, int size) override;

private:
	MidiInPin pinMIDIIn;
	IntInPin pinChannel;
};

#endif

