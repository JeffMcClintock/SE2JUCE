#pragma once
#include "ug_base.h"
#include "modules/se_sdk3/mp_sdk_audio.h"
#include <vector>

class ug_patch_param_setter : public ug_base
{
public:
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_patch_param_setter);
	ug_patch_param_setter();
	int Open() override;
	void HandleEvent(SynthEditEvent* e) override;
	void setPolyPin( timestamp_t clock, int plugNumber, int physicalVoiceNumber, int datatype, int32_t data_size, void* data );
	void SetupDynamicPlugs() override;
	virtual ug_base* Clone( CUGLookupList& UGLookupList ) override;
	void TransferConnectiontoSecondary(UPlug*);
	void ConnectVoiceHostControl(ug_container* voiceControlContainer, HostControls hostConnect, UPlug* toPlug);
	void ConnectHostParameter(class dsp_patch_parameter_base* parameter, UPlug* toPlug);
	void ConnectParameter(dsp_patch_parameter_base * parameter, UPlug * toPlug);
	void ConnectParameter(int32_t moduleHandle, int moduleParameterId, UPlug* plug);
	void ReRoutePlugs() override;

private:
	// Connection are usually to a parameter.
	// Exception is two special host controls which are identified by attached container and host control type.
	struct connectionInfo
	{
		class dsp_patch_parameter_base* parameter;
		class ug_container* container;
		HostControls hostConnect;
	};

	std::vector<connectionInfo> parameters_;
	int VoiceRefreshPeriod_;
};
