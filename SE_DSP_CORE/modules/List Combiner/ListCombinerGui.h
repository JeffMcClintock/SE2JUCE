#ifndef LISTCOMBINERGUI_H_INCLUDED
#define LISTCOMBINERGUI_H_INCLUDED

#include "mp_sdk_gui.h"

class ListCombinerGui : public MpGuiBase
{
public:
	ListCombinerGui( IMpUnknown* host );

 	void onSetItemListIn();
 	virtual void onSetChoiceOut();

	IntGuiPin choiceA;
 	StringGuiPin itemListA;
 	IntGuiPin choiceB;
 	StringGuiPin itemListB;
 	IntGuiPin choiceOut;
 	StringGuiPin itemListOut;
 	BoolGuiPin AMomentary;
 	BoolGuiPin BMomentary;
};

class ListCombinerGui2 : public ListCombinerGui
{
public:
    ListCombinerGui2(IMpUnknown* host);
    void onSetChoiceOut() override;
};

#endif


