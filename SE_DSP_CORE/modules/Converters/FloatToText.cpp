// prevent MS CPP - 'swprintf' was declared deprecated warning
#if defined(_MSC_VER)
  #define _CRT_SECURE_NO_DEPRECATE 
  #pragma warning(disable : 4996)
#endif

#include <math.h>
#include "FloatToText.h"

REGISTER_GUI_PLUGIN( FloatToTextGui, L"SE FloatToText GUI" );
SE_DECLARE_INIT_STATIC_FILE(FloatToTextGUI_Gui);

FloatToTextGui::FloatToTextGui( IMpUnknown* host ) : MpGuiBase( host )
{
	inputValue.initialize( this, 0, static_cast<MpGuiBaseMemberPtr>( &FloatToTextGui::onInputChanged ) );
	decimalPlaces.initialize( this, 1, static_cast<MpGuiBaseMemberPtr>( &FloatToTextGui::onInputChanged ) );
	outputValue.initialize( this, 2, static_cast<MpGuiBaseMemberPtr>( &FloatToTextGui::onOutputChanged ) );
};

void FloatToTextGui::onInputChanged()
{
	int decimals = decimalPlaces;

	if( decimals < 0 ) // -1 : automatic decimal places.
	{
		float absolute = fabsf( inputValue );
		decimals = 2;

		if( absolute < 0.1f )
		{
			if( absolute == 0.0f )
				decimals = 1;
			else
				decimals = 4;
		}
		else
			if( absolute > 10.f )
				decimals = 1;
			else
				if( absolute > 100.f )
					decimals = 0;
	}

	// longest float value is about 40 characters.
	const int maxSize = 50;

	wchar_t formatString[maxSize];

	// Use safe printf if available.
	#if defined(_MSC_VER)
		swprintf_s( formatString, maxSize, L"%%.%df", decimals );
	#else
		swprintf( formatString, maxSize, L"%%.%df", decimals );
	#endif

	wchar_t outputString[maxSize];

	//#if defined(_MSC_VER)
	//	swprintf_s( outputString, maxSize, formatString, (double) (float) inputValue );
	//#else
		swprintf( outputString, maxSize, formatString, (double) (float) inputValue );
//	#endif

	// Replace -0.0 with 0.0 ( same for -0.00 and -0.000 etc).
	// deliberate 'feature' of printf is to round small negative numbers to -0.0
	if( outputString[0] == L'-' && (float) inputValue > -1.0f )
	{
		int i = (int) wcslen(outputString) - 1;
		while( i > 0 )
		{
			if( outputString[i] != L'0' && outputString[i] != L'.' )
			{
				break;
			}
			--i;
		}
		if( i == 0 ) // nothing but zeros (or dot). remove leading minus sign.
		{
			wcscpy(outputString, outputString + 1);
		}
	}


	// due to truncation, output string is now an approximation of input value.
	// retain that appoximate value to compare against changes to input string.
	approximatedOutput_ = myTypeConvert<std::wstring,float>( std::wstring(outputString) );

	outputValue = outputString;
};

void FloatToTextGui::onOutputChanged()
{
	float newValue = myTypeConvert<std::wstring,float>(outputValue);

	if( newValue != approximatedOutput_ )
	{
		inputValue = newValue;
		approximatedOutput_ = newValue;
	}
};
