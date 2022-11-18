#ifndef VOLTMETERGUI_H_INCLUDED
#define VOLTMETERGUI_H_INCLUDED

#include "Drawing.h"
#include "ClassicControlGuiBase.h"

class VoltMeterGui : public ClassicControlGuiBase
{
public:
	VoltMeterGui();
	void onSetTitle();

	virtual int32_t MP_STDCALL arrange(GmpiDrawing_API::MP1_RECT finalRect) override;
	virtual int32_t MP_STDCALL measure(GmpiDrawing_API::MP1_SIZE availableSize, GmpiDrawing_API::MP1_SIZE* returnDesiredSize) override;

	int32_t MP_STDCALL getToolTip(GmpiDrawing_API::MP1_POINT point, gmpi::IString* returnString) override
	{
		return gmpi::MP_UNHANDLED;
	}

private:
 	void onSetpatchValue();
	FloatGuiPin pinpatchValue;
};

#endif


