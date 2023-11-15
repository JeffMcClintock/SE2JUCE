#include "./PlainImageGui.h"
#include <algorithm>
#include "../se_sdk3/Drawing.h"

using namespace std;
using namespace gmpi;
using namespace GmpiDrawing;

GMPI_REGISTER_GUI(MP_SUB_TYPE_GUI2, PlainImageGui, L"SE Plain Image" );
SE_DECLARE_INIT_STATIC_FILE(PlainImage_Gui);

#define USE_BITMAP_BRUSH 1

PlainImageGui::PlainImageGui()
{
	// initialise pins.
	initializePin( pinFilename, static_cast<MpGuiBaseMemberPtr2>(&PlainImageGui::onSetFilename) );
	initializePin( pinStretchMode, static_cast<MpGuiBaseMemberPtr2>(&PlainImageGui::onSetMode) );
}

void PlainImageGui::onSetFilename()
{
	bitmap_ = nullptr;

	if ((int)StretchMode::Tiled != pinStretchMode.getValue() || USE_BITMAP_BRUSH)
	{
		const string filename = pinFilename;
		bitmap_ = GetImage(getHost(), getGuiHost(), filename.c_str(), &bitmapMetadata_);
	}

	invalidateMeasure();
	invalidateRect();
}

void PlainImageGui::onSetMode()
{
	invalidateMeasure();
}

int32_t PlainImageGui::OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext)
{
#if 0 // dead code
	// Create tiled image (need to be arranged first in order to know module dimensions).
	if (bitmap_.isNull() && !USE_BITMAP_BRUSH)
	{
		if ((int)StretchMode::Tiled != pinStretchMode.getValue())
		{
			return MP_FAIL;
		}

		const string filename = pinFilename;
		auto bitmap = GetImage(getHost(), getGuiHost(), filename.c_str(), &bitmapMetadata_);

		if (bitmap.isNull())
		{
			return MP_FAIL;
		}

		auto moduleRect = getRect();
		auto sourceSize = bitmap.GetSize();
		RectL bitmapRect(0, 0, sourceSize.width, sourceSize.height);
		SizeU destSize(
			(std::max)(1, static_cast<int32_t>(moduleRect.getWidth())),
			(std::max)(1, static_cast<int32_t>(moduleRect.getHeight())));

		bitmap_ = GetGraphicsFactory().CreateImage(destSize);

		if (bitmap_.isNull()) // can fail if dimensions are too big.
		{
			return MP_FAIL;
		}

		auto pixelsSource = bitmap.lockPixels();
		auto pixelsDest = bitmap_.lockPixels(GmpiDrawing_API::MP1_BITMAP_LOCK_WRITE);

		for (int x = 0; x != sourceSize.width; ++x)
		{
			for (int y = 0; y != sourceSize.height; ++y)
			{
				auto pixel = pixelsSource.getPixel(x, y);

				for (int y2 = y; y2 < static_cast<int>(destSize.height); y2 += sourceSize.height)
				{
					for (int x2 = x; x2 < static_cast<int>(destSize.width); x2 += sourceSize.width)
					{
						pixelsDest.setPixel(x2, y2, pixel);
					}
				}
			}
		}
        
        pixelsDest.setPixel(0, 0, 0x00ff00ff);
	}
#endif
	
	Graphics g(drawingContext);
	auto moduleRect = getRect();

	if(bitmap_.isNull())
	{
		auto fallbackBrush = g.CreateSolidColorBrush(Color::Gray);
		g.FillRectangle(moduleRect, fallbackBrush);
		return MP_OK;
	}

	auto bitmapSize = bitmap_.GetSize();
	Rect bitmapRect(0, 0, static_cast<float>(bitmapSize.width), static_cast<float>(bitmapSize.height));

	switch (pinStretchMode)
	{
		case (int)StretchMode::Fixed:
		{
			bitmapRect.right = (std::min)(bitmapRect.right, moduleRect.right);
			bitmapRect.bottom = (std::min)(bitmapRect.bottom, moduleRect.bottom);
			g.DrawBitmap(bitmap_, bitmapRect,  bitmapRect);
		}
		break;

		case (int)StretchMode::Tiled:
		{
			if(USE_BITMAP_BRUSH)
			{
				auto brush = g.CreateBitmapBrush(bitmap_);
				g.FillRectangle(moduleRect, brush);
			}
			else
			{
				bitmapRect.right = (std::min)(bitmapRect.right, moduleRect.right);
				bitmapRect.bottom = (std::min)(bitmapRect.bottom, moduleRect.bottom);
				g.DrawBitmap(bitmap_, bitmapRect, bitmapRect);
			}
		}
/* old, drew faint lines at joins on certain DPI settings
		{
			for (float x = 0; x <= moduleRect.right; x += bitmapRect.getWidth())
			{
				Rect source(bitmapRect);
				source.right = (std::min)(moduleRect.right - x, source.right);

				for (float y = 0; y <= moduleRect.bottom; y += bitmapRect.getHeight())
				{
					source.bottom = (std::min)(moduleRect.bottom - y, source.bottom);

					dc.DrawBitmap(bitmap_, Point(x, y), source);
				}
			}
		}
*/
		break;

	case (int)StretchMode::Stretch:
		g.DrawBitmap(bitmap_, moduleRect,  bitmapRect);
		break;
	}

	return gmpi::MP_OK;
}

int32_t PlainImageGui::measure(GmpiDrawing_API::MP1_SIZE availableSize, GmpiDrawing_API::MP1_SIZE* returnDesiredSize)
{
	switch (pinStretchMode)
	{
	case (int) StretchMode::Fixed:
		if (bitmap_.isNull())
		{
			*returnDesiredSize = Size(10, 10);
		}
		else
		{
			*returnDesiredSize = bitmapMetadata_->getPaddedFrameSize();
		}
		break;

	case (int) StretchMode::Tiled:
	case (int) StretchMode::Stretch:
		*returnDesiredSize = availableSize;
		break;
	}

	return gmpi::MP_OK;
}

int32_t PlainImageGui::arrange(GmpiDrawing_API::MP1_RECT finalRect)
{
	auto r = gmpi_gui::MpGuiGfxBase::arrange(finalRect);

	if (pinStretchMode == (int)StretchMode::Tiled && !bitmap_.isNull())
	{
		if (bitmap_.GetSizeF() != GmpiDrawing::Rect(finalRect).getSize())
			onSetFilename();
	}

	return r;
}
