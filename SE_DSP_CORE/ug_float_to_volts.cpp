#include "pch.h"
#include "ug_float_to_volts.h"
#include "module_register.h"

SE_DECLARE_INIT_STATIC_FILE(ug_float_to_volts)

#define PD_SMOOTHING	0
#define PD_VALUE_IN		1
#define PD_VALUE_OUT	2

namespace
{
static pin_description plugs[] =
{
	L"Smoothing", DR_IN, DT_ENUM	,L"2", L"None,Fast (4 samp),Smooth (30ms)", 0, L"",
	L"Float In"	, DR_IN, DT_FLOAT	,L"", L"", 0,  L"",// fix ui_param_float_in::Up grade() if renaming
	L"Volts Out", DR_OUT, DT_FSAMPLE,L"", L"", 0,  L"",// fix ui_param_float_in::Up grade() if renaming
};

static module_description_dsp mod =
{
	"FloatToVolts", IDS_FLOAT_TO_VOLTS, IDS_MG_CONVERSION , &ug_float_to_volts::CreateObject, CF_STRUCTURE_VIEW,plugs, sizeof(plugs)/sizeof(pin_description)
};

bool res = ModuleFactory()->RegisterModule( mod );
}

void ug_float_to_volts::assignPlugVariable(int p_plug_desc_id, UPlug* p_plug)
{
	switch( p_plug_desc_id )
	{
	case PD_VALUE_IN:
		p_plug->AssignVariable( &m_input );
		break;

	case PD_SMOOTHING:
		p_plug->AssignVariable( &m_smoothing );
		break;

	default:
		break;
	};
}

int ug_float_to_volts::Open()
{
	ug_base::Open();
	SET_CUR_FUNC( &ug_float_to_volts::sub_process );
	value_out_so.SetPlug( GetPlug(PD_VALUE_OUT) );
	SET_CUR_FUNC( &ug_base::process_sleep );
	//	RUN_AT( SampleClock(), OnFirstSample );
	return 0;
}

/* seemed to get called after some stat changes, renderiung it useless
void ug_float_to_volts::OnFirstSample()
{
	// jump to initial value on startup, regardless off smoothing. (doesn't work in Open())
	value_out_so.Set( SampleClock(), m_input * 0.1f, 0 );
}
*/
void ug_float_to_volts::Resume()
{
	ug_base::Resume();
	value_out_so.Resume();
}

void ug_float_to_volts::sub_process(int start_pos, int sampleframes)
{
	bool can_sleep = true;
	value_out_so.Process( start_pos, sampleframes, can_sleep);

	if( can_sleep )
	{
		SET_CUR_FUNC( &ug_base::process_sleep );
	}
}

void ug_float_to_volts::onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state)
{
	if( p_to_plug == GetPlug( PD_VALUE_IN ) )
	{
		int smooth_amnt = 0;

		if( p_clock != 0 ) // must jump to initial value at startup
		{
			switch( m_smoothing )
			{
			case 1:
				smooth_amnt = 4;
				break;

			case 2:
				smooth_amnt = (int)getSampleRate() / 32; // allow 1/32 sec for transition
				break;

            case 0:
				break;

			default:
				assert(false && "invalid enum value");
			};
		}

		value_out_so.Set( p_clock, m_input * 0.1f, smooth_amnt );
		SET_CUR_FUNC( &ug_float_to_volts::sub_process );
	}
}
