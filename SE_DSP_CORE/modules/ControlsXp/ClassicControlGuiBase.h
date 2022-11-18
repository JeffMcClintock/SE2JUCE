#ifndef CLASSICCONTROLGUIBASE_H_INCLUDED
#define CLASSICCONTROLGUIBASE_H_INCLUDED

#include "../se_sdk3/Drawing_API.h"
#include "../sharedLegacyWidgets/WidgetHost.h"

class ClassicControlGuiBase : public WidgetHost
{
public:

	int32_t MP_STDCALL setHost(gmpi::IMpUnknown* host) override
	{
		return WidgetHost::setHost(host);
	}

	virtual int32_t MP_STDCALL initialize() override;
	/*
	// IMpGraphics overrides.
	virtual int32_t MP_STDCALL onPointerUp(int32_t flags, GmpiDrawing_API::MP1_POINT point) override;
	virtual int32_t MP_STDCALL measure(GmpiDrawing_API::MP1_SIZE availableSize, GmpiDrawing_API::MP1_SIZE* returnDesiredSize) override;
	virtual int32_t MP_STDCALL arrange(GmpiDrawing_API::MP1_RECT finalRect) override;
	*/
	int32_t MP_STDCALL getToolTip(GmpiDrawing_API::MP1_POINT point, gmpi::IString* returnString) override
	{
		auto utf8String = (std::string)pinHint;
		returnString->setData(utf8String.data(), static_cast<int32_t>(utf8String.size()));

		return gmpi::MP_OK;
	}

	bool useBackwardCompatibleArrangement();

protected:
	void onSetTitle();

	StringGuiPin pinHint;
	StringGuiPin pinTitle;

private:
	int backwardCompatibleVerticalArrange = -1;
};

#endif


