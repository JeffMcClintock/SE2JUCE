#include "GuiPatchAutomator3.h"
#include "IGuiHost2.h"

using namespace gmpi;
using namespace std;

GMPI_REGISTER_GUI(MP_SUB_TYPE_GUI2, GuiPatchAutomator3, L"PatchAutomator" );

namespace
{
	int32_t r = RegisterPluginXml(
R"XML(
<?xml version="1.0" encoding="utf-8" ?>
<PluginList>
  <Plugin id="PatchAutomator" name="PatchAutomator3" category="Debug">
    <GUI/>
  </Plugin>
</PluginList>
)XML");

}

GuiPatchAutomator3::~GuiPatchAutomator3()
{
	if(patchManager_)
		patchManager_->UnRegisterGui2(this);
}

void GuiPatchAutomator3::Sethost( class IGuiHost2* host)
{
	patchManager_ = host;
	if (patchManager_)
		patchManager_->RegisterGui2(this);
}

int32_t GuiPatchAutomator3::initialize()
{
	const auto res = SeGuiInvisibleBase::initialize();

	for (const auto& p : parameterToPinIndex_)
	{
		const auto parameterId = static_cast<int32_t>(p.first & 0xffffffff);
		const auto fieldId = p.first >> 32;
		patchManager_->initializeGui(this, parameterId, (gmpi::FieldType)fieldId);
	}

	return res;
}

int32_t GuiPatchAutomator3::setPin( int32_t pinId, int32_t voice, int32_t size, const void* data )
{
	assert(pinId >= 0 && pinId < (int)parameterToPinIndex_.size());

	patchManager_->setParameterValue(RawView(data, size), pinToParameterIndex_[pinId].first, (gmpi::FieldType) pinToParameterIndex_[pinId].second, voice);

	return GuiPinOwner::setPin2( pinId, voice, size, data );
}

inline int64_t makePinIndexKey(int32_t parameterHandle, int32_t fieldId)
{
	return (((int64_t)fieldId) << 32) | (int64_t)parameterHandle;
}

// Host control indicated by negative parameter ID.
int GuiPatchAutomator3::Register(int moduleHandle, int moduleParamId, ParameterFieldType paramField)
{
	int32_t parameterHandle = -1;
	// avoid very slow lookup if possible.
	if (cacheModuleHandle == moduleHandle && cacheModuleParamId == moduleParamId)
	{
		parameterHandle = cacheParamId;

		assert(parameterHandle == patchManager_->getParameterHandle(moduleHandle, moduleParamId));
	}
	else
	{
		cacheModuleHandle = moduleHandle;
		cacheModuleParamId = moduleParamId;
		cacheParamId = parameterHandle = patchManager_->getParameterHandle(moduleHandle, moduleParamId);
	}

	if (parameterHandle == -1) // not available
	{
		return -1;
	}

	// host-controls might already be registered.
	if (moduleParamId < 0)
	{
		auto it = parameterToPinIndex_.find(makePinIndexKey(parameterHandle, paramField));
		if (it != parameterToPinIndex_.end())
		{
			return (*it).second;
		}
	}
#ifdef _DEBUG
	else
	{
		// verify my assumption that only host controls will already be registered.
		assert(parameterToPinIndex_.find(makePinIndexKey(parameterHandle, paramField)) == parameterToPinIndex_.end());
	}
#endif

	auto pinId = (int) parameterToPinIndex_.size();
	parameterToPinIndex_[makePinIndexKey(parameterHandle, paramField)] = pinId;

	pinToParameterIndex_.push_back({ parameterHandle, paramField });

	return pinId;
}

int32_t GuiPatchAutomator3::setParameter(int32_t parameterHandle, int32_t fieldId, int32_t voice, const void* data, int32_t size)
{
	auto it = parameterToPinIndex_.find(makePinIndexKey(parameterHandle, fieldId));
	if (it != parameterToPinIndex_.end())
	{
		getHost()->pinTransmit((*it).second, size, data, voice);
	}

	return gmpi::MP_OK;
}
