#ifndef MIDITOCV2_H_INCLUDED
#define MIDITOCV2_H_INCLUDED

#include "modules/se_sdk3/mp_sdk_audio.h"
#include "modules/se_sdk3/smart_audio_pin.h" 

using namespace gmpi;

class MidiToCv2 : public MpBase2
{
public:
	MidiToCv2( );
	virtual int32_t MP_STDCALL open();
	void subProcess(int sampleFrames);
	virtual void onSetPins();

private:
	void CleanVelocityAndAftertouch();

	MidiInPin pinMIDIIn;
	IntInPin pinChannel;
	SmartAudioPin pinTrigger;
	SmartAudioPin pinGate;
	AudioOutPin pinPitch;
	SmartAudioPin pinVelocity;
	FloatInPin pinVoiceActive;
	FloatInPin pinVoiceGate;
	FloatInPin pinVoiceTrigger;
	FloatInPin pinVoiceVelocityKeyOn;
	FloatInPin pinVoicePitch;
	FloatInPin pinVoiceBender;
	SmartAudioPin pinAftertouchOut;
	FloatInPin pinVoiceAftertouch;
	FloatInPin pinChannelPressure;
	IntInPin pinVoiceVirtualVoiceId;
	FloatInPin pinBender;
	FloatInPin pinBenderRange;
	FloatInPin pinHoldPedal;
	FloatInPin pinGlideStartPitch;
	IntInPin pinVoiceAllocationMode;
	FloatInPin pinPortamento;

	RampGenerator pitchInterpolator_;
	RampGeneratorAdaptive benderInterpolator_;

	float previousGate_;
	float currentGate_;

	// by default use poly-aftertouch, use mono-aftertouch (channel-pressure) as a last-resort.
	bool monoAftertouchDetected = {};
	bool polyAftertouchDetected = {};
	bool usePolyAftertouch() const
	{
		return polyAftertouchDetected || !monoAftertouchDetected;
	}

#ifdef _DEBUG
	int debugVoice;
#endif
};

#endif

