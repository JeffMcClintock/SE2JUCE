#pragma once

#include <vector>
#include "math.h"
#include "assert.h"
#include "csoundfont.h"

class Jzone;
class SoundfontPatch;
struct sfSample;

struct sampleChannel
{
	int s_increment = 1;
	short* s_ptr = {};
	unsigned int s_ptr_fine = 0;
	short* s_end_section = {};
	short* s_loop_st = {};
	short* s_loop_end = {};
	short* s_end = {};
	sfSample* cur_sample = {};
	char loop_mode = 0;

	static short silence[8];

#if defined( _DEBUG )
	short* sampleStartAddress = {};
#endif

	short* const IncrementPointer(bool gateState)
	{
		s_ptr_fine += s_increment;
		if (s_ptr_fine > 0xffffff)
		{
			s_ptr += s_ptr_fine >> 24;
			s_ptr_fine &= 0xffffff;

			if (s_ptr >= s_end_section)
			{
				DoLoop(gateState);
			}
			//			s_ptr_l = s_ptr_r + s_ptr_l_offset;
		}

		assert((s_ptr >= sampleStartAddress && s_ptr <= s_end)
			|| (s_ptr >= silence + 3 && s_ptr <= silence + 7)
		);

#if 0
		// near loop point?
		const auto distToLoop = s_loop_end - s_ptr_r;
		if (distToLoop <= 4)
		{
			return { clickBufferL + 8 - distToLoop , clickBufferR + 8 - distToLoop };
		}
#endif
		return s_ptr;
	}

	void DoLoop(bool gateState)
	{
		// has loop ended, or entire sample ended?
		// loop mode 0 - None, 1 - continuous loop, 3 loop while key on
		if (loop_mode == 0 || (loop_mode == 3 && gateState == false))
		{
			s_end_section = s_end;

			// has entire sample ended?
			if (s_ptr >= s_end)
			{
				// switch output to silent sample.
				s_ptr = /*s_ptr_l =*/ s_loop_st = &(silence[3]);
				s_loop_end = &(silence[7]);
				s_increment = 0;

#if defined( _DEBUG )
				sampleStartAddress = silence;
#endif
			}
		}
		else
		{
			// very safe way to loop, even with extreme increments
			s_ptr = s_loop_st + ((s_ptr - s_loop_end) % (s_loop_end - s_loop_st));
			assert(s_ptr >= sampleStartAddress && s_ptr <= s_end);
		}
	}


	inline bool IsDone()
	{
		return s_loop_st == &(silence[3]);
	}

#if 0
	// buffer enough samples to avoid clicks near start of end of sample.
	// 4 before loop, 4 after.
	short clickBufferL[12];
	short clickBufferR[12];

	void init()
	{
		// init loop-end declicking buffers. 8 samples before loop-end thru 4 samples after loop-start
		if (s_loop_st && s_loop_end)
		{
			assert(s_loop_st < s_loop_end);

			const int loopLength = s_loop_end - s_loop_st;
			auto r = s_loop_st;
			for (int i = 8; i < 12; i++)
			{
				clickBufferR[i] = *r;
				clickBufferL[i] = *(r + s_ptr_l_offset);
				if (++r >= s_loop_end)
					r = s_loop_st;
			}

			r = s_loop_end - 1;
			for (int i = 7; i >= 0; i--)
			{
				clickBufferR[i] = *r;
				clickBufferL[i] = *(r + s_ptr_l_offset);
				if (--r < s_loop_st)
					r = s_loop_end - 1;
			}
		}
	}

#endif
};

struct partial
{
	inline bool IsStereo()
	{
		return left.cur_sample != 0;
	}

	inline void CalculateIncrement( float p_pitch )
	{
		float transposition = scale_tune * (p_pitch - root_pitch);
		float ratio = powf(2.f,transposition) * relative_sample_rate;
		float temp_float = 0x1000000 * ratio;
		left.s_increment = (int)temp_float;
		right.s_increment = (int)temp_float;
	}

	void SetPan( float pan ); // 0.5 = hard right. -0.5 = hard left.
	bool UsesPanning()
	{
//		return pan_right_level != 1.0f; ??WTF??
		return pan_right_level != pan_left_level;
	}
	bool IsDone()
	{
		return right.IsDone() && (left.cur_sample == 0 || left.IsDone());
	}

	sampleChannel left;
	sampleChannel right;

	float root_pitch; // the input voltage coresponding to root pitch
	float scale_tune;
	float relative_sample_rate;

	float pan_left_level; // calculated from pan.
	float pan_right_level;

	Jzone* zone = {};
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
