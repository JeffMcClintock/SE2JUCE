#include <algorithm>
#include "./PanelGroupGui.h"
#include "../se_sdk3/Drawing.h"
#include "../se_sdk3/Drawing.h"
#include "../shared/ImageCache.h"

using namespace gmpi;
using namespace GmpiDrawing;
using namespace GmpiDrawing_API;

GMPI_REGISTER_GUI(MP_SUB_TYPE_GUI2, PanelGroupGui, L"SE Panel Group" );
SE_DECLARE_INIT_STATIC_FILE(PanelGroup_Gui)

PanelGroupGui::PanelGroupGui() :
	bitmapMetadata_(nullptr)
{
	// initialise pins.
	initializePin( pinText, static_cast<MpGuiBaseMemberPtr2>(&PanelGroupGui::onSetText) );
}

// handle pin updates.
void PanelGroupGui::onSetText()
{
	if (!textFormat_.isNull())
	{
		text_size = textFormat_.GetTextExtentU(pinText.getValue());
		text_size.height = floor( 0.5f * (textData->pixelHeight_ + textFormat_.GetTextExtentU("8M").height)); // SE compatible "average" character height.
	}
	title_utf8 = (std::string) pinText;

//	cachedRender_.setNull();
	invalidateRect();
}

int32_t PanelGroupGui::initialize()
{
	auto r = gmpi_gui::MpGuiGfxBase::initialize(); // ensure all pins initialised (so widgets are built).

	bitmap_ = GetImage("lines", &bitmapMetadata_);

	const char* fontCategory = "panel_group";
	textFormat_ = GetTextFormat(fontCategory);
	textData = GetFontMetatdata(fontCategory);

	onSetText();

	if(textData && textData->verticalSnapBackwardCompatibilityMode)
	{
		text_y = textData->getLegacyVerticalOffset() -2;
	}
	else
	{
		// Center text vertically.
		if(bitmap_ && bitmapMetadata_ && textFormat_)
		{
			const auto size_source = bitmap_.GetSize();
			const auto bitmap_size = bitmapMetadata_->frameSize;
			const int width = size_source.width - bitmap_size.width;

			float top = width / 2 + floorf(text_size.height / 4); // see render for all this crap.
			top += floorf(text_size.height / 4);

			auto metrics = textFormat_.GetFontMetrics();
			text_y = top + (width / 2) - (metrics.bodyHeight() * 0.5);
		}
	}

	return r;
}

int32_t PanelGroupGui::arrange(GmpiDrawing_API::MP1_RECT finalRect)
{
//	cachedRender_.setNull();
	return MpGuiGfxBase::arrange(finalRect);
}

int32_t PanelGroupGui::hitTest(MP1_POINT point)
{
	auto r = getRect();

	if (!r.ContainsPoint(point) || bitmap_.isNull())
		return MP_FAIL;

	// ignore clicks within border
	auto bitmap_size = bitmapMetadata_->frameSize;
	int width = 4 + bitmap_.GetSize().width - bitmap_size.width;
	int font_height = textData->pixelHeight_;
	r.Deflate(width, width + font_height / 4);
	r.Offset(0, font_height / 4);

	return r.ContainsPoint(point) ? MP_FAIL : MP_OK;
}

int32_t PanelGroupGui::OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext )
{
	const bool cacheRender = true;

	if (bitmap_.isNull())
		return MP_FAIL;

	Graphics g(drawingContext);
	auto r = getRect();
	Size mysize = r.getSize();

	const auto size_source = bitmap_.GetSize();
	const auto bitmap_size = bitmapMetadata_->frameSize;
	const int end_length = bitmapMetadata_->line_end_length;
	const int corner_cx = bitmap_size.width / 2;
	const int corner_cy = bitmap_size.height / 2;
	const int width = size_source.width - bitmap_size.width;
	const float halfPixel{ 0.5f };

	// create frame rect, adjusted for text height
	auto moduleRect = getRect();
	moduleRect.Deflate(width / 2, width / 2 + floorf(text_size.height / 4));
	moduleRect.Offset(0, floorf(text_size.height / 4));
	const int left = moduleRect.left;
	const int top = moduleRect.top;
	const int right = moduleRect.right;
	const int bottom = moduleRect.bottom;

	// Draw title.
	const bool draw_title = !pinText.getValue().empty();

#if 1 // draw from "exploded" skin file.

	const char* customImageId = "PanelGroupGui::lines-exploded.bpm";
	auto fixedBitmap = GetCustomImage(g.GetFactory().Get(), customImageId);

	const int padding = 2;
	if (fixedBitmap.isNull())
	{
		const auto mode = 0; // GmpiDrawing_API::MP1_BITMAP_INTERPOLATION_MODE_LINEAR; // MP1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR;

		const int totalpadding = padding * 5;

		// "Explode" skin bitmap so that adjacent pixels don't bleed into each other on shitty GFX cards.
		fixedBitmap = g.GetFactory().CreateImage(size_source.width + totalpadding, size_source.height + totalpadding);
		{
			auto sourceBitmapSize = size_source;

			auto skinBitmapPixels = bitmap_.lockPixels();
			auto surface = fixedBitmap.lockPixels(GmpiDrawing_API::MP1_BITMAP_LOCK_WRITE);

			// top-left corner
			int x = padding;
			int y = padding;

			RectL rSource{ 0, 0, corner_cx + 1, corner_cy + 1 };
			rTopLeft = { x, y, x + rSource.getWidth(), y + rSource.getHeight() };
			surface.Blit(skinBitmapPixels, rTopLeft, rSource, mode);

			// top-right corner
			x += padding + corner_cx;
			rSource.Offset(corner_cx, 0);
			rTopRight = { x, y, x + rSource.getWidth(), y + rSource.getHeight() };
			surface.Blit(skinBitmapPixels, rTopRight, rSource, mode);

			// bottom-right corner
			y += padding + corner_cy;
			rSource.Offset(0, corner_cy);
			rBottomRight = { x, y, x + rSource.getWidth(), y + rSource.getHeight() };
			surface.Blit(skinBitmapPixels, rBottomRight, rSource, mode);

			// bottom-left corner
			x -= padding + corner_cx;
			rSource.Offset(-corner_cx, 0);
			rBottomLeft = { x, y, x + rSource.getWidth(), y + rSource.getHeight() };
			surface.Blit(skinBitmapPixels, rBottomLeft, rSource, mode);

			// left end-cap
			y += corner_cy + padding * 2; // extra padding between circle and lines. else lines overlap.
			int sx = 0;
			int sy = (int32_t)bitmap_size.height;
			rSource = { sx, sy, sx + end_length + 1, sy + width };
			rleftEnd = { x, y, x + rSource.getWidth(), y + rSource.getHeight() };
			surface.Blit(skinBitmapPixels, PointL(x, y), rSource, mode);

			// horizontal lines
			sx += end_length;
			x += end_length;

			// fill for gaps left by padding. 1-pixel long line.
			rSource = { sx, sy, sx + padding, sy + width };
			RectL rHPad = { x, y, x + rSource.getWidth(), y + rSource.getHeight() };
			surface.Blit(skinBitmapPixels, rHPad, rSource, mode);

			// horizontal section
			int tile_l = bitmap_size.width - end_length * 2;
			x += padding;
			rSource = { sx, sy, sx + tile_l + 1, sy + width };
			rHorizontal = { x, y, x + rSource.getWidth(), y + rSource.getHeight() };
			surface.Blit(skinBitmapPixels, rHorizontal, rSource, mode);

			x += tile_l;

			// fill for gaps left by padding. 1-pixel long line.
			rSource = { sx, sy, sx + padding, sy + width };
			rHPad = { x, y, x + rSource.getWidth(), y + rSource.getHeight() };
			surface.Blit(skinBitmapPixels, rHPad, rSource, mode);

			// right end-cap
			sx += tile_l;
			x += padding;
			rSource = { sx, sy, sx + end_length + 1, sy + width };
			rRightEnd = { x, y, x + rSource.getWidth(), y + rSource.getHeight() };
			surface.Blit(skinBitmapPixels, PointL(x, y), rSource, mode);

			// top end-cap
			sx = (int32_t)bitmap_size.width;
			sy = 0;
			x = corner_cy * 2 + padding * 4; // extra padding between circle and lines. else lines overlap.
			y = padding;
			rSource = { sx, sy, sx + width, sy + end_length + 1 };
			surface.Blit(skinBitmapPixels, PointL(x, y), rSource, mode);

			// vertical section
			tile_l = bitmap_size.height - end_length * 2;
			sy += end_length;
			y += end_length + padding;
			rSource = { sx, sy, sx + width, sy + tile_l + 1 };
			rVertical = { x, y, x + rSource.getWidth(), y + rSource.getHeight() };
			surface.Blit(skinBitmapPixels, rVertical, rSource, mode);

			// bottom end-cap
			sy += tile_l;
			y += tile_l + padding;
			rSource = { sx, sy, sx + width, sy + end_length + 1 };
			surface.Blit(skinBitmapPixels, PointL(x, y), rSource, mode);
		}
		
		RegisterCustomImage(customImageId, fixedBitmap);
	}
	else
	{
		auto sourceBitmapSize = size_source;

		// top-left corner
		int x = padding;
		int y = padding;

		RectL rSource{ 0, 0, corner_cx + 1, corner_cy + 1 };
		rTopLeft = { x, y, x + rSource.getWidth(), y + rSource.getHeight() };

		// top-right corner
		x += padding + corner_cx;
		rSource.Offset(corner_cx, 0);
		rTopRight = { x, y, x + rSource.getWidth(), y + rSource.getHeight() };

		// bottom-right corner
		y += padding + corner_cy;
		rSource.Offset(0, corner_cy);
		rBottomRight = { x, y, x + rSource.getWidth(), y + rSource.getHeight() };

		// bottom-left corner
		x -= padding + corner_cx;
		rSource.Offset(-corner_cx, 0);
		rBottomLeft = { x, y, x + rSource.getWidth(), y + rSource.getHeight() };

		// left end-cap
		y += corner_cy + padding * 2; // extra padding between circle and lines. else lines overlap.
		int sx = 0;
		int sy = (int32_t)bitmap_size.height;
		rSource = { sx, sy, sx + end_length + 1, sy + width };
		rleftEnd = { x, y, x + rSource.getWidth(), y + rSource.getHeight() };

		// horizontal lines
		sx += end_length;
		x += end_length;

		// fill for gaps left by padding. 1-pixel long line.
		rSource = { sx, sy, sx + padding, sy + width };
		RectL rHPad = { x, y, x + rSource.getWidth(), y + rSource.getHeight() };

		// horizontal section
		int tile_l = bitmap_size.width - end_length * 2;
		x += padding;
		rSource = { sx, sy, sx + tile_l + 1, sy + width };
		rHorizontal = { x, y, x + rSource.getWidth(), y + rSource.getHeight() };

		x += tile_l;

		// fill for gaps left by padding. 1-pixel long line.
		rSource = { sx, sy, sx + padding, sy + width };
		rHPad = { x, y, x + rSource.getWidth(), y + rSource.getHeight() };

		// right end-cap
		sx += tile_l;
		x += padding;
		rSource = { sx, sy, sx + end_length + 1, sy + width };
		rRightEnd = { x, y, x + rSource.getWidth(), y + rSource.getHeight() };

		// top end-cap
		sx = (int32_t)bitmap_size.width;
		sy = 0;
		x = corner_cy * 2 + padding * 4; // extra padding between circle and lines. else lines overlap.
		y = padding;
		rSource = { sx, sy, sx + width, sy + end_length + 1 };

		// vertical section
		tile_l = bitmap_size.height - end_length * 2;
		sy += end_length;
		y += end_length + padding;
		rSource = { sx, sy, sx + width, sy + tile_l + 1 };
		rVertical = { x, y, x + rSource.getWidth(), y + rSource.getHeight() };

		// bottom end-cap
		sy += tile_l;
		y += tile_l + padding;
		rSource = { sx, sy, sx + width, sy + end_length + 1 };
	}

	auto&dc = g;
	const auto mode = GmpiDrawing_API::MP1_BITMAP_INTERPOLATION_MODE_LINEAR;

//dc.DrawBitmap(fixedBitmap, GmpiDrawing::Point((mysize.width + size_source.width)/2, (mysize.height + size_source.height) / 2), GmpiDrawing::Rect(0, 0, mysize.width, mysize.height), mode);

	// Rounded corners
	dc.DrawBitmap(fixedBitmap, PointL(left, top), rTopLeft, mode);
	dc.DrawBitmap(fixedBitmap, PointL(right - corner_cx - 1, top), rTopRight, mode);
	dc.DrawBitmap(fixedBitmap, PointL(left, bottom - corner_cy - 1), rBottomLeft, mode);
	dc.DrawBitmap(fixedBitmap, PointL(right - corner_cx - 1, bottom - corner_cy - 1), rBottomRight, mode);

	int start_x = left + corner_cx;

	if (draw_title)
	{
		// rounded end on top left curve
		dc.DrawBitmap(fixedBitmap, PointL(left + corner_cx, top), rRightEnd, mode);

		Point text_pos(left + corner_cx + end_length * 2, text_y);
		auto brush = dc.CreateSolidColorBrush(textData->getColor());
		start_x = text_pos.x + text_size.width + end_length;

		// rounded end right of text
		int rnd_x = (std::min)(right - corner_cx - end_length, start_x);
		dc.DrawBitmap(fixedBitmap, PointL(rnd_x, top), rleftEnd, mode);
		start_x += end_length;

		dc.DrawTextU(title_utf8, textFormat_, Rect(text_pos.x, text_pos.y, text_pos.x + text_size.width, text_pos.y + text_size.height), brush);
	}

	// top edge
	int fin_x = right - corner_cx;
	int tile_l = bitmap_size.width - end_length * 2;

	for (int x = start_x; x < fin_x; x += tile_l - 1)
	{
		int source_len = (std::min)(tile_l, fin_x - x);
//		Rect sourceRect(end_length, bitmap_size.height, end_length + source_len, bitmap_size.height + width);
		RectL sourceRect = rHorizontal;
		sourceRect.right = (std::min)(sourceRect.right, sourceRect.left + source_len);

		dc.DrawBitmap(fixedBitmap, PointL(x, top), sourceRect, mode);
	}

	// bot edge
	for (int x = left + corner_cx; x < fin_x; x += tile_l - 1) // -1 to provide a little overlap to preven fine lines between joins
	{
		int source_len = (std::min)(tile_l, fin_x - x);
		RectL sourceRect = rHorizontal;
		sourceRect.right = (std::min)(sourceRect.right, sourceRect.left + source_len);
		PointL destPoint(x, bottom - width);
		dc.DrawBitmap(fixedBitmap, destPoint, sourceRect, mode);
	}

	// left & right edges
	tile_l = bitmap_size.height - end_length * 2;
	int fin_y = bottom - corner_cy;
	for (int y = corner_cy + top; y < fin_y; y += tile_l - 1) // -1 to provide a little overlap
	{
		int source_len = (std::min)(tile_l, fin_y - y);
//		Rect sourceRect(bitmap_size.width, end_length, bitmap_size.width + width, end_length + source_len);
		auto sourceRect = rVertical;
		sourceRect.bottom = (std::min)(sourceRect.bottom, sourceRect.top + source_len);

		// Right.
		dc.DrawBitmap(fixedBitmap, PointL(right - width, y), sourceRect, mode);

		// Left
		dc.DrawBitmap(fixedBitmap, PointL(left, y), sourceRect, mode);
	}

#else

	Graphics dc;

	BitmapRenderTarget bitmapSurface;
	if (cacheRender)
	{
		if (cachedRender_.isNull())
		{

			const auto mode = GmpiDrawing_API::MP1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR;
			int start_x;

			// Create a bitmap
			{
				cachedRender_ = g.GetFactory().CreateImage(mysize.width, mysize.height);
//				bitmapSurface = g.CreateCompatibleRenderTarget(mysize);

				{
					/*
					// NO CAN DO! draw AND blit to same bitmap.
					bitmapSurface.BeginDraw();

					// Clear bitmap.
					bitmapSurface.Clear(Color((uint32_t)0x34535, 0.0f));
					*/
#if 1


//					bitmapSurface.EndDraw();
				}

				// Get pixel data to bit-blit the remainder of the drawing.
//				cachedRender_ = bitmapSurface.GetBitmap();
			}

			auto skinBitmapPixels = bitmap_.lockPixels();
			auto surface = cachedRender_.lockPixels(GmpiDrawing_API::MP1_BITMAP_LOCK_WRITE | GmpiDrawing_API::MP1_BITMAP_LOCK_READ);

			// Rounded corners
			surface.Blit(skinBitmapPixels, PointL(left, top), RectL(0, 0, corner_cx, corner_cy), mode);
			surface.Blit(skinBitmapPixels, PointL(right - corner_cx, top), RectL(corner_cx + 1, 0, corner_cx + 1 + corner_cx, corner_cy), mode);
			surface.Blit(skinBitmapPixels, PointL(left, bottom - corner_cy), RectL(0, corner_cy + 1, corner_cx, corner_cy + 1 + corner_cy), mode);
			surface.Blit(skinBitmapPixels, PointL(right - corner_cx, bottom - corner_cy), RectL(corner_cx + 1, corner_cy + 1, corner_cx + 1 + corner_cx, corner_cy + 1 + corner_cy), mode);

			start_x = left + corner_cx;

			if (draw_title)
			{
				// rounded end on top left curve
				surface.Blit(skinBitmapPixels, PointL(left + corner_cx, top), RectL(bitmap_size.width - end_length, bitmap_size.height, bitmap_size.width - end_length + end_length, bitmap_size.height + width), mode);

				Point text_pos(left + corner_cx + end_length * 2, -2);
				start_x = text_pos.x + text_size.width + end_length;

				// rounded end right of text
				int rnd_x = (std::min)(right - corner_cx - end_length, start_x);
				surface.Blit(skinBitmapPixels, PointL(rnd_x, top), RectL(0, bitmap_size.height, end_length, bitmap_size.height + width), mode);
				start_x += end_length;
			}

			// top edge
			int fin_x = right - corner_cx;
			int tile_l = bitmap_size.width - end_length * 2;

			for (int x = start_x; x < fin_x; x += tile_l)
			{
				int source_len = (std::min)(tile_l, fin_x - x);
				RectL sourceRect(end_length, bitmap_size.height, end_length + source_len, bitmap_size.height + width);
				surface.Blit(skinBitmapPixels, PointL(x, top), sourceRect, mode);
			}

			// bot edge
			for (int x = left + corner_cx; x < fin_x; x += tile_l)
			{
				int source_len = (std::min)(tile_l, fin_x - x);
				RectL sourceRect(end_length, bitmap_size.height, end_length + source_len, bitmap_size.height + width);

				PointL destPoint(x, bottom - width);
				surface.Blit(skinBitmapPixels, destPoint, sourceRect, mode);
			}

			// left & right edges
			tile_l = bitmap_size.height - end_length * 2;
			int fin_y = bottom - corner_cy;
			for (int y = corner_cy + top; y < fin_y; y += tile_l)
			{
				int source_len = (std::min)(tile_l, fin_y - y);
				RectL sourceRect(bitmap_size.width, end_length, bitmap_size.width + width, end_length + source_len);

				// Right.
				surface.Blit(skinBitmapPixels, PointL(right - width, y), sourceRect, mode);

				// Left
				surface.Blit(skinBitmapPixels, PointL(left, y), sourceRect, mode);
			}
		}
	}
#else
					// NO CAN DO! draw AND blit to same bitmap.
					bitmapSurface.BeginDraw();

					// Clear bitmap.
					bitmapSurface.Clear(Color((uint32_t)0x34535, 0.0f));
					dc = bitmapSurface.Get();
				}
			}
		}
		else
		{
			dc = g.Get();
		}
	}

	if (cachedRender_.isNull() || cacheRender == false)
	{
		const bool draw_title = !pinText.getValue().empty();
		const auto size_source = bitmap_.GetSize();
		const auto bitmap_size = bitmapMetadata_->frameSize;
		const int end_length = bitmapMetadata_->line_end_length;
		const int corner_cx = bitmap_size.width / 2;
		const int corner_cy = bitmap_size.height / 2;
		const int width = size_source.width - bitmap_size.width;
		const float halfPixel{ 0.5f };

		// create frame rect, adjusted for text height
		auto moduleRect = getRect();
		moduleRect.Deflate(width / 2, width / 2 + floorf(text_size.height / 4));
		moduleRect.Offset(0, floorf(text_size.height / 4));
		const int left = moduleRect.left;
		const int top = moduleRect.top;
		const int right = moduleRect.right;
		const int bottom = moduleRect.bottom;
		const auto mode = GmpiDrawing_API::MP1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR;

		// Rounded corners
        dc.DrawBitmap(bitmap_, GmpiDrawing::Point(left, top), GmpiDrawing::Rect(0, 0, corner_cx, corner_cy), mode);
		dc.DrawBitmap(bitmap_, Point(right - corner_cx, top), Rect(corner_cx + 1, 0, corner_cx + 1 + corner_cx, corner_cy), mode);
		dc.DrawBitmap(bitmap_, Point(left, bottom - corner_cy), Rect(0, corner_cy + 1, corner_cx, corner_cy + 1 + corner_cy), mode);
		dc.DrawBitmap(bitmap_, Point(right - corner_cx, bottom - corner_cy), Rect(corner_cx + 1, corner_cy + 1, corner_cx + 1 + corner_cx, corner_cy + 1 + corner_cy), mode);

		int start_x = left + corner_cx;

		if (draw_title)
		{
			// rounded end on top left curve
			dc.DrawBitmap(bitmap_, Point(left + corner_cx, top), Rect(bitmap_size.width - end_length, bitmap_size.height, bitmap_size.width - end_length + end_length, bitmap_size.height + width), mode);

			Point text_pos(left + corner_cx + end_length * 2, -2);
			auto brush = dc.CreateSolidColorBrush(textData->getColor());
			start_x = text_pos.x + text_size.width + end_length;

			// rounded end right of text
            int rnd_x = (std::min)(right - corner_cx - end_length, start_x);
			dc.DrawBitmap(bitmap_, Point(rnd_x, top), Rect(0, bitmap_size.height, end_length, bitmap_size.height + width), mode);
			start_x += end_length;

			text_pos.y += textData->getLegacyVerticalOffset();
			dc.DrawTextU(title_utf8, textFormat_, Rect(text_pos.x, text_pos.y, text_pos.x + text_size.width, text_pos.y + text_size.height), brush);
		}

		// top edge
		int fin_x = right - corner_cx;
		int tile_l = bitmap_size.width - end_length * 2;

		for (int x = start_x; x < fin_x; x += tile_l)
		{
			int source_len = (std::min)(tile_l, fin_x - x);
			Rect sourceRect(end_length, bitmap_size.height, end_length + source_len, bitmap_size.height + width);
			dc.DrawBitmap(bitmap_, Point(x, top), sourceRect, mode);
		}

		// bot edge
		for (int x = left + corner_cx; x < fin_x; x += tile_l)
		{
			int source_len = (std::min)(tile_l, fin_x - x);
			Rect sourceRect(end_length, bitmap_size.height, end_length + source_len, bitmap_size.height + width);

			Point destPoint(x, bottom - width);
			dc.DrawBitmap(bitmap_, destPoint, sourceRect, mode);
		}

		// left & right edges
		tile_l = bitmap_size.height - end_length * 2;
		int fin_y = bottom - corner_cy;
		for (int y = corner_cy + top; y < fin_y; y += tile_l)
		{
			int source_len = (std::min)(tile_l, fin_y - y);
			Rect sourceRect(bitmap_size.width, end_length, bitmap_size.width + width, end_length + source_len);

			// Right.
			dc.DrawBitmap(bitmap_, Point(right - width, y), sourceRect, mode);

			// Left
			dc.DrawBitmap(bitmap_, Point(left, y), sourceRect, mode);
		}

		if (!bitmapSurface.isNull())
		{
			dc.EndDraw();
			cachedRender_ = bitmapSurface.GetBitmap();
		}
	}

	if (cacheRender)
#endif

	{
		g.DrawBitmap(cachedRender_, Point(0, 0), r);
	}
#endif

	return gmpi::MP_OK;
}
