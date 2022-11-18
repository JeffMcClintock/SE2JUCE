// MS CPP - prevent warning: 'swprintf' was declared deprecated
#define _CRT_SECURE_NO_DEPRECATE
#pragma warning(disable : 4996)

// Prevent stupid 'min' macro overriding std::min
#define NOMINMAX

#include <algorithm>
#include <limits>
#include <cmath>
#include "./waveshapers.h"
#include "../shared/expression_evaluate.h"
#include "../shared/unicode_conversion.h"
#include "../shared/xp_simd.h"

SE_DECLARE_INIT_STATIC_FILE(Waveshapers);
SE_DECLARE_INIT_STATIC_FILE(Waveshaper2B);

REGISTER_PLUGIN ( Waveshaper2b, L"SE Waveshaper2B" );

Waveshaper::Waveshaper(IMpUnknown* host) : MpBase(host)
{
	// Associate each pin object with it's ID in the XML file.
	initializePin(0, pinShape);
	initializePin(1, pinInput);
	initializePin(2, pinOutput);
}

int32_t Waveshaper::open()
{
	MpBase::open();	// always call the base class.

	SetupLookupTable();
	SET_PROCESS(&Waveshaper::subProcess);

	return gmpi::MP_OK;
}

// one or more pins_ updated.  Check pin update flags to determin which ones.
void Waveshaper::onSetPins()
{
	if(pinShape.isUpdated())
	{
		FillLookupTable();
		onLookupTableChanged();
	}

	if(pinInput.isUpdated())
	{
		pinOutput.setStreaming(pinInput.isStreaming());
	}

	// now automatic. setSleep(pinInput.isStreaming() == false);
}

void Waveshaper::onLookupTableChanged()
{
//	_RPT1(_CRT_WARN, "onLookupTableChanged() %x\n",this);

	if(!pinInput.isStreaming()) // 'kick' static output if waveshape changed
	{
		pinOutput.setStreaming(false);
	}
}

void Waveshaper::SetupLookupTable()
{
	int32_t handle;
	getHost()->getHandle(handle);

	wchar_t name[40];
	swprintf(name, sizeof(name)/sizeof(wchar_t), L"SE waveshaper3 %x curve", handle);

	void** temp = (void**) &m_lookup_table;
	/*bool need_initialise = */CreateSharedLookup(name, temp, -1, TABLE_SIZE+2);
}

bool Waveshaper::CreateSharedLookup(wchar_t* p_name, void** p_returnPointer, float p_sampleRate, int p_size)
{
	int32_t need_initialise;
	getHost()->allocateSharedMemory(p_name, p_returnPointer, p_sampleRate, p_size * sizeof(float), need_initialise);
	return need_initialise != 0;
}

// Faster.
// Clips to upper limit of 1.0,and lower limit of zero.
inline float fastInterpolationClipped( float index, int tableSize, float* lookupTable )
{
	if ((int&)index & 0x80000000) // bitwise negative test.
	{
		return lookupTable[0];
	}

	index *= (float)tableSize;

	int i = FastRealToIntTruncateTowardZero(index); // fast float-to-int using SSE. truncation toward zero.

	if (i >= 0 && i < tableSize)
	{
		float idx_frac = index - (float)i;
		float* p = lookupTable + i;
		return p[0] + idx_frac * (p[1] - p[0]);
	}

	// Very large 'index' values overflow 'i' to 0x80000000 (-2147483648). Clip to max.
	return lookupTable[tableSize];
}

void Waveshaper::subProcess(int bufferOffset, int sampleFrames)
{
	// get pointers to in/output buffers
	float* in1	= bufferOffset + pinInput.getBuffer();
	float* out1 = bufferOffset + pinOutput.getBuffer();

	for( int s = sampleFrames ; s > 0 ; s-- )
	{
		//float count = *in1++ + 0.5f; // map +/-5 volt to 0.0 -> 1.0
		// count = std::min( count, 1.0f );

		*out1 = fastInterpolationClipped(*in1++ + 0.5f, TABLE_SIZE, m_lookup_table); // map +/-5 volt to 0.0 -> 1.0
        assert( !std::isnan(*out1) );
        ++out1;
	}
}

// WAVESHAPER 2B - math formula-based.
void Waveshaper2b::FillLookupTable()
{
	Evaluator ee;
    std::string formulaUtf8 = JmUnicodeConversions::WStringToUtf8( pinShape );

	// With latency compensation, formula won't be set for a short time (it will be blank). Prevent crash/slowdown.
	if (formulaUtf8.empty())
	{
		formulaUtf8 = "0";
	}

	double y;
	int flags = 0;

	for( int i = 0 ; i <= TABLE_SIZE; ++i )
	{
		double x = 10.0 * (((double) i / (double)TABLE_SIZE ) - 0.5 );
		ee.SetValue( "x", &x );
		ee.Evaluate( formulaUtf8.c_str(), &y, &flags );

		if( !std::isfinite(y) )
		{
			y = 0.0;
		}

		m_lookup_table[i] = (float) (y * 0.1);
	}
}

