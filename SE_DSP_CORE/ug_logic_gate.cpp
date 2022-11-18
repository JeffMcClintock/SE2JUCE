#include "pch.h"
#include <algorithm>
#include "ug_logic_gate.h"

#include "Logic.h"
#include "module_register.h"

SE_DECLARE_INIT_STATIC_FILE(ug_logic_gate)

using namespace std;

namespace
{
REGISTER_MODULE_1_BC(57,L"NOR Gate", IDS_MN_NOR_GATE,IDS_MG_LOGIC,ug_logic_NOR ,CF_STRUCTURE_VIEW,L"Simulated Logic Gate. Output goes high if all inputs low");
REGISTER_MODULE_1_BC(55,L"NAND Gate", IDS_MN_NAND_GATE,IDS_MG_LOGIC,ug_logic_NAND ,CF_STRUCTURE_VIEW,L"Simulated Logic Gate. Output goes high when any inputs low.");
REGISTER_MODULE_1_BC(54,L"XOR Gate", IDS_MN_XOR_GATE,IDS_MG_LOGIC,ug_logic_XOR ,CF_STRUCTURE_VIEW,L"Simulated Logic Gate. Output goes high when exactly one input high.");
REGISTER_MODULE_1_BC(53,L"OR Gate", IDS_MN_OR_GATE,IDS_MG_LOGIC,ug_logic_OR ,CF_STRUCTURE_VIEW,L"Simulated Logic Gate. Output goes high when any one input high.");
REGISTER_MODULE_1_BC(52,L"AND Gate", IDS_MN_AND_GATE,IDS_MG_LOGIC,ug_logic_AND ,CF_STRUCTURE_VIEW,L"Simulated Logic Gate. Output goes high when all inputs high.");
}

ug_logic_gate::ug_logic_gate() :
	output(-99.f) // impossible initial val to force stat-change event on output
	,input_count(0)
	,in_ptr(NULL)
	,output_changed( true )
{
	SET_CUR_FUNC( &ug_logic_gate::sub_process );
}

ug_logic_gate::~ug_logic_gate()
{
	delete [] in_ptr;
}

void ug_logic_gate::ListInterface2(InterfaceObjectArray& PList)
{
	//	ug_base::ListInterface2(PList);	// Call base class
	// IO Var, Direction, Datatype, Name, Default, defid (index into unit_gen::PlugFormats)
	// defid used to name a enum list or range of values
	LIST_PIN2( L"Output", output_blk, DR_OUT, L"", L"", 0, L"");
	LIST_PIN( L"Input", DR_IN, L"", L"", IO_POLYPHONIC_ACTIVE | IO_AUTODUPLICATE|IO_CUSTOMISABLE, L"");
}

int ug_logic_gate::Open()
{
	input_count = GetPlugCount() - 1;
	in_level.assign(input_count, false);
	in_state.assign(input_count, ST_STATIC);
	in_ptr = new float*[input_count];

	ug_base::Open();
	RUN_AT( SampleClock(), &ug_logic_gate::OnFirstSample );
	return 0;
}

void ug_logic_gate::OnFirstSample()
{
	// need to caclulate initial state.
	// NOTE: All plugs receive an initial ONE_OFF state change,
	// but that won't nesc update the output if all inputs are zero (NOR gate requires output to go HI in this case)
	RecalcOutput( SampleClock() );
}

void ug_logic_gate::onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state)
{
	int pn = p_to_plug->getPlugIndex() - 1; // -1 for output plug

	if( pn >= input_count )
		return;

	// check to see which input has changed, record its state
	overall_input_status = ST_STOP;
	in_state[pn] = p_state;

	for( int c = (int) in_state.size() - 1 ; c >= 0 ; c-- )
	{
		if( c != pn && in_state[c] == ST_ONE_OFF ) // ignore older one_offs
		{
			in_state[c] = ST_STOP;
		}

		overall_input_status = max( overall_input_status, in_state[c] );
	}

	SET_CUR_FUNC( &ug_logic_gate::sub_process ); // wake up
}

void ug_logic_gate::sub_process(int start_pos, int sampleframes)
{
	for( int input_num = 0 ; input_num < input_count ; input_num++ )
	{
		in_ptr[input_num] = GetPlug(input_num+1)->GetSamplePtr() + start_pos;
	}

	float* out = output_blk + start_pos;
	timestamp_t l_sampleclock = SampleClock();
	int static_sample_count = sampleframes;
	bool change_flag = false;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		for( int input_num = 0 ; input_num < input_count ; input_num++ )
		{
			float cur_in_level = *(in_ptr[input_num]++);
			bool new_level;

			if( cur_in_level < LOGIC_LO )
			{
				new_level = false;
			}
			else
			{
				if( cur_in_level > LOGIC_HI )
				{
					new_level = true;
				}
				else
				{
					new_level = in_level[input_num]; // indeterminate, use existing value
				}
			}

			if( in_level[input_num] != new_level )
			{
				change_flag = true;
				in_level[input_num] = new_level;
			}
		}

		if( change_flag == true )
		{
			RecalcOutput( l_sampleclock );
			change_flag = false;
			static_sample_count = s;
		}

		*out++ = output;
		l_sampleclock++;
	}

	// note: only decrementing sta-tic output count by actual number of unchanging samples
	if( overall_input_status < ST_RUN )
	{
		SleepIfOutputStatic( static_sample_count );
	}
}

void ug_logic_gate::RecalcOutput( timestamp_t p_sample_clock )
{
	float new_output = 0.f;

	if( OnInputLogicChange() == true )
	{
		new_output = 0.5f; // 5 Volts
	}

	if( output != new_output )
	{
		output_changed = true;
		output = new_output;
		OutputChange( p_sample_clock, GetPlug(L"Output"), ST_ONE_OFF );
		//			_RPT1(_CRT_WARN, "ug_logic_AND::tick() output = %f\n", SampleToVoltage(output) );
		ResetStaticOutput();
		SET_CUR_FUNC( &ug_logic_gate::sub_process );
	}
}

//////////////////////////////////////////////////////////////////////////////////
bool ug_logic_XOR::OnInputLogicChange()
{
	int count = 0;

	for( int c = 0 ; c < input_count ; c++ )
	{
		if( in_level[c] == true )
		{
			count++;
		}
	}

	//return count == 1;
	// In general, an XOR gate gives an output value of 1
	// when there are an odd number of 1's on the inputs to the gate
	return (count & 1) == 1;
}

bool ug_logic_OR::OnInputLogicChange()
{
	for( int c = 0 ; c < input_count ; c++ )
	{
		if( in_level[c] == true )
		{
			return true;
		}
	}

	return false;
}

bool ug_logic_NOR::OnInputLogicChange()
{
	for( int c = 0 ; c < input_count ; c++ )
	{
		if( in_level[c] == true )
		{
			return false;
		}
	}

	return true;
}

bool ug_logic_NAND::OnInputLogicChange()
{
	for( int c = 0 ; c < input_count ; c++ )
	{
		if( in_level[c] == false )
		{
			return true;
		}
	}

	return false;
}

bool ug_logic_AND::OnInputLogicChange()
{
	for( int c = 0 ; c < input_count ; c++ )
	{
		if( in_level[c] == false )
		{
			return false;
		}
	}

	return true;
}

