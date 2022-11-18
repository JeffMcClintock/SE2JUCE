#ifndef VOLTSTOMIDICC_H_INCLUDED
#define VOLTSTOMIDICC_H_INCLUDED

#include "../se_sdk3/mp_sdk_audio.h"

class VoltsToMidiCc : public MpBase
{
	uint32_t currentCcValue = 8; // an out-of-band value to force initial update. (becuase floatToU32 duplicate lower 2 bits).

public:
	VoltsToMidiCc( IMpUnknown* host );
	void subProcess( int bufferOffset, int sampleFrames );
	virtual void onSetPins() override;

private:
	AudioInPin pinValue;
	IntInPin pinChannel;
	MidiOutPin pinMIDIOut;
	IntInPin pinMidiCc;

	void SendValue( int bufferPosition = -1 );
};

#endif

