#include "./TriggerScope.h"

REGISTER_PLUGIN(TriggerScope, L"SE TrigScope3 XP");

/* TODO !!!
	properties->flags = UGF_VOICE_MON_IGNORE;
	properties->gui_flags = CF_CONTROL_VIEW|CF_STRUCTURE_VIEW;
*/

TriggerScope::TriggerScope(IMpUnknown* host) : Scope3(host)
{
	// Associate each pin object with it's ID in the XML file
	initializePin(pinTrigger);
}
