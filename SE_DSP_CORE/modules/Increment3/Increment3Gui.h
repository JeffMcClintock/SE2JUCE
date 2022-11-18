#ifndef INCREMENT3GUI_H_INCLUDED
#define INCREMENT3GUI_H_INCLUDED

#include "mp_sdk_gui.h"

class Increment3Gui : public MpGuiBase
{
public:
	Increment3Gui(IMpUnknown* host);

private:
 	void onSetIncrement();
 	void onSetDecrement();
	void nextValue( int direction );

 	IntGuiPin choice;
	StringGuiPin itemList;
 	BoolGuiPin increment;
 	BoolGuiPin decrement;
 	BoolGuiPin wrap;

    // prevent initial 'false' value from triggering inc/dec.
    bool prev_increment = {};
    bool prev_decrement = {};

    int valueOnMouseDown = {};
};

#endif


