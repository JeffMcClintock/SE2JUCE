#include "pch.h"

#include "ug_logic_Bin_Count.h"

#include "Logic.h"

#include "module_register.h"

SE_DECLARE_INIT_STATIC_FILE(ug_logic_Bin_Count)

namespace
{
REGISTER_MODULE_1_BC(82,L"Binary Counter", IDS_MN_BINARY_COUNTER,IDS_MG_LOGIC,ug_logic_Bin_Count ,CF_STRUCTURE_VIEW,L"Output steps though binary count when clocked");
}



#define PN_INPUT 0

#define PN_RESET 1



void ug_logic_Bin_Count::ListInterface2(InterfaceObjectArray& PList)

{
	ug_base::ListInterface2(PList);	// Call base class
	// IO Var, Direction, Datatype, Name, Default, defid (index into unit_gen::PlugFormats)
	// defid used to name a enum list or range of values
	LIST_PIN( L"Clock", DR_IN, L"", L"", IO_POLYPHONIC_ACTIVE, L"");
	LIST_PIN( L"Reset", DR_IN, L"", L"", IO_POLYPHONIC_ACTIVE, L"");
	LIST_PIN( L"B0", DR_OUT, L"", L"", 0, L"");
	LIST_PIN( L"B1", DR_OUT, L"", L"", 0, L"");
	LIST_PIN( L"B2", DR_OUT, L"", L"", 0, L"");
	LIST_PIN( L"B3", DR_OUT, L"", L"", 0, L"");
	LIST_PIN( L"B4", DR_OUT, L"", L"", 0, L"");
	LIST_PIN( L"B5", DR_OUT, L"", L"", 0, L"");
	LIST_PIN( L"B6", DR_OUT, L"", L"", 0, L"");
	LIST_PIN( L"B7", DR_OUT, L"", L"", 0, L"");
}



ug_logic_Bin_Count::ug_logic_Bin_Count() :

	cur_input(false)

	,cur_reset(false)

	,cur_count(0)

{
}



void ug_logic_Bin_Count::input_changed()

{
	int prev_count = cur_count;

	if( check_logic_input( GetPlug(PN_RESET)->getValue(), cur_reset ) )
	{
		if( cur_reset == true )
		{
			cur_count = 0;
		}
	}

	if( check_logic_input( GetPlug(PN_INPUT)->getValue(), cur_input ) )
	{
		if( cur_input == true && cur_reset == false && SampleClock() > 0)
		{
			cur_count++;
		}
	}

	if( prev_count != cur_count )
	{
		// calc new output values
		int cur = cur_count; // temp var

		for( int b = 0 ; b < 8 ; b++ )
		{
			if( (cur & 1) != (prev_count & 1) )
			{
				if( ( cur & 1 ) != 0 )
				{
					outputs[b].Set( SampleClock(), 0.5 );
				}
				else
				{
					outputs[b].Set( SampleClock(), 0.0 );
				}
			}

			cur = cur >> 1;
			prev_count = prev_count >> 1;
		}
	}
}









