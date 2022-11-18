#include "./DbToAnimationGui.h"

REGISTER_GUI_PLUGIN( DbToAnimationGui, L"SE dBToAnimation" );

DbToAnimationGui::DbToAnimationGui( IMpUnknown* host ) : MpGuiBase(host)
{
	// initialise pins.
	animationPosition.initialize( this, 0, static_cast<MpGuiBaseMemberPtr>(&DbToAnimationGui::onSetAnimationPosition) );
	db.initialize( this, 1, static_cast<MpGuiBaseMemberPtr>(&DbToAnimationGui::onSetDb) );
}

// handle pin updates.
void DbToAnimationGui::onSetAnimationPosition()
{
	// animationPosition changed. Not handled. This is a one-way conversion.
}

void DbToAnimationGui::onSetDb()
{
	float normalised;

	if( db >= -3.f )
	{
		normalised = (db + 3.f) * 0.1f + 0.4f;
	}
	else
	{
		if( db >= -8.75f )
		{
			normalised = (db+7.f) * 0.05f + 0.2f;
		}
		else
		{
			normalised = (db+20.f) * 0.01f;
		}
	}

	if( ! (normalised >= 0.f ) ) // negative and NaNs
		normalised = 0.f;

	if( normalised > 1.f )
		normalised = 1.f;

	animationPosition = normalised;
}

