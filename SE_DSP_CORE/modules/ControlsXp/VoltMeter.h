#ifndef VOLTMETER_H_INCLUDED
#define VOLTMETER_H_INCLUDED

#include "../se_sdk3/mp_sdk_audio.h"

class VoltMeter : public MpBase2
{
public:
	VoltMeter( );
	void subProcess(int sampleFrames);
	void subProcessAc(int sampleFrames);
	virtual void onSetPins() override;
	void UpdateGui();

private:
	AudioInPin pinSignalin;
	IntInPin pinMode;
	IntInPin pinUpdateRate;
	FloatOutPin pinpatchValue;

	int count;
	float last_value = -123456.789f; // unlikely value
	float m_value;
};

#endif

