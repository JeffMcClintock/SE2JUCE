#include "../se_sdk3/mp_sdk_audio.h"
#include "mp_sdk_gui2.h"
#include <codecvt>

SE_DECLARE_INIT_STATIC_FILE(RegistrationCheck)

using namespace gmpi;

std::wstring Int64ToHex(int64_t serial)
{
	wchar_t serialText[20];
	swprintf(serialText, sizeof(serialText)/sizeof(wchar_t), L"%016llX", serial);

	return std::wstring(serialText);
}

inline void hash_combine(uint64_t& seed, const uint64_t v)
{
	const uint64_t kMul = 0x9ddfea08eb382d69ULL;
	uint64_t a = (v ^ seed) * kMul;
	a ^= (a >> 47);
	uint64_t b = (seed ^ a) * kMul;
	b ^= (b >> 47);
	seed = b * kMul;
}

std::wstring GenerateSerial(std::wstring regoName, uint64_t randomSeed)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;
	const auto regoNameU = convert.to_bytes(regoName);

	// hash string
	uint64_t serial = 5381; // <<<<< Place your own 'magic' number here.
	{
		for( auto c : regoNameU)
		{
			serial = ((serial << 5) + serial) + c; // magic * 33 + c
		}
	}

	hash_combine(serial, randomSeed | (randomSeed << 32));

	return Int64ToHex(serial);
}

class RegistrationSerialGeneratorGui : public SeGuiInvisibleBase
{
	void generateSerial()
	{
		pinSerial = GenerateSerial(pinRegoName.getValue(), pinRandomSeed);
	}

	StringGuiPin pinRegoName;
	IntGuiPin pinRandomSeed;
	StringGuiPin pinSerial;

public:
	RegistrationSerialGeneratorGui()
	{
		initializePin(pinRegoName, static_cast<MpGuiBaseMemberPtr2>(&RegistrationSerialGeneratorGui::generateSerial));
		initializePin(pinRandomSeed, static_cast<MpGuiBaseMemberPtr2>(&RegistrationSerialGeneratorGui::generateSerial));
		initializePin(pinSerial);
	}
};

namespace
{
	auto r2 = Register<RegistrationSerialGeneratorGui>::withXml(
R"XML(
<?xml version="1.0" ?>
<PluginList>
  <Plugin id="SE Registration Serial Generator" name="Registration Serial Generator" category="Special" helpUrl="http://synthedit.com/downloads/preview#Registration">
    <GUI>
      <Pin name="Rego Name" datatype="string" default="Elisha Gray" />
      <Pin name="Random Seed" datatype="int" />
      <Pin name="Serial" datatype="string" direction="out"/>
    </GUI>
  </Plugin>
</PluginList>
)XML");
}

class RegistrationCheck : public MpBase2
{
	StringInPin pinRegoName;
	StringInPin pinSerial;
	IntInPin pinRandomSeed;
	BoolOutPin pinBoolVal;

public:
	RegistrationCheck()
	{
		initializePin(pinRegoName);
		initializePin(pinSerial);
		initializePin(pinRandomSeed);
		initializePin(pinBoolVal);
	}

	void onSetPins() override
	{
		pinBoolVal = pinSerial == GenerateSerial(pinRegoName.getValue(), pinRandomSeed);
	}
};

namespace
{
	auto r = Register<RegistrationCheck>::withXml(
R"XML(
<?xml version="1.0" ?>
<PluginList>
  <Plugin id="SE Registration Check" name="Registration Check" category="Special" helpUrl="http://synthedit.com/downloads/preview#Registration">
    <Audio>
      <Pin name="Rego Name" datatype="string" default="Elisha Gray" />
      <Pin name="Serial" datatype="string" default="7F1DF070E066DDC4"/>
      <Pin name="Random Seed" datatype="int" isMinimised="true" />
      <Pin name="Is Valid" datatype="bool" direction="out" />
    </Audio>
  </Plugin>
</PluginList>
)XML");
}

class RegistrationCheckGui : public SeGuiInvisibleBase
{
	void checkSerial()
	{
		pinBoolVal = pinSerial == GenerateSerial(pinRegoName.getValue(), pinRandomSeed);
	}

	StringGuiPin pinRegoName;
	StringGuiPin pinSerial;
	IntGuiPin pinRandomSeed;
	BoolGuiPin pinBoolVal;

public:
	RegistrationCheckGui()
	{
		initializePin(pinRegoName, static_cast<MpGuiBaseMemberPtr2>(&RegistrationCheckGui::checkSerial));
		initializePin(pinSerial, static_cast<MpGuiBaseMemberPtr2>(&RegistrationCheckGui::checkSerial));
		initializePin(pinRandomSeed, static_cast<MpGuiBaseMemberPtr2>(&RegistrationCheckGui::checkSerial));
		initializePin(pinBoolVal);
	}
};

namespace
{
	auto r3 = Register<RegistrationCheckGui>::withXml(
R"XML(
<?xml version="1.0" ?>
<PluginList>
  <Plugin id="SE Registration Check Gui" name="Registration Check (GUI)" category="Special" helpUrl="http://synthedit.com/downloads/preview#Registration">
    <GUI>
      <Pin name="Rego Name" datatype="string" default="Elisha Gray" />
      <Pin name="Serial" datatype="string" default="7F1DF070E066DDC4"/>
      <Pin name="Random Seed" datatype="int"/>
      <Pin name="Is Valid" datatype="bool" direction="out" />
    </GUI>
  </Plugin>
</PluginList>
)XML");
}
