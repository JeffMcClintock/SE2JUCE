#include "../se_sdk3/mp_sdk_audio.h"

using namespace gmpi;

class Gain : public MpBase2
{
	AudioInPin pinInput1;
	AudioInPin pinInput2;
	AudioOutPin pinOutput1;
	FloatInPin pinGain;

public:
	Gain()
	{
		initializePin(pinInput1);
		initializePin(pinInput2);
		initializePin(pinOutput1);
		initializePin(pinGain);
	}

	void subProcess(int sampleFrames)
	{
		// get pointers to in/output buffers.
		auto input1_ptr = getBuffer(pinInput1);
		auto input2_ptr = getBuffer(pinInput2);
		auto output1_ptr = getBuffer(pinOutput1);

		for (int s = sampleFrames; s > 0; --s)
		{
			float input1 = *input1_ptr;	// get the sample 'POINTED TO' by in1.
			float input2 = *input2_ptr;

			// Multiplying the two input's samples together.
			float result = input1 * input2;

			// store the result in the output buffer.
			*output1_ptr = result;

			// increment the pointers (move to next sample in buffers).
			++input1_ptr;
			++input2_ptr;
			++output1_ptr;
		}
	}

	void onSetPins() override
	{
		/*
		// LEVEL 1 - Simplest way to handle streaming status...
		// Do nothing. That's it. Module will not 'sleep'.
		*/

		// LEVEL 2 - Determin if output is silent or active, then notify downstream modules.
		//           Downstream modules can then 'sleep' (save CPU) when processing silence.

		// If either input is active, output will be active. ( "||" means "or" ).
		bool OutputIsActive = pinInput1.isStreaming() || pinInput2.isStreaming();

		// Exception...
		// If either input zero, output is silent.
		if (!pinInput1.isStreaming() && pinInput1 == 0.0f)
		{
			OutputIsActive = false;
		}

		if (!pinInput2.isStreaming() && pinInput2 == 0.0f)
		{
			OutputIsActive = false;
		}

		// Transmit new output state to modules 'downstream'.
		pinOutput1.setStreaming(OutputIsActive);


		// LEVEL 3 - control sleep mode manually (advanced).
		// Normally modules will automatically sleep when zero inputs and outputs are streaming.
		// This heuristic works well for most modules, including this module.
		// However some modules may have additional opportunities to sleep.
		// e.g. This particular module can sleep when the volume is zero,
		// regardless of the other input.
		setSleep(!OutputIsActive); // '!' means 'not'


		// Choose which function is used to process audio.
		// You can have one processing method, or several variations (each specialized for a certain condition).
		// For this example only one processing function needed.
		setSubProcess(&Gain::subProcess);
	}
};

namespace
{
	auto r = Register<Gain>::withId(L"SynthEdit Gain example V3");
}
