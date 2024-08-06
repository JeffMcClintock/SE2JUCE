#include ".\SubpatchFloat.h"

REGISTER_PLUGIN ( SubpatchFloat, L"SE Sub-Patch Float" );

SubpatchFloat::SubpatchFloat( IMpUnknown* host ) : MpBase( host )
{
	// Register pins.
	initializePin( 0, pinValueIn );
	initializePin( 1, pinSubPatch );
	initializePin( 2, pinValueOut );
}

void SubpatchFloat::onSetPins(void)
{
	// Check which pins are updated.
	if( pinValueIn.isUpdated() || pinSubPatch.isUpdated() )
	{
		if( pinValueIn.rawSize() == sizeof(float) * PatchCount_ && pinSubPatch >= 0 && pinSubPatch < PatchCount_)
		{
			float* patchData = (float*) pinValueIn.rawData();
			pinValueOut = patchData[ pinSubPatch ];
		}
	}

	// Set processing method.
	//SET_PROCESS(&SubpatchFloat::subProcess);

	// Set sleep mode (optional).
	// setSleep(false);
}

