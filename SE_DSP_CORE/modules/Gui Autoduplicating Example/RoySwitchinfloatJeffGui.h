#include "mp_sdk_gui2.h"
#include <vector>
#include <algorithm>

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _SEM_SWITCHIN_GUI_H_
#define _SEM_SWITCHIN_GUI_H_

class SemSwitchInGui : public MpGuiBase
{
public:
	SemSwitchInGui(IMpUnknown* host)
		: MpGuiBase(host)
		, _autoPinCount(0)
		, pinValuesIn(0)
	{
		initializePin(0, pinChoice, static_cast<MpGuiBaseMemberPtr>( &SemSwitchInGui::onChoiceChanged ));
		initializePin(1, pinValueOut, static_cast<MpGuiBaseMemberPtr>( &SemSwitchInGui::onValueOutChanged ));
	}
	//--------------------------------------------------------------
	~SemSwitchInGui(void) {
		for( auto pin : pinValuesIn )
		{
			delete pin;
		}
	}
	//--------------------------------------------------------------
	virtual int32_t MP_STDCALL initialize(void) {
		onChoiceChanged();
		return gmpi::MP_OK;
	}
	//--------------------------------------------------------------
	void onChoiceChanged(void) {
		const int choice = ( std::min )( ( std::max )( 0, pinChoice.getValue() ), _autoPinCount - 1 );

		if( _autoPinCount > 0 ) {
			pinValueOut = *pinValuesIn[choice];
		}
	}
	//--------------------------------------------------------------
	void onValueOutChanged(void) {
		const int choice = ( std::min )( ( std::max )( 0, pinChoice.getValue() ), _autoPinCount - 1 );

		if( _autoPinCount > 0 ) {
			*pinValuesIn[choice] = pinValueOut;
		}
	}
	//--------------------------------------------------------------
	virtual int32_t MP_STDCALL setPin(int32_t pinId, int32_t voice, int32_t size, void* data) {

		int plugIndex = pinId - _fixedPinCount; // Calc index of autoduplicating pin.
		// Add autoduplicate pins as needed.
		while( (int) pinValuesIn.size() < plugIndex + 1 )
		{
			const int pinId = pinValuesIn.size() + _fixedPinCount;
			pinValuesIn.push_back(new FloatGuiPin());
			initializePin(pinId, *pinValuesIn.back() );
			_autoPinCount = pinValuesIn.size();
		}

		int32_t result = MpGuiBase::setPin(pinId, voice, size, data);

		if( _autoPinCount > 0 && pinId >= _fixedPinCount ) {
			const int choice = ( std::min )( ( std::max )( 0, pinChoice.getValue() ), _autoPinCount - 1 );
			const int currentPinId = choice + _fixedPinCount;

			if( pinId == currentPinId ) {
				pinValueOut = *pinValuesIn[choice];
			}
		}

		return result;
	}
	//--------------------------------------------------------------
private:
	IntGuiPin           pinChoice;
	FloatGuiPin  pinValueOut;
	std::vector< FloatGuiPin *> pinValuesIn;
	int                 _autoPinCount;
	static const int    _fixedPinCount = 2;
};
//==================================================================
#endif // _SEM_SWITCHIN_GUI_H_