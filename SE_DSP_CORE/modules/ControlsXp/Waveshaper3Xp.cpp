#include "../se_sdk3/mp_sdk_audio.h"
#include "../shared/xp_simd.h"
#include "../shared/expression_evaluate.h"
#include "../shared/unicode_conversion.h"

SE_DECLARE_INIT_STATIC_FILE(Waveshaper3Xp);

using namespace std;
using namespace gmpi;

class SeWaveshaperBase : public MpBase2
{
protected:
	StringInPin pinShape;
	AudioInPin pinSignalIn;
	AudioOutPin pinSignalOut;
	static const int WS_NODE_COUNT = 11;
	float* m_lookup_table;
	static const int TABLE_SIZE = 512;

public:

	int32_t MP_STDCALL open() override
	{
		int32_t handle;
		getHost()->getHandle(handle);
		wchar_t name[40];
		swprintf(name, sizeof(name) / sizeof(wchar_t), L"SE waveshaper3 %x curve", handle);
		const int size = sizeof(m_lookup_table[0]) * (TABLE_SIZE + 2);
		int32_t needInitialize;
		getHost()->allocateSharedMemory(name, (void**)&m_lookup_table, -1, size, needInitialize);

		/*
		if (needInitialize)
		{
			FillLookupTable();
		}
		*/

		return MpBase2::open();
	}

	virtual void FillLookupTable() = 0;

	// Clips to upper limit of 1.0,and lower limit of zero.
	inline float fastInterpolationClipped(float index, int tableSize, float* lookupTable)
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
	void subProcess(int sampleFrames)
	{
		// get pointers to in/output buffers.
		auto signalIn = getBuffer(pinSignalIn);
		auto signalOut = getBuffer(pinSignalOut);

		for (int s = sampleFrames; s > 0; --s)
		{
			*signalOut++ = fastInterpolationClipped(*signalIn++ + 0.5f, TABLE_SIZE, m_lookup_table); // map +/-5 volt to 0.0 -> 1.0
		}
	}

	virtual void onSetPins() override
	{
		if (pinShape.isUpdated())
		{
			FillLookupTable();
		}

		// Set state of output audio pins.
		pinSignalOut.setStreaming(pinSignalIn.isStreaming());

		// Set processing method.
		setSubProcess(&SeWaveshaperBase::subProcess);
	}
};

class SeWaveshaper3Xp : public SeWaveshaperBase
{
public:
	SeWaveshaper3Xp()
	{
		initializePin(pinSignalIn);
		initializePin(pinSignalOut);
		initializePin(pinShape);
	}

	// Line-segment based.
	void FillLookupTable() override
	{
		float nodes[2][WS_NODE_COUNT]; // x,y co-ords of control points

		wstring s = pinShape;

		// convert CString of numbers to array of screen co-ords
		int i = 0;
		while (s.length() > 0 && i < WS_NODE_COUNT)
		{
			int p1 = (int)s.find(L"(");
			int p2 = (int)s.find(L",");
			int p3 = (int)s.find(L")");
			if (p3 < 1)
				return;

			wchar_t* temp;	// convert string to float
			float x = (float)wcstod(s.substr(p1 + 1, p2 - p1 - 1).c_str(), &temp);
			float y = (float)wcstod(s.substr(p2 + 1, p3 - p2 - 1).c_str(), &temp);
			x = (x + 5.f) * 10.f; // convert to screen co-ords
			y = (-y + 5.f) * 10.f;

			nodes[0][i] = x;
			nodes[1][i++] = y;

			p3++;
			s = s.substr(p3, s.length() - p3); //.Right( s.length() - p3 - 1);
		}

		int segments = 11;
		double from_x = 0.f; // first x co=ord
		int table_index = 0;
		for (int i = 1; i < segments; ++i)
		{
			double to_x = nodes[0][i] * TABLE_SIZE * 0.01;
			double delta_y = nodes[1][i] - nodes[1][i - 1];
			double delta_x = from_x - to_x;
			double slope = 0.01 * delta_y / delta_x;
			double c = 0.5 - 0.01 * nodes[1][i - 1] - from_x * slope;

			int int_to = (int)to_x;

			if (int_to > TABLE_SIZE)
				int_to = TABLE_SIZE;

			if (i == segments - 1)
				int_to = TABLE_SIZE + 1; // fill in last 'extra' sample.

			for (; table_index < int_to; table_index++)
			{
				m_lookup_table[table_index] = (float)(((double)table_index) * slope + c);
				//			_RPT2(_CRT_WARN, "%3d : %f\n", table_index, m_lookup_table[table_index] );
			}

			from_x = to_x;
		}
	}
};

class SeWaveshaper2Xp : public SeWaveshaperBase
{
public:
	SeWaveshaper2Xp()
	{
		initializePin(pinShape);
		initializePin(pinSignalIn);
		initializePin(pinSignalOut);
	}

	// WAVESHAPER 2B - math formula-based.
	void FillLookupTable() override
	{
		Evaluator ee;
		std::string formulaUtf8 = JmUnicodeConversions::WStringToUtf8(pinShape);

		// With latency compensation, formula won't be set for a short time (it will be blank). Prevent crash/slowdown.
		if (formulaUtf8.empty())
		{
			formulaUtf8 = "0";
		}

		double y;
		int flags = 0;

		for (int i = 0; i <= TABLE_SIZE; ++i)
		{
			double x = 10.0 * (((double)i / (double)TABLE_SIZE) - 0.5);
			ee.SetValue("x", &x);
			ee.Evaluate(formulaUtf8.c_str(), &y, &flags);

#if _MSC_VER >= 1600 // Only Avail in VS2010 or later.
			if (isnan(y))
			{
				y = 0.0;
			}
#endif
			m_lookup_table[i] = (float)(y * 0.1);
		}
	}
};

namespace
{
	auto r1 = Register<SeWaveshaper3Xp>::withId(L"SE Waveshaper3 XP");
	auto r2 = Register<SeWaveshaper2Xp>::withId(L"SE Waveshaper2 XP");
}
