#pragma once

#include "mp_sdk_gui.h"

class ImageToFrameGui :
	public MpGuiBase
{
public:
	ImageToFrameGui(IMpUnknown* host);
	static IMpUserInterface* create(IMpUnknown* host){ return new ImageToFrameGui(host); };

	FloatGuiPin animationPosition;
	IntGuiPin frameCount;
	IntGuiPin frameNumber;
private:
	void OnSetAnimationPosition();
	void OnSetFrameNumber();
};
