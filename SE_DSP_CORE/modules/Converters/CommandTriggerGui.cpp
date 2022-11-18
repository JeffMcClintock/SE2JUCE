#include "./CommandTriggerGui.h"

REGISTER_GUI_PLUGIN( CommandTriggerGui, L"SE Command Trigger" );

static const int nonRepeatingPinCount = 1;

CommandTriggerGui::CommandTriggerGui( IMpUnknown* host ) : MpGuiBase(host)
,outputPinCount_(0)
{
	// initialise pins
	value.initialize( this, 0 );
}

int32_t CommandTriggerGui::initialize()
{
	// figure out how many autoduplicating pins.
	getHost()->getPinCount( outputPinCount_ );

	previousValues.resize( outputPinCount_);

	outputPinCount_ = outputPinCount_ - nonRepeatingPinCount;

	// always update all output pins. 
	for( int i = nonRepeatingPinCount ; i <= outputPinCount_ ; ++i )
	{
		bool boolPinValue = false;
		getHost()->pinTransmit( i, sizeof(boolPinValue), &boolPinValue );

		previousValues[i] = false;
	}

	return MpGuiBase::initialize( );
}

int32_t CommandTriggerGui::setPin( int32_t pinId, int32_t voice, int32_t size, void* data )
{
	int32_t r = MpGuiBase::setPin( pinId, voice, size, data );

	if( pinId >= nonRepeatingPinCount )
	{
		bool boolPinValue;
		VariableFromRaw<bool>( size, data, boolPinValue );

		if( boolPinValue == false && previousValues[pinId] == true )
		{
			value = pinId;
			// reset 'value'.
			value = 0;
		}

		previousValues[pinId] = boolPinValue;
	}

	return r;
}

