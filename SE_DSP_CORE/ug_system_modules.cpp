// ug_base.cpp: implementation of the ug_base class.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "ug_base.h"
#include "UgDatabase.h"
#include "ug_latency_adjust.h"
#include "ug_latency_adjust_event.h"
#include "ug_plugin3.h"
#include "ug_oversampler.h"

SE_DECLARE_INIT_STATIC_FILE(ug_system_modules);

using namespace std;



namespace {
	int32_t r = RegisterPluginXml(
R"XML(
<?xml version="1.0" ?>
<PluginList>

	<Plugin id="SE LatencyAdjust" name="LA" category="Debug">
		<Audio>
			<Pin name="In" datatype="float" rate="audio" linearInput="true" />
			<Pin name="Out" datatype="float" rate="audio" direction="out" />
		</Audio>
	</Plugin>

	<Plugin id="SE LatencyAdjust-eventbased" name="LA" category="Debug">
		<Audio/>
	</Plugin>

	<Plugin id="SE DAW Sample Rate" name="DAW Sample Rate" category="Diagnostic" >
		<Audio>
			<Pin name="DAW Sample Rate" datatype="float" direction="out"/>
		</Audio>
	</Plugin>

</PluginList>
)XML"
);
}

REGISTER_PLUGIN2(LatencyAdjust, L"SE LatencyAdjust");

namespace
{
	static module_description_dsp mod =
	{
		"SE LatencyAdjust-eventbased2", IDS_MN_UNNAMED, IDS_MG_DEBUG , &LatencyAdjustEventBased2::CreateObject, CF_STRUCTURE_VIEW, nullptr, 0
	};

	bool res = ModuleFactory()->RegisterModule(mod);
}
void LatencyAdjustEventBased2::ListInterface2(InterfaceObjectArray& /*PList*/) {}



class DawSampleRate : public MpBase2
{
public:
	DawSampleRate()
	{
		initializePin(pinSampleRate);
	}

	virtual void onGraphStart() override
	{
		MpBase2::onGraphStart();

		auto am = dynamic_cast<ug_plugin3Base*> (getHost())->AudioMaster();
		while (dynamic_cast<ug_oversampler*>(am) != nullptr)
		{
			am = dynamic_cast<ug_oversampler*>(am)->AudioMaster();
		}

		pinSampleRate = am->SampleRate();
	}

private:
	FloatOutPin pinSampleRate;
};


REGISTER_PLUGIN2(DawSampleRate, L"SE DAW Sample Rate");

SE_DECLARE_INIT_STATIC_FILE(DAWSampleRate)
