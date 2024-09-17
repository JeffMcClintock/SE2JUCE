#include "pch.h"
#include <random>
#include "ug_random.h"
#include "mp_sdk_audio.h"
#include "Logic.h"
#include "conversion.h"
#include "module_register.h"

SE_DECLARE_INIT_STATIC_FILE(ug_random)

namespace
{
REGISTER_MODULE_1( L"random", IDS_MN_RANDOM,IDS_MG_OLD,ug_random,CF_STRUCTURE_VIEW,L"Generates a random voltage when triggered");
}

#define PLG_OUT 1

// Fill an array of InterfaceObjects with plugs and parameters
void ug_random::ListInterface2(InterfaceObjectArray& PList)
{
	LIST_VAR3( L"Trigger in",m_trigger,  DR_IN,DT_BOOL, L"1", L"", IO_POLYPHONIC_ACTIVE, L"");
	LIST_PIN( L"Output", DR_OUT, L"", L"", 0, L"");
}

// invert input, unless indeterminate voltage
void ug_random::sub_process(int start_pos, int sampleframes)
{
	bool can_sleep = true;
	output_so.Process( start_pos, sampleframes, can_sleep );

	if( can_sleep )
	{
		SET_CUR_FUNC( &ug_base::process_sleep );
	}
}

void ug_random::onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state)
{
	if( m_trigger || p_clock == 0)
		ChangeOutput();
}

ug_random::ug_random() :
	output_so(SOT_LINEAR)
{
}

int ug_random::Open()
{
	ug_base::Open();
	output_so.SetPlug( GetPlug(PLG_OUT) );
	return 0;
}

void ug_random::Resume()
{
	ug_base::Resume();
	output_so.Resume();
}

void ug_random::ChangeOutput()
{
	constexpr float scale = 1.f / (float)RAND_MAX;
	float output_val = scale * (float) rand();
	output_so.Set( SampleClock(), output_val, 1 );
	SET_CUR_FUNC( &ug_random::sub_process );
	//	ResetStaticOutput();
}

////////////////////////////////////////////////////////////////////////////

using namespace gmpi;

class RandomFloat final : public MpBase2
{
	IntInPin pinRenderMode;
	BoolInPin pinTriggerin;
	FloatOutPin pinOutput;

	std::default_random_engine e;
	std::uniform_real_distribution<float> dist;

public:
	RandomFloat() :
		dist(0.0f, 10.0f)
	{
		initializePin(pinRenderMode);
		initializePin(pinTriggerin);
		initializePin(pinOutput);
	}

	void onSetPins() override
	{
		// if not offline mode, randomize rng seed.
		if (pinRenderMode.isUpdated())
		{
			if (pinRenderMode == 2) // offline, randomize seed in a consistant manner.
			{
				std::random_device rd;
				e.seed(host.getHandle());
			}
			else
			{
				std::random_device rd; // actually random
				e.seed(rd());
			}
		}

		if(pinTriggerin)
			pinOutput = dist(e);
	}
};

namespace
{
	auto r = Register<RandomFloat>::withXml(
R"XML(
<?xml version="1.0" ?>
<PluginList>
    <Plugin id="SE Random Voltage" name="Random Voltage2" category="Special">
        <Audio>
			<Pin name = "RenderMode" datatype = "int" hostConnect = "Processor/OfflineRenderMode" />
            <Pin name="Trigger in" datatype="bool" default="1"/>
            <Pin name="Output" datatype="float" direction="out"/>
        </Audio>
    </Plugin>
</PluginList>
)XML");
}