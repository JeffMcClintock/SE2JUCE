#ifndef NOTEEXPRESSION_H_INCLUDED
#define NOTEEXPRESSION_H_INCLUDED

#include "../se_sdk3/mp_sdk_audio.h"
#include "../se_sdk3/smart_audio_pin.h"

class NoteExpression : public MpBase2
{
public:
	NoteExpression();
	virtual int32_t MP_STDCALL open();

	void subProcess(int sampleFrames);
	void onSetPins() override;

private:
	FloatInPin inPins[8];
	SmartAudioPin outPins[8];
};

#endif