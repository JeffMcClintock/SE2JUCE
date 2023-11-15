#pragma once

#include "mp_sdk_audio.h"
#include "mp_midi.h"

class GuitarDechannelizer : public MpBase
{
public:
	GuitarDechannelizer( IMpUnknown* host );
	void onMidiMessage(int pin, unsigned char* midiMessage, int size) override;
	void onMidi2Message(const gmpi::midi::message_view& msg);

private:
	void SendPitch(int keyNumber);

	MidiInPin pinMIDIIn;
	MidiOutPin pinMIDIOut;

	static const int VirtualVoiceCount = 128;
	static const int MidiChannelCount = 16;
	static const int MidiKeyCount = 128;
	int stringKeyNumber[MidiChannelCount];
	float bender[MidiChannelCount] = {};
	float benderRange[MidiChannelCount];
	gmpi::midi_2_0::MidiConverter2 midiConverter;

	float tuningTable[MidiChannelCount][MidiKeyCount];
	float tuningOut[MidiChannelCount];
};

