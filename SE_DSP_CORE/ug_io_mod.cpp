#include "pch.h"
#include "ug_io_mod.h"
#include "module_register.h"
#include "ug_event.h"

SE_DECLARE_INIT_STATIC_FILE(ug_io_mod)

namespace
{
REGISTER_MODULE_1_BC(4,L"IO Mod", IDS_MN_IO_MOD,0,ug_io_mod , /*CF_HAS_IO_PLUGS|*/CF_IO_MOD|CF_STRUCTURE_VIEW,L"Provides a link in/out of a container.  Any plug you connect to this will appear as a plug on the outside of the container (and vice versa).");
}

void ug_io_mod::ListInterface2(InterfaceObjectArray& PList)
{
	// IO Var, Direction, Datatype, Name, Default, defid (index into unit_gen::PlugFormats)
	// defid used to name a enum list or range of values
	LIST_PIN( L"Spare", DR_IN, L"0", L"", IO_CONTAINER_PLUG|IO_RENAME|IO_AUTODUPLICATE|IO_CUSTOMISABLE, L"Connection to modules outside (at higher level of structure)");
}

ug_io_mod::ug_io_mod()
{
	// yes this always sums voices
	SetFlag(UGF_IO_MOD|UGF_POLYPHONIC_AGREGATOR);	// flag this as an IO module
}

int ug_io_mod::Open()
{
	ug_base::Open();
	// NO PREVENTS WAKE ON PLUG DEFAULT CHANGE AudioMaster()->SuspendProcess( this ); // don't wast time on calling Do Process
	SET_CUR_FUNC( &ug_base::process_sleep );

	// send inital updates on all outputs.
	//AddEvent( new_SynthEditEvent( SampleClock(), UET_UI_NOTIFY, NUG_UPDATE_OUTPUT, 0, 0,0) );
	RUN_AT(SampleClock(), &ug_io_mod::SendPendingOutputChanges);

	// needed in dsp_patch_parameter_base::OnValueChanged()
	assert( patch_control_container );
	return 0;
}

