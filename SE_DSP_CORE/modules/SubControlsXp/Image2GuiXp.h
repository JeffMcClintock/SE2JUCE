#ifndef IMAGE3GUI_H_INCLUDED
#define IMAGE3GUI_H_INCLUDED

#include "../sharedLegacyWidgets/ImageBase.h"

class Image2GuiXp: public ImageBase
{
protected:
	FloatGuiPin pinAnimationPosition;
	IntGuiPin pinFrameCount;
	IntGuiPin pinFrameCountLegacy;
	IntGuiPin pinMouseResponse;
	bool useMouseResponsePin_;

public:
	Image2GuiXp(bool useMouseResponsePin = true);

	float getAnimationPos() override{
		return pinAnimationPosition;
	}
	void setAnimationPos(float p) override;
	void onLoaded() override;
	std::wstring getHint() override
	{
		return pinHint;
	}
	int getMouseResponse() override;
	// overrides.
//	int32_t MP_STDCALL OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext) override;
	int32_t MP_STDCALL onPointerDown(int32_t flags, GmpiDrawing_API::MP1_POINT point) override;
	int32_t MP_STDCALL onPointerMove(int32_t flags, GmpiDrawing_API::MP1_POINT point) override;
	int32_t MP_STDCALL onPointerUp(int32_t flags, GmpiDrawing_API::MP1_POINT point) override;
	int32_t MP_STDCALL measure(GmpiDrawing_API::MP1_SIZE availableSize, GmpiDrawing_API::MP1_SIZE* returnDesiredSize) override;
	int32_t MP_STDCALL onMouseWheel(int32_t flags, int32_t delta, GmpiDrawing_API::MP1_POINT point) override;

	// MP_OK = hit, MP_UNHANDLED/MP_FAIL = miss.
	// Default to MP_OK to allow user to select by clicking.
	// point will always be within bounding rect.
	int32_t MP_STDCALL hitTest(GmpiDrawing_API::MP1_POINT point) override
	{
		return skinBitmap::bitmapHitTestLocal(point) ? gmpi::MP_OK : gmpi::MP_UNHANDLED;
	}
};


#endif


