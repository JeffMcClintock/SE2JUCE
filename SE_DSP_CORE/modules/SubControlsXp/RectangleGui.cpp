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
	initializePin( pinCornerRadius, static_cast<MpGuiBaseMemberPtr2>(&RectangleGui::onRedraw) );
	initializePin( pinTopLeft, static_cast<MpGuiBaseMemberPtr2>(&RectangleGui::onRedraw) );
	initializePin( pinTopRight, static_cast<MpGuiBaseMemberPtr2>(&RectangleGui::onRedraw) );
	initializePin( pinBottomLeft, static_cast<MpGuiBaseMemberPtr2>(&RectangleGui::onRedraw) );
	initializePin( pinBottomRight, static_cast<MpGuiBaseMemberPtr2>(&RectangleGui::onRedraw) );
	initializePin( pinTopColor, static_cast<MpGuiBaseMemberPtr2>(&RectangleGui::onRedraw) );
	initializePin( pinBottomColor, static_cast<MpGuiBaseMemberPtr2>(&RectangleGui::onRedraw) );
}

void RectangleGui::onRedraw()
{
	invalidateRect();
}

Color FromHexStringBackwardCompatible(const std::wstring &s)
{
	constexpr float oneOver255 = 1.0f / 255.0f;

	wchar_t* stopString;
	uint32_t hex = wcstoul(s.c_str(), &stopString, 16);
	float alpha = (hex >> 24) * oneOver255;

	return Color(se_sdk::FastGamma::sRGB_to_float((hex >> 16) & 0xff), se_sdk::FastGamma::sRGB_to_float((hex >> 8) & 0xff), se_sdk::FastGamma::sRGB_to_float(hex & 0xff), alpha);
}

int32_t RectangleGui::OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext )
{
	Graphics g(drawingContext);

	auto r = getRect();
	int width = r.right - r.left;
	int height = r.bottom - r.top;

	auto topCol = FromHexStringBackwardCompatible(pinTopColor);
	auto botCol = topCol;
	if( !pinBottomColor.getValue().empty() )
	{
		botCol = FromHexStringBackwardCompatible(pinBottomColor);
	}

	int radius = (int)pinCornerRadius;

	radius = (std::min)(radius, width/2);
	radius = (std::min)(radius, height/2);

	auto geometry = g.GetFactory().CreatePathGeometry();
	auto sink = geometry.Open();

	// define a corner 
	const float rightAngle = M_PI * 0.5f;
	// top left
	if (pinTopLeft)
	{
		sink.BeginFigure(Point(0, radius), FigureBegin::Filled);
		ArcSegment as(Point(radius, 0), Size(radius,radius), rightAngle);
		sink.AddArc(as);
	}
	else
	{
		sink.BeginFigure(Point(0,0), FigureBegin::Filled);
	}
	/*
	// tweak needed for radius of 10
	if(radius == 20)
	{
	Corner.Width += 1;
	Corner.Height += 1;
	width -=1; height -= 1;
	}
	*/
	// top right
	if (pinTopRight)
	{
		sink.AddLine(Point(width - radius, 0));
		//		sink.AddArc(Corner, 270, 90);
		ArcSegment as(Point(width, radius), Size(radius, radius), rightAngle);
		sink.AddArc(as);
	}
	else
	{
		sink.AddLine( Point(width, 0) );
	}

	// bottom right
	if (pinBottomRight)
	{
		sink.AddLine(Point(width , height -radius));
		//		sink.AddArc(Corner, 0, 90);
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

	Point point1(1, 0);
	Point point2(1, height);

	GradientStop gradientStops[]
	{
		{ 0.0f, topCol },
		{ 1.0f, botCol },
	};

	auto gradientBrush = g.CreateLinearGradientBrush(gradientStops, point1, point2 );

	g.FillGeometry(geometry, gradientBrush);

	return gmpi::MP_OK;
}

