#include "./RectangleGui.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include "../se_sdk3/Drawing.h"
#include "../shared/fast_gamma.h"

using namespace gmpi;
using namespace GmpiDrawing;

GMPI_REGISTER_GUI(MP_SUB_TYPE_GUI2, RectangleGui, L"SE Rectangle XP" );

RectangleGui::RectangleGui()
{
	// initialise pins.
	initializePin( pinCornerRadius, static_cast<MpGuiBaseMemberPtr2>(&RectangleGui::onChangeRadius) );
	initializePin( pinTopLeft, static_cast<MpGuiBaseMemberPtr2>(&RectangleGui::onRedraw) );
	initializePin( pinTopRight, static_cast<MpGuiBaseMemberPtr2>(&RectangleGui::onRedraw) );
	initializePin( pinBottomLeft, static_cast<MpGuiBaseMemberPtr2>(&RectangleGui::onRedraw) );
	initializePin( pinBottomRight, static_cast<MpGuiBaseMemberPtr2>(&RectangleGui::onRedraw) );
	initializePin( pinTopColor, static_cast<MpGuiBaseMemberPtr2>(&RectangleGui::onChangeTopCol) );
	initializePin( pinBottomColor, static_cast<MpGuiBaseMemberPtr2>(&RectangleGui::onChangeBotCol) );
}

void RectangleGui::onRedraw()
{
	invalidateRect();
}

int32_t RectangleGui::arrange(GmpiDrawing_API::MP1_RECT finalRect)
{
	geometry = {};
	return gmpi_gui::MpGuiGfxBase::arrange(finalRect);
}

Color FromHexStringBackwardCompatible(const std::wstring &s)
{
	constexpr float oneOver255 = 1.0f / 255.0f;

	wchar_t* stopString;
	uint32_t hex = static_cast<uint32_t>(wcstoul(s.c_str(), &stopString, 16));
	float alpha = (hex >> 24) * oneOver255;

	return Color(se_sdk::FastGamma::sRGB_to_float((hex >> 16) & 0xff), se_sdk::FastGamma::sRGB_to_float((hex >> 8) & 0xff), se_sdk::FastGamma::sRGB_to_float(hex & 0xff), alpha);
}

void RectangleGui::onChangeRadius()
{
	geometry = {};
	invalidateRect();
}

void RectangleGui::onChangeTopCol()
{
	topColor = FromHexStringBackwardCompatible(pinTopColor);

	if (pinBottomColor.getValue().empty())
	{
		bottomColor = topColor;
	}

	invalidateRect();
}

void RectangleGui::onChangeBotCol()
{
	if (pinBottomColor.getValue().empty())
	{
		bottomColor = topColor;
	}
	else
	{
		bottomColor = FromHexStringBackwardCompatible(pinBottomColor);
	}
	invalidateRect();
}

int32_t RectangleGui::OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext )
{
	Graphics g(drawingContext);

	auto r = getRect();
	int width = r.right - r.left;
	int height = r.bottom - r.top;

	int radius = (int)pinCornerRadius;

	radius = (std::min)(radius, width/2);
	radius = (std::min)(radius, height/2);

	if (geometry.isNull())
	{
		geometry = g.GetFactory().CreatePathGeometry();
		auto sink = geometry.Open();

		// define a corner 
		constexpr float rightAngle = M_PI * 0.5f;
		// top left
		if (pinTopLeft)
		{
			sink.BeginFigure(Point(0, radius), FigureBegin::Filled);
			ArcSegment as(Point(radius, 0), Size(radius, radius), rightAngle);
			sink.AddArc(as);
		}
		else
		{
			sink.BeginFigure(Point(0, 0), FigureBegin::Filled);
		}

		// top right
		if (pinTopRight)
		{
			sink.AddLine(Point(width - radius, 0));
			ArcSegment as(Point(width, radius), Size(radius, radius), rightAngle);
			sink.AddArc(as);
		}
		else
		{
			sink.AddLine(Point(width, 0));
		}

		// bottom right
		if (pinBottomRight)
		{
			sink.AddLine(Point(width, height - radius));
			ArcSegment as(Point(width - radius, height), Size(radius, radius), rightAngle);
			sink.AddArc(as);
		}
		else
		{
			sink.AddLine(Point(width, height));
		}

		// bottom left
		if (pinBottomLeft)
		{
			sink.AddLine(Point(radius, height));
			ArcSegment as(Point(0, height - radius), Size(radius, radius), rightAngle);
			sink.AddArc(as);
		}
		else
		{
			sink.AddLine(Point(0, height));
		}

		// end path
		sink.EndFigure();
		sink.Close();
	}

	if (topColor == bottomColor)
	{
		auto brush = g.CreateSolidColorBrush(topColor);
		g.FillGeometry(geometry, brush);
	}
	else
	{
		// Gradient.
		Point point1(1, 0);
		Point point2(1, height);

		GradientStop gradientStops[]
		{
			{ 0.0f, topColor },
			{ 1.0f, bottomColor },
		};

		auto gradientBrush = g.CreateLinearGradientBrush(gradientStops, point1, point2);

		g.FillGeometry(geometry, gradientBrush);
	}

	return gmpi::MP_OK;
}

