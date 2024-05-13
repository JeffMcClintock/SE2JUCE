#include "mp_sdk_gui2.h"
#include "../se_sdk3/Drawing.h"

using namespace gmpi;
using namespace GmpiDrawing;

class StructureGroupGui : public gmpi_gui::MpGuiGfxBase
{
  	void redraw()
	{
		invalidateRect();
	}

	StringGuiPin pinText; // mayby could be a host control connected to module name
	StringGuiPin pinColor;

	static const int headerHeight = 34;

public:
	StructureGroupGui()
	{
		initializePin( pinText, static_cast<MpGuiBaseMemberPtr2>(&StructureGroupGui::redraw) );
		initializePin( pinColor, static_cast<MpGuiBaseMemberPtr2>(&StructureGroupGui::redraw) );
	}

	int32_t MP_STDCALL OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext ) override
	{
		Graphics g(drawingContext);
		auto r = getRect();
		ClipDrawingToBounds x(g, r);
		
		// Remove leading legacy hash symbol.
		std::wstring col = pinColor;
		if (!col.empty() && col[0] == L'#')
			col = col.substr(1);

		auto brush = g.CreateSolidColorBrush(Color::FromHexString(col));
		g.FillRectangle(r, brush);
		brush.SetColor(Color::FromArgb(0x88767373));
		r.bottom = (std::min)(r.bottom, r.top + headerHeight);
		g.FillRectangle(r, brush);

		auto textFormat = GetGraphicsFactory().CreateTextFormat(18, "Sans Serif", FontWeight::DemiBold);

		brush.SetColor(Color::White);
		g.DrawTextU( (std::string) pinText, textFormat, 10.0f, 3.0f, brush, DrawTextOptions::Clip); // clip don't work!

		return gmpi::MP_OK;
	}

	int32_t MP_STDCALL hitTest(GmpiDrawing_API::MP1_POINT point) override
	{
		return point.y < headerHeight ? gmpi::MP_OK : gmpi::MP_FAIL;
	}
};

namespace
{
	auto r = Register<StructureGroupGui>::withId(L"SE Structure Group2");
}
