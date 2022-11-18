#ifndef OVERSAMPLINGCONTROLGUI_H_INCLUDED
#define OVERSAMPLINGCONTROLGUI_H_INCLUDED

#include "mp_sdk_gui2.h"

class OversamplingControlGui : public gmpi_gui::MpGuiInvisibleBase
{
public:
	OversamplingControlGui();

	// overrides

private:
	void onSetHostLists();
	void onSetHostOversampling();
	void onSetHoatOversamplingFilter();
	void onSetOversampling();
	void onSetOversamplingFilter();

	IntGuiPin pinHostOversampling;
	IntGuiPin pinHostOversamplingFilter;
	StringGuiPin pinHostListItems;
	StringGuiPin pinHostListItems2;

	IntGuiPin pinOversampling;
	StringGuiPin pinListItems;
	IntGuiPin pinOversamplingFilter;
	StringGuiPin pinListItems2;
};

#endif