#include "pch.h"
#include "ug_logic_counter.h"
#include "Logic.h"
#include "module_register.h"
#include "./modules/se_sdk3/mp_sdk_audio.h"

using namespace gmpi;

#define PN_INPUT 0
#define PN_RESET 1

SE_DECLARE_INIT_STATIC_FILE(ug_logic_counter)
SE_DECLARE_INIT_STATIC_FILE(StepSequencer2);
SE_DECLARE_INIT_STATIC_FILE(StepSequencer3);

namespace
{
REGISTER_MODULE_1_BC(125,L"Step Counter2", IDS_MN_STEP_COUNTER2,IDS_MG_LOGIC,ug_logic_counter2,CF_STRUCTURE_VIEW,L"When clocked, each output in turn goes HI (5 Volts). Similar to decade counter, but can be configured to any number of outputs");
REGISTER_MODULE_1_BC(93,L"Step Counter", IDS_MN_STEP_COUNTER,IDS_MG_DEBUG,ug_logic_counter ,CF_STRUCTURE_VIEW,L"");
}

void ug_logic_counter::ListInterface2(InterfaceObjectArray& PList)
{
	ug_base::ListInterface2(PList);	// Call base class
	// IO Var, Direction, Datatype, Name, Default, defid (index into unit_gen::PlugFormats)
	// defid used to name a enum list or range of values
	LIST_PIN( L"Clock", DR_IN, L"", L"", IO_POLYPHONIC_ACTIVE, L"");
	LIST_PIN( L"Reset", DR_IN, L"", L"", IO_POLYPHONIC_ACTIVE, L"");
	LIST_PIN( L"Out", DR_OUT, L"", L"", IO_AUTODUPLICATE, L"");
}

ug_logic_counter::ug_logic_counter() :
	cur_count(0)
	,cur_reset(false)
	,cur_input(false)
{
}

int ug_logic_counter::Open()
{
	ug_logic_complex::Open();
	max_count = (int) plugs.size() - 2;

	if( max_count > 0 )
	{
		outputs[0].Set( SampleClock(), 0.5f ); // set first output high
	}

	return 0;
}

// OLD VERSION (see below for new)
void ug_logic_counter::input_changed()
{
	int prev_count = cur_count;

	if( check_logic_input( GetPlug(PN_RESET)->getValue(), cur_reset ) )
	{
		if( cur_reset == true )
		{
			cur_count = 0;
		}
	}

	// increment count on clock
	if( check_logic_input( GetPlug(PN_INPUT)->getValue(), cur_input ) )
	{
		if( cur_input == true && cur_reset == false )
		{
			cur_count++;

			if( cur_count >= max_count ) // must be >= for case when there are zero outputs
			{
				cur_count = 0;
			}
		}
	}

	if( prev_count != cur_count )
	{
		outputs[prev_count].Set( SampleClock(), 0.f );
		outputs[cur_count].Set( SampleClock(), 0.5f );
	}
}

// NEW VERSION (see above for old)
void ug_logic_counter2::input_changed()
{
	int prev_count = cur_count;

	if( check_logic_input( GetPlug(PN_RESET)->getValue(), cur_reset ) )
	{
		if( cur_reset == true )
		{
			cur_count = 0;
		}
	}

	// increment count on clock
	if( check_logic_input( GetPlug(PN_INPUT)->getValue(), cur_input ) )
	{
		// don't want to step count on sample zero (better behaviour when clocked from BPM clock, avoids 'count off by one' hassles)
		if( cur_input == true && cur_reset == false && SampleClock() > 0 )
		{
			cur_count++;

			if( cur_count >= max_count ) // must be >= for case when there are zero outputs
			{
				cur_count = 0;
			}
		}
	}

	if( prev_count != cur_count )
	{
		outputs[prev_count].Set( SampleClock(), 0.f );
		outputs[cur_count].Set( SampleClock(), 0.5f );
	}
}

//////////////////////////////////////////////////////////

class IntegerCounter : public MpBase2
{
	BoolInPin pinGate;
	BoolInPin pinReset;
	IntInPin pinSteps;
	IntOutPin pinOut;

public:
	IntegerCounter()
	{
		initializePin( pinGate );
		initializePin( pinReset );
		initializePin( pinSteps );
		initializePin( pinOut );
	}

	void onSetPins() override
	{
		if( pinGate.isUpdated() && pinGate.getValue() && !pinReset.getValue()) // Reset overrides gating until after we get a clean down transition.
		{
			auto newOut = pinOut.getValue() + 1;
			if (newOut >= pinSteps.getValue())
			{
				newOut = 0;
			}
			pinOut = newOut;
		}
		if( pinReset.isUpdated() && pinReset.getValue() )
		{
			pinOut = 0;
		}
	}
};

namespace
{
	auto r = gmpi::Register<IntegerCounter>::withXml(
		R"XML(
<?xml version="1.0" ?>
<PluginList>
  <Plugin id="SE Integer Counter2" name="Integer Counter" category="Experimental">
    <Audio>
      <Pin name="Gate" datatype="bool" />
      <Pin name="Reset" datatype="bool" />
      <Pin name="Steps" datatype="int" default="4" />
      <Pin name="Out" datatype="int" direction="out" />
    </Audio>
  </Plugin>
</PluginList>
)XML");
}

class StepSequencer : public MpBase2
{
protected:
	BoolInPin pinGate;
	BoolInPin pinReset;
	AudioOutPin pinOut;
	std::vector<AudioInPin> pinInputs;
	std::vector<AudioInPin>::iterator currentInput;
	std::vector<AudioInPin>::iterator currentEnd;

public:

	int32_t MP_STDCALL open() override
	{
		GmpiSdk::ProcessorPins hostPins(getHost());
		const auto autoduplicatingPinCount = hostPins.size() - pins_.size();
		AudioInPin initVal = {};
		pinInputs.assign(autoduplicatingPinCount, initVal);

		for (auto& p : pinInputs)
		{
			initializePin(p);
		}

		if(!pinInputs.empty())
		{
			currentInput = pinInputs.begin();
		}

		currentEnd = pinInputs.end();

		return MpBase2::open();
	}

	void onSetPins() override
	{
		if (pinInputs.empty())
		{
			pinOut.setStreaming(false);
			return;
		}

		// use state machine?
		auto oldInput = currentInput;
		if( pinGate.isUpdated() && pinGate.getValue() && !pinReset.getValue()) // Reset overrides gating until after we get a clean down transition.
		{
			++currentInput;
			if( currentInput == currentEnd || currentInput == pinInputs.end())
			{
				currentInput = pinInputs.begin();
			}
		}
		if( pinReset.isUpdated() && pinReset.getValue() )
		{
			currentInput = pinInputs.begin();
		}

		if (oldInput != currentInput || currentInput->isUpdated())
		{
			pinOut.setStreaming(currentInput->isStreaming());
		}

		setSubProcess( &StepSequencer::subProcess );

		// TODO: in case of input pin sleeping (but other inputs not), we could Sleep. Currently we don't.
	}

	void subProcess( int sampleFrames )
	{
		const float* signalIn = getBuffer(*currentInput);
		float* signalOut = getBuffer(pinOut);

		for( int s = sampleFrames; s > 0; --s )
		{
			*signalOut++ = *signalIn++;
		}
	}
};

class StepSequencer2 : public StepSequencer
{
public:
	StepSequencer2()
	{
		initializePin(pinGate);
		initializePin(pinReset);
		initializePin(pinOut);
	}
};

class StepSequencer3 : public StepSequencer
{
	IntInPin pinStepCount;

public:
	StepSequencer3()
	{
		initializePin(pinGate);
		initializePin(pinReset);
		initializePin(pinStepCount);
		initializePin(pinOut);
	}

	void onSetPins() override
	{
		if (pinStepCount.isUpdated())
		{
			if (pinStepCount < pinInputs.size() && pinStepCount > 0)
			{
				currentEnd = pinInputs.begin() + pinStepCount.getValue();
			}
			else
			{
				currentEnd = pinInputs.end();
			}
		}

		StepSequencer::onSetPins();
	}
};

namespace
{
	auto r2 = gmpi::Register<StepSequencer2>::withXml(
		R"XML(
<?xml version="1.0" ?>
<PluginList>
  <Plugin id="SE StepSequencer2" name="Step Sequencer2" category="Old">
    <Audio>
      <Pin name="Gate" datatype="bool" />
      <Pin name="Reset" datatype="bool" />
      <Pin name="Out" datatype="float" rate="audio" direction="out" />
      <Pin name="In" datatype="float" rate="audio" autoDuplicate="true" />
    </Audio>
  </Plugin>
</PluginList>
)XML");

	auto r3 = gmpi::Register<StepSequencer3>::withXml(
		R"XML(
<?xml version="1.0" ?>
<PluginList>
  <Plugin id="SE StepSequencer3" name="Step Sequencer3" category="Experimental">
    <Audio>
      <Pin name="Gate" datatype="bool" />
      <Pin name="Reset" datatype="bool" />
      <Pin name="Steps" datatype="int" default="8"/>
      <Pin name="Out" datatype="float" rate="audio" direction="out" />
      <Pin name="In" datatype="float" rate="audio" autoDuplicate="true" />
    </Audio>
  </Plugin>
</PluginList>
)XML");

}

