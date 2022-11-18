// Copyright 2006 Jeff McClintock

#include "gain.h"
#include "it_enum_list.h"

REGISTER_PLUGIN( Gain, L"SynthEdit datatype test" );

Gain::Gain(IMpUnknown* host) : MpBase(host)
{
	initializePin(0, pinInput1);
	initializePin(1, pinInt);
	initializePin(2, pinFloat);
	initializePin(3, pinText);
	initializePin(4, pinBlob);

	initializePin(5, pinOutput1);
	initializePin(6, pinIntOut);
	initializePin(7, pinFloatOut);
	initializePin(8, pinTextOut);
	initializePin(9, pinBlobOut);
	initializePin(10, pinMidiIn);
	initializePin(11, pinMidiOut);

	// testing
	it_enum_list it(L"cat,dog=3,moose");
	it.First();
	it.Next();
}

// The most important function, processing the audio
void Gain::subProcess(int bufferOffset, int sampleFrames)
{
	float* in1  = bufferOffset + pinInput1.getBuffer();
	float* out1 = bufferOffset + pinOutput1.getBuffer();

	for(int s = sampleFrames ; s > 0 ; s--) // sampleFrames = how many samples to process (can vary). repeat (loop) that many times
	{
		*out1++ = *in1++;
	}
}

int32_t Gain::open()
{
	// choose which function is used to process audio
	SET_PROCESS(&Gain::subProcess);

	return MpBase::open();
}

// An input plug has changed value_
void Gain::onSetPins()
{
	int i = pinInt;
	float f = pinFloat;
	std::wstring s = pinText;

	if(pinInt.isUpdated())
	{
		_RPT1(_CRT_WARN, "INT in %d\n", i);
	}
	if(pinFloat.isUpdated())
	{
		_RPT1(_CRT_WARN, "FLOAT in %f\n", f);
	}
	if(pinText.isUpdated())
	{
		_RPTW1(_CRT_WARN, L"TEXT in RawSize %d\n", (int) pinText.rawSize() );
		_RPTW1(_CRT_WARN, L"TEXT in %s\n", s.c_str());
	}
	if(pinBlob.isUpdated())
	{
		_RPT0(_CRT_WARN, "BLOB in [");
		char* d = (char*) pinBlob.rawData();
		for(int i = 0 ; i < pinBlob.rawSize() ; i++)
		{
			_RPT1(_CRT_WARN, " %x", (int) d[i]);
		}
		_RPT0(_CRT_WARN, "]\n");
	}

	pinIntOut = pinInt;
	pinFloatOut = pinFloat;
	pinTextOut = pinText;
	pinTextOut = L"moose";
	pinTextOut.setValue( L"duck", 2 );
	pinBlobOut = pinBlob;
	if(pinBlobOut.rawSize() == 0)
	{
		char dat[] = "abc";
		pinBlobOut.setValueRaw(3,dat);
		pinBlobOut.sendPinUpdate();
	}
}

void Gain::onMidiMessage(int pin, unsigned char* midiMessage, int size)
{
	// size = 4 for short msg (4 or less), or > 4 for sysex
	_RPTW1(_CRT_WARN, L"onMidiMessage pin %d\n[", pin);
	for(int i = 0 ; i < size ; i++)
	{
		_RPT1(_CRT_WARN, " %2x", (int) midiMessage[i]);
	}
	_RPT0(_CRT_WARN, "]\n");

	// echo MIDI
	pinMidiOut.send(midiMessage, size);

	// test error
	//pinMidiIn.send(midiMessage, size);
}

void Gain::OnStreamingChange(int pin, bool is_streaming)
{
	pinOutput1.setStreaming(pinInput1.isStreaming());
}
