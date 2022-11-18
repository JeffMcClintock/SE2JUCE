#ifndef TEXTENTRYGUI_H_INCLUDED
#define TEXTENTRYGUI_H_INCLUDED

#include "ClassicControlGuiBase.h"
#include "../se_sdk3/mp_gui.h"

class TextEntryGui : public ClassicControlGuiBase
{
public:
	TextEntryGui();

	virtual int32_t MP_STDCALL arrange(GmpiDrawing_API::MP1_RECT finalRect) override;
	virtual int32_t MP_STDCALL measure(GmpiDrawing_API::MP1_SIZE availableSize, GmpiDrawing_API::MP1_SIZE* returnDesiredSize) override;
	/*
	// overrides.
	virtual int32_t MP_STDCALL onPointerUp(int32_t flags, GmpiDrawing_API::MP1_POINT point) override;
	virtual int32_t MP_STDCALL initialize() override;
	*/

private:
	void OnWidgetUpdate(const std::string& newvalue);
	void OnBrowseButton(float newvalue);
	void onSetpatchValue();
 	void onSetExtension();
	void OnPopupmenuComplete(int32_t result);

 	StringGuiPin pinpatchValue;
	StringGuiPin pinExtension;

	float browseButtonState;
	GmpiGui::FileDialog nativeFileDialog;
};

#endif


