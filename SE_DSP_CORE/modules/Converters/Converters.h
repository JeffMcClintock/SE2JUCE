#ifndef GUICONVERTERSGUI_H_INCLUDED
#define GUICONVERTERSGUI_H_INCLUDED

#include "../se_sdk3/mp_sdk_audio.h"
#include "./my_type_convert.h"

template<typename T1, typename T2>
class SimpleConverter : public MpBase
{
public:
	SimpleConverter(IMpUnknown* host) : MpBase(host)
	{
		initializePin( 0, inputValue );
		initializePin( 1, outputValue );
	}

	void onSetPins()
	{
		if( inputValue.isUpdated() )
		{
			outputValue = myTypeConvert<T1,T2>(inputValue);
		}
		/* ??
		if( outputValue.isUpdated() )
		{
			inputValue = myTypeConvert<T2,T1>(outputValue);
		}
		*/

		// now automatic. setSleep(true);
	}

private:
 	MpControlPin<T1, gmpi::MP_IN> inputValue;
 	MpControlPin<T2, gmpi::MP_OUT> outputValue;
};

#endif


