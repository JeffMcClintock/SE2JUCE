// Copyright 2006 Jeff McClintock

#pragma once

#include "../../modules/se_sdk3/mp_sdk_audio.h"

class VoiceMute: public MpBase
{
public:
	VoiceMute(IMpUnknown* host);

	// overrides
	int32_t MP_STDCALL open() override;

	// methods
	void subProcess3SamplePreGate(int bufferOffset, int sampleFrames);
	void subProcess(int bufferOffset, int sampleFrames);
	void subProcessMuting(int bufferOffset, int sampleFrames);
	void subProcessSilence(int bufferOffset, int sampleFrames);
	void onSetPins() override;  // one or more pins_ updated.  Check pin update flags to determin which ones.

private:
	AudioInPin pinInput1;
	FloatInPin pinVoiceActive;
	BoolInPin pinLegacyMode;
	AudioOutPin pinOutput1;
	int gainIndex_;
	float* currentfadeCurve;
	float* fadeCurveFast;
	float* fadeCurveSlow;
	int fadeSamplesFast;
	int fadeSamplesSlow;
	int startupDelayCount;
	bool previousActiveState;

#if defined( _DEBUG )
	int physicalVoiceNumber;
	int64_t clock_;
#endif
};

