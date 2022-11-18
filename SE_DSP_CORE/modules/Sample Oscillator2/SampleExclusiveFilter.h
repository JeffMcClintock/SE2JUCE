#ifndef SAMPLEEXCLUSIVEFILTER_H_INCLUDED
#define SAMPLEEXCLUSIVEFILTER_H_INCLUDED

#include "mp_sdk_audio.h"

class SampleExclusiveFilter : public MpBase
{
public:
	SampleExclusiveFilter( IMpUnknown* host );
	void onMidiMessage( int pin, unsigned char* midiMessage, int size ) override;

private:
	MidiInPin pinMidiIn;
	MidiOutPin pinMidiOut;
};

#endif

