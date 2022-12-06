#include "pch.h"
#include <algorithm>
#include <cmath>
#include "ug_switch.h"
#include "ug_switch2.h"

#include "SeAudioMaster.h"
#include "module_register.h"
#include "modules/shared/xp_simd.h"

SE_DECLARE_INIT_STATIC_FILE(ug_switch)

using namespace std;


namespace
{
REGISTER_MODULE_1(L"Switch (1->Many)", IDS_MN_SWITCH_1_MANY,IDS_MG_FLOW_CONTROL,ug_switch ,CF_STRUCTURE_VIEW,L"Used to provide switchable signal routing, or on/off control of a patch cord. Right-click, Properties... to rename choices.");
REGISTER_MODULE_1(L"Switch (Many->1)", IDS_MN_SWITCH_MANY_1,IDS_MG_FLOW_CONTROL,ug_switch2 ,CF_STRUCTURE_VIEW,L"Used to provide switchable signal routing, or on/off control of a patch cord. Right-click, Properties... to rename choices.");
}

ug_switch::ug_switch() :
	out_num(0)
	,cur_state(ST_STOP)
	,current_output_plug(NULL)
	,static_output_counts(0)
	,current_output_plug_number(-1)
{
	//SetFlag(UGF_SSE_OVERWRITES_BUFFER_END;
	SetFlag(UGF_SSE_OVERWRITES_BUFFER_END);
	SET_CUR_FUNC( &ug_switch::sub_process );
}

ug_switch::~ug_switch()
{
	delete [] static_output_counts;
}

#define PLG_CHOICE	0
#define PLG_IN		1
#define PLG_OUT1	2

#define PLG_OUT		1
#define PLG_IN1		2

void ug_switch::ListInterface2(InterfaceObjectArray& PList)
{
	//ug_base::ListInterface2(PList);	// Call base class
	// IO Var, Direction, Datatype, Name, Default, defid (index into ug_base::PlugFormats)
	// defid used to name a enum list or range of values
	LIST_VAR3( L"Choice",out_num, DR_IN,DT_ENUM, L"0", L"{AUTO}", 0, L"Chooses which output to route the input signal to.  Connect to a 'List Entry' module.");
	LIST_PIN2( L"Input", in_ptr, DR_IN, L"0", L"", IO_LINEAR_INPUT, L"");
	LIST_PIN( L"Spare Output", DR_OUT, L"", L"", IO_RENAME|IO_AUTODUPLICATE|IO_CUSTOMISABLE, L"");
}

void ug_switch::onSetPin(timestamp_t /*p_clock*/, UPlug* p_to_plug, state_type p_state)
{
	int in = p_to_plug->getPlugIndex();

	if( current_output_plug == 0 ) //indicates startup.
	{
		// send ST_STATIC on all outputs.
		for( int i = numOutputs - 1 ; i >= 0 ; i-- )
		{
			if( i != out_num )
			{
				OutputChange( SampleClock(), GetPlug(i+PLG_OUT1), ST_STATIC );
			}
		}
	}

	if( in == PLG_CHOICE || current_output_plug == 0 )
	{
		OnNewSetting();
	}

	if( in == PLG_IN )
	{
		cur_state = p_state;

		if( current_output_plug != NULL )
		{
			OutputChange( SampleClock(), current_output_plug, cur_state );
		}
	}

	if( numOutputs < 1 )
	{
		SET_CUR_FUNC( &ug_base::process_sleep );
	}
	else
	{
		if( cur_state == ST_RUN )
		{
			// any prev outputs still clearing buffers?
			SET_CUR_FUNC( &ug_switch::sub_process );

			for(int i = numOutputs-1; i >= 0 ; i-- )
			{
				if( static_output_counts[i] > 0 )
				{
					SET_CUR_FUNC( &ug_switch::sub_process2 );
					break;
				}
			}

			// no sta-tic processing on current output
			static_output_counts[current_output_plug_number - PLG_OUT1] = -1;
		}
		else
		{
			static_output_counts[current_output_plug_number - PLG_OUT1] = AudioMaster()->BlockSize();
			SET_CUR_FUNC( &ug_switch::sub_process_static );
			ResetStaticOutput();
		}
	}
}

void ug_switch::OnNewSetting()
{
	if( numOutputs < 1 )
		return;

	// previous output plug (if exists) goes ST_STATIC.
	if( current_output_plug_number >= 0 )
	{
		static_output_counts[current_output_plug_number - PLG_OUT1] = AudioMaster()->BlockSize();
#ifdef USE_BYPASS
		ClearBypassRoutes(PLG_IN, current_output_plug_number);
#endif
		OutputChange(SampleClock(), current_output_plug, ST_ONE_OFF);
	}

	//	assert( out_num >= 0 );
	// constrain output number to legal range
	int output_number = max( (int)out_num, 0);
	output_number = min( output_number, numOutputs - 1 );
	current_output_plug_number = output_number + PLG_OUT1;
	current_output_plug = GetPlug( current_output_plug_number );
	out_ptr = current_output_plug->GetSamplePtr();
	state_type out_stat = cur_state;

	if( out_stat == ST_STOP )
		out_stat = ST_ONE_OFF;

	OutputChange( SampleClock(), current_output_plug, out_stat );
	ResetStaticOutput();
}

#ifdef USE_BYPASS
void ug_switch::DoProcess(int buffer_offset, int sampleframes)
{
	if( buffer_offset == 0 && process_function == &ug_switch::sub_process && GetPlug(PLG_IN)->isStreaming() && events.empty() )
	{
		if( AddBypassRoute(PLG_IN, current_output_plug_number) )
		{
			SetSampleClock(SampleClock() + sampleframes);
			SleepMode();
			return;
		}
	}

	ug_base::DoProcess(buffer_offset, sampleframes);
}
#endif

void ug_switch::sub_process(int start_pos, int sampleframes)
{
	// process fiddly non-sse-aligned prequel.
	float* i = in_ptr + start_pos;
	float* o = out_ptr + start_pos;

#ifndef GMPI_SSE_AVAILABLE
	// No SSE. Use C++ instead.
	for (int s = sampleframes; s > 0; --s)
	{
		*o++ = *i++;
	}

#else
	// Use SSE instructions.
#ifdef _DEBUG
	if (!GetPlug(PLG_IN)->isStreaming())
	{
		for (int s = 1; s < sampleframes; ++s)
		{
            assert(i[0] == i[s] || (std::isnan(i[0]) && std::isnan(i[s]))); // Is input signal self-consistant? (upstream module has status bug). (weird syntax to handle Nan).
		}
	}
#endif
	while (reinterpret_cast<intptr_t>(o) & 0x0f)
	{
		*o++ = *i++;
		--sampleframes;
	}

	// SSE intrinsics for remainder.
	__m128* pSource = (__m128*) i;
	__m128* pDest = (__m128*) o;

	while (sampleframes > 0)
	{
		*pDest++ = *pSource++;
		sampleframes -= 4;
	}
#endif

}

// feed silence out previous selected outputs, untill buffer full
void ug_switch::sub_process2(int start_pos, int sampleframes)
{
	bool prev_outputs_all_static = true;

	for( int i = numOutputs - 1 ; i >= 0 ; i-- )
	{
		if( static_output_counts[i] > 0 && i != out_num )
		{
			float* out_prev = GetPlug(i+PLG_OUT1)->GetSamplePtr() + start_pos;

			for( int s = sampleframes ; s > 0 ; s-- )
			{
				*out_prev++ = 0.f;
			}

			static_output_counts[i] -= sampleframes;

			// more to do?
			if( static_output_counts[i] > 0 )
			{
				prev_outputs_all_static = false;
			}
		}
	}

	if( prev_outputs_all_static )
	{
		if( process_function == static_cast<process_func_ptr>(&ug_switch::sub_process2) ) // and not sub_process_static
		{
			SET_CUR_FUNC(&ug_switch::sub_process);
		}
	}

	// Now pass input signal.
	sub_process( start_pos, sampleframes );
}

void ug_switch::sub_process_static(int start_pos, int sampleframes)
{
	sub_process2( start_pos, sampleframes );
	SleepIfOutputStatic(sampleframes);
}

int ug_switch::Open()
{
	ug_base::Open();

	numOutputs = GetPlugCount() - 2;
	static_output_counts = new int[numOutputs];

	for( int i = numOutputs - 1 ; i >= 0 ; i-- )
	{
		static_output_counts[i] = AudioMaster()->BlockSize();
	}

	return 0;
}

void ug_switch::Resume()
{
	ug_base::Resume();

	for( int i = numOutputs - 1 ; i >= 0 ; i-- )
	{
		if( static_output_counts[i] > 0 )
		{
			static_output_counts[i] = AudioMaster()->BlockSize();
		}
	}
}

///////////////////////////// SWITCH2 ////////////////////////////////////////////////////


ug_switch2::ug_switch2() :
	safe_in_num(0)
	,unchecked_in_num(0)
	, in_plg(0)
{
	//SetFlag(UGF_SSE_OVERWRITES_BUFFER_END;
	SetFlag(UGF_SSE_OVERWRITES_BUFFER_END);
	SET_CUR_FUNC( &ug_switch2::sub_process );
}

void ug_switch2::ListInterface2(InterfaceObjectArray& PList)
{
	ug_base::ListInterface2(PList);	// Call base class
	// IO Var, Direction, Datatype, Name, Default, defid (index into ug_base::PlugFormats)
	// defid used to name a enum list or range of values
	LIST_VAR3( L"Choice", unchecked_in_num, DR_IN,DT_ENUM, L"0", L"{AUTO}", 0, L"Chooses which output to route the input signal to.  Connect to a 'List Entry' module.");
	LIST_PIN2( L"Output", out_ptr, DR_OUT, L"", L"", 0, L"");
	LIST_PIN( L"Spare Input", DR_IN, L"", L"", IO_LINEAR_INPUT|IO_RENAME|IO_AUTODUPLICATE|IO_CUSTOMISABLE, L"");
}

void ug_switch2::onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state)
{
	int in = p_to_plug->getPlugIndex();

	if( in == PLG_CHOICE )
	{
		//		_RPT2(_CRT_WARN, "ug_switch2::onSetPin %d, Choice=%d\n", SampleClock(), in_num );
		if( numInputs > 0 ) // ensure there are some connections (there don't have to be any)
		{
			OnNewSetting();
			OnOutStatChanged( in_state[safe_in_num] );
		}
		else
		{
			OnOutStatChanged( ST_STOP );
		}
	}
	else // must be an input plug
	{
		assert( SampleClock() == p_clock );
		int input_number = in - PLG_IN1;
		assert( input_number >= 0 );
		in_state[input_number] = p_state;

		if( input_number == safe_in_num )
		{
			OutputChange( SampleClock(), GetPlug( PLG_OUT ), p_state );
			OnOutStatChanged( p_state );
		}
	}
}

void ug_switch2::sub_process(int start_pos, int sampleframes)
{
	float* i = in_plg->GetSamplePtr() + start_pos; // not need to get ptr every time to cope with upstream bypass.
	float* o = out_ptr + start_pos;

#ifndef GMPI_SSE_AVAILABLE
	// No SSE. Use C++ instead.
	for (int s = sampleframes; s > 0; --s)
	{
		*o++ = *i++;
	}
#else
	// Use SSE instructions.
	//	float *in1 = in_ptr + start_pos;
	//	float *out = out_ptr + start_pos;
	//
	//	for( int s = sampleframes ; s > 0 ; s-- )
	//	{
	//		*out++ = *in1++;
	////		*out++ = *in1++ + 1.f; // deliberate fail passthru test
	//	}
	// process fiddly non-sse-aligned prequel.
	while (reinterpret_cast<intptr_t>(o) & 0x0f)
	{
		*o++ = *i++;
		--sampleframes;
	}

	// SSE intrinsics for remainder.
	__m128* pSource = (__m128*) i;
	__m128* pDest = (__m128*) o;

	while (sampleframes > 0)
	{
		*pDest++ = *pSource++;
		sampleframes -= 4;
	}
#endif
}

void ug_switch2::sub_process_no_inputs(int start_pos, int sampleframes)
{
	float* out = out_ptr + start_pos;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		*out++ = 0.0f;
	}

	SleepIfOutputStatic(sampleframes);
}

void ug_switch2::sub_process_static(int start_pos, int sampleframes)
{
	sub_process( start_pos, sampleframes );
	SleepIfOutputStatic(sampleframes);
}

#ifdef USE_BYPASS
void ug_switch2::DoProcess(int buffer_offset, int sampleframes)
{
	if( buffer_offset == 0 && process_function == &ug_switch2::sub_process && GetPlug(PLG_IN1)->isStreaming() && events.empty() )
	{
		if( AddBypassRoute(safe_in_num + PLG_IN1, PLG_OUT) )
		{
			SetSampleClock(SampleClock() + sampleframes);
			SleepMode();
			return;
		}
	}

	ug_base::DoProcess(buffer_offset, sampleframes);
}
#endif

void ug_switch2::OnNewSetting()
{
#ifdef USE_BYPASS
	ClearBypassRoutes(safe_in_num + PLG_IN1, PLG_OUT);
#endif

	// constrain output number to legal range. UI can send if user connects combo to different list, some patch may be illeagal vals
	safe_in_num = max( unchecked_in_num, (short)0);
	safe_in_num = min( safe_in_num, (short)(numInputs -1) );
	state_type new_out_stat = ST_ONE_OFF;

	if( safe_in_num >= 0 )
	{
		in_plg = GetPlug(safe_in_num + PLG_IN1); // ->GetSamplePtr(); never cache buffer ptr.
		new_out_stat = in_state[safe_in_num];
		new_out_stat = max(new_out_stat,ST_ONE_OFF);
	}

	OutputChange( SampleClock(), GetPlug( PLG_OUT ), new_out_stat );
}

int ug_switch2::Open()
{
	ug_base::Open();
	SetFlag(UGF_SSE_OVERWRITES_BUFFER_END);
	numInputs = GetPlugCount() - 2;
	// initialise input state array
	in_state.assign( numInputs, ST_STOP );
	OnNewSetting();
	return 0;
}

void ug_switch2::OnOutStatChanged( state_type /*new_state*/ )
{
	if( numInputs < 1 ) // no connections yet
	{
		SET_CUR_FUNC( &ug_switch2::sub_process_no_inputs );
		return;
	}

	if( in_state[safe_in_num] == ST_RUN )
	{
		SET_CUR_FUNC( &ug_switch2::sub_process );
	}
	else
	{
		SET_CUR_FUNC( &ug_switch2::sub_process_static );
		ResetStaticOutput();
	}
}
