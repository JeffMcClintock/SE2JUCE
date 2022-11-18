#include "./RectangleGui.h"
#include <windows.h>
#include <gdiplus.h>

using namespace Gdiplus;

REGISTER_GUI_PLUGIN( RectangleGui, L"SE Rectangle" );

RectangleGui::RectangleGui( IMpUnknown* host ) : SeGuiCompositedGfxBase(host)
{
	// initialise pins.
	cornerRadius.initialize( this, 0, static_cast<MpGuiBaseMemberPtr>(&RectangleGui::onRedraw) );
	TopLeft.initialize( this, 1, static_cast<MpGuiBaseMemberPtr>(&RectangleGui::onRedraw) );
	TopRight.initialize( this, 2, static_cast<MpGuiBaseMemberPtr>(&RectangleGui::onRedraw) );
	BottomLeft.initialize( this, 3, static_cast<MpGuiBaseMemberPtr>(&RectangleGui::onRedraw) );
	BottomRight.initialize( this, 4, static_cast<MpGuiBaseMemberPtr>(&RectangleGui::onRedraw) );
	topColor.initialize( this, 5, static_cast<MpGuiBaseMemberPtr>(&RectangleGui::onRedraw) );
	bottomColor.initialize( this, 6, static_cast<MpGuiBaseMemberPtr>(&RectangleGui::onRedraw) );
}

// handle pin updates.
void RectangleGui::onRedraw()
{
	invalidateRect();
}

int32_t RectangleGui::paint( HDC hDC )
{
	MpRect r = getRect();
	int width = r.right - r.left;
	int height = r.bottom - r.top;

	wchar_t* stopString;
	int topCol = wcstoul( topColor.getValue().c_str(), &stopString, 16 );
	int botCol;
	if( bottomColor.getValue().empty() )
	{
		botCol = topCol;
	}
	else
	{
		botCol = wcstoul( bottomColor.getValue().c_str(), &stopString, 16 );
	}

	// Create GDI+ Graphics object to draw on screen.
	Graphics graphics( hDC );

	graphics.SetSmoothingMode( SmoothingModeAntiAlias );

	int dia = (int)cornerRadius * 2;

	dia = min( dia, width );
	dia = min( dia, height );

	GraphicsPath path;
	// define a corner 
	Rect Corner(0, 0, dia, dia);

	// begin path
	path.Reset();

	int prev = 0;

	// top left
	if( TopLeft )
	{
		path.AddArc(Corner, 180, 90);
		prev = dia;
	}
/*
	// tweak needed for radius of 10 (dia of 20)
	if(dia == 20)
	{
		Corner.Width += 1; 
		Corner.Height += 1; 
		width -=1; height -= 1;
	}
*/
	// top right
	Corner.X += (width - dia - 1);
	if( TopRight )
	{
		path.AddArc(Corner, 270, 90);
		prev = dia;
	}
	else
	{
		path.AddLine( prev, 0, width, 0 );
		prev = 0;
	}
	
	// bottom right
	Corner.Y += (height - dia - 1);
	if( BottomRight )
	{
		path.AddArc(Corner, 0, 90);
		prev = dia;
	}
	else
	{
		path.AddLine(width, height + prev, width, height);
		prev = 0;
	}
	
	// bottom left
	Corner.X -= (width - dia - 1);
	if( BottomLeft )
	{
		path.AddArc(Corner,  90, 90);
		prev = dia;
	}
	else
	{
		path.AddLine(width - prev, height, 0, height);
		prev = 0;
	}

	if( !TopLeft )
	{
		path.AddLine(0, height - prev, 0, 0);
	}


	// end path
	path.CloseFigure();


	LinearGradientBrush linGrBrush(
		Point(1, 0),
		Point(1, height),
		Color((BYTE)(topCol >> 24), (BYTE)(topCol >> 16) & 0xff, (BYTE)(topCol >> 8) & 0xff, (BYTE)(topCol) & 0xff),
		Color((BYTE)(botCol >> 24), (BYTE)(botCol >> 16) & 0xff, (BYTE)(botCol >> 8) & 0xff, (BYTE)(botCol) & 0xff)
		);

//	graphics.FillRectangle(&linGrBrush, 0, 0, width, height);
	graphics.FillPath(&linGrBrush, &path);

	return gmpi::MP_OK;
}

