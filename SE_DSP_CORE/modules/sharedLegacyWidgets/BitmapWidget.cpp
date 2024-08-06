#include "BitmapWidget.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <algorithm>
#include "../shared/xplatform_modifier_keys.h"
#include "../se_sdk3/MpString.h"
#include "../shared/xp_simd.h"

using namespace std;
using namespace gmpi;
using namespace gmpi_sdk;
using namespace GmpiDrawing;
using namespace GmpiDrawing_API;

BitmapWidget::BitmapWidget()
{
}

void BitmapWidget::OnRender(Graphics& dc)
{
	renderBitmap(dc, SizeU(position.left, position.top) );
}

void BitmapWidget::Load(const char* imageFile)
{
	skinBitmap::Load(getHost(), getGuiHost(), imageFile);
	calcDrawAt();
}

void BitmapWidget::SetNormalised(float newAnimationPosition)
{
	if( !( newAnimationPosition >= 0.f ) ) // reverse logic catches NANs
		newAnimationPosition = 0.f;

	if( newAnimationPosition > 1.f )
		newAnimationPosition = 1.f;

	animationPosition = newAnimationPosition;
	calcDrawAt();
}

bool BitmapWidget::onPointerDown(int32_t flags, GmpiDrawing_API::MP1_POINT point)
{
	if (!bitmapMetadata_ || bitmapMetadata_->orientation == (int) ImageMetadata::MouseResponse::Ignore ) // -1) // ignores mouse.
		return false;

	// make relative.
	point.x -= position.left;
	point.y -= position.top;

	m_ptPrev = point;	// note first point.

	//	smallDragSuppression = true;
	bool setCapture = false;

	if( !bitmap_.isNull() ) // && (GetVoFlags() & VO_READ_ONLY) == 0)
	{
		setCapture = true;

		switch( bitmapMetadata_->orientation )
		{
		case (int)ImageMetadata::MouseResponse::ClickToggle: // clicks
		{
			if (toggleMode2 == ToggleMode::Alternate)
			{
				SetNormalised(animationPosition == 0.f);
			}
			else
			{
				SetNormalised(1.f);
			}
		}
		break;

		case (int)ImageMetadata::MouseResponse::Stepped: // step through frames on click
		{
//			auto imageSize = bitmap_.GetSize();

//			int last_frame_idx = ( (int)imageSize.height ) / (int) bitmapMetadata_->frameSize.height - 1;
			// scale pos (0-100) by number of frames (rounding to nearest)
			int draw_at = FastRealToIntTruncateTowardZero( 0.5f + animationPosition * last_frame_idx );

			draw_at *= (int) bitmapMetadata_->frameSize.height;
			// when used with stepped switch, need to keep increasing position till
			// frame changes, as intermediate frames may not be used
			//			int last_frame_idx = bitmap_.GetMaxIndex();
			int initial_draw_at = draw_at;
			//float initial_pos = normalised_pos;
			// some modules like bitmapfreezer forcily override normalised_pos every time it's updated.
			// to avoid infinite loop,limit looping.
			int timeout = last_frame_idx;

			float np = animationPosition;
			while( initial_draw_at == draw_at && timeout-- >= 0 )
			{
				int frame_num = (int)( 0.5f + animationPosition * last_frame_idx );
				frame_num++;

				if( frame_num > last_frame_idx )
					frame_num = 0;

				// calc midpoint between frame steps
				np = (float)frame_num / last_frame_idx;
				//					_RPT1(_CRT_WARN, " %d\n", limited_pos );
			}
			SetNormalised(np);
		}
		break;

		default:
		{
			m_rot_mode = 0.f;
		}
		}
	}

	return setCapture;
}

GmpiDrawing::Point BitmapWidget::getKnobCenter()
{
	return Point(position.getWidth() / 2, position.getHeight() / 2);
}

void BitmapWidget::onPointerMove(int32_t flags, GmpiDrawing_API::MP1_POINT point)
{
	// make relative.
	point.x -= position.left;
	point.y -= position.top;

	Point offset(point.x - m_ptPrev.x, point.y - m_ptPrev.y); // TODO overload subtraction.

	float coarseness = 0.005f;

//	if (modifier_keys::isHeldCtrl()) // <cntr> key magnifies
	if (flags & gmpi_gui_api::GG_POINTER_KEY_CONTROL) // <cntr> key magnifies
		coarseness = 0.001f;

	float new_pos = animationPosition;

	switch( bitmapMetadata_->orientation )
	{
		case (int)ImageMetadata::MouseResponse::ClickToggle:// 2:	// cl
		case (int)ImageMetadata::MouseResponse::Stepped:// :	// stepped
			break;

		case (int)ImageMetadata::MouseResponse::Rotary:// 3:	// rotary / vertical
		{
			GmpiDrawing::Rect r(0,0, position.getWidth(), position.getHeight());

			// make movement relative to center
			Point cent = getKnobCenter(); // (r.getWidth() / 2, r.getHeight() / 2);
			Point old_point(m_ptPrev.x - cent.x, m_ptPrev.y - cent.y);
			Point new_point(point.x - cent.x, point.y - cent.y);

			// rotary movements don't count at knob center
			float rot_amnt = (float)( fabs(new_point.x) + fabs(new_point.y) ); // rough 'radius calc
			rot_amnt = 2.f * rot_amnt / (float)( r.getWidth() + r.getHeight() );
			rot_amnt = min(rot_amnt, 1.f);
			//			_RPT1(_CRT_WARN, " rot_amnt %f\n", rot_amnt );
			// calc rotary movement
			float a1 = atan2f((float)old_point.y, (float)old_point.x);
			float a2 = atan2f((float)new_point.y, (float)new_point.x);
			float rot = ( a2 - a1 );

			if( rot < -M_PI )
				rot += 2.f * M_PI;

			if( rot > M_PI )
				rot -= 2.f * M_PI;

			rot *= rot_amnt * 20.f;
			// vertical movement
			// signifigance fades further from vertical center
			float center_offst = fabs(new_point.x);
			const int vertical_zone_width = 10; // pixels

			if( center_offst >vertical_zone_width )
				center_offst = vertical_zone_width;

			float vert_amnt = 1.f - (float)center_offst / vertical_zone_width;
			vert_amnt = max(vert_amnt, 0.f);
			//			_RPT1(_CRT_WARN, " vert amnt %f\n", vert_amnt );
			Point offset(new_point.x - old_point.x, new_point.y - old_point.y);
			float vert = vert_amnt * (float)offset.y;

			//			_RPT2(_CRT_WARN, " rot %f       vert %f\n", rot, vert );
			// average out mode calculation for smoothness
			if( fabs(rot) > fabs(vert) )
			{
				m_rot_mode = ( 0.9f * m_rot_mode + 0.1f );
			}
			else
			{
				m_rot_mode = ( 0.9f * m_rot_mode - 0.1f );
			}

			//			_RPT1(_CRT_WARN, " rot mode %f\n", m_rot_mode );
			float delta;

			if( m_rot_mode > 0 )
			{
				delta = rot;
			}
			else
			{
				delta = (float)-offset.y;
			}

//			if( modifier_keys::isHeldCtrl() ) //GetKeyState(VK_CONTROL) < 0 ) // cntrl keyu magnifies
			if (flags & gmpi_gui_api::GG_POINTER_KEY_CONTROL) // <cntr> key magnifies
				delta *= 0.1f;

			new_pos = animationPosition + delta * 0.01f;
		}
		break;

		default:
		{
			float coarseness = 0.005f;

//			if( modifier_keys::isHeldCtrl() )// cntrl key magnifies.
			if (flags & gmpi_gui_api::GG_POINTER_KEY_CONTROL) // <cntr> key magnifies
				coarseness = 0.001f;

			switch (bitmapMetadata_->orientation)
			{
			case (int)ImageMetadata::MouseResponse::Vertical:
				new_pos = animationPosition - coarseness * (float)offset.y;
				break;

			case (int)ImageMetadata::MouseResponse::Horizontal:
				new_pos = animationPosition + coarseness * (float)offset.x;
				break;

			case (int)ImageMetadata::MouseResponse::ReverseVertical:
				new_pos = animationPosition + coarseness * (float)offset.y;
				break;
			}
/*
			if( bitmapMetadata_->orientation == (int)ImageMetadata::MouseResponse::Vertical)// )// moving mouse up or right increments 'pos'
			{
				new_pos = animationPosition - coarseness * (float)offset.y;
			}
			else
			{
				new_pos = animationPosition + coarseness * (float)offset.x;
			}
*/

			//			_RPT1(_CRT_WARN, "CMyBmpCtrl3 <- %f\n", new_pos );
			//	_RPT1(_CRT_WARN, "%d: ", pos );
		}
	};

	SetNormalised(new_pos);

	m_ptPrev = point;
}

void BitmapWidget::onPointerUp(int32_t flags, GmpiDrawing_API::MP1_POINT point)
{
	// make relative.
	point.x -= position.left;
	point.y -= position.top;

	if (bitmapMetadata_->orientation == (int)ImageMetadata::MouseResponse::ClickToggle) //2) // clicks
	{
		if (toggleMode2 == ToggleMode::Momentary)
		{
			SetNormalised(0.f);
		}
	}
}

GmpiDrawing::Size BitmapWidget::getSize()
{
	if( !bitmap_.isNull() )
	{
		return Size(bitmapMetadata_->frameSize.width + bitmapMetadata_->padding_left + bitmapMetadata_->padding_right, bitmapMetadata_->frameSize.height + bitmapMetadata_->padding_top + bitmapMetadata_->padding_bottom);
	}

	return Size(10,10);
}

void BitmapWidget::calcDrawAt()
{
	if (skinBitmap::calcDrawAt(animationPosition))
	{
		dirty = true;

		if (OnChangedEvent != nullptr)
			OnChangedEvent(animationPosition);
	}
}
