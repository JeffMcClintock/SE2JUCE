#include "pch.h"
#include <math.h>
#include "ug_generic_1_1.h"
#include "UgDatabase.h"

SE_DECLARE_INIT_STATIC_FILE(ug_generic_1_1)

// Fill an array of InterfaceObjects with plugs and parameters
void ug_generic_1_1::ListInterface2(InterfaceObjectArray& PList)
{
	LIST_PIN2( L"Signal in",in1_ptr,  DR_IN, L"0", L"", IO_POLYPHONIC_ACTIVE, L"");
	LIST_PIN2( L"Signal Out",out1_ptr,  DR_OUT, L"", L"", 0, L"");
}

void ug_generic_1_1::onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state)
{
	//	Wake();
	OutputChange( p_clock, GetPlug(PN_OUT1), p_state );

	if( p_state == ST_RUN )
	{
		SET_CUR_FUNC( &ug_generic_1_1::sub_process );
	}
	else
	{
		ResetStaticOutput();
		SET_CUR_FUNC( &ug_generic_1_1::sub_process_static );
	}
}

void ug_generic_1_1::sub_process_static(int start_pos, int sampleframes)
{
	sub_process(start_pos, sampleframes);
	SleepIfOutputStatic(sampleframes);
}

int ug_generic_1_1::Open()
{
	SET_CUR_FUNC( &ug_generic_1_1::sub_process );
	return ug_base::Open();
}

///////////////////// RECTIFIER ///////////////////////////
namespace

{
    REGISTER_MODULE_1(L"Rectifier", IDS_MN_RECTIFIER,IDS_MG_MODIFIERS,ug_rectifier ,CF_STRUCTURE_VIEW,L"'Flips' negative voltages to positive");
}

void ug_rectifier::sub_process(int start_pos, int sampleframes)
{
    float* in = in1_ptr + start_pos;
    float* out = out1_ptr + start_pos;

    for( int s = sampleframes ; s > 0 ; s-- )
    {
        *out++ = fabs( *in++ );
    }
}


