// Copyright 2006 Jeff McClintock

#ifndef GAIN_H_INCLUDED
#define GAIN_H_INCLUDED

#include "mp_sdk_audio.h"

class Gain: public MpBase
{
public:
	Gain(IMpUnknown* host);

	// overrides
	virtual int32_t MP_STDCALL open();

	// methods
	void subProcess(int bufferOffset, int sampleFrames);
	virtual void onSetPins(void);
	virtual void OnStreamingChange(int pin, bool is_streaming);
	virtual void onMidiMessage(int pin, unsigned char* midiMessage, int size);
private:
	AudioInPin pinInput1;
	IntInPin pinInt;
	FloatInPin pinFloat;
	StringInPin pinText;
	BlobInPin pinBlob;
	MidiInPin pinMidiIn;

	AudioOutPin pinOutput1;
	IntOutPin pinIntOut;
	FloatOutPin pinFloatOut;
	StringOutPin pinTextOut;
	BlobOutPin pinBlobOut;
	MidiOutPin pinMidiOut;
};

#endif
