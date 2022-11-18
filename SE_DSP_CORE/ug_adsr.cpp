#include "pch.h"
#include "ug_adsr.h"
#include "module_register.h"

SE_DECLARE_INIT_STATIC_FILE(ug_adsr)

namespace
{
REGISTER_MODULE_1_BC(2,L"ADSR", IDS_MN_ADSR,IDS_MG_OLD,ug_adsr ,CF_STRUCTURE_VIEW,L"This module provides a standard 5 section envelope.  The attack, decay and release times are based on an exponential scale.  You can use negative voltages for shorter durations. See <a href=signals.htm>Signal Levels and Conversions</a> for Votage-Time conversion info");
}

#define PN_ATTACK	1
#define PN_DECAY	2
#define PN_SUSTAIN	3
#define PN_RELEASE	4

void ug_adsr::ListInterface2(InterfaceObjectArray& PList)
{
	// IO Var, Direction, Datatype, Name, Default, defid (index into unit_gen::PlugFormats)
	// defid used to name a enum list or range of values
//	LIST_PIN(L"Gate", DR_IN, L"1", L"", IO_POLYPHONIC_ACTIVE, L"Triggers the Attack phase when greater then 0 V ");
	LIST_PIN2(L"Gate", gate_ptr, DR_IN, L"1", L"", IO_POLYPHONIC_ACTIVE, L"Triggers the Attack phase when greater then 0 V ");
	LIST_PIN(L"Attack", DR_IN, L"0", L"", IO_POLYPHONIC_ACTIVE, L"Controls the rate of the attack, 0 V is quickest, 10 V slowest");
	LIST_PIN( L"Decay",		DR_IN, L"5", L"", IO_POLYPHONIC_ACTIVE, L"The Decay rate");
	LIST_PIN( L"Sustain",		DR_IN, L"7", L"", IO_POLYPHONIC_ACTIVE, L"The 'hold' level, while the gate is high (key is down)");
	LIST_PIN( L"Release",		DR_IN, L"4", L"", IO_POLYPHONIC_ACTIVE, L"The Release rate");
//	LIST_PIN(L"Overall Level", DR_IN, L"10", L"", IO_POLYPHONIC_ACTIVE, L"Controls the overall output level.  Handy for adding Velocity sensitivity");
	LIST_PIN2(L"Overall Level", level_ptr, DR_IN, L"10", L"", IO_POLYPHONIC_ACTIVE, L"Controls the overall output level.  Handy for adding Velocity sensitivity");
	LIST_PIN(L"Signal Out", DR_OUT, L"0", L"", 0, L"");
}



ug_adsr::ug_adsr()

{
	fixed_levels[0] = 1.0f;
	fixed_levels[2] = 0.0f;
	sustain_segment	= 1;//&adsr_sustain_segment;
	end_segment		= 2;//&adsr_end_segment;
	num_segments	= 3;
}



int ug_adsr::Open()

{
	rate_plugs[0] = GetPlug(PN_ATTACK);
	rate_plugs[1] = GetPlug(PN_DECAY);
	rate_plugs[2] = GetPlug(PN_RELEASE);
	level_plugs[0] = NULL;
	level_plugs[1] = GetPlug(PN_SUSTAIN);
	level_plugs[2] = NULL;
	return ug_envelope::Open();
}

