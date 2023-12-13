#include "./PatchPointsGui.h"
#include "Drawing.h"
#define _USE_MATH_DEFINES
#include <math.h>

SE_DECLARE_INIT_STATIC_FILE(PatchPointsGui)

using namespace gmpi;
using namespace GmpiDrawing;

GMPI_REGISTER_GUI(MP_SUB_TYPE_GUI2, PatchPointsGuiIn, L"SE Patch Point in");
GMPI_REGISTER_GUI(MP_SUB_TYPE_GUI2, PatchPointsGui, L"SE Patch Point out");

int32_t PatchPointsGui::OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext )
{
	Graphics g(drawingContext);
	const Point center(10, 10);

	g.FillEllipse(GmpiDrawing::Ellipse(center, 9), g.CreateSolidColorBrush(Color::White));
	g.FillEllipse(GmpiDrawing::Ellipse(center, 5), g.CreateSolidColorBrush(Color::Black));
	g.DrawEllipse(GmpiDrawing::Ellipse(center, 9.5), g.CreateSolidColorBrush(Color::Gray));

	// Hexagon 
	{
		const float penWidth = 1;
		auto factory = g.GetFactory();
		auto geometry = factory.CreatePathGeometry();
		auto sink = geometry.Open();

		Rect r = getRect();

		float offset = 0.5f;
		r.top += offset;
		r.left += offset;
		r.bottom -= offset;
		r.right -= offset;

		const double startA = getHandle() * 0.0001;
		const int numSides = 6;
//		const double radius = 9;
		for (int i = 0; i < numSides; ++i)
		{
			double a = startA + (2.0 * M_PI * i) / (double)numSides;
			float x = radius * sinf(a);
			float y = radius * cosf(a);
			Point p = center + Size(x, y);

			if (i == 0)
			{
				sink.BeginFigure(p, FigureBegin::Filled);
			}
			else
			{
				sink.AddLine(p);
			}
		}

		sink.EndFigure();
		sink.Close();

		auto brush = g.CreateSolidColorBrush(Color::White);
		brush.SetColor(Color::FromBytes(100, 100, 100));

		g.DrawGeometry(geometry, brush, penWidth);
	}

	return gmpi::MP_OK;
}
