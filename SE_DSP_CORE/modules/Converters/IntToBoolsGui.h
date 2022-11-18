#ifndef BOOLSTOINTGUI_H_INCLUDED
#define BOOLSTOINTGUI_H_INCLUDED

#include "mp_sdk_gui.h"
#include "../se_sdk3/mp_sdk_gui2.h"

class IntToBoolsGui : public MpGuiBase
{
public:
	IntToBoolsGui( IMpUnknown* host );

	virtual int32_t MP_STDCALL setPin( int32_t pinId, int32_t voice, int32_t size, void* data ) override;
	virtual int32_t MP_STDCALL initialize() override;

protected:
	int nonRepeatingPinCount_;

private:
 	void onSetValue();

	IntGuiPin value;

	int currentOutputpinId_;
	int outputPinCount_;
};

class BoolsToIntGui : public SeGuiInvisibleBase
{
public:
	BoolsToIntGui();

	virtual int32_t MP_STDCALL setPin(int32_t pinId, int32_t voice, int32_t size, const void* data) override;

private:
	void onSetIntVal();
	IntGuiPin pinIntVal;

	int numInputPins;
	const int firstInputIdx = 1;
};

#endif


