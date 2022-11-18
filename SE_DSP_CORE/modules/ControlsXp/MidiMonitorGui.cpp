#include "mp_sdk_gui2.h"
#include "Drawing.h"

using namespace gmpi;
using namespace GmpiDrawing;

class MidiMonitorGui : public gmpi_gui::MpGuiGfxBase
{
 	void onSetDispIn()
	{
		invalidateRect();
	}

 	StringGuiPin pinDispIn;

public:
	MidiMonitorGui()
	{
		initializePin( pinDispIn, static_cast<MpGuiBaseMemberPtr2>(&MidiMonitorGui::onSetDispIn) );
	}

	int32_t MP_STDCALL OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext ) override
	{
		Graphics g(drawingContext);
		ClipDrawingToBounds x(g, getRect());

		auto brush = g.CreateSolidColorBrush(Color::FromRgb(0x002000));
		g.FillRectangle(getRect(), brush);

		auto tf = g.GetFactory().CreateTextFormat();
		brush.SetColor(Color::Lime);
		g.DrawTextW(pinDispIn.getValue(), tf, getRect(), brush);

		return gmpi::MP_OK;
	}
};

namespace
{
	auto r = Register<MidiMonitorGui>::withId(L"SE Midi Monitor");
	auto r2 = Register<MidiMonitorGui>::withId(L"SE Monitor");
}
