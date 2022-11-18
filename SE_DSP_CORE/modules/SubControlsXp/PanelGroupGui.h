#ifndef PANELGROUPGUI_H_INCLUDED
#define PANELGROUPGUI_H_INCLUDED

#include "mp_sdk_gui2.h"
#include "../shared/ImageCache.h"
#include "../shared/FontCache.h"
#include "../se_sdk3/Drawing.h"

class PanelGroupGui : public gmpi_gui::MpGuiGfxBase, public ImageCacheClient, public FontCacheClient
{
public:
	PanelGroupGui();

	// overrides.
	virtual int32_t MP_STDCALL OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext ) override;
	virtual int32_t MP_STDCALL initialize() override;
	virtual int32_t MP_STDCALL arrange(GmpiDrawing_API::MP1_RECT finalRect) override;
	virtual int32_t MP_STDCALL hitTest(GmpiDrawing_API::MP1_POINT point) override;

private:
 	void onSetText();
 	StringGuiPin pinText;

	GmpiDrawing::Bitmap bitmap_;
	GmpiDrawing::TextFormat_readonly textFormat_;
	ImageMetadata* bitmapMetadata_;
	const FontMetadata* textData;
	GmpiDrawing::Size text_size;
	std::string title_utf8;
	float text_y = {};

//	GmpiDrawing::Bitmap cachedRender_;
	GmpiDrawing::RectL rTopLeft, rTopRight, rBottomRight, rBottomLeft, rVertical, rHorizontal, rleftEnd, rRightEnd;
};

#endif


