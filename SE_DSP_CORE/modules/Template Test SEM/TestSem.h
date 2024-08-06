#ifndef TESTSEM_H_INCLUDED
#define TESTSEM_H_INCLUDED

#include "../se_sdk3/mp_sdk_audio.h"

class TestSem : public MpBase2
{
public:
	TestSem( );
	void subProcess( int sampleFrames );
	virtual void onSetPins(void) override;

private:
	BlobOutPin pinCaptureDataA;
	BlobOutPin pinCaptureDataB;
	AudioInPin pinSignalA;
	AudioInPin pinSignalB;
	FloatInPin pinVoiceActive;
	BoolOutPin pinpolydetect;
};

#endif

