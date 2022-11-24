#include "./Image2GuiXp.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <algorithm>
#include "../shared/unicode_conversion.h"
#include "../shared/xplatform_modifier_keys.h"

using namespace gmpi;
using namespace gmpi_gui;
using namespace GmpiDrawing;

GMPI_REGISTER_GUI(MP_SUB_TYPE_GUI2, Image2GuiXp, L"Image3");
SE_DECLARE_INIT_STATIC_FILE(Image3_Gui);

Image2GuiXp::Image2GuiXp(bool useMouseResponsePin) :
	useMouseResponsePin_(useMouseResponsePin)
{
	initializePin(0, pinAnimationPosition, static_cast<MpGuiBaseMemberPtr2>( &Image2GuiXp::calcDrawAt ));
	initializePin(1, pinFilename, static_cast<MpGuiBaseMemberPtr2>(&Image2GuiXp::onSetFilename));
	initializePin(2, pinHint);
	initializePin(3, pinMenuItems);
	initializePin(4, pinMenuSelection);
	initializePin(5, pinMouseDown);
	initializePin(6, pinMouseDownLegacy);
	initializePin(7, pinFrameCount);
	initializePin(8, pinFrameCountLegacy);
	
	if(useMouseResponsePin_)
		initializePin(9, pinMouseResponse);
}

void Image2GuiXp::setAnimationPos(float p)
{
	pinAnimationPosition = p;
}

void Image2GuiXp::onLoaded()
{
	if (bitmapMetadata_)
	{
		int fc = bitmapMetadata_->getFrameCount();
		pinFrameCount = fc;
		pinFrameCountLegacy = fc;
	}
}

int Image2GuiXp::getMouseResponse()
{
	if( bitmapMetadata_ != nullptr && pinMouseResponse == -1 )
	{
		return bitmapMetadata_->orientation;
	}

	if (useMouseResponsePin_)
		return (std::max)(-1, pinMouseResponse.getValue()); // -2 in enum = off, which is mouse_reponse -1
	else
		return 0;
}

int32_t Image2GuiXp::onMouseWheel(int32_t flags, int32_t delta, GmpiDrawing_API::MP1_POINT point)
{
	// ignore horizontal scrolling
	if (0 != (flags & gmpi_gui_api::GG_POINTER_KEY_SHIFT))
		return gmpi::MP_UNHANDLED;

	if (!skinBitmap::bitmapHitTestLocal(point))
		return MP_UNHANDLED;

	const float scale = (flags & gmpi_gui_api::GG_POINTER_KEY_CONTROL) ? 1.0f / 12000.0f : 1.0f / 1200.0f;
	const float newval = getAnimationPos() + delta * scale;
	setAnimationPos(std::clamp(newval, 0.0f, 1.0f));
	calcDrawAt();

	return gmpi::MP_OK;
}

int32_t Image2GuiXp::onPointerDown(int32_t flags, GmpiDrawing_API::MP1_POINT point)
{
	//	_RPT2(_CRT_WARN, "onPointerDown (%f,%f)\n", point.x, point.y);
	if(!skinBitmap::bitmapHitTestLocal(point))
		return MP_UNHANDLED;

	// Let host handle right-clicks.
	if ((flags & GG_POINTER_FLAG_FIRSTBUTTON) == 0)
	{
		return gmpi::MP_OK; // Indicate successful hit, so right-click menu can show.
	}

	m_ptPrev = point;	// note first point.

	//	smallDragSuppression = true;

	switch( getMouseResponse() )
	{
		case -1: // no mouse response.
// this is invisible, not correct.			return MP_UNHANDLED;
		return gmpi::MP_OK;
			break;

		case 2: // clicks
		{
			if( getAnimationPos() == 0.f )
			{
				setAnimationPos( 1.f );
			}
			else
			{
				setAnimationPos( 0.f );
			}
			calcDrawAt();
		}
		break;

		case 4: // step through frames on click
		{
			// when used with stepped switch, need to keep increasing position till
			// frame changes, as intermediate frames may not be used
//			int last_frame_idx = bitmapMetadata_->getFrameCount() - 1;
			int initial_draw_at = drawAt;
			//float initial_pos = normalised_pos;
			// some modules like bitmapfreezer forcily override normalised_pos every time it's updated.
			// to avoid infinite loop,limit looping.
			int timeout = last_frame_idx;

			while( initial_draw_at == drawAt && timeout-- >= 0 )
			{
				int frame_num = (int)( 0.5f + getAnimationPos() * last_frame_idx );
				frame_num++;

				if( frame_num > last_frame_idx )
					frame_num = 0;

				// calc midpoint between frame steps
				float new_pos = (float)frame_num / last_frame_idx;
				setAnimationPos(new_pos);
				calcDrawAt();
				//					_RPT1(_CRT_WARN, " %d\n", limited_pos );
			}
		}
		break;

		default:
		{
			m_rot_mode = 0.f;
		}
	}

	if (!bitmap_.isNull())
	{
		pinMouseDown = true;
		pinMouseDownLegacy = true;
	}

	setCapture();

	return gmpi::MP_OK;
}

int32_t Image2GuiXp::onPointerMove(int32_t flags, GmpiDrawing_API::MP1_POINT point)
{
	//	_RPT2(_CRT_WARN, "onPointerMove (%f,%f)\n", point.x, point.y);

	if( !getCapture() )
	{
		return gmpi::MP_UNHANDLED;
	}
	
//	_RPT2(_CRT_WARN, "onPointerMove (%f,%f)\n", point.x, point.y);

	Point offset(point.x - m_ptPrev.x, point.y - m_ptPrev.y); // TODO overload subtraction.

	float coarseness = 0.005f;

//	if (modifier_keys::isHeldCtrl()) // <cntr> key magnifies
	if (flags & gmpi_gui_api::GG_POINTER_KEY_CONTROL) // <cntr> key magnifies
		coarseness = 0.001f;

	float new_pos = getAnimationPos();// pinAnimationPosition - coarseness * (float)offset.y;

	switch( getMouseResponse() )
	{
		case 2:	// cl
		case 4:	// stepped
			break;

		case 3:	// rotary / vertical
		{
			auto r = getRect();

			// make movement relative to center
			Point cent(r.getWidth() / 2, r.getHeight() / 2);
			Point old_point(m_ptPrev.x - cent.x, m_ptPrev.y - cent.y);
			Point new_point(point.x - cent.x, point.y - cent.y);

			// rotary movements don't count at knob center
			float rot_amnt = (float)( abs(new_point.x) + abs(new_point.y) ); // rough 'radius calc
			rot_amnt = 2.f * rot_amnt / (float)( r.getWidth() + r.getHeight() );
			rot_amnt = (std::min)(rot_amnt, 1.f);
			//			_RPT1(_CRT_WARN, " rot_amnt %f\n", rot_amnt );
			// calc rotary movement
			float a1 = atan2f((float)old_point.y, (float)old_point.x);
			float a2 = atan2f((float)new_point.y, (float)new_point.x);
			float rot = ( a2 - a1 );

			if( rot < -M_PI )
				rot += 2.f * (float) M_PI;

			if( rot > M_PI )
				rot -= 2.f * (float) M_PI;

			rot *= rot_amnt * 20.f;
			// vertical movement
			// signifigance fades further from vertical center
			int center_offst = (int) abs(new_point.x);
			const int vertical_zone_width = 10; // pixels

			if( center_offst > vertical_zone_width )
				center_offst = vertical_zone_width;

			float vert_amnt = 1.f - (float)center_offst / vertical_zone_width;
			vert_amnt = (std::max)(vert_amnt, 0.f);
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

//			if(modifier_keys::isHeldCtrl() ) // GetKeyState(VK_CONTROL) < 0 ) // cntrl key magnifies
			if (flags & gmpi_gui_api::GG_POINTER_KEY_CONTROL) // <cntr> key magnifies
				delta *= 0.1f;

			new_pos = getAnimationPos() + delta * 0.01f;
		}
		break;

		default:
		{
			if( getMouseResponse() == 0 ) // moving mouse up or right increments 'pos'
			{
				new_pos = getAnimationPos() - coarseness * (float)offset.y;
			}
			else
			{
				new_pos = getAnimationPos() + coarseness * (float)offset.x;
			}

			//			_RPT1(_CRT_WARN, "CMyBmpCtrl3 <- %f\n", new_pos );
			//	_RPT1(_CRT_WARN, "%d: ", pos );
		}
	};

	if( new_pos < 0.f )
		new_pos = 0.f;

	if( new_pos > 1.f )
		new_pos = 1.f;

	setAnimationPos(new_pos);

	m_ptPrev = point;

	calcDrawAt();

	return gmpi::MP_OK;
}

int32_t Image2GuiXp::onPointerUp(int32_t flags, GmpiDrawing_API::MP1_POINT point)
{
	//	_RPT2(_CRT_WARN, "onPointerUp (%f,%f)\n", point.x, point.y);

	if( !getCapture() )
	{
		return gmpi::MP_UNHANDLED;
	}

	releaseCapture();
	pinMouseDown = false;
	pinMouseDownLegacy = false;

	if( getMouseResponse() == 2 && toggleMode2 == ToggleMode::Momentary) // clicks
	{
		setAnimationPos(0.f);
		calcDrawAt();
	}

	return gmpi::MP_OK;
}

int32_t Image2GuiXp::measure(GmpiDrawing_API::MP1_SIZE availableSize, GmpiDrawing_API::MP1_SIZE* returnDesiredSize)
{
	if( !bitmap_.isNull() )
	{
		*returnDesiredSize = bitmapMetadata_->getPaddedFrameSize();
	}
	else
	{
		returnDesiredSize->width = 10;
		returnDesiredSize->height = 10;
	}

	return gmpi::MP_OK;
}
