#include "pch.h"
#include "ug_random.h"
#include "Logic.h"
#include "conversion.h"
#include "module_register.h"

SE_DECLARE_INIT_STATIC_FILE(ug_random)

namespace
{
REGISTER_MODULE_1( L"random", IDS_MN_RANDOM,IDS_MG_SPECIAL,ug_random,CF_STRUCTURE_VIEW,L"Generates a random voltage when triggered");
}

#define PLG_OUT 1

// Fill an array of InterfaceObjects with plugs and parameters
void ug_random::ListInterface2(InterfaceObjectArray& PList)
{
	LIST_VAR3( L"Trigger in",m_trigger,  DR_IN,DT_BOOL, L"1", L"", IO_POLYPHONIC_ACTIVE, L"");
	LIST_PIN( L"Output", DR_OUT, L"", L"", 0, L"");
}

// invert input, unless indeterminate voltage
void ug_random::sub_process(int start_pos, int sampleframes)
{
	bool can_sleep = true;
	output_so.Process( start_pos, sampleframes, can_sleep );

	if( can_sleep )
	{
		SET_CUR_FUNC( &ug_base::process_sleep );
	}
}

void ug_random::onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state)
{
	if( m_trigger || p_clock == 0)
		ChangeOutput();
}

ug_random::ug_random() :
	output_so(SOT_LINEAR)
{
}

int ug_random::Open()
{
	ug_base::Open();
	output_so.SetPlug( GetPlug(PLG_OUT) );
	return 0;
}

void ug_random::Resume()
{
	ug_base::Resume();
	output_so.Resume();
}

void ug_random::ChangeOutput()
{
	constexpr float scale = 1.f / (float)RAND_MAX;
	float output_val = scale * (float) rand();
	output_so.Set( SampleClock(), output_val, 1 );
	SET_CUR_FUNC( &ug_random::sub_process );
	//	ResetStaticOutput();
}