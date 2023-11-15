#include "./Converters.h"
#include "../shared/xp_simd.h"

SE_DECLARE_INIT_STATIC_FILE(Converters);

SE_DECLARE_INIT_STATIC_FILE(BoolToInt);
SE_DECLARE_INIT_STATIC_FILE(BoolToText);
SE_DECLARE_INIT_STATIC_FILE(BoolToFloat);

SE_DECLARE_INIT_STATIC_FILE(FloatToBool);
SE_DECLARE_INIT_STATIC_FILE(FloatToInt);
SE_DECLARE_INIT_STATIC_FILE(FloatToText);

SE_DECLARE_INIT_STATIC_FILE(IntToBool);
SE_DECLARE_INIT_STATIC_FILE(IntToFloat);
SE_DECLARE_INIT_STATIC_FILE(IntToText);
SE_DECLARE_INIT_STATIC_FILE(IntToVolts);
//SE_DECLARE_INIT_STATIC_FILE(IntToInt64);
//SE_DECLARE_INIT_STATIC_FILE(Int64ToInt);

SE_DECLARE_INIT_STATIC_FILE(ListToInt);

SE_DECLARE_INIT_STATIC_FILE(TextToText8);
SE_DECLARE_INIT_STATIC_FILE(TextToBool);
SE_DECLARE_INIT_STATIC_FILE(TextToInt);
SE_DECLARE_INIT_STATIC_FILE(TextToFloat);
SE_DECLARE_INIT_STATIC_FILE(Text8ToText);

SE_DECLARE_INIT_STATIC_FILE(VoltsToInt);
SE_DECLARE_INIT_STATIC_FILE(VoltsToBool);

typedef SimpleConverter<int, bool> IntToBool;
typedef SimpleConverter<int, float> IntToFloat;
typedef SimpleConverter<int, std::wstring> IntToText;
typedef SimpleConverter<int, std::string> IntToText8;

typedef SimpleConverter<float, int> FloatToInt;
typedef SimpleConverter<float, bool> FloatToBool;
typedef SimpleConverter<float, std::wstring> FloatToText;
typedef SimpleConverter<float, std::string> FloatToText8;

typedef SimpleConverter<bool, int> BoolToInt;
typedef SimpleConverter<bool, float> BoolToFloat;
typedef SimpleConverter<bool, std::wstring> BoolToText;
typedef SimpleConverter<bool, std::string> BoolToText8;

typedef SimpleConverter<std::wstring, int> TextToInt;
typedef SimpleConverter<std::wstring, float> TextToFloat;
typedef SimpleConverter<std::wstring, bool> TextToBool;
typedef SimpleConverter<std::string, bool> Text8ToBool;

typedef SimpleConverter<std::wstring, std::string> Text16ToText8;
typedef SimpleConverter<std::string, std::wstring> Text8ToText16;

typedef SimpleConverter<int, int> ListToInt;
typedef SimpleConverter<int32_t, int64_t> IntToInt64;
typedef SimpleConverter<int64_t, int32_t> Int64ToInt;
typedef SimpleConverter<float, double> FloatToFloat64;
typedef SimpleConverter<double, float> Float64ToFloat;


REGISTER_PLUGIN( IntToBool, L"SE IntToBool" );
REGISTER_PLUGIN( IntToFloat, L"SE IntToFloat" );
REGISTER_PLUGIN( IntToText, L"SE IntToText" );
REGISTER_PLUGIN(IntToText8, L"SE IntToText8");
REGISTER_PLUGIN(IntToInt64, L"SE IntToInt64");
REGISTER_PLUGIN(Int64ToInt, L"SE Int64ToInt");

REGISTER_PLUGIN( FloatToInt, L"SE FloatToInt" );
REGISTER_PLUGIN( FloatToBool, L"SE FloatToBool" );
REGISTER_PLUGIN( FloatToText, L"SE FloatToText" );
REGISTER_PLUGIN(FloatToText8, L"SE FloatToText8");
REGISTER_PLUGIN(FloatToFloat64, L"SE FloatToFloat64");
REGISTER_PLUGIN(Float64ToFloat, L"SE Float64ToFloat");

REGISTER_PLUGIN( BoolToInt, L"SE BoolToInt" );
REGISTER_PLUGIN( BoolToFloat, L"SE BoolToFloat" );
REGISTER_PLUGIN( BoolToText, L"SE BoolToText" );
REGISTER_PLUGIN( BoolToText8, L"SE BoolToText8" );

REGISTER_PLUGIN( TextToInt, L"SE TextToInt" );
REGISTER_PLUGIN( TextToFloat, L"SE TextToFloat" );
REGISTER_PLUGIN( TextToBool, L"SE TextToBool" );
REGISTER_PLUGIN( Text8ToBool, L"SE Text8ToBool" );

REGISTER_PLUGIN( Text8ToText16, L"SE Text8ToText" );
REGISTER_PLUGIN( Text16ToText8, L"SE TextToText8" );

REGISTER_PLUGIN( ListToInt, L"SE ListToInt" );


class VoltsToBool : public MpBase2
{
	AudioInPin pinVoltsIn;
	BoolOutPin pinBoolOut;
	bool state;

public:
	VoltsToBool() :
		state(false)
	{
		initializePin(pinVoltsIn);
		initializePin(pinBoolOut);
	}

	void subProcess(int sampleFrames)
	{
		const float* voltsIn = getBuffer(pinVoltsIn);

		for (int s = sampleFrames; s > 0; --s)
		{
			if (state != (*voltsIn > 0.0f))
			{
				state = (*voltsIn > 0.0f);
				auto blockPos = getBlockPosition() + sampleFrames - s;
				pinBoolOut.setValue(state, blockPos);
			}

			// Increment buffer pointers.
			++voltsIn;
		}
	}

	virtual void onSetPins() override
	{
		setSubProcess(&VoltsToBool::subProcess);
	}
};

REGISTER_PLUGIN2(VoltsToBool, L"SE VoltsToBool");

class BoolToVolts : public MpBase2
{
	BoolInPin pinBoolIn;
	AudioOutPin pinVoltsOut;

public:
	BoolToVolts()
	{
		initializePin(pinBoolIn);
		initializePin(pinVoltsOut);
	}

	void subProcess(int sampleFrames)
	{
		auto output = getBuffer(pinVoltsOut);

		float value = pinBoolIn ? 1.0f : 0.0f;

		for (int s = sampleFrames; s > 0; --s)
		{
			*output++ = value;
		}
	}

	void onSetPins() override
	{
		setSubProcess(&BoolToVolts::subProcess);
		pinVoltsOut.setStreaming(false);
	}
};

REGISTER_PLUGIN2(BoolToVolts, L"SE BoolToVolts");
SE_DECLARE_INIT_STATIC_FILE(BoolToVolts);

class VoltsToInt : public MpBase2
{
	AudioInPin pinVoltsIn;
	IntOutPin pinIntOut;
	int state;

public:
	VoltsToInt() :
		state(0)
	{
		initializePin(pinVoltsIn);
		initializePin(pinIntOut);
	}

	void subProcess(int sampleFrames)
	{
		const float* voltsIn = getBuffer(pinVoltsIn);

		for (int s = sampleFrames; s > 0; --s)
		{
			int v = FastRealToIntFloor(*voltsIn * 10.0f + 0.5f);
			if (state != v)
			{
				state = v;
				auto blockPos = getBlockPosition() + sampleFrames - s;
				pinIntOut.setValue(state, blockPos);
			}

			// Increment buffer pointers.
			++voltsIn;
		}
	}

	virtual void onSetPins() override
	{
		setSubProcess(&VoltsToInt::subProcess);
	}
};

REGISTER_PLUGIN2(VoltsToInt, L"SE VoltsToInt");

class IntToVolts : public MpBase2
{
	IntInPin pinIntIn;
	AudioOutPin pinVoltsOut;

public:
	IntToVolts()
	{
		initializePin(pinIntIn);
		initializePin(pinVoltsOut);
	}

	void subProcess(int sampleFrames)
	{
		auto output = getBuffer(pinVoltsOut);

		float value = pinIntIn * 0.1f;

		for (int s = sampleFrames; s > 0; --s)
		{
			*output++ = value;
		}
	}

	virtual void onSetPins() override
	{
		setSubProcess(&IntToVolts::subProcess);
		pinVoltsOut.setStreaming(false);
	}
};

REGISTER_PLUGIN2(IntToVolts, L"SE IntToVolts");

class CancellationFolder : public MpBase2
{
	StringOutPin pinFolder;

public:
	CancellationFolder()
	{
		initializePin(pinFolder);
	}

	void onGraphStart() override	// called on very first sample.
	{
		MpBase2::onGraphStart();
		std::string test1{ "moose" };
		const std::wstring test2{ L"cat" };
		pinFolder = test1;
		pinFolder = test2;
		pinFolder = L"C:\\SE\\SE15\\UnitTest\\Output\\";
	}
};

REGISTER_PLUGIN2(CancellationFolder, L"SE CancellationFolder");

class StringConcat : public MpBase2
{
	StringInPin pinIn1;
	StringInPin pinIn2;
	StringOutPin pinOut;

public:
	StringConcat()
	{
		initializePin(pinIn1);
		initializePin(pinIn2);
		initializePin(pinOut);
	}

	void onSetPins() override
	{
		pinOut = pinIn1.getValue() + pinIn2.getValue();
	}
};

REGISTER_PLUGIN2(StringConcat, L"SE StringConcat");