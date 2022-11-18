#pragma once
#include "ug_oversampler_io.h"

class ug_oversampler_in :
	public ug_oversampler_io
{
	static const int sincSize = 16;
	// sinc is quite 'ripply' which can be problematic on control signals, but has much better high freq response on audio.
	typedef UpsamplingInterpolator3<sincSize> InterpolatorType;
//	typedef UpsamplingInterpolator InterpolatorType; // seems to have quite a low-pass effect

	float* InterpolatorCoefs = {};
	float* InterpolatorCoefs_gaussian = {};
	std::vector<InterpolatorType> interpolators_;
	int settleSampleCount;

	//void calcSettleSamples()
	//{
	//	settleSampleCount = AudioMaster()->BlockSize() + InterpolatorType::calcLatency(oversampleFactor_) + sincSize;
	//}

public:

	DECLARE_UG_BUILD_FUNC(ug_oversampler_in);

	void calcLatency(int factor)
	{
		latencySamples = InterpolatorType::calcLatency(factor);
	}

	void OnFirstSample();
	void HandleEvent(SynthEditEvent* e) override;
	void TransmitInitialPinValues();
	void subProcessUpsample(int start_pos, int sampleframes);
	int Open() override;
	void OnOversamplerResume();
	int calcDelayCompensation() override;
};



