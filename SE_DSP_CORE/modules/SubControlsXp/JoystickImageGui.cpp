#include "./JoystickImageGui.h"
#include <algorithm>
#include "../shared/unicode_conversion.h"
#include "../shared/xplatform_modifier_keys.h"

using namespace std;
using namespace gmpi;
using namespace gmpi_gui;
using namespace JmUnicodeConversions;
using namespace GmpiDrawing;
using namespace GmpiDrawing_API;

GMPI_REGISTER_GUI(MP_SUB_TYPE_GUI2, JoystickImageGui, L"ImageJoystick");

JoystickImageGui::JoystickImageGui()
{
	// initialise pins.
	initializePin(pinAnimationPosition, static_cast<MpGuiBaseMemberPtr2>( &JoystickImageGui::calcDrawAt ));
	initializePin( pinFilename, static_cast<MpGuiBaseMemberPtr2>(&JoystickImageGui::onSetFilename) );
	initializePin( pinHint );
	initializePin( pinMenuItems );
	initializePin( pinMenuSelection);
	initializePin(pinMouseDown);
	initializePin(pinMouseDown2);
	initializePin( pinMouseDownLegacy);
	initializePin( pinFrameCount);
	initializePin( pinFrameCountLegacy);
	initializePin( pinPositionX, static_cast<MpGuiBaseMemberPtr2>( &JoystickImageGui::reDraw ));
	initializePin( pinPositionY, static_cast<MpGuiBaseMemberPtr2>(&JoystickImageGui::reDraw) );
	initializePin(pinJumpToMouse);
}

void JoystickImageGui::setAnimationPos(float p)
{
	pinAnimationPosition = p;
}

void JoystickImageGui::onLoaded()
{
	int fc = bitmapMetadata_->getFrameCount();
	pinFrameCount = fc;
	pinFrameCountLegacy = fc;
}

int32_t JoystickImageGui::OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext)
{
	GmpiDrawing::Graphics g(drawingContext);
	GmpiDrawing::Rect r = getRect();

	ClipDrawingToBounds cd(g, r);

	if (bitmap_.isNull())
		return gmpi::MP_OK;

	switch( bitmapMetadata_->mode )
	{
	case ABM_ANIMATED:
	default:
	{
		float normalisedX = ( std::max )( 0.0f, ( std::min )( 1.0f, pinPositionX.getValue() ) );
		float normalisedY = 1.0f - ( std::max )( 0.0f, ( std::min )( 1.0f, pinPositionY.getValue() ) );
		float x = bitmapMetadata_->padding_left + normalisedX * ( r.getWidth() - ( bitmapMetadata_->frameSize.width + bitmapMetadata_->padding_left ));
		float y = bitmapMetadata_->padding_top + normalisedY * ( r.getHeight() - ( bitmapMetadata_->frameSize.height + bitmapMetadata_->padding_top ) );

		GmpiDrawing::Rect knob_rect(0.f, (float)drawAt, (float)bitmapMetadata_->frameSize.width, (float)( drawAt + bitmapMetadata_->frameSize.height));
		GmpiDrawing::Rect dest_rect(x, y, x + (float)bitmapMetadata_->frameSize.width, y + (float)bitmapMetadata_->frameSize.height);
		g.DrawBitmap(bitmap_, dest_rect, knob_rect);
	}
	break;
	}

	return gmpi::MP_OK;
}

int32_t JoystickImageGui::hitTest(GmpiDrawing_API::MP1_POINT point)
{
	// With jump-to-mouse, any click within boarder counts.
	if (pinJumpToMouse || bitmap_.isNull())
		return gmpi::MP_OK;

	auto r = getRect();
	float normalisedX = (std::max)(0.0f, (std::min)(1.0f, pinPositionX.getValue()));
	float normalisedY = 1.0f - (std::max)(0.0f, (std::min)(1.0f, pinPositionY.getValue()));
	float x = bitmapMetadata_->padding_left + normalisedX * (r.getWidth() - (bitmapMetadata_->frameSize.width + bitmapMetadata_->padding_left));
	float y = bitmapMetadata_->padding_top + normalisedY * (r.getHeight() - (bitmapMetadata_->frameSize.height + bitmapMetadata_->padding_top));
	GmpiDrawing::Rect dest_rect(x, y, x + (float)bitmapMetadata_->frameSize.width, y + (float)bitmapMetadata_->frameSize.height);

	if (dest_rect.ContainsPoint(point) && bitmapHitTestLocal(GmpiDrawing::Point(point) - GmpiDrawing::Size(x - bitmapMetadata_->padding_left, y - bitmapMetadata_->padding_top)))
		return gmpi::MP_OK;

	return gmpi::MP_UNHANDLED;
}

int32_t JoystickImageGui::onPointerDown(int32_t flags, GmpiDrawing_API::MP1_POINT point)
{
	// Let host handle right-clicks.
	if ((flags & GG_POINTER_FLAG_FIRSTBUTTON) == 0)
	{
		return gmpi::MP_OK; // Indicate successful hit, so right-click menu can show.
	}
	//	_RPT2(_CRT_WARN, "onPointerDown (%f,%f)\n", point.x, point.y);

	m_ptPrev = point;	// note first point.

	//	smallDragSuppression = true;

	if( !bitmap_.isNull() )
	{
		setCapture();
		pinMouseDown = true;
		pinMouseDownLegacy = true;
		pinMouseDown2 = true;

		if (pinJumpToMouse)
		{
			auto r = getRect();

			float border_x = (bitmapMetadata_->padding_left + bitmapMetadata_->frameSize.width + bitmapMetadata_->padding_right) / 2.0f;
			float border_y = (bitmapMetadata_->padding_top + bitmapMetadata_->frameSize.height + bitmapMetadata_->padding_bottom) / 2.0f;

			float current_x = border_x + pinPositionX * (r.getWidth() - border_x * 2.0f);
			float current_y = border_y + (1.0f - pinPositionY) * (r.getHeight() - border_y * 2.0f);

			// did we click away from joystick knob?
			if (fabs(point.x - current_x) > border_x * 0.5f || fabs(point.y - current_y) > border_x * 0.5f)
			{
				float new_pos_x = (point.x - border_x) / (r.getWidth() - border_x * 2.0f);
				float new_pos_y = 1.0f - (point.y - border_y) / (r.getHeight() - border_y * 2.0f);

				new_pos_x = (std::min)(1.0f, (std::max)(0.0f, new_pos_x));
				new_pos_y = (std::min)(1.0f, (std::max)(0.0f, new_pos_y));

				pinPositionX = new_pos_x;
				pinPositionY = new_pos_y;

				reDraw();
			}
		}
	}

	return gmpi::MP_OK;
}

int32_t JoystickImageGui::onPointerMove(int32_t flags, GmpiDrawing_API::MP1_POINT point)
{
	//	_RPT2(_CRT_WARN, "onPointerMove (%f,%f)\n", point.x, point.y);

	if( !getCapture() )
	{
		return gmpi::MP_UNHANDLED;
	}

	Point offset(point.x - m_ptPrev.x, point.y - m_ptPrev.y); // TODO overload subtraction.

	float coarseness = 0.005f;

//	if( modifier_keys::isHeldCtrl() ) //GetKeyState(VK_CONTROL) < 0 ) // cntrl keyu magnifies
	if (flags & gmpi_gui_api::GG_POINTER_KEY_CONTROL) // <cntr> key magnifies
		coarseness = 0.001f;

	float new_pos_x = pinPositionX + coarseness * (float)offset.x;
	float new_pos_y = pinPositionY - coarseness * (float)offset.y; // Y Inverted

	if( new_pos_x < 0.f )
		new_pos_x = 0.f;

	if( new_pos_x > 1.f )
		new_pos_x = 1.f;

	if( new_pos_y < 0.f )
		new_pos_y = 0.f;

	if( new_pos_y > 1.f )
		new_pos_y = 1.f;

	pinPositionX = new_pos_x;
	pinPositionY = new_pos_y;

	m_ptPrev = point;

	reDraw();

	return gmpi::MP_OK;
}

