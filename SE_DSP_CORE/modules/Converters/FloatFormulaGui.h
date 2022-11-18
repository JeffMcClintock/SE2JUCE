#ifndef FLOATFORMULAGUI_H_INCLUDED
#define FLOATFORMULAGUI_H_INCLUDED

#include "mp_sdk_gui.h"
#include "../shared/expression_evaluate.h"
#include <cmath>

class FloatFormulaGui : public MpGuiBase
{
public:
	FloatFormulaGui( IMpUnknown* host );

private:
	void onSetValueIn();
	void onSetValueOut();
	void onSetFormulaA();
	void onSetFormulaB();

	FloatGuiPin valueIn;
	FloatGuiPin valueOut;
	StringGuiPin FormulaA;
	StringGuiPin FormulaB;

private:
	Evaluator ee;

	std::string formulaA_ascii;
	std::string formulaB_ascii;

	float currentValue = 0.0f;
	bool currentValueNull = true;
};

#endif


