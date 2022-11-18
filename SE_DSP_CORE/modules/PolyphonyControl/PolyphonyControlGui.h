#ifndef POLYPHONYCONTROLGUI_H_INCLUDED
#define POLYPHONYCONTROLGUI_H_INCLUDED

#include "../se_sdk3/mp_sdk_gui2.h"

class PolyphonyControlGui_base : public gmpi_gui::MpGuiInvisibleBase
{
public:
	virtual int32_t MP_STDCALL initialize() override;

	void onSetPolyphony();
	void onSetHostPolyphony();
	void onSetNotePriority();
	void onSetPolyphonyReserve();
	void onSetHostPolyphonyReserve();

	IntGuiPin hostPolyphony;
	IntGuiPin hostReserveVoices;
	IntGuiPin hostVoiceAllocationMode;

	IntGuiPin polyphony;
	IntGuiPin reserveVoices;
	IntGuiPin monoNotePriority;
	StringGuiPin itemList2;
	StringGuiPin itemList3;
	StringGuiPin itemList4;
};

class PolyphonyControlGui1 : public PolyphonyControlGui_base
{
	IntGuiPin voiceAllocationMode;
	StringGuiPin itemList_voiceAllocationMode;

public:
	PolyphonyControlGui1();
	virtual int32_t MP_STDCALL initialize() override;

	void onSetHostVoiceAllocationMode();
	void onSetVoiceAllocationMode();
};

class PolyphonyControlGui2 : public PolyphonyControlGui_base
{
	BoolGuiPin monoMode;
	BoolGuiPin monoRetrigger;

	IntGuiPin voiceStealMode;
	StringGuiPin itemList_voiceSteal;

	IntGuiPin GlideType;
	IntGuiPin GlideTiming;
	StringGuiPin itemList_GlideType;
	StringGuiPin itemList_GlideTiming;
	FloatGuiPin host_PortamentoTime;
	FloatGuiPin PortamentoTime;

	FloatGuiPin hostBendRange;
	IntGuiPin BendRange;
	StringGuiPin itemList_BendRange;

	IntGuiPin VoiceRefresh;
	StringGuiPin itemList_VoiceRefresh;

public:
	PolyphonyControlGui2();
	virtual int32_t MP_STDCALL initialize() override;

	void onSetVoiceRefresh();

	void onSetHostVoiceAllocationMode();
	void onSetMonoMode();
	void onSetVoiceStealMode();
	void onSetGlide();
	void onSetGlideTiming();
	void onSetPortamento();
	void onSetHostPortamento();
	void onSetBendRange();
	void onSetHostBendRange();
};


#endif
