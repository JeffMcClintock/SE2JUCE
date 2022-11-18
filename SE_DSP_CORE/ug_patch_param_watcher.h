#pragma once

#include <vector>
#include "ug_base.h"

class ug_patch_param_watcher : public ug_base
{
public:
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_patch_param_watcher);
	ug_patch_param_watcher();
	void CreatePlug(int32_t parameterHandle, UPlug* plug);
	void CreateParameterPlug(int32_t moduleHandle, int moduleParameterId, UPlug* plug);
	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state) override;
	virtual void SetupDynamicPlugs() override;
	ug_base* Clone( CUGLookupList& UGLookupList ) override;

	void RemoveDuplicateConnections();

private:
	std::vector<class dsp_patch_parameter_base*> patchParams;
	int voiceId_;
};
