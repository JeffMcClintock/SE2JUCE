#pragma once

#include <vector>
#include "math.h"
#include "assert.h"
#include "csoundfont.h"

class Jzone;
class SoundfontPatch;
struct sfSample;

struct partial
{
	// Section dealing with loop-point better not inline
	// because it happens relativly rarely.
	void DoLoop( bool gateState );

	inline void IncrementPointer( bool gateState )
	{
		s_ptr_fine += s_increment;
		if( s_ptr_fine > 0xffffff )
		{
			s_ptr_r += s_ptr_fine >> 24;
			s_ptr_fine &= 0xffffff;

			if( s_ptr_r >= s_end_section )
			{
				DoLoop( gateState );
			}
			s_ptr_l = s_ptr_r + s_ptr_l_offset;
		}

		assert(	(s_ptr_r >= sampleStartAddressRight && s_ptr_r <= s_end)
			||  (s_ptr_r >= silence + 3 && s_ptr_r <= silence + 7)
		);
	};

	inline bool IsDone()
	{
		return s_loop_st == &( silence[3] );
	};

	inline bool IsStereo()
	{
		return cur_sample_l != 0;
	};

	inline void CalculateIncrement( float p_pitch )
	{
		float transposition = scale_tune * (p_pitch - root_pitch);
		float ratio = powf(2.f,transposition) * relative_sample_rate;
		float temp_float = 0x1000000 * ratio;
		s_increment = (int) temp_float;
	};

	void SetPan( float pan ); // 0.5 = hard right. -0.5 = hard left.
	bool UsesPanning()
	{
//		return pan_right_level != 1.0f; ??WTF??
		return pan_right_level != pan_left_level;
	}

	sfSample* cur_sample_r;
	sfSample* cur_sample_l;
	short* s_ptr_r;
	short* s_ptr_l;
	char loop_mode;
	int s_increment;
	unsigned int s_ptr_fine;
	short* s_loop_st;
	short* s_loop_end;
	short* s_end;
	short* s_end_section;
	int s_ptr_l_offset;
	static short silence[8];

	float root_pitch; // the input voltage coresponding to root pitch
	float scale_tune;
	float relative_sample_rate;

	float pan_left_level; // calculated from pan.
	float pan_right_level;

	Jzone* zone = {};
	#if defined( _DEBUG )
	short* sampleStartAddressRight;
	short* sampleStartAddressLeft;
	#endif
};

typedef std::vector<partial> activePartialListType;

class soundfontUser
{
public:
	virtual ~soundfontUser() {}
	void GetZone(short p_chan, short p_note, short p_vel);
	activePartialListType partials;
protected:

	float* interpolation_table2;
	int m_status;
	int sampleHandle = -1;

	activeZoneListType playingZonesTemp; // here only to avoid reallocating it on the real-time thread
};

/*
float soundfontUser::do_interpolation(short* sample_ptr, int fine )
{
	short* ptr = sample_ptr - 3;

	// reduce fraction part to range 0-31 then multiply by 8 (zeros lower 3 bits)
	float * interpolation_table_ptr = interpolation_table2 + (fine >> 19) * 8;

	// now look up interpolation filter table
	// perform 2 filters at once, one 1 sub sample ahead
	float a = 0.f;
	for( int x = 4 ; x > 0 ; x-- )
	{
		a += *ptr++ * *interpolation_table_ptr++;
		a += *ptr++ * *interpolation_table_ptr++; //unrolled
	}
	
	float a2 = 0.f;
	ptr = sample_ptr - 3;
	for( int x = 4 ; x > 0 ; x-- )
	{
		a2 += *ptr++ * *interpolation_table_ptr++;
		a2 += *ptr++ * *interpolation_table_ptr++;
	}

	// now linear interpolation between 2 subsamples
	const float scale = (1.f / 0x1000)/(float) 0x1000000; //shorts
	// save lowest 19 bits for interpolation
	int fract_part_fine = fine & 0x7ffff;
	return scale * (a * ( 0x80000 - fract_part_fine) + a2 * fract_part_fine);
}

float soundfontUser::do_interpolation_linear(short* sample_ptr, int fine )
{
	float a = *sample_ptr;
	float a2 = *(sample_ptr+1);

	// now linear interpolation between 2 samples
	const float scale = (1.f / 0x20000)/(float) 0x1000000; //shorts
//	return scale * (a * ( 0x1000000 - fine) + a2 * fine);
	return scale * ( (float) sample_ptr[0] * ( 0x1000000 - fine) + (float) sample_ptr[1] * fine);
}
*/
