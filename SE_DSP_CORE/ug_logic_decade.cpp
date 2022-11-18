#include "pch.h"

#include "ug_logic_decade.h"

#include "module_register.h"

SE_DECLARE_INIT_STATIC_FILE(ug_logic_decade)

namespace
{
REGISTER_MODULE_1_BC(83,L"Decade Counter", IDS_MN_DECADE_COUNTER,IDS_MG_DEBUG,ug_logic_decade ,CF_STRUCTURE_VIEW,L"When triggered, send MIDI note messages");
}

void ug_logic_decade::ListInterface2(InterfaceObjectArray& PList)

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
	LIST_PIN( L"B8", DR_OUT, L"", L"", 0, L"");
	LIST_PIN( L"B9", DR_OUT, L"", L"", 0, L"");
}



int ug_logic_decade::Open()

{
	ug_logic_counter::Open();
	max_count = (int)plugs.size() - 2; // overide base
	return 0;
}

