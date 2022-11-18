#include "./ImageBase.h"
#include <algorithm>
#include "../shared/unicode_conversion.h"
#include "../shared/xplatform_modifier_keys.h"
#include "../shared/string_utilities.h"
#include "../shared/it_enum_list.h"
#include "../shared/ImageCache.h"
#include "../shared/xp_simd.h"

using namespace std;
using namespace gmpi;
using namespace gmpi_gui;
using namespace JmUnicodeConversions;
using namespace GmpiDrawing;
using namespace GmpiDrawing_API;

// handle pin updates.
void ImageBase::reDraw()
{
	invalidateRect();
}

void ImageBase::calcDrawAt()
{
	if(skinBitmap::calcDrawAt(getAnimationPos()))
		invalidateRect();
/*
	if (bitmap_.isNull())
		return;

	float animationPosition = getAnimationPos();
	animationPosition = ( std::min )( ( std::max )( animationPosition, 0.0f ), 1.0f );

	int draw_at = 0;

	switch( bitmapMetadata_->mode )
	{
	case ABM_ANIMATED:
	default:
	{
		// scale pos by number of frames (rounding to nearest)
		draw_at = FastRealToIntTruncateTowardZero( 0.5f + animationPosition * last_frame_idx );
		draw_at *= bitmapMetadata_->frameSize.height;
	}
	break;

	case ABM_SLIDER:
	{
		draw_at = FastRealToIntTruncateTowardZero( animationPosition * ( bitmapMetadata_->handle_range_hi - bitmapMetadata_->handle_range_lo ) );
		// draw slider knob
		if( bitmapMetadata_->orien tation == 0 ) // bitmap->GetMouseMode() == 0) // vertical
		{
			draw_at = bitmapMetadata_->handle_range_hi - draw_at;
		}
		else
		{
			draw_at = bitmapMetadata_->handle_range_lo + draw_at;
		}
	}
	break;

	case ABM_BAR_GRAPH:
	{
		draw_at = FastRealToIntTruncateTowardZero( animationPosition * ( bitmapMetadata_->line_end_length + 1 ) );
		draw_at *= bitmapMetadata_->handle_range_hi;
		draw_at += bitmapMetadata_->handle_range_lo - 1;
	}
	break;
	}

	setDrawAt(draw_at);
	*/
}

int32_t ImageBase::OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext)
{
	GmpiDrawing::Graphics dc2(drawingContext);

	renderBitmap(dc2, SizeU(0, 0));

	/*
	

	auto dc = dc2.Get();

	auto bitmap = getDrawBitmap();

	if (!bitmap)
	{
		// Draw "not loaded" graphic.
		auto brush = dc2.CreateSolidColorBrush(Color::White);
		dc2.FillRectangle(getRect(), brush);

		brush.SetColor(Color::Black);
		auto tf = GetGraphicsFactory().CreateTextFormat(6);
		dc2.DrawTextU("X", tf, getRect(), brush);

		return gmpi::MP_OK;
	}

	switch (bitmapMetadata_->mode)
	{
	case ABM_ANIMATED:
	default:
	{
		//			_RPT2(_CRT_WARN, "knob Blits to 0,0, W=%d,H=%d\n", s.cx, s.cy);
		float x = (float)bitmapMetadata_->padding_left;
		float y = (float)bitmapMetadata_->padding_top;

		GmpiDrawing::Rect knob_rect(0.f, (float)drawAt, (float)bitmapMetadata_->frameSize.width, (float)(drawAt + bitmapMetadata_->frameSize.height));
		GmpiDrawing::Rect dest_rect(x, y, x + (float)bitmapMetadata_->frameSize.width, y + (float)bitmapMetadata_->frameSize.height);
		dc->DrawBitmap(bitmap, &dest_rect, 1.0f, 1, &knob_rect);
	}
	break;

	case ABM_SLIDER:
	{
		GmpiDrawing::Rect backgroundRect(0, 0, (float)bitmapMetadata_->frameSize.width, (float)bitmapMetadata_->frameSize.height);

		// draw background
		// bitmap->TransparentBlit2(*pDC, draw_p.x, draw_p.y, s.cx, s.cy, 0, 0);
		dc->DrawBitmap(bitmap, &backgroundRect, 1.0f, 1, &backgroundRect);

		// draw slider knob
		if (bitmapMetadata_->orien tation == 0) // bitmap->GetMouseMode() == 0) // vertical
		{
			//int drawAt = (int) (bitmapMetadata_->handle_range_hi - ( animationPosition * ( bitmapMetadata_->handle_range_hi - bitmapMetadata_->handle_range_lo ) ));
			// fix for 'hamond' vertical draw bars
			// clip drawing to top of control
			float draw_pos = (float)drawAt;
			float source_y = bitmapMetadata_->handle_rect.top;
			float dest_height = bitmapMetadata_->handle_rect.getHeight();

			if (draw_pos < 0)
			{
				float chop_off_top = -draw_pos;
				draw_pos = 0;
				dest_height -= chop_off_top;
				source_y += chop_off_top;
			}

			// scale pos (0-100) by knob range
			//				bitmap->TransparentBlit2(*pDC, draw_p.x + knob_rect.left, draw_p.y + draw_pos, knob_rect.Width(), dest_height, knob_rect.left, source_y);
			GmpiDrawing::Rect knob_rect(bitmapMetadata_->handle_rect.left, source_y, bitmapMetadata_->handle_rect.right, bitmapMetadata_->handle_rect.bottom);
			GmpiDrawing::Rect dest_rect(bitmapMetadata_->handle_rect.left, draw_pos, bitmapMetadata_->handle_rect.right, draw_pos + dest_height);
			dc->DrawBitmap(bitmap, &dest_rect, 1.0f, 1, &knob_rect);
		}
		else
		{
			// TODO, proper clipping on all sides !!!
			// fix for horizontal draw bars
			// clip drawing to left of control
			float draw_pos = (float)drawAt;
			float source_x = bitmapMetadata_->handle_rect.left;
			float dest_width = bitmapMetadata_->handle_rect.getWidth();

			if (draw_pos < 0)
			{
				float chop_off_left = -draw_pos;
				draw_pos = 0;
				dest_width -= chop_off_left;
				source_x += chop_off_left;
			}

			// scale pos (0-100) by knob range
			//				bitmap->TransparentBlit2(*pDC, draw_p.x + draw_pos, draw_p.y + knob_rect.top, dest_width, knob_rect.Height(), source_x, knob_rect.top);
			GmpiDrawing::Rect knob_rect(source_x, bitmapMetadata_->handle_rect.top, bitmapMetadata_->handle_rect.right, bitmapMetadata_->handle_rect.bottom);
			GmpiDrawing::Rect dest_rect(draw_pos, bitmapMetadata_->handle_rect.top, draw_pos + dest_width, bitmapMetadata_->handle_rect.bottom);
			dc->DrawBitmap(bitmap, &dest_rect, 1.0f, 1, &knob_rect);
		}
	}
	break;

	case ABM_BAR_GRAPH:
	{
		float y = bitmapMetadata_->frameSize.height - drawAt;
		// draw lit seqments
		{
			GmpiDrawing::Rect knob_rect(0.f, y, (float)bitmapMetadata_->frameSize.width, bitmapMetadata_->frameSize.height);
			GmpiDrawing::Rect dest_rect(0.f, y, (float)bitmapMetadata_->frameSize.width, bitmapMetadata_->frameSize.height);
			dc->DrawBitmap(bitmap, &dest_rect, 1.0f, 1, &knob_rect);
		}
		// draw unlit seqments
		{
			GmpiDrawing::Rect knob_rect(0.f, bitmapMetadata_->frameSize.height, (float)bitmapMetadata_->frameSize.width, (float)bitmapMetadata_->frameSize.height + y);
			GmpiDrawing::Rect dest_rect(0.f, 0.f, (float)bitmapMetadata_->frameSize.width, y);
			dc->DrawBitmap(bitmap, &dest_rect, 1.0f, 1, &knob_rect);
		}
	}
	break;
	}
	*/
	return gmpi::MP_OK;
}

void ImageBase::onSetFilename()
{
	string imageFile = WStringToUtf8(pinFilename);
	Load(imageFile);
}

void ImageBase::Load(const string& imageFile)
{/*
	bitmap_ = GetImage(getHost(), getGuiHost(), imageFile.c_str(), &bitmapMetadata_);
	if( !bitmap_.isNull() )
	{
		auto imageSize = bitmap_.GetSize();
		last_frame_idx = ((int)imageSize.height) / bitmapMetadata_->frameSize.height - 1;

		onLoaded();
	}
	else
	{
		last_frame_idx = 0;
	}
 */

	if (MP_OK == skinBitmap::Load(getHost(), getGuiHost(), imageFile.c_str()))
	{
		onLoaded();

		getGuiHost()->invalidateMeasure();
		calcDrawAt();
		reDraw();
	}
}

int32_t ImageBase::onPointerUp(int32_t flags, GmpiDrawing_API::MP1_POINT point)
{
	//	_RPT2(_CRT_WARN, "onPointerUp (%f,%f)\n", point.x, point.y);

	if( !getCapture() )
	{
		return gmpi::MP_UNHANDLED;
	}

	releaseCapture();
	pinMouseDown = false;
	pinMouseDownLegacy = false;

	return gmpi::MP_OK;
}

int32_t ImageBase::populateContextMenu(float x, float y, gmpi::IMpUnknown* contextMenuItemsSink)
{
	/*
	if (!hitTest(Point(x,y))) // hit test should be communicated already during On PointerDown via return code.
		return MP_UNHANDLED;
	*/

	gmpi::IMpContextItemSink* sink;
	contextMenuItemsSink->queryInterface(gmpi::MP_IID_CONTEXT_ITEMS_SINK, reinterpret_cast<void**>( &sink));

	it_enum_list itr(pinMenuItems);

	for (itr.First(); !itr.IsDone(); ++itr)
	{
		int32_t flags = 0;

		// Special commands (sub-menus)
		switch (itr.CurrentItem()->getType())
		{
			case enum_entry_type::Separator:
			case enum_entry_type::SubMenu:
				flags |= gmpi_gui::MP_PLATFORM_MENU_SEPARATOR;
				break;

			case enum_entry_type::SubMenuEnd:
			case enum_entry_type::Break:
				continue;

			default:
				break;
		}

		sink->AddItem(WStringToUtf8(itr.CurrentItem()->text).c_str(), itr.CurrentItem()->value, flags);
	}
	sink->release();
	return gmpi::MP_OK;
}

int32_t ImageBase::onContextMenu(int32_t selection)
{
	pinMenuSelection = selection; // send menu momentarily, then reset.
	pinMenuSelection = -1;

	return gmpi::MP_OK;
}

int32_t ImageBase::getToolTip(float x, float y, gmpi::IMpUnknown* returnToolTipString)
{
	IString* returnValue = 0;

	auto hint = getHint();

	if(hint.empty() || MP_OK != returnToolTipString->queryInterface(gmpi::MP_IID_RETURNSTRING, reinterpret_cast<void**>( &returnValue)) )
	{
		return gmpi::MP_NOSUPPORT;
	}

	auto utf8ToolTip = WStringToUtf8(hint);

	returnValue->setData(utf8ToolTip.data(), (int32_t)utf8ToolTip.size());

	return gmpi::MP_OK;
}

