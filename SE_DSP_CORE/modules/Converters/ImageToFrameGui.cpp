#include "ImageToFrameGui.h"

REGISTER_GUI_PLUGIN( ImageToFrameGui, L"SynthEdit Image to Frame" );
REGISTER_GUI_PLUGIN( ImageToFrameGui, L"SynthEdit Image to Frame (old)" );

ImageToFrameGui::ImageToFrameGui(IMpUnknown* host) : MpGuiBase(host)
{
	animationPosition.initialize( this, 0, static_cast<MpGuiBaseMemberPtr>(&ImageToFrameGui::OnSetAnimationPosition));
	frameCount.initialize( this, 1, static_cast<MpGuiBaseMemberPtr>(&ImageToFrameGui::OnSetFrameNumber) );
	frameNumber.initialize( this, 2, static_cast<MpGuiBaseMemberPtr>(&ImageToFrameGui::OnSetFrameNumber));
}

void ImageToFrameGui::OnSetAnimationPosition()
{
	float next_idx = 0.5f + animationPosition * ( frameCount -1.0f );
	frameNumber = (int) next_idx;
}

void ImageToFrameGui::OnSetFrameNumber()
{
	int last_frame = frameCount - 1;
	if( last_frame < 1 ) // prevent divide-by-zero
		last_frame = 1;

//	_RPT3(_CRT_WARN, "%f/ ( %f - 1) = %f\n", (float) m_frame_number.getvalue(), (float) last_frame,(float) m_frame_number.getvalue() / ( (float) last_frame ) );
	float next_val = (float) frameNumber / ( (float) last_frame );
	if( next_val < 0.f )
		next_val = 0.f;
	else
	{
		if( next_val > 1.f )
			next_val = 1.f;
	}

	animationPosition = next_val;
}
