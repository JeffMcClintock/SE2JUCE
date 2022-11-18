#include ".\db_to_voltage.h"
//by Xhun Audio
#include <math.h>

REGISTER_PLUGIN ( db_to_voltage, L"db_to_voltage" );

db_to_voltage::db_to_voltage( IMpUnknown* host ) : MpBase( host )
{
	// Register pins.
	initializePin( 0, pindBin );
	initializePin( 1, pinVref );
	initializePin( 2, pinVoltsout );
}

void db_to_voltage::subProcess( int bufferOffset, int sampleFrames )
{
	// get pointers to in/output buffers.
	float* dBin	= bufferOffset + pindBin.getBuffer();
	float* vrefin	= bufferOffset + pinVref.getBuffer();
	float* voltsout	= bufferOffset + pinVoltsout.getBuffer();

	for( int s = sampleFrames; s > 0; --s )
	{
		// TODO: Signal processing goes here.
		
		float dB    = *dBin;	// get the sample 'POINTED TO' by input.
		float Vref  = *vrefin;

		// Volts=Vref*pow(10,(dB/20))
		
		// pow(float,float)
		//float base = 10.0f;
		//float exponent = (dB*10.0f)/20.0f;
		//float Volts=Vref*pow(base,exponent);
		
		// Conversion Volts=Vref*pow(10,(dB/20))
		float Volts=Vref*pow(10.0f,(dB*10.0f)/20.0f);
		
		// store the result in the output buffer.
		*voltsout = Volts;

		// Increment buffer pointers.
		++dBin;
		++vrefin;
		++voltsout;
	}
}

void db_to_voltage::onSetPins(void)
{
	bool OutputIsActive = false;
	
	// Check which pins are updated.
	if( pindBin.isStreaming() )
	{
	OutputIsActive = true;
	}
	/*
	if( pinVref.isStreaming() )
	{
	}
	*/
	
	// Set state of output audio pins.
	pinVoltsout.setStreaming(OutputIsActive);

	// Set processing method.
	SET_PROCESS(&db_to_voltage::subProcess);

	// Set sleep mode (optional).
	// setSleep(false);
	setSleep( !OutputIsActive );
}

