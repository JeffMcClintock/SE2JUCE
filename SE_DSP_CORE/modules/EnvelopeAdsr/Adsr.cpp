#include "./Adsr.h"

REGISTER_PLUGIN( Adsr, L"SynthEdit ADSR" );
SE_DECLARE_INIT_STATIC_FILE(ADSR)

Adsr::Adsr( IMpUnknown* host ) : Envelope( host )
{
	// Associate each pin object with it's ID in the XML file.
	initializePin( 0, pinTrigger );
	initializePin( 1, pinGate );
	initializePin( 2, pinAttack );
	initializePin( 3, pinDecay );
	initializePin( 4, pinSustain );
	initializePin( 5, pinRelease );
	initializePin( 6, pinOverallLevel );
	initializePin( 7, pinSignalOut );
	initializePin( 8, pinVoiceReset );

	fixed_levels[0] = 1.0f;
	fixed_levels[2] = 0.0f;

	sustain_segment	= 1;
	end_segment		= 2;
	num_segments	= 3;

	rate_plugs[0] = &pinAttack;
	rate_plugs[1] = &pinDecay;
	rate_plugs[2] = &pinRelease;

	level_plugs[0] = 0;
	level_plugs[1] = &pinSustain;
	level_plugs[2] = level_plugs[3] = level_plugs[4] = level_plugs[5] = level_plugs[6] = level_plugs[7] = 0;
}

