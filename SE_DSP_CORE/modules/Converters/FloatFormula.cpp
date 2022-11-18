#include <math.h>
#include <cmath>
#include "./FloatFormula.h"
#include "./my_type_convert.h"

SE_DECLARE_INIT_STATIC_FILE(FloatFormula);

REGISTER_PLUGIN ( FloatFormula, L"SE Float Function DSP" );

using namespace std;

FloatFormula::FloatFormula( IMpUnknown* host ) : MpBase( host )
{
	// Register pins.
	initializePin( 0, pinA );
	initializePin( 1, pinB );
	initializePin( 2, pinFormulaB );
}

void FloatFormula::onSetPins()
{
	if( pinFormulaB.isUpdated() )
	{
		formulaB_ascii = UnicodeToAscii( pinFormulaB );
		if( formulaB_ascii.empty() )
		{
			formulaB_ascii = "0";
		}
	}

	double B;
	double A = pinA;
	int flags = 0;
	ee.SetValue( "a", &A );
	ee.Evaluate( formulaB_ascii.c_str(), &B, &flags );

	if( !std::isfinite(B) )
	{
		pinB = 0.0f;
	}

	pinB = (float) B;
}

