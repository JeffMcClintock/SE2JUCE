#pragma once

#include "Drawing.h"

using namespace GmpiDrawing;

//#define MAX_KEYS 128
class DrawKeyBoard
{
	static const int baseWhiteKeyNum = 21;
public:

inline static int PointToKeyNum(GmpiDrawing_API::MP1_POINT point, Rect r, const int baseKey)
{
	float keyHeight = r.bottom - r.top;
	float keyWidth = floor(keyHeight * 0.2);

	// first assume mouse over white key
	int white_key = 2 * floorf(point.x / keyWidth);
	int k_num = white_key;

	float blackKeyHeight = (2 * keyHeight + 1) / 3;
	if (point.y < blackKeyHeight)
	{
		// nearest white-key-gap (not all contain black keys)
		int black_key = -1 + 2 * floorf((point.x + keyWidth / 2) / keyWidth);

		// ignore gaps without black keys
		int t = (black_key + 14) % 14;
		if (t != 5 && t != 13)
		{
			// Offset black key left or right depending on note.
			float blackKeyCenter = (black_key + 1) * keyWidth / 2;
			if (t == 1 || t == 7)
			{
				blackKeyCenter -= keyWidth / 3;
			}
			if (t == 3 || t == 11)
			{
				blackKeyCenter += keyWidth / 3;
			}

			// test agaist width of black keys.
			if( fabsf(point.x - blackKeyCenter) < keyWidth * 0.5 )
				k_num = black_key;
		}
	}

	int octave = k_num / 14;
	int key = k_num % 14;

	if (key > 4)
		key--;
	if (key > 11)
		key--;

	int lBaseKey = baseKey;

	{
		// don't draw partialy visible keys.
		const int maxWhiteKeys = 75;
		int visibleWhiteKeyCount = r.getWidth() / keyWidth;
		visibleWhiteKeyCount = (std::min)(visibleWhiteKeyCount, maxWhiteKeys);

		if (visibleWhiteKeyCount + baseWhiteKeyNum > maxWhiteKeys)
		{
			int extraOctaves = (6 + visibleWhiteKeyCount + baseWhiteKeyNum - maxWhiteKeys) / 7;
			lBaseKey -= extraOctaves * 12;
		}
	}

	int note = lBaseKey + octave * 12 + key;

	if (note < 0)
		note = 0;
	if (note > 127)
		note = 127;

	return note;
}

inline static int32_t Render(GmpiDrawing_API::IMpDeviceContext* drawingContext, Rect r, std::vector<bool>& keyStates, int& baseKey)
{
	Graphics g(drawingContext);

	g.PushAxisAlignedClip(r);

	int width = r.right - r.left;
	float keyHeight = r.bottom - r.top;
	float keyWidth = floor(keyHeight * 0.2);

	bool prevWhiteKeyState = false;
	auto brushShadow = g.CreateSolidColorBrush(Color::SlateGray);
	auto highlightBrush = g.CreateSolidColorBrush(Color::White);

	int lBaseKey = baseKey;

	// don't draw partialy visible keys.
	const int maxWhiteKeys = 75;
	int visibleWhiteKeyCount = width / keyWidth;
	visibleWhiteKeyCount = (std::min)(visibleWhiteKeyCount, maxWhiteKeys);

	if (visibleWhiteKeyCount + baseWhiteKeyNum > maxWhiteKeys)
	{
		int extraOctaves = (6 + visibleWhiteKeyCount + baseWhiteKeyNum - maxWhiteKeys) / 7;
		lBaseKey -= extraOctaves * 12;
	}

	int usableWidth = visibleWhiteKeyCount * keyWidth;

	auto blueBrush = g.CreateSolidColorBrush(Color::FromBytes(0, 0, 255));
	g.FillRectangle(usableWidth, 0, width - usableWidth, keyHeight, blueBrush);

	// light-grey white-key fill
	Color keyColor = Color::FromBytes(220, 220, 220);
	Color keyGRadColor = Color::FromBytes(200, 200, 200);
	auto wbrush = g.CreateSolidColorBrush(keyColor);
	g.FillRectangle(0, 0, usableWidth, keyHeight, wbrush);

	// top edge shadow.
	int top_shadow = keyHeight / 16;
	Point grad1(0.f, -1.f);
	Point grad2(0.f, (float)top_shadow);

	GradientStop gradientStops[] = {
		{ 0.0f, Color::SlateGray},
		{ 1.0f, keyColor}
	};

	auto gradientStopCollection = g.CreateGradientStopCollection(gradientStops);
	LinearGradientBrushProperties lgbp1(grad1, grad2);
	auto gradientBrush2 = g.CreateLinearGradientBrush(lgbp1, BrushProperties(), gradientStopCollection);
	g.FillRectangle(0.0f, 0.0f, (float)usableWidth, (float)top_shadow - 1.0f, gradientBrush2);

	// white keys highlights + shadows on left
	grad1 = Point(0.f, (float)keyHeight);
	grad2 = Point(0.f, (float)keyHeight / 2);

	auto wkgradientBrush = g.CreateLinearGradientBrush(keyGRadColor, keyColor, grad1, grad2);

	int key = lBaseKey;
	for (float x = 0; x < usableWidth; x += keyWidth)
	{
		if (keyStates[key])
		{
			// gradient on white key
			g.FillRectangle(x, keyHeight / 2 + 1, x + keyWidth, keyHeight, wkgradientBrush);

			if (!prevWhiteKeyState) // key on, draw shadow
			{
				std::vector<Point> points = {
					{ x + 1 , 0 },
				{ x , keyHeight },
				{ x + keyWidth / 4 , keyHeight },
				{ x + keyWidth / 8 , 0 },
				};
				/*
				// shadow falling on left of white key.
				pnts[0].x = x + 1; // top left
				pnts[0].y = 0;
				pnts[1].x = x;		// bottom left
				pnts[1].y = keyHeight;
				pnts[2].x = x + keyWidth / 4;		// bottom right
				pnts[2].y = keyHeight;
				pnts[3].x = x + keyWidth / 8;		// top right
				pnts[3].y = 0;
				*/
				g.FillPolygon(points, brushShadow);
			}
		}
		else
		{
			g.DrawLine(x + 1, top_shadow, x + 1, keyHeight, highlightBrush); // white highlight on left
		}

		prevWhiteKeyState = keyStates[key];

		key += 2;

		int t = key % 12;
		if (t == 1 || t == 6) // skip 'gaps' in black keys
		{
			key--;
		}
	}

	// black key shadows
	int kn = 0;
	if (lBaseKey == 21) // Special case for piano.
	{
		kn = 5;
	}
	key = lBaseKey + 1;
	float blackKeyWidth = (2 * keyWidth + 1) / 3;
	float blackKeyHeight = (2 * keyHeight + 1) / 3;
	float blackKeyTopIndent = (blackKeyWidth + 2) / 4;
	float highlightY = blackKeyHeight / 2;
	for (float wx = 0; wx < usableWidth; wx += keyWidth)
	{
		int t = kn++ % 7;
		if (t != 0 && t != 3)
		{
			float x = wx - blackKeyWidth / 2;

			switch (t)
			{
			case 1:
			case 4:
				x -= (blackKeyWidth + 2) / 6;
				break;
			case 6:
			case 2:
				x += (blackKeyWidth + 2) / 6;
				break;
			};

			float top_indent;
			if (keyStates[key]) // key on, draw highlight different
				top_indent = blackKeyTopIndent;
			else
				top_indent = (blackKeyTopIndent * 2) / 3;

			float shadowLength = blackKeyWidth / 4;
			float bottomshadowLength = shadowLength;
			if (keyStates[key] && !keyStates[key - 1]) // key down, bottom shadow reduced, unless white key down too
			{
				bottomshadowLength = bottomshadowLength / 4;
			}

			if (keyStates[key - 1] && !keyStates[key]) // left white key down, bottom shadow increased
			{
				bottomshadowLength += shadowLength;
			}
			if (bottomshadowLength > wx - x) // prevent shadow overlapping adjacent white key
			{
				bottomshadowLength = wx - x;
			}

			// shadow of black key falling on left white key.
			/*
			pnts[0].x = x;					// bottom left of key
			pnts[0].y = blackKeyHeight - 1;

			pnts[1].x = x + bottomshadowLength;	// bottom left of shadow
			pnts[1].y = blackKeyHeight + bottomshadowLength;

			pnts[2].x = wx;						// bottom right
			pnts[2].y = blackKeyHeight + bottomshadowLength;

			pnts[3].x = wx;						// top right
			pnts[3].y = blackKeyHeight - 1;
			*/

			std::vector<Point> points = {
				{ x , blackKeyHeight - 1 },
			{ x + bottomshadowLength , blackKeyHeight + bottomshadowLength },
			{ wx , blackKeyHeight + bottomshadowLength },
			{ wx , blackKeyHeight - 1 },
			};

			g.FillPolygon(points, brushShadow);

			// shadow of black key falling on right white key.
			bottomshadowLength = shadowLength;
			if (keyStates[key] && !keyStates[key + 1]) // key down, bottom shadow reduced, unless white key down too
			{
				bottomshadowLength = bottomshadowLength / 4;
			}
			if (keyStates[key + 1] && !keyStates[key]) // left white key down, bottom shadow increased
			{
				bottomshadowLength += shadowLength;
			}

			/*
			pnts[0].x = wx;					// bottom center of key
			pnts[0].y = blackKeyHeight - 1;

			pnts[1].x = wx;	// bottom left of shadow
			pnts[1].y = blackKeyHeight + bottomshadowLength;

			pnts[2].x = x + blackKeyWidth + bottomshadowLength;		// bottom right
			pnts[2].y = blackKeyHeight + bottomshadowLength;

			pnts[3].x = x + (blackKeyWidth * 5) / 4 + (bottomshadowLength + shadowLength) / 2;		// middle right
			pnts[3].y = blackKeyHeight / 2 + shadowLength;

			pnts[4].x = x + blackKeyWidth + shadowLength;		// top right
			pnts[4].y = 0;

			pnts[5].x = x;		// top left
			pnts[5].y = 0;
			*/

			std::vector<Point> points2 = {
				{ wx, blackKeyHeight - 1 },
			{ wx, blackKeyHeight + bottomshadowLength },
			{ x + blackKeyWidth + bottomshadowLength, blackKeyHeight + bottomshadowLength },
			{ x + (blackKeyWidth * 5) / 4 + (bottomshadowLength + shadowLength) / 2, blackKeyHeight / 2 + shadowLength },
			{ x + blackKeyWidth + shadowLength, 0 },
			{ x, 0 },
			};

			g.FillPolygon(points2, brushShadow);

			key += 2;

			int t2 = key % 12;
			if (t2 == 0 || t2 == 5) // skip 'gaps' in black keys
			{
				key++;
			}
		}
	}

	auto pen = g.CreateSolidColorBrush(Color::FromBytes(30, 30, 30)); // blackish

	// white keys
	key = lBaseKey;
	for (int x = 0; x < usableWidth; x += keyWidth)
	{
		g.DrawLine(x, 0, x, keyHeight, pen); // black line between keys.

		key += 2;

		int t = key % 12;
		if (t == 1 || t == 6) // skip 'gaps' in black keys
		{
			key--;
		}
	}

	// black keys
	kn = 0;
	if (lBaseKey == 21) // Special case for piano.
	{
		kn = 5;
	}
	key = lBaseKey + 1;

	auto brush = g.CreateSolidColorBrush(Color::Black);
	grad1 = Point(0.f, 0.f);
	grad2 = Point(0.f, (float)blackKeyHeight);
	//auto gradientBrush = g.CreateLinearGradientBrush( grad1, grad2, Color::Black, Color::Gray);
	auto gradientBrush = g.CreateLinearGradientBrush(Color::Black, Color::Gray, grad1, grad2);
	//auto gradientBrush3 = g.CreateLinearGradientBrush( grad1, Point(0.f, (float)blackKeyHeight / 2.0f), Color::Gray, Color::Black);
	auto gradientBrush3 = g.CreateLinearGradientBrush(Color::Gray, Color::Black, grad1, Point(0.f, (float)blackKeyHeight / 2.0f));

	// trapezoid on left of black keys
	for (int wx = 0; wx < usableWidth; wx += keyWidth)
	{
		int t = kn++ % 7;
		if (t != 0 && t != 3)
		{
			float x = wx - blackKeyWidth * 0.5f;

			switch (t)
			{
			case 1:
			case 4:
				x -= (blackKeyWidth + 2) / 6.f;
				break;
			case 6:
			case 2:
				x += (blackKeyWidth + 2) / 6.f;
				break;
			};

			float top_indent;
			if (keyStates[key]) // key on, draw highlight different
				top_indent = blackKeyTopIndent;
			else
				top_indent = (blackKeyTopIndent * 2) / 3.f;

			// key background.
			g.FillRectangle(x, -1, x + blackKeyWidth, blackKeyHeight - 1, brush);

			// highlight on upper half.
			if (keyStates[key]) // key on, draw highlight different
			{
				g.FillRectangle(x + blackKeyTopIndent, top_indent, x + blackKeyTopIndent + blackKeyWidth - 2 * blackKeyTopIndent, blackKeyHeight / 2 - top_indent, gradientBrush3);
			}
			else
			{
				g.FillRectangle(x + blackKeyTopIndent, top_indent, x + blackKeyTopIndent + blackKeyWidth - 2 * blackKeyTopIndent, blackKeyHeight / 2 - top_indent, gradientBrush);
			}

			// left side of black key, trapezoid.
			Point points[] {
					{ x + 1, 0 },
					{ x + blackKeyTopIndent, top_indent },
					{ 0, 0 },
					{ x + 1, blackKeyHeight - 2 },
				};

			if (keyStates[key]) // key on, draw highlight different
			{
				points[2].x = x + blackKeyTopIndent / 2;
				points[2].y = blackKeyHeight - (blackKeyTopIndent * 4) / 3;
			}
			else
			{
				points[2].x = x + blackKeyTopIndent;
				points[2].y = blackKeyHeight - blackKeyTopIndent * 2;
			}

			// gradient
			auto gradientBrush2 = g.CreateLinearGradientBrush(Color::Black, Color::Gray, Point(x, 0.f), Point(x + blackKeyTopIndent, 0.f));

			// draw it.
			g.FillPolygon(points, gradientBrush2);

			key += 2;

			int t2 = key % 12;
			if (t2 == 0 || t2 == 5) // skip 'gaps' in black keys
			{
				key++;
			}
		}
	}

	g.PopAxisAlignedClip();

	return gmpi::MP_OK;
}
};

