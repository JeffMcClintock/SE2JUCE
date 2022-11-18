#include "./GraphicsFrameRateGui.h"
#include <algorithm>
#include "../se_sdk3/Drawing.h"

using namespace gmpi;
using namespace GmpiDrawing;

GMPI_REGISTER_GUI(MP_SUB_TYPE_GUI2, GraphicsFrameRateGui, L"SE Graphics Frame Rate" );

GraphicsFrameRateGui::GraphicsFrameRateGui() :
	framesPerSecond(20)
{
	text[0] = 0;
}

int32_t GraphicsFrameRateGui::initialize()
{
	auto r = gmpi_gui::MpGuiGfxBase::initialize(); // ensure all pins initialised (so widgets are built).

	StartTimer(4);

	return r;
}

int32_t GraphicsFrameRateGui::OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext )
{
	auto now = std::chrono::steady_clock::now();
	auto duration = now - lastDrawTime;
	lastDrawTime = now;

	auto us = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
	if (us < 1)
		us = 1; // prevent divide-by-zero.

	auto instantFPS = 1000000.0f / (float) us;
	
	// median smoothing.
	float delta = instantFPS - framesPerSecond;
	const float maxSlew = 0.1f;
	framesPerSecond += (std::min)( maxSlew, (std::max)(-maxSlew, delta) );

	sprintf(text, "%4.1f FPS", framesPerSecond);

	Graphics g(drawingContext);

	if (textFormat.isNull())
	{
		textFormat = GetGraphicsFactory().CreateTextFormat();
		brush = g.CreateSolidColorBrush(Color::Red);
	}

    GmpiDrawing::Rect rect(0, 0, 100, 30);
	g.DrawTextU(text, textFormat, rect, brush);

// problematic
//	invalidateRect();

	return gmpi::MP_OK;
}

bool GraphicsFrameRateGui::OnTimer()
{
	invalidateRect();
	return true;
}
