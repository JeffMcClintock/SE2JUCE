#ifndef POPUPMENUGUI_H_INCLUDED
#define POPUPMENUGUI_H_INCLUDED

#include "TextSubcontrol.h"

class PopupMenuGui : public TextSubcontrol
{
	GmpiGui::PopupMenu nativeMenu;

public:
	PopupMenuGui();

	// overrides.
	int32_t MP_STDCALL onPointerDown(int32_t flags, GmpiDrawing_API::MP1_POINT point) override;
	int32_t MP_STDCALL onPointerUp(int32_t flags, GmpiDrawing_API::MP1_POINT point) override;

	std::string getDisplayText() override;
	void OnPopupComplete(int32_t result);

private:

	IntGuiPin pinChoice;
 	StringGuiPin pinItemList;
 	StringGuiPin pinHeading;
	BoolGuiPin pinMomentary;
	BoolGuiPin pinEnableSpecialStrings;
};

#endif


