// ULookup Table, Stores a function in a table for quick access
//
#pragma once
#include "sample.h"
#include <assert.h>
#include "modules/shared/xp_simd.h"
#include <algorithm>
#include <string>

// Handy. No clipping. Limits must be enforced by caller.
inline float fastInterpolationNoClip( float index, int tableSize, float* lookupTable )
{
	assert( index <= 1.0f && index >= 0.0f );

	index *= (float) tableSize;
	int i = FastRealToIntTruncateTowardZero(index); // fast float-to-int using SSE. truncation toward zero.
	float idx_frac = index - (float) i;
	float* p = lookupTable + i;
	return p[0] + idx_frac * (p[1] - p[0]);
}

// Clips to upper limit of 1.0,and lower limit of zero.
inline float fastInterpolationClipped( float index, int tableSize, float* lookupTable )
{
	if( (int&)index & 0x80000000 ) // bitwise negative test.
	{
		return lookupTable[0];
	}

	index *= (float) tableSize;

	int i = FastRealToIntTruncateTowardZero(index); // fast float-to-int using SSE. truncation toward zero.

	if( i >= 0 && i < tableSize )
	{
		float idx_frac = index - (float) i;
		float* p = lookupTable + i;
		return p[0] + idx_frac * (p[1] - p[0]);
	}

	// Very large 'index' values overflow 'i' to 0x80000000 (-2147483648). Clip to max.
	return lookupTable[tableSize];
}

// Index pre-multiplied by tableSize (provides for tables that contain say 20V worth of data instead of the usual 10).
// Clips to upper limit of tableSize, and lower limit of zero.
inline float fastInterpolationClipped2( float index, int tableSize, float* lookupTable )
{
/* fast floor (works with negative values).

	cvttss2si eax, float_value
	mov ecx, float_value
	shr ecx, 31
	sub eax, ecx

	int i = FastRealToIntTruncateTowardZero(index )); // fast float-to-int using SSE. truncation toward zero.
	i += (*(int*)&index) >> 31; // taking sign bit off floating point value to round to -ve inf.

*/

	if ((int&)index & 0x80000000) // bitwise negative test.
	{
		return lookupTable[0];
	}

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

inline float fastInterpolationClippedLow( float index, int tableSize, float* lookupTable )
{
	assert( index <= 1.0f );

	if ((int&)index & 0x80000000) // bitwise negative test.
	{
		return lookupTable[0];
	}

	index *= (float)tableSize;

	int i = FastRealToIntTruncateTowardZero(index); // fast float-to-int using SSE. truncation toward zero.

	if (i >= 0 /*&& i < tableSize */)
	{
		float idx_frac = index - (float)i;
		float* p = lookupTable + i;
		return p[0] + idx_frac * (p[1] - p[0]);
	}

	// Very large 'index' values overflow 'i' to 0x80000000 (-2147483648). Clip to max.
	return lookupTable[tableSize];
}


class LookupTable
{
public:
	LookupTable( );
	virtual ~LookupTable( );
	void SetInitialised( void )
	{
		init_flag = true;
	};
	void ClearInitialised( void )
	{
		init_flag = false;
	};
	bool GetInitialised()
	{
		return init_flag;
	};
	void SetSize( int s );
	int GetSize( )
	{
		return size;
	};
	bool CheckOverwrite( );
	void* table;

protected:
	int size;
	bool init_flag;
};


class ULookup : public LookupTable
{
public:
	inline float* GetBlock( void )
	{
		return (float*) table;
	};
	inline float GetValueFloatNoInterpolation( int idx )
	{
		return GetBlock()[idx];
	};
	inline float GetValueInterp2( float p_normalised_idx ) const
	{
/*
		const float l_table_size = 512.f;
		// Fast C++ with SSE.
		return fastInterpolation( p_normalised_idx, l_table_size, table );
*/
		const int l_table_size = 512;
		return fastInterpolationNoClip( p_normalised_idx, l_table_size, (float*) table );
	};

	inline float GetValueInterpClipped( float p_normalised_idx ) const
	{
		const int l_table_size = 512;

		// Fast C++ with SSE.
		return fastInterpolationClipped( p_normalised_idx, l_table_size, (float*) table );
	};
	void SetValue( int index, float val );
	float GetValueIndexed2( int actual_index );
};


class ULookup_Integer : public LookupTable
{
public:
	void SetValue( int index, int val );
};

struct OscWaveTable
{
	static const int WAVE_TABLE_SIZE = 512;

	unsigned int min_band_inc; // set 1 semitone lower than min inc. to provide 'guard band' between wavetables
	// What freq range does this sample cover. expressed as wavetable increment
	unsigned int max_inc;
	unsigned int min_inc;
	float wave[WAVE_TABLE_SIZE + 2];
};

// This class wraps a standard lookup table to make it appear as a OscWaveTable struct.
class SharedOscWaveTable
{
public:
	SharedOscWaveTable( int nothing = 0 );
	OscWaveTable* WaveTable()
	{
		return reinterpret_cast<OscWaveTable*>(	lookup_->GetBlock() );
	}
	void Init( ULookup* lookup, int sampleRate )
	{
		lookup_ = lookup;
		sampleRate_ = sampleRate;
	}
	float* WaveData()
	{
		return WaveTable()->wave;
	}
	int Size()
	{
		return sizeof(WaveTable()->wave) / sizeof(WaveTable()->wave[0]);
	}
	bool Create( class ug_base* ug, const std::wstring &name, int sampleRate = -1 );
	int SampleRate()
	{
		return sampleRate_;
	}

private:
	ULookup* lookup_;
	int sampleRate_;
};


