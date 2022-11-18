#include "../pch.h"
#include "../modules/se_sdk3/mp_sdk_audio.h"
#include "../modules/se_sdk3/mp_sdk_gui2.h"
#include "../modules/shared/RawView.h"
#include "../modules/shared/xp_simd.h"
#include "../modules/shared/xplatform.h"
#include "../iseshelldsp.h"
#include "../ug_plugin3.h"

#if (GMPI_IS_PLATFORM_JUCE==0)
#include "../modules/se_sdk3_hosting/ModuleView.h"
#include "../modules/se_sdk3_hosting/ViewBase.h"
#endif

SE_DECLARE_INIT_STATIC_FILE(PatchPoints)

namespace {

	int32_t r = RegisterPluginXml( R"XML(
<?xml version="1.0" ?>
<PluginList>
  <Plugin id="SE Patch Point in" name="Patch Point&lt;-" category="Flow Control" graphicsApi="GmpiGui" simdOverwritesBufferEnd="true">
    <Audio>
		<Pin name="Input" datatype="float" rate="audio" isMinimised="true" linearInput="true"/>
		<Pin name="Output" datatype="float" direction="out" rate="audio" />
    </Audio>
    <GUI/>
    <PatchPoints>
		<PatchPoint pinId="0" center="10,10" radius="5" />
    </PatchPoints>
  </Plugin>

  <Plugin id="SE Patch Point out" name="Patch Point-&gt;" category="Flow Control" graphicsApi="GmpiGui" simdOverwritesBufferEnd="true">
   <Audio>
      <Pin name="Input" datatype="float" rate="audio" linearInput="true"/>
      <Pin name="Output" datatype="float" direction="out" rate="audio" isMinimised="true" />
    </Audio>
    <GUI/>
    <PatchPoints>
		<PatchPoint pinId="1" center="10,10" radius="5" />
    </PatchPoints>
  </Plugin>

  <Plugin id="SE PatchCableChangeNotifier" name="Patch Cable Notifier" category="Debug">
    <GUI>
       <Pin name="" datatype="blob" private="true" hostConnect="PatchCables" />
   </GUI>
  </Plugin>

</PluginList>
)XML");

}

class PatchPoint : public MpBase2
{
public:
	PatchPoint()
	{
		initializePin(pinInput);
		initializePin(pinoutput);
	}

	void subProcess(int sampleFrames)
	{
		auto in = getBuffer(pinInput);
		auto out = getBuffer(pinoutput);

#ifndef GMPI_SSE_AVAILABLE
		// No SSE. Use C++ instead.
		for (int s = sampleFrames; s > 0; --s)
		{
			*out++ = *in++;
		}
#else
		// Use SSE instructions.
		// process fiddly non-sse-aligned prequel.
		while (reinterpret_cast<intptr_t>(out) & 0x0f)
		{
			*out++ = *in++;
			--sampleFrames;
		}

		// SSE intrinsics. 4 samples at a time.
		__m128* pIn1 = (__m128*) in;
		__m128* pDest = (__m128*) out;

		while (sampleFrames > 0)
		{
			*pDest++ = *pIn1++;
			sampleFrames -= 4;
		}
#endif
	}

	virtual void onSetPins() override
	{
		pinoutput.setStreaming(pinInput.isStreaming());
		setSubProcess(&PatchPoint::subProcess);
	}

	AudioInPin pinInput;
	AudioOutPin pinoutput;
};


#if (GMPI_IS_PLATFORM_JUCE==0)
class PatchCableChangeNotifier : public gmpi_gui::MpGuiInvisibleBase
{
public:
	PatchCableChangeNotifier()
	{
		initializePin(pinPatchCableXml, static_cast<MpGuiBaseMemberPtr2>(&PatchCableChangeNotifier::onSetPatchcableXml));
	}

	void onSetPatchcableXml()
	{
		// get the XML describing the patch cables
		RawView raw(pinPatchCableXml.rawData(), (size_t)pinPatchCableXml.rawSize());

		dynamic_cast<SynthEdit2::ModuleView*>(getHost())->parent->OnPatchCablesUpdate(raw);
	}

private:
	BlobGuiPin pinPatchCableXml;
};

namespace
{
	auto r3 = gmpi::Register<PatchCableChangeNotifier>::withId(L"SE PatchCableChangeNotifier");
}
#endif

REGISTER_PLUGIN2(PatchPoint, L"SE Patch Point in");
REGISTER_PLUGIN2(PatchPoint, L"SE Patch Point out");