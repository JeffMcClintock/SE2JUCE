#include ".\SubpatchFloatGui.h"

REGISTER_GUI_PLUGIN( SubpatchFloatGui, L"SE Sub-Patch Float" );

SubpatchFloatGui::SubpatchFloatGui( IMpUnknown* host ) : MpGuiBase(host)
{
	// Values coming from host. (hidden pins).
	pinValueIn.initialize( this, 0, static_cast<MpGuiBaseMemberPtr>(&SubpatchFloatGui::onSetValueIn) );
	pinNameIn.initialize( this, 1, static_cast<MpGuiBaseMemberPtr>(&SubpatchFloatGui::onSetNameIn) );

	// Visible input pins.
	pinMinimum.initialize( this, 2, static_cast<MpGuiBaseMemberPtr>(&SubpatchFloatGui::onSetValueIn) );
	pinMaximum.initialize( this, 3, static_cast<MpGuiBaseMemberPtr>(&SubpatchFloatGui::onSetValueIn) );
	pinSubPatch.initialize( this, 4, static_cast<MpGuiBaseMemberPtr>(&SubpatchFloatGui::onSetValueIn) );

	// Visible output pins.
	pinName.initialize( this, 5, static_cast<MpGuiBaseMemberPtr>(&SubpatchFloatGui::onSetName) );
	pinAnimationPos.initialize( this, 6, static_cast<MpGuiBaseMemberPtr>(&SubpatchFloatGui::onSetAnimationPos) );
	pinValue.initialize( this, 7, static_cast<MpGuiBaseMemberPtr>(&SubpatchFloatGui::onSetValue) );
}

// Patch value or Patch-Number Changed, copy value to GUI control.
void SubpatchFloatGui::onSetValueIn()
{
	// Copy individual value from blob array to output.
	int patch = pinSubPatch;
	if( pinValueIn.rawSize() == sizeof(float) * PatchCount_ && patch >= 0 && patch < PatchCount_)
	{
		float* patchData = (float*) pinValueIn.rawData();
		pinValue = patchData[patch];

		// Compute animation position.
		float range = (pinMaximum - pinMinimum);
		if( range == 0.0f ) // prevent divide-by-zero.
		{
			pinAnimationPos = 0.0f;
		}
		else
		{
			pinAnimationPos = ( pinValue - pinMinimum ) / range;
		}
	}
}

void SubpatchFloatGui::onSetAnimationPos()
{
	// Compute value.
	float range = (pinMaximum - pinMinimum);
	pinValue = pinAnimationPos * range + pinMinimum;
	onSetValue();
}

// User changed control, send it to Patch-Memeory.
void SubpatchFloatGui::onSetValue()
{
	float temp[PatchCount_];

	if( pinValueIn.rawSize() != sizeof(float) * PatchCount_ )
	{
		memset( temp, 0, sizeof(temp) );
	}
	else
	{
		memcpy( temp, pinValueIn.rawData(), pinValueIn.rawSize() );
	}

	int patch = pinSubPatch;
	if( patch >= 0 && patch < PatchCount_ )
	{
		temp[patch] = pinValue;

		MpBlob tempBlob(sizeof(temp), temp);
		pinValueIn = tempBlob;
	}
}

void SubpatchFloatGui::onSetNameIn()
{
	pinName = pinNameIn;
}

void SubpatchFloatGui::onSetName()
{
	pinNameIn = pinName;
}
