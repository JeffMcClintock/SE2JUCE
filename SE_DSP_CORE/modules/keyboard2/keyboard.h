// Copyright 2006 Jeff McClintock

#ifndef waveshaper_dsp_H_INCLUDED
#define waveshaper_dsp_H_INCLUDED

#include "mp_sdk_audio.h"
#include "smart_audio_pin.h" 

class Keyboard: public MpBase
{
public:
	Keyboard(IMpUnknown* host);

	// overrides
	virtual int32_t MP_STDCALL open();

	// methods
	void subProcess( int bufferOffset, int sampleFrames );
	void onSetPins();
private:
	// pins_
	SmartAudioPin pinGateOut;
	AudioOutPin pinPitchOut;
	SmartAudioPin pinTriggerOut;
	SmartAudioPin pinVelocityOut;
	SmartAudioPin pinAftertouchOut;

	FloatInPin pinGate;
	FloatInPin pinPitch;
	FloatInPin pinHoldPedal;
	FloatInPin Bender;
	FloatInPin pinVelocity;
	FloatInPin pinVelocityOff;
	FloatInPin pinAftertouch;
	FloatInPin	pinTrigger;
	FloatInPin	pinVoiceActive;
	IntInPin	GlideType;
	AudioInPin	GlideRate;
	AudioInPin	BenderRange;

	FloatOutPin pinRawPitchOut;
	FloatOutPin pinRawVelocityOut;
	FloatOutPin pinRawVelocityOffOut;

	float previousGate_;

	RampGenerator pitchInterpolator_;
	RampGenerator benderInterpolator_;

	float currentGate_;
};

#endif