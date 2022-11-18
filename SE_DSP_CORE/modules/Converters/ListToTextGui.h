#ifndef LISTTOTEXTGUI_H_INCLUDED
#define LISTTOTEXTGUI_H_INCLUDED

#include "../se_sdk3/mp_sdk_gui.h"

class ListToTextGui : public MpGuiBase
{
public:
	ListToTextGui( IMpUnknown* host );

private:
 	void onListChanged();
 	IntGuiPin choice;
 	StringGuiPin itemList;
 	StringGuiPin itemText;
};

#endif


