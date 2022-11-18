#ifndef MIDILOGGER_H_INCLUDED
#define MIDILOGGER_H_INCLUDED

#include "mp_sdk_audio.h"

class MidiLogPlayback : public MpBase
{
public:
	MidiLogPlayback( IMpUnknown* host );
	~MidiLogPlayback();
	void onSetPins() override;
	void onMidiMessage( int pin, unsigned char* midiMessage, int size ) override;
	void subProcess( int bufferOffset, int sampleFrames );
	void readMessage();

private:
	StringInPin pinFileName;
	MidiOutPin pinMidi;

	int sampleClock;
	FILE* inputStream;
	int timestamp;
	int byteCount;
	unsigned char midiMessage[100];
};

#endif

