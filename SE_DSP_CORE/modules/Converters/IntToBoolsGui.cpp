#include "./IntToBoolsGui.h"
#include <algorithm>

using namespace gmpi;
using namespace gmpi_gui;

REGISTER_GUI_PLUGIN( IntToBoolsGui, L"SE Int to Bools" );
REGISTER_GUI_PLUGIN( IntToBoolsGui, L"SE Bools to Int" ); // old obsolete version.
GMPI_REGISTER_GUI(MP_SUB_TYPE_GUI2, BoolsToIntGui, L"SE Bools To Int2");

static const int nonRepeatingPinCount = 1;

IntToBoolsGui::IntToBoolsGui( IMpUnknown* host ) : MpGuiBase(host)
,currentOutputpinId_( -1 )
,outputPinCount_(0)
{
	// initialise pins
	value.initialize( this, 0, static_cast<MpGuiBaseMemberPtr>(&IntToBoolsGui::onSetValue) );
}

int32_t IntToBoolsGui::initialize()
{
	// figure out how many autoduplicating pins.
	getHost()->getPinCount( outputPinCount_ );
	outputPinCount_ = outputPinCount_ - nonRepeatingPinCount;

	currentOutputpinId_ = value + nonRepeatingPinCount;

	// always update all output pins. 
	for( int i = nonRepeatingPinCount ; i <= outputPinCount_ ; ++i )
	{
		bool boolPinValue = ( i == currentOutputpinId_ );
		getHost()->pinTransmit( i, sizeof(boolPinValue), &boolPinValue );
	}

	return gmpi::MP_OK;
}

// handle pin updates
void IntToBoolsGui::onSetValue()
{
	int v = value;
	int newOutputpinId_ = v + nonRepeatingPinCount;

	if( newOutputpinId_ != currentOutputpinId_ ) // saves toggling it for no reason at startup.
	{
		bool boolPinValue = false;
		if( currentOutputpinId_ > 0 && currentOutputpinId_ <= outputPinCount_ )
		{
			getHost()->pinTransmit( currentOutputpinId_, sizeof(boolPinValue), &boolPinValue );
		}

		currentOutputpinId_ = newOutputpinId_;
		if( currentOutputpinId_ > 0 && currentOutputpinId_ <= outputPinCount_ )
		{
			boolPinValue = true;
			getHost()->pinTransmit( currentOutputpinId_, sizeof(boolPinValue), &boolPinValue );
		}
	}
}

int32_t IntToBoolsGui::setPin( int32_t pinId, int32_t voice, int32_t size, void* data )
{
	int32_t r = MpGuiBase::setPin( pinId, voice, size, data );

	if( pinId >= nonRepeatingPinCount )
	{
		bool boolPinValue;
		VariableFromRaw<bool>( size, data, boolPinValue );

		if( boolPinValue )
		{
			value = pinId - nonRepeatingPinCount;
		}
	}

	return r;
}

BoolsToIntGui::BoolsToIntGui() :
numInputPins(0)
{
	initializePin(pinIntVal, static_cast<MpGuiBaseMemberPtr2>( &BoolsToIntGui::onSetIntVal ));
}

int32_t BoolsToIntGui::setPin(int32_t pinId, int32_t voice, int32_t size, const void* data)
{
	numInputPins = (std::max)( numInputPins, pinId );

	auto inPinIdx = pinId - firstInputIdx;
	if( inPinIdx >= 0 )
	{
		bool value = *((bool*)data);
		if( value )
		{
			pinIntVal = inPinIdx;
		}
	}

	return SeGuiInvisibleBase::setPin(pinId, voice, size, data);
}

void BoolsToIntGui::onSetIntVal()
{
	for( int i = 0; i < numInputPins; ++i )
	{
		bool value = i == pinIntVal;
		getHost()->pinTransmit(i + firstInputIdx, sizeof(value), &value);
	}
}

