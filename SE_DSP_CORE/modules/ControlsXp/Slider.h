#ifndef SLIDER_H_INCLUDED
#define SLIDER_H_INCLUDED

#include "../se_sdk3/mp_sdk_audio.h"
#include "../se_sdk3/smart_audio_pin.h"

class Slider : public MpBase2
{
public:
	Slider();
	int32_t MP_STDCALL open()
	{
		pinValueOut.setCurveType(SmartAudioPin::LinearAdaptive); // Automatic.
		return MpBase2::open();
	}
	void subProcess(int sampleFrames);
	virtual void onSetPins();

private:
	FloatInPin pinValueIn;
	SmartAudioPin pinValueOut;
};

#endif

