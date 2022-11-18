#ifndef IMAGEBASEGUI_H_INCLUDED
#define IMAGEBASEGUI_H_INCLUDED

#include "../se_sdk3/mp_sdk_gui2.h"
#include "../shared/ImageCache.h"
#include "../shared/skinBitmap.h"

class ImageBase : public gmpi_gui::MpGuiGfxBase, public skinBitmap
{
public:
	virtual int32_t MP_STDCALL OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext) override;
	virtual int32_t MP_STDCALL onPointerUp(int32_t flags, GmpiDrawing_API::MP1_POINT point) override;
	virtual int32_t MP_STDCALL populateContextMenu(float x, float y, gmpi::IMpUnknown* contextMenuItemsSink) override;
	virtual int32_t MP_STDCALL onContextMenu(int32_t selection) override;
	virtual int32_t MP_STDCALL getToolTip(float x, float y, gmpi::IMpUnknown* returnToolTipString) override;

	void calcDrawAt();
	void Load(const std::string&imageFile);
	virtual float getAnimationPos() = 0;
	virtual void setAnimationPos(float p) = 0;
	virtual void onLoaded() = 0;
	virtual std::wstring getHint() = 0;
	virtual int getMouseResponse()
	{
		return bitmapMetadata_->orientation;
	}

	int32_t MP_STDCALL getToolTip(GmpiDrawing_API::MP1_POINT point, gmpi::IString* returnString) override
	{
		auto utf8String = (std::string)pinHint;
		returnString->setData(utf8String.data(), static_cast<int32_t>(utf8String.size()));

		return gmpi::MP_OK;
	}

protected:
	void reDraw();
	void onSetFilename();

	StringGuiPin pinFilename;
	StringGuiPin pinMenuItems;
	IntGuiPin pinMenuSelection;
	BoolGuiPin pinMouseDown;
	StringGuiPin pinHint;

	// legacy, wrong-direction pins.
	BoolGuiPin pinMouseDownLegacy;
};

#endif


