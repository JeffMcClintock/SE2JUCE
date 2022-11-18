#ifndef TESTMIDI_H_INCLUDED
#define TESTMIDI_H_INCLUDED

#include "mp_sdk_audio.h"

class TestMidi : public MpBase
{
public:
	TestMidi( IMpUnknown* host );
	void subProcess( int bufferOffset, int sampleFrames );

private:
	MidiOutPin pinMidiOut;
	int count;
	int key;
};

#endif

