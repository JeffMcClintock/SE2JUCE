#include "pch.h"
#include "ug_fixed_values.h"
#include "module_register.h"
#include "modules/se_sdk3/mp_sdk_audio.h"
#include "SeAudioMaster.h"

SE_DECLARE_INIT_STATIC_FILE(ug_fixed_values)

using namespace std;

namespace
{
	REGISTER_MODULE_1_BC(62, L"Fixed Values", IDS_MN_FIXED_VALUES, IDS_MG_CONTROLS, ug_fixed_values, CF_STRUCTURE_VIEW, L"Provides a fixed voltage source, handy if you need a control voltage that won't appear on the control panel.");
	REGISTER_MODULE_1(L"SE:Fixed Values_Text", L"Fixed Values (text)", L"Controls", ug_fixed_values_text, CF_STRUCTURE_VIEW, L"Provides a fixed voltage source, handy if you need a control voltage that won't appear on the control panel.");
	REGISTER_MODULE_1(L"SE:Fixed Values_Float", L"Fixed Values (float)", L"Controls", ug_fixed_values_float, CF_STRUCTURE_VIEW, L"Provides a fixed voltage source, handy if you need a control voltage that won't appear on the control panel.");
	REGISTER_MODULE_1(L"SE:Fixed Values_Int" , L"Fixed Values (int)", L"Controls", ug_fixed_values_int, CF_STRUCTURE_VIEW, L"Provides a fixed voltage source, handy if you need a control voltage that won't appear on the control panel.");
	REGISTER_MODULE_1(L"SE:Fixed Values_Bool", L"Fixed Values (bool)", L"Controls", ug_fixed_values_bool, CF_STRUCTURE_VIEW, L"Provides a fixed voltage source, handy if you need a control voltage that won't appear on the control panel.");
}

// Fill an array of InterfaceObjects with plugs and parameters
void ug_fixed_values::ListInterface2(InterfaceObjectArray& PList)
{
	// IO Var, Direction, Datatype, Template, ppactive, Default,Range/enum list
	LIST_PIN( L"Spare Value", DR_OUT, L"0", L"", IO_AUTODUPLICATE|IO_RENAME|IO_SETABLE_OUTPUT|IO_CUSTOMISABLE, L"Pre-Set Voltage");
}

void ug_fixed_values_text::ListInterface2(InterfaceObjectArray& PList)
{
	LIST_VAR_UI(L"Spare Value", DR_OUT, DT_TEXT, L"", L"", IO_AUTODUPLICATE | IO_RENAME | IO_SETABLE_OUTPUT | IO_CUSTOMISABLE, L"");
}

void ug_fixed_values_float::ListInterface2(InterfaceObjectArray& PList)
{
	LIST_VAR_UI(L"Spare Value", DR_OUT, DT_FLOAT, L"", L"", IO_AUTODUPLICATE | IO_RENAME | IO_SETABLE_OUTPUT | IO_CUSTOMISABLE, L"");
}

void ug_fixed_values_int::ListInterface2(InterfaceObjectArray& PList)
{
	LIST_VAR_UI(L"Spare Value", DR_OUT, DT_INT, L"", L"", IO_AUTODUPLICATE | IO_RENAME | IO_SETABLE_OUTPUT | IO_CUSTOMISABLE, L"");
}

void ug_fixed_values_bool::ListInterface2(InterfaceObjectArray& PList)
{
	LIST_VAR_UI(L"Spare Value", DR_OUT, DT_BOOL, L"", L"", IO_AUTODUPLICATE | IO_RENAME | IO_SETABLE_OUTPUT | IO_CUSTOMISABLE, L"");
}

int ug_fixed_values::Open()
{
	int r = ug_base::Open();
	SET_CUR_FUNC(&ug_base::process_sleep);

	for(auto p : plugs)
	{
		if (p->DataType == DT_FSAMPLE)
		{
			p->TransmitState(SampleClock(), ST_STATIC);
		}
		else
		{
			SendPinsCurrentValue(SampleClock(), p);
		}
	}

	return r;
}

ug_base* ug_fixed_values::Clone(CUGLookupList& UGLookupList)
{
	auto clone = ug_base::Clone(UGLookupList);
	auto audiomaster = AudioMaster2();

	auto it1 = plugs.begin();
	for (auto it2 = clone->plugs.begin(); it2 != clone->plugs.end(); ++it1, ++it2)
	{
		auto p1 = *it1;
		auto p2 = *it2;

		if (p2->DataType == DT_FSAMPLE)
		{
			auto defVal = audiomaster->getConstantPinVal(p1);
			audiomaster->UnRegisterPin(p2); // don't need any buffer allocated.
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

ug_base* ug_fixed_values_base::Clone(CUGLookupList& UGLookupList)
{
	auto clone = ug_base::Clone(UGLookupList);
	auto audiomaster = AudioMaster2();

	auto it1 = plugs.begin();
	for (auto it2 = clone->plugs.begin(); it2 != clone->plugs.end(); ++it1, ++it2)
	{
		auto p1 = *it1;
		auto p2 = *it2;

		if (p2->DataType == DT_FSAMPLE)
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