#ifndef LISTENTRY_H_INCLUDED
#define LISTENTRY_H_INCLUDED

#include "../se_sdk3/mp_sdk_audio.h"

class ListEntry : public MpBase2
{
public:
	ListEntry();
	void onSetPins() override;

private:
	IntInPin pinValueIn;
	IntOutPin pinValueOut;
};

#endif

