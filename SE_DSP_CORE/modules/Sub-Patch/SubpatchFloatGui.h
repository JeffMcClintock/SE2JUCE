#ifndef SUBPATCHFLOATGUI_H_INCLUDED
#define SUBPATCHFLOATGUI_H_INCLUDED

#include "MP_SDK_GUI.h"

class SubpatchFloatGui : public MpGuiBase
{
public:
	SubpatchFloatGui( IMpUnknown* host );

private:
	void onSetValueIn();
	void onSetNameIn();
	void onSetName();
	void onSetValue();
	void onSetAnimationPos();
	void onSetMaximum();
	void onSetMinimum();

	BlobGuiPin pinValueIn;
	IntGuiPin pinSubPatch;
	StringGuiPin pinNameIn;
	StringGuiPin pinName;
	FloatGuiPin pinValue;
	FloatGuiPin pinMinimum;
	FloatGuiPin pinMaximum;
	FloatGuiPin pinAnimationPos;

	static const int PatchCount_ = 128;
};

#endif


