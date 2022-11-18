#include "pch.h"
#include "ug_monostable.h"
#include "Logic.h"
#include "SeAudioMaster.h"
#include "module_register.h"

SE_DECLARE_INIT_STATIC_FILE(ug_monostable)

using namespace std;

namespace
{
REGISTER_MODULE_1_BC(89,L"Monostable", IDS_MN_MONOSTABLE,IDS_MG_LOGIC,ug_monostable ,CF_STRUCTURE_VIEW,L"When triggered, produces a fixed length pulse");
}

#define PN_PULSE_LEN	1
#define PN_OUT1			2

// Fill an array of InterfaceObjects with plugs and parameters
void ug_monostable::ListInterface2(InterfaceObjectArray& PList)
{
	LIST_PIN2( L"Signal in",in1_ptr,  DR_IN, L"0", L"", IO_POLYPHONIC_ACTIVE, L"");
	LIST_PIN( L"Pulse Length (ds)",  DR_IN, L"0.1", L"", IO_POLYPHONIC_ACTIVE, L"10 Volt = 1 Second");
	LIST_PIN2( L"Signal Out",out1_ptr,  DR_OUT, L"", L"", 0, L"");
}

void ug_monostable::sub_process(int start_pos, int sampleframes)
{
	float* in = in1_ptr + start_pos;
	float* out = out1_ptr + start_pos;
	int count = sampleframes;
	timestamp_t l_sampleclock = SampleClock();

	while( count > 0 )
	{
		if( l_sampleclock == pulse_off_time )
		{
			//			SetSampleClock( l_sampleclock );
			//			pulse_end();
			pulse_off_time = SE_TIMESTAMP_MAX;
			output = 0.f;
			SET_CUR_FUNC( &ug_monostable::sub_process );
			ResetStaticOutput();
			OutputChange( l_sampleclock, GetPlug(PN_OUT1), ST_ONE_OFF );
		}

		timestamp_t todo = count;
		timestamp_t time_to_off = pulse_off_time - l_sampleclock;
		todo = min(todo, time_to_off );
		int done = sub_process2(in, out, (int) todo);
		count -= done;
		l_sampleclock += done;
		in += done;
		out += done;
	}
}

int ug_monostable::sub_process2(float* in, float* out, int count)
{
	assert( count > 0 );

	float* first = in;
	float* last = in + count;

	if( cur_state )
	{
		while( *in >= LOGIC_LO && in < last )
		{
			in++;
			*out++ = output;
		}
	}
	else
	{
		while( *in <= LOGIC_HI && in < last )
		{
			in++;
			*out++ = output;
		}
	}

	int done = (int)(in - first);
	SetSampleClock( SampleClock() + done );

	if( in_state < ST_RUN )
	{
		SleepIfOutputStatic(done);
	}

	if( in < last )
	{
		input_changed();
	}

	return done;
}


void ug_monostable::onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state)
{
	//	Wake();
	// any input running?
	if( p_to_plug == GetPlug(0) )
	{
		in_state = p_state;

		if( in_state == ST_RUN )
		{
			SET_CUR_FUNC( &ug_monostable::sub_process );
			//why			ResetStaticOutput();
		}
		else
		{
			input_changed();
		}
	}
}

ug_monostable::ug_monostable() :
	output(0.0f)
	,cur_state(false)
	,in_state(ST_STOP)
	,pulse_off_time(SE_TIMESTAMP_MAX)
{
}

int ug_monostable::Open()
{
	SET_CUR_FUNC( &ug_monostable::sub_process );
	// This must happen b4 first stat change arrives
	RUN_AT( SampleClock(), &ug_monostable::OnFirstSample );
	return ug_base::Open();
}

void ug_monostable::OnFirstSample()
{
	// need output initial state.
	// NOTE: All plugs receive an initial ONE_OFF state change,
	// but that won't nesc update the output
	OutputChange( SampleClock(), GetPlug(PN_OUT1), ST_ONE_OFF );
}

void ug_monostable::input_changed()
{
	if( check_logic_input( GetPlug(0)->getValue(), cur_state ) )
	{
		if( cur_state && output == 0.f )
		{
			output = 0.5;
			SET_CUR_FUNC( &ug_monostable::sub_process );
			ResetStaticOutput();
			OutputChange( SampleClock(), GetPlug(PN_OUT1), ST_ONE_OFF );
			float delay = GetPlug(PN_PULSE_LEN)->getValue();
			delay = delay * getSampleRate();
			delay = max( delay, 1.f );
			pulse_off_time = SampleClock() + (int) delay;

			// if pulse end not due during this block, use delayed event
			if( delay > AudioMaster()->BlockSize() )
			{
				RUN_AT( pulse_off_time, &ug_monostable::pulse_end );
			}
		}
	}
}

void ug_monostable::pulse_end()
{
	pulse_off_time = SE_TIMESTAMP_MAX;
	output = 0.f;
	SET_CUR_FUNC( &ug_monostable::sub_process );
	ResetStaticOutput();
	OutputChange( SampleClock(), GetPlug(PN_OUT1), ST_ONE_OFF );
}

