#include "./FloatLimiterGui.h"

REGISTER_GUI_PLUGIN( FloatLimiterGui, L"SE Float Limiter" );
SE_DECLARE_INIT_STATIC_FILE(FloatLimiter_Gui);

FloatLimiterGui::FloatLimiterGui( IMpUnknown* host ) : MpGuiBase(host)
{
	// initialise pins.
	min.initialize( this, 0, static_cast<MpGuiBaseMemberPtr>(&FloatLimiterGui::onSetValue) );
	max.initialize( this, 1, static_cast<MpGuiBaseMemberPtr>(&FloatLimiterGui::onSetValue) );
	value.initialize( this, 2, static_cast<MpGuiBaseMemberPtr>(&FloatLimiterGui::onSetValue) );

	SetTimerIntervalMs( 20 );
}

void FloatLimiterGui::onSetValue()
{
	if( max > min && (value > max || value < min) )
	{
		StartTimer();
	}
}

bool FloatLimiterGui::OnTimer()
{
	if( value > max )
	{
		value = max;
	}

	if( value < min )
	{
		value = min;
	}

	return false; // disable timer.
}
