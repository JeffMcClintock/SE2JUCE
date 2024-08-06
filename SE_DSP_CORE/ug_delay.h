// ug_delay module
//
#pragma once
#include "ug_base.h"
#include "modules/shared/xp_simd.h"

#define interpolationExtraSamples 1

inline void CalculateModulation(float modulationOffset, float*& modulation, int buffer_size, int padded_buffer_size, int& read_offset_int, float&read_offset_fine)
{
	float readOffset = ( modulationOffset - *modulation ) * buffer_size + ( padded_buffer_size - buffer_size );
//	read_offset_int = FastRealToIntTruncateTowardZero(readOffset)); // fast float-to-int using SSE. truncation toward zero.
	read_offset_int = FastRealToIntTruncateTowardZero(readOffset);

	read_offset_fine = readOffset - (float) read_offset_int;

	if( read_offset_int < 1 )
	{
		read_offset_int = 1;
		read_offset_fine = 0.0f;
	}
	else
	{
		if( read_offset_int >= padded_buffer_size )
		{
			read_offset_int = padded_buffer_size;
			read_offset_fine = 0.0f;
		}
	}
	++modulation;
};

class PolicyModulationDigitalChanging
{
public:
	inline static void CalculateInitial(float modulationOffset, float*& modulation, int buffer_size, int padded_buffer_size, int& read_offset_int, float&read_offset_fine)
	{
		// do nothing. Hopefully optimizes away to nothing.
	}
	inline static void Calculate(float modulationOffset, float*& modulation, int buffer_size, int padded_buffer_size, int& read_offset_int, float&read_offset_fine)
	{
		CalculateModulation(modulationOffset, modulation, buffer_size, padded_buffer_size, read_offset_int, read_offset_fine);
	}
};

class PolicyModulationFixed
{
public:
	inline static void CalculateInitial(float modulationOffset, float*& modulation, int buffer_size, int padded_buffer_size, int& read_offset_int, float&read_offset_fine)
	{
		CalculateModulation(modulationOffset, modulation, buffer_size, padded_buffer_size, read_offset_int, read_offset_fine);
	}

	inline static void Calculate(float modulationOffset, float*& modulation, int buffer_size, int padded_buffer_size, int& read_offset_int, float&read_offset_fine)
	{
		// do nothing. Hopefully optimizes away to nothing.
	}
};

class PolicyFeedbackOff
{
public:
	inline static float Calculate(  float prev_out, float*&feedback  )
	{
		return 0.0f;
	}
};
class PolicyFeedbackModulated
{
public:
	inline static float Calculate( float prev_out, float*&feedback )
	{
		return prev_out * *feedback++;
	}
};

class PolicyFeedbackFixed
{
public:
	inline static float Calculate( float prev_out, float*&feedback  )
	{
		return prev_out * *feedback;
	}
};

class PolicyInterpolationCubic
{
public:
	inline static float Calculate( int count, float* buffer, int read_offset, float fraction, int buffer_size )
	{
		int index = count + read_offset - 1;

		if( index >= buffer_size )
		{
			index -= buffer_size;
		}

		assert( index >= 0 && index < buffer_size + interpolationExtraSamples );

		// Buffer has 3 extra samples 'off-end' filled with duplicate samples from buffer start. Avoid need to handle wrapping.
		float y0 = buffer[ index+0 ];
		float y1 = buffer[ index+1 ];
		float y2 = buffer[ index+2 ];
		float y3 = buffer[ index+3 ];

		return y1 + 0.5f * fraction*(y2 - y0 + fraction*(2.0f*y0 - 5.0f*y1 + 4.0f*y2 - y3 + fraction*(3.0f*(y1 - y2) + y3 - y0)));
	}
};

class PolicyInterpolationLinear
{
public:
	inline static float Calculate(int count, float* buffer, int read_offset, float read_offset_fine, int padded_size)
	{
		int index = count + read_offset;

		if( index >= padded_size )
		{
			index -= padded_size;
		}

		assert(index >= 0 && index < padded_size );
		assert(index != count || read_offset_fine < 0.001f); // can't read the value past we just wrote?

		// Buffer has 3 extra samples 'off-end' filled with duplicate samples from buffer start. Avoid need to handle wrapping.
		float y0 = buffer[ index+0 ];
		float y1 = buffer[ index+1 ];

		return y0 + read_offset_fine * ( y1 - y0 );
	}
};

class PolicyInterpolationNone
{
public:
	inline static float Calculate(int count, float* buffer, int read_offset, float read_offset_fine, int padded_size)
	{
		int index = count + read_offset;

		if( index >= padded_size )
		{
			index -= padded_size;
		}

		assert(index >= 0 && index < padded_size + interpolationExtraSamples);
		return buffer[ index ];
	}
};


class ug_delay : public ug_base
{
public:
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_delay);

	template< class ModulationPolicy, class InterpolationPolicy, class FeedbackPolicy >
	void subProcess( int bufferOffset, int sampleFrames )
	{
		float* signalIn		= bufferOffset + input_ptr;
		float* modulation	= bufferOffset + modulation_ptr;
		float* signalOut	= bufferOffset + output_ptr;
		float* feedback		= bufferOffset + feedback_ptr;

		float prev_out = m_prev_out;

		int read_offset;
		float read_offset_fine;
		ModulationPolicy::CalculateInitial( m_modulation_input_offset, modulation, buffer_size, padded_size, read_offset, read_offset_fine );

		for( int s = sampleFrames; s > 0; --s )
		{
			ModulationPolicy::Calculate(m_modulation_input_offset, modulation, buffer_size, padded_size, read_offset, read_offset_fine);

			buffer[count] = *signalIn++ + FeedbackPolicy::Calculate( prev_out, feedback );

			// mirror initial few samples "off end" too, to simplify interpolation.
			if( count < interpolationExtraSamples )
			{
				buffer[count + padded_size] = buffer[count];
			}

			*signalOut++ = prev_out = InterpolationPolicy::Calculate(count, buffer, read_offset, read_offset_fine, padded_size);

			// increment count.
			if( ++count == padded_size )
			{
				count = 0;
			}
		}

		m_prev_out = prev_out;
	}

	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state) override;
	void sub_process_static(int start_pos, int sampleframes);

	~ug_delay();
	ug_delay();
	int Open() override;
	void CreateBuffer();

	void resetStaticCounter()
	{
		static_output_count = buffer_size + AudioMaster()->BlockSize();
	}

protected:
	float delay_time;
	float m_modulation_input_offset; // for backward compatability
	bool interpolate;
	int count;
	int buffer_size;
	int padded_size;

	float* buffer;
	float* input_ptr;
	float* output_ptr;
	float* modulation_ptr;
	float* feedback_ptr;
	process_func_ptr current_process_func;
	float m_prev_out;
};

class ug_delay2 :

	public ug_delay

{

public:

	ug_delay2()
	{
		m_modulation_input_offset = 1.f;
	}

	DECLARE_UG_INFO_FUNC2;

	DECLARE_UG_BUILD_FUNC(ug_delay2);

};