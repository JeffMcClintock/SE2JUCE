#include "pch.h"
#include "ug_default_setter.h"
#include "module_register.h"
#include "ug_event.h"
#include "SeAudioMaster.h"

SE_DECLARE_INIT_STATIC_FILE(ug_default_setter)

namespace
{
REGISTER_MODULE_1(L"SE Default Setter", IDS_MN_DEFAULT_SETTER, IDS_MG_DEBUG, ug_default_setter ,CF_STRUCTURE_VIEW,L"");
}

ug_default_setter::ug_default_setter()
{
	// yes this always sums voices
	SetFlag(UGF_DEFAULT_SETTER|UGF_IO_MOD|UGF_POLYPHONIC_AGREGATOR);	// flag this as an IO module
}

int ug_default_setter::Open()
{
	ug_base::Open();
	// NO PREVENTS WAKE ON PLUG DEFAULT CHANGE AudioMaster()->SuspendProcess( this ); // don't wast time on calling Do Process
	SET_CUR_FUNC( &ug_base::process_sleep );

	// send inital updates on all outputs.
	//AddEvent( new_SynthEditEvent( SampleClock(), UET_UI_NOTIFY, NUG_UPDATE_OUTPUT, 0, 0,0) );
	RUN_AT(SampleClock(), &ug_default_setter::SendPendingOutputChanges);

	// needed in dsp_patch_parameter_base::OnValueChanged()
	assert( patch_control_container );
	return 0;
}

ug_base* ug_default_setter::Clone( CUGLookupList& UGLookupList )
{
	ug_base* clone = ug_base::Clone(UGLookupList);
	auto audiomaster = AudioMaster2();

	auto it1 = plugs.begin();
	for( auto it2 = clone->plugs.begin() ; it2 != clone->plugs.end() ; ++it1, ++it2 )
	{
		UPlug* p1 = *it1;
		UPlug* p2 = *it2;

		if( p2->DataType == DT_FSAMPLE )
		{
			auto defVal = audiomaster->getConstantPinVal(p1);
			audiomaster->UnRegisterPin(p2);
			audiomaster->RegisterConstantPin(p2, defVal);
		}
		else
		{
			p2->CreateBuffer();
			p2->CloneBufferValue(*p1);
		}
	}

	return clone;
}