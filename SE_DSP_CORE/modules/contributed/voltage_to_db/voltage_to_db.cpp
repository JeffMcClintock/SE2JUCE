#include "./voltage_to_db.h"
//by Xhun Audio
#include <math.h>

REGISTER_PLUGIN ( voltage_to_db, L"voltage_to_db" );

voltage_to_db::voltage_to_db( IMpUnknown* host ) : MpBase( host )
{
	// Register pins.
	initializePin( 0, pinVoltsin );
	initializePin( 1, pinVref );
	initializePin( 2, pindBout );
}

void voltage_to_db::subProcess( int bufferOffset, int sampleFrames )
{
	// get pointers to in/output buffers.
	float* voltsin	= bufferOffset + pinVoltsin.getBuffer();
	float* vrefin	= bufferOffset + pinVref.getBuffer();
	float* dBout	= bufferOffset + pindBout.getBuffer();

	for( int s = sampleFrames; s > 0; --s )
	{
		// TODO: Signal processing goes here.

		float Volts = *voltsin;	// get the sample 'POINTED TO' by input.
		float Vref  = *vrefin;
		float dB = 0.0f;
		
		// dB = 20*log10(Volts/Vref)
		
		if ( Volts > 0.0f )
		{
		// Conversion dB = 20*log10(Volts/Vref)
		dB = (20.0f*log10(Volts/Vref))/10.0f;
		}
		if ( Volts == 0.0f )
		{
		dB = -15.0f;
		}
		if ( Volts < 0.0f )
		{
		dB = 0.0f;
		}
		
		// store the result in the output buffer.
		*dBout = dB;
		
		// Increment buffer pointers.
		++voltsin;
		++vrefin;
		++dBout;
	}
}

void voltage_to_db::onSetPins(void)
{
	bool OutputIsActive = false;
	
	// Check which pins are updated.
	if( pinVoltsin.isStreaming() )
	{
	OutputIsActive = true;
	}
	/*
	if( pinVref.isStreaming() )
	{
	}
	*/

	// Set state of output audio pins.
	pindBout.setStreaming(OutputIsActive);

	// Set processing method.
	SET_PROCESS(&voltage_to_db::subProcess);

	// Set sleep mode (optional).
	// setSleep(false);
	setSleep( !OutputIsActive );
}

