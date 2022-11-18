#ifndef FLOATFORMULA_H_INCLUDED
#define FLOATFORMULA_H_INCLUDED

#include "../se_sdk3/mp_sdk_audio.h"
#include "../shared/expression_evaluate.h"

class FloatFormula : public MpBase
{
public:
	FloatFormula( IMpUnknown* host );
	virtual void onSetPins();

private:
	FloatInPin pinA;
	FloatOutPin pinB;
	StringInPin pinFormulaB;

private:
	Evaluator ee;
	std::string formulaB_ascii;
};

#endif

