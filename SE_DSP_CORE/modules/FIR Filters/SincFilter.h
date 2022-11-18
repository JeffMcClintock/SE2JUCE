#ifndef FIRFILTERS_H_INCLUDED
#define FIRFILTERS_H_INCLUDED

#include <vector>
#include "../se_sdk3/mp_sdk_audio.h"

class SincFilterLpHp : public MpBase2
{
public:
	SincFilterLpHp();

	void subProcess(int sampleFrames);
	void subProcessStatic(int sampleFrames);
	virtual void onSetPins() override;

protected:
	virtual bool isHighPass() { return false; }

private:
	AudioInPin pinSignal;
	IntInPin pinTaps;
	FloatInPin pinCuttoffkHz;
	AudioOutPin pinOutput;

	std::vector<float>coefs;
	std::vector<float>hist;

	int staticCount;
};

class SincFilterHp : public SincFilterLpHp
{
protected:
	bool isHighPass() override{ return true; }
};

#endif

