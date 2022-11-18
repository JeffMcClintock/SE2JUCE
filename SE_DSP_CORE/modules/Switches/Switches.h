#ifndef SWITCHFLOAT_H_INCLUDED
#define SWITCHFLOAT_H_INCLUDED

#include "../se_sdk3/mp_sdk_audio.h"
#include <vector>

template<class T>
class SwitchManyToOne : public MpBase2
{
public:
	SwitchManyToOne()
	{
		// Register pins.
		initializePin(pinChoice);
		initializePin(pinOutput);
	}
	int32_t MP_STDCALL open() override
	{
		// initialise auto-duplicate pins.
		const int numOtherPins = 2;
		gmpi_sdk::mp_shared_ptr<gmpi::IMpPinIterator> it;
		if (gmpi::MP_OK == getHost()->createPinIterator(it.getAddressOf()))
		{
			int32_t inPinCount;
			it->getCount(inPinCount);
			inPinCount -= numOtherPins; // calc number of input values (not counting "choice" anf "output")

			T initVal = {};
			pinInputs.assign(inPinCount, initVal);

			for (auto& p : pinInputs)
			{
				initializePin(p);
			}
		}

		return MpBase2::open();
	}

	void onSetPins() override
	{
		if (pinInputs.size() > 0 && (pinChoice.isUpdated() || pinInputs[pinChoice].isUpdated()))
		{
			pinOutput = pinInputs[pinChoice];
		}
	}

private:
	IntInPin pinChoice;
	MpControlPin<T, gmpi::MP_OUT> pinOutput;
	std::vector< MpControlPin<T, gmpi::MP_IN> > pinInputs;
};

template<class T>
class SwitchOneToMany : public MpBase2
{
public:
	SwitchOneToMany() : 
		prevOutput(-1)
	{
		// Register pins.
		initializePin(pinChoice);
		initializePin(pinInput);
	}
	int32_t MP_STDCALL open() override
	{
		// initialise auto-duplicate pins.
		const int numOtherPins = 2;

		gmpi_sdk::mp_shared_ptr<gmpi::IMpPinIterator> it;
		if (gmpi::MP_OK == getHost()->createPinIterator(it.getAddressOf()))
		{
			int32_t inPinCount;
			it->getCount(inPinCount);
			inPinCount -= numOtherPins; // calc number of input values (not counting "choice" anf "output")

			T initVal = {};
			pinOutputs.assign(inPinCount, initVal);

			for (auto& p : pinOutputs)
			{
				initializePin(p);
			}
		}

		return MpBase2::open();
	}

	void onSetPins() override
	{
		if (prevOutput > -1)
		{
			T initVal = {};
			pinOutputs[prevOutput] = initVal;
		}

		if(pinChoice < pinOutputs.size() ) // cope with zero outputs situation.
			pinOutputs[pinChoice] = pinInput;

		prevOutput = pinChoice;
	}

private:
	int prevOutput;
	IntInPin pinChoice;
	MpControlPin<T, gmpi::MP_IN> pinInput;
	std::vector< MpControlPin<T, gmpi::MP_OUT> > pinOutputs;
};
#endif

