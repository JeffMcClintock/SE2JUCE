#ifndef SAMPLELOADER2_H_INCLUDED
#define SAMPLELOADER2_H_INCLUDED

#include "mp_sdk_audio.h"

class SampleLoader2 : public MpBase
{
public:
	SampleLoader2( IMpUnknown* host );
	~SampleLoader2();
	void onSetPins() override;

private:
	StringInPin pinFilename;
	IntInPin pinBank;
	IntInPin pinPatch;
	IntOutPin pinSampleId;
	int sampleHandle;
};

#endif
