#include "pch.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include "ug_test_tone.h"

#include "ug_test_tone.h"

#include "module_register.h"


SE_DECLARE_INIT_STATIC_FILE(ug_test_tone)

namespace
{
REGISTER_MODULE_1(L"1 KHz Tone", IDS_MN_1_KHZ_TONE,IDS_MG_DIAGNOSTIC,ug_test_tone ,CF_STRUCTURE_VIEW,L"Produces a pure 1 KHz tone");
}

//#define d2PI 6.28318530717958647692

// define plug numbers as constants
//#define PN_INPUT 0
#define PN_OUTPUT 0

// Fill an array of InterfaceObjects with plugs and parameters
IMPLEMENT_UG_INFO_FUNC2(ug_test_tone)
{
	// IO Var, Direction, Datatype, ug_test_tone, ppactive, Default,Range/enum list
	//    LIST_VAR3( L"Enum in",input, DR_IN,DT_ENUM, L"0", L"cat,dog,moose", 0, L"Choice of stuff");
	// TODO set IO_POLYPHONIC_ACTIVE on in plugs if this module can't agregate voices (performs non-linear processing)
	//    LIST_PIN( L"Signal in",  DR_IN, L"0", L"", IO_POLYPHONIC_ACTIVE, L"Input signal");
	LIST_PIN2( L"Signal Out",out_ptr, DR_OUT, L"0", L"", 0, L"Output Signal");
}

ug_test_tone::ug_test_tone()
{
	SET_CUR_FUNC( &ug_test_tone::sub_process );
}

// This is called when the input changes state
// IF IT RECEIVES ONE_OFF CHANGE MESSAGE, REMEMBER THAT the input concerned
// won't receive the new sample untill the next sample clock. So don't try
// to access any inputs values
void ug_test_tone::onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state)
{
	//int in = p_to_plug->getPlugIndex();
	//	if( in == PN_INPUT ) // input
	{
		/*
				cur_state = p_state;
				if( current_output_plug != NULL )
				{
					OutputChange( SampleClock(), current_output_plug, cur_state );
				}
		*/
	}
}


int ug_test_tone::Open()
{
	ug_base::Open(); // call base class
	// NOTES: don't try to access input plug values here if posible
	//	count = 0;
	//	run_function( Process, ST_RUN); //Enable Process function
	OutputChange( SampleClock(), GetPlug(PN_OUTPUT), ST_RUN );
	m_phase = 0.f;
	// 1kHz test tone, 5V peak
	m_increment = M_PI * 2.0 * 1000. / getSampleRate();
	return 0;
}

void ug_test_tone::sub_process(int start_pos, int sampleframes)
{
	float* out = out_ptr + start_pos;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		m_phase += m_increment;
		*out++ = 0.5f * sinf( static_cast<float>(m_phase) );

		if( m_phase > (M_PI * 2.0))
		{
			m_phase -= (M_PI * 2.0);
		}
	}
}
