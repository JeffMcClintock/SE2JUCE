#include "pch.h"
#include "ug_logic_complex.h"
#include "Logic.h"

using namespace std;

ug_logic_complex::ug_logic_complex() :
	in_ptr(NULL)
	,outputs(0)
	,inputs_running(false)
{
	SET_CUR_FUNC( &ug_logic_complex::sub_process );
}

ug_logic_complex::~ug_logic_complex()
{
	delete [] in_ptr;
	delete [] outputs;
}

void ug_logic_complex::process_run(int start_pos, int sampleframes)
{
	/*
	for( int input_num = 0 ; input_num < numInputs ; input_num++ )
	{
	//		in_ ptr[input_num] = input_array[input_num]->GetBlock() + start_pos;
		in_ptr[input_num] = GetPlug(input_num+1)->Get SamplePtr() + start_pos;
	}*/
	int input_num = 0;

	//for( int p = 0 ; p < plugs.GetSize() ; p++ )
	for( vector<UPlug*>::iterator it = plugs.begin() ; it != plugs.end() ; ++it )
	{
		UPlug* plg = *it;

		if( plg->DataType == DT_FSAMPLE && plg->Direction == DR_IN )
		{
			in_ptr[input_num++] = plg->GetSamplePtr() + start_pos;
		}
	}

	timestamp_t l_sampleclock = SampleClock();
	bool change_flag = false;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		for( int input_num = 0 ; input_num < numInputs ; input_num++ )
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
			int sub_count = (int) (l_sampleclock - SampleClock());

			if( sub_count > 0 )
			{
				sub_process( start_pos, sub_count );
				start_pos += sub_count;
			}

			SetSampleClock( l_sampleclock );
			input_changed();
			change_flag = false;
		}

		l_sampleclock++;
	}

	// remaining
	int sub_count = (int) (l_sampleclock - SampleClock());

	if( sub_count > 0 )
	{
		sub_process( start_pos, sub_count );
		//		start_pos += sub_count;
	}
}

void ug_logic_complex::sub_process(int start_pos, int sampleframes)
{
	bool can_sleep = true;

	for( int b = 0 ; b < numOutputs ; b++ )
	{
		outputs[b].Process( start_pos, sampleframes, can_sleep );
	}

	if( can_sleep && !inputs_running )
	{
		SET_CUR_FUNC( &ug_base::process_sleep );
	}
}


/*
// reassign the dynamic input's variable to a new array
// must be done before any connections are made
void ug_logic_complex::Setup DynamicPlugs()
*/
int ug_logic_complex::Open()
{
	numInputs = 0;
	numOutputs = 0;

	for( vector<UPlug*>::iterator it = plugs.begin() ; it != plugs.end() ; ++it )
	{
		if( (*it)->Direction == DR_IN )
		{
			++numInputs;
		}
		else
		{
			++numOutputs;
		}
	}

	in_level.assign(numInputs, false);
	in_ptr = new float*[numInputs];
	outputs = new smart_output[numOutputs];
	int b = 0;

	for( vector<UPlug*>::iterator it = plugs.begin() ; it != plugs.end() ; ++it )
	{
		UPlug* p = *it;

		if( p->Direction == DR_OUT )
		{
			outputs[b].SetPlug( p );
			outputs[b].curve_type = SOT_NO_SMOOTHING;
			++b;
		}
	}

	return ug_base::Open();
}

void ug_logic_complex::onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state)
{
	Wake();

	// any input running?
	//for( int j = plugs.GetUpperBound(); j >= 0 ; j-- )
	//{
	for( vector<UPlug*>::iterator it = plugs.begin() ; it != plugs.end() ; ++it )
	{
		UPlug* p = *it;

		if( p->Direction == DR_IN && p->getState() == ST_RUN )
		{
			inputs_running = true;
			SET_CUR_FUNC( &ug_logic_complex::process_run );
			return;
		}
	}

	// implied 'else'
	inputs_running = false;
	SET_CUR_FUNC( &ug_logic_complex::sub_process );
	input_changed();
}



