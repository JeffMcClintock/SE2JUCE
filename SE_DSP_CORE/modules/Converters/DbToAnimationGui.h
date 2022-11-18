#ifndef DBTOANIMATIONGUI_H_INCLUDED
#define DBTOANIMATIONGUI_H_INCLUDED

#include "mp_sdk_gui.h"

class DbToAnimationGui : public MpGuiBase
{
public:
	DbToAnimationGui( IMpUnknown* host );

	// overrides

private:
 	void onSetAnimationPosition();
 	void onSetDb();
 	FloatGuiPin animationPosition;
 	FloatGuiPin db;
};

#endif


