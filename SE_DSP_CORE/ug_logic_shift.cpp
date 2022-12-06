#include "pch.h"

#include "ug_logic_shift.h"

#include "Logic.h"

#include "module_register.h"

SE_DECLARE_INIT_STATIC_FILE(ug_logic_shift)

namespace
{
REGISTER_MODULE_1(L"Shift Register", IDS_MN_SHIFT_REGISTER,IDS_MG_LOGIC,ug_logic_shift ,CF_STRUCTURE_VIEW,L"Digital Shift Register. For you seriously warped experimenters");
}

#define PN_CLOCK 0

#define PN_INPUT 1

#define PN_RESET 2



void ug_logic_shift::ListInterface2(InterfaceObjectArray& PList)

{
	ug_base::ListInterface2(PList);	// Call base class
	// IO Var, Direction, Datatype, Name, Default, defid (index into unit_gen::PlugFormats)
	// defid used to name a enum list or range of values
	LIST_PIN( L"Clock", DR_IN, L"", L"", IO_POLYPHONIC_ACTIVE, L"");
	LIST_PIN( L"Input", DR_IN, L"", L"", IO_POLYPHONIC_ACTIVE, L"");
	LIST_PIN( L"Reset", DR_IN, L"", L"", IO_POLYPHONIC_ACTIVE, L"");
	LIST_PIN( L"B0", DR_OUT, L"", L"", 0, L"");
	LIST_PIN( L"B1", DR_OUT, L"", L"", 0, L"");
	LIST_PIN( L"B2", DR_OUT, L"", L"", 0, L"");
	LIST_PIN( L"B3", DR_OUT, L"", L"", 0, L"");
	LIST_PIN( L"B4", DR_OUT, L"", L"", 0, L"");
	LIST_PIN( L"B5", DR_OUT, L"", L"", 0, L"");
	LIST_PIN( L"B6", DR_OUT, L"", L"", 0, L"");
	LIST_PIN( L"B7", DR_OUT, L"", L"", 0, L"");
	LIST_PIN( L"B8", DR_OUT, L"", L"", 0, L"");
	LIST_PIN( L"B9", DR_OUT, L"", L"", 0, L"");
}



ug_logic_shift::ug_logic_shift() :

	cur_count(0)

	,cur_reset(false)

	,cur_clock(false)

	,cur_input(false)

{
}



void ug_logic_shift::input_changed()

{
	int prev_count = cur_count;

	if( check_logic_input( GetPlug(PN_RESET)->getValue(), cur_reset ) )
	{
		if( cur_reset == true )
		{
			cur_count = 0;
		}
	}

	check_logic_input( GetPlug(PN_INPUT)->getValue(), cur_input );

	if( check_logic_input( GetPlug(PN_CLOCK)->getValue(), cur_clock ) )
	{
		if( cur_clock == true )
		{
			cur_count = cur_count << 1;
			cur_count |= (int)cur_input;
		}
	}

	if( prev_count != cur_count )
	{
		// calc new output values
		int cur = cur_count; // temp var

		for( int b = 0 ; b < numOutputs ; b++ )
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



