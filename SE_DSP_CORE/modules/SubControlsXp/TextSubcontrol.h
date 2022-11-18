#pragma once
#include "mp_sdk_gui2.h"
#include "../se_sdk3/mp_gui.h"
#include "../se_sdk3/Drawing.h"
#include "../shared/FontCache.h"

class TextSubcontrol : public gmpi_gui::MpGuiGfxBase, public FontCacheClient
{
protected:
	static const int border = 3;

	StringGuiPin pinStyle;
	BoolGuiPin pinWriteable;
	BoolGuiPin pinGreyed;
	StringGuiPin pinHint;
	StringGuiPin pinMenuItems;
	IntGuiPin pinMenuSelection;

public:
	void onSetStyle();
	void redraw()
	{
		invalidateRect();
	}

	virtual int32_t MP_STDCALL OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext) override;
	virtual int32_t MP_STDCALL populateContextMenu(float x, float y, gmpi::IMpUnknown* contextMenuItemsSink) override;
	virtual int32_t MP_STDCALL onContextMenu(int32_t selection) override;
	// Old way.
	virtual int32_t MP_STDCALL getToolTip(float x, float y, gmpi::IMpUnknown* returnToolTipString) override;
	virtual int32_t MP_STDCALL measure(GmpiDrawing_API::MP1_SIZE availableSize, GmpiDrawing_API::MP1_SIZE* returnDesiredSize) override;

	// new way. IMpGraphics2.
	int32_t MP_STDCALL getToolTip(GmpiDrawing_API::MP1_POINT point, gmpi::IString* returnString) override
	{
		auto utf8String = (std::string)pinHint;
		returnString->setData(utf8String.data(), (int32_t) utf8String.size());
		return gmpi::MP_OK;
	}
	virtual std::string getDisplayText() = 0;
};

