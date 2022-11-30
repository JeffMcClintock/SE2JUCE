#ifndef MIDIPLAYER1_H_INCLUDED
#define MIDIPLAYER1_H_INCLUDED

#include "MidiPlayer2.h"

class MidiPlayer1 : public MidiPlayer2
{
public:
	MidiPlayer1( IMpUnknown* host );
	void onSetPins() override;
	virtual bool loopEntireFile()
	{
		return pinLoopMode == 1;
	};

private:
	AudioInPin pinTempo;
	IntInPin pinLoopMode;
};

#endif

