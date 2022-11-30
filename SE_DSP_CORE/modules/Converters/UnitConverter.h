#ifndef UNITCONVERTER_H_INCLUDED
#define UNITCONVERTER_H_INCLUDED

#include "../se_sdk3/mp_sdk_audio.h"

class UnitConverter : public MpBase
{
public:
	UnitConverter( IMpUnknown* host );
	void onSetPins() override;

private:
	FloatInPin pinA;
	FloatOutPin pinB;
	EnumInPin pinMode;
};

//#define UNITCONVERTER_VOLTS_USE_LOOKUP

class UnitConverterVolts : public MpBase2
{
	enum { UCV_HZ_TO_VOLTS, UCV_DB_TO_VCA, UCV_DB_TO_EXP, UCV_VOLTS_TO_HZ = 100, UCV_VCA_TO_DB, UCV_EXP_TO_DB };

#ifdef UNITCONVERTER_VOLTS_USE_LOOKUP
	static const int LOOKUP_SIZE = 400;
	static const int MAX_KHZ = 40;
	float* lookup_;
#endif
	float hz_to_volts_const1;
	float hz_to_volts_const2;

	AudioInPin pinSignalIn;
	AudioOutPin pinSignalOut;
	EnumInPin pinMode;

public:
	UnitConverterVolts();
	virtual int32_t MP_STDCALL open();
	void subProcessHzToVolts(int sampleFrames);
	void subProcessVoltsToHz(int sampleFrames);
	void subProcessDbToVca(int sampleFrames);
	void subProcessDbToExp(int sampleFrames);
	void subProcessVcaToDb(int sampleFrames);
	void subProcessExpToDb(int sampleFrames);

	void onSetPins() override;

#ifdef UNITCONVERTER_VOLTS_USE_LOOKUP
	inline static float TableLookupCubic(const float* table, float v)
	{
		const int maxTableIndex = LOOKUP_SIZE;

		float index = v * 100.0f;
		int table_floor = FastRealToIntTruncateTowardZero(index); // fast float-to-int using SSE. truncation toward zero.
		float fraction = index - (float)table_floor;

		if( table_floor <= 0 ) // indicated index *might* be less than zero. e.g. Could be 0.1 which is valid, or -0.1 which is not.
		{
			if( !( index >= 0.0f ) ) // reverse logic to catch Nans.
			{
				return table[0];
			}
		}
		else
		{
			if( table_floor >= maxTableIndex )
			{
				return table[maxTableIndex];
			}
		}

		// Cubic interpolator.
		assert(table_floor >= 0 && table_floor <= maxTableIndex);

		float y0 = table[table_floor - 1];
		float y1 = table[table_floor + 0];
		float y2 = table[table_floor + 1];
		float y3 = table[table_floor + 2];

		return y1 + 0.5f * fraction*( y2 - y0 + fraction*( 2.0f*y0 - 5.0f*y1 + 4.0f*y2 - y3 + fraction*( 3.0f*( y1 - y2 ) + y3 - y0 ) ) );
	}
#endif
};

#endif

