#ifndef GUITARDECHANNELIZER_H_INCLUDED
#define GUITARDECHANNELIZER_H_INCLUDED

#include "mp_sdk_audio.h"
#include "hasMidiTuning.h"

class GuitarDechannelizer : public MpBase, public hasMidiTuning
{
public:
	GuitarDechannelizer( IMpUnknown* host );
	void onMidiMessage(int pin, unsigned char* midiMessage, int size) override; // size < 4 for short msg, or > 4 for sysex
	void OnKeyTuningChanged(int p_clock, int MidiNoteNumber, int tune) override;

private:
	void SendPitch(int keyNumber);

	MidiInPin pinMIDIIn;
	MidiOutPin pinMIDIOut;

	static const int VirtualVoiceCount = 128;
	static const int MidiChannelCount = 16;
	static const int MidiKeyCount = 128;
	int stringKeyNumber[MidiChannelCount];
	int bender[MidiChannelCount];
	int benderRange[MidiChannelCount];
	int incoming_rpn;
};

#endif

