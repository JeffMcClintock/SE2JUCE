#ifndef TEXTENTRY4GUI_H_INCLUDED
#define TEXTENTRY4GUI_H_INCLUDED

#include <functional>
#include "TextSubcontrol.h"

class TextEntry4Gui : public TextSubcontrol
{
	GmpiGui::TextEdit nativeEdit;

public:
	TextEntry4Gui();

	void onSetStyle();

	int32_t MP_STDCALL OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext) override;
	int32_t MP_STDCALL onPointerDown(int32_t flags, GmpiDrawing_API::MP1_POINT point) override;
	int32_t MP_STDCALL onPointerUp(int32_t flags, GmpiDrawing_API::MP1_POINT point) override;

	std::string getDisplayText() override;

private:
	void OnTextEnteredComplete(int32_t result);

	StringGuiPin pinText;
 	BoolGuiPin pinMultiline;
 	BoolGuiPin pinMouseDown_LEGACY;
 	BoolGuiPin pinMouseDown;

	GmpiDrawing::TextFormat_readonly textFormat;
	FontMetadata* fontmetadata = nullptr;
};

#endif


