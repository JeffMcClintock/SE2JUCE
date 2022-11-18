#include "./StepSequencer.h"
#include <algorithm>

using namespace std;

REGISTER_PLUGIN ( StepSequencer, L"SE Step Sequencer" );

/* LIMITATION: Only sample-and-holds each input.  If input changes mid-step, it's ignored
*/

StepSequencer::StepSequencer( IMpUnknown* host ) : MpBase( host )
 ,gate_state(false)
 ,reset_state(false)
 ,step(15) // start on last step, so first pulse resets to step 0
{
	// Register pins.
	initializePin( 0, pingate );
	initializePin( 1, pinreset );
	initializePin( 2, pinsteps );
	initializePin( 3, pinout );

	for( int i = 0 ; i < maximumSteps ; ++i )
	{
		initializePin( 4 + i, InputPins[i] );
	}

	initializePin( 20, pinresetto );
}

int PinToInt( float v )
{
	return (int) (0.5f + v * 10.0f);
}

void StepSequencer::subProcess( int bufferOffset, int sampleFrames )
{
	// get pointers to in/output buffers.
	float* gate		= bufferOffset + pingate.getBuffer();
	float* reset	= bufferOffset + pinreset.getBuffer();
	float* out		= bufferOffset + pinout.getBuffer();

	float currentInputValue = InputPins[step].getValue( bufferOffset );

	for( int s = sampleFrames; s > 0; --s )
	{
		if( (*reset > 0.f ) != reset_state )
		{
			reset_state = (*reset > 0.f );
			if( reset_state )
			{
				int blockPosition = bufferOffset + sampleFrames - s;
				//step = ( PinToInt( pinresetto.getValue( bufferOffset + sampleFrames - s ) ) - 2 ) & 0xf;
				step = (int) ( pinresetto.getValue( blockPosition ) * 10.0f) - 2;
				step = min( step, 14 );
				step = max( step, -1 );
				step &= 0x0f;
				currentInputValue = InputPins[step].getValue( blockPosition );
				pinout.setStreaming( false, blockPosition );
			}
		}
		if( (*gate > 0.f ) != gate_state )
		{
			gate_state = (*gate > 0.f );
			if( gate_state )
			{
				int blockPosition = bufferOffset + sampleFrames - s;
				step = step + 1;
				int steps = PinToInt( pinsteps.getValue( blockPosition ) );
				if( step >= steps || step >= 16)
				{
					step = 0;
				}
				currentInputValue = InputPins[step].getValue( blockPosition );
				pinout.setStreaming( false, blockPosition );
			}
		}

		*out++ = currentInputValue;
		++gate;
		++reset;
	}
}

void StepSequencer::onSetPins(void)
{
	// Check which pins are updated.
	if( pingate.isStreaming() )
	{
	}
	if( pinreset.isStreaming() )
	{
	}
	if( pinsteps.isStreaming() )
	{
	}
	if( pinresetto.isStreaming() )
	{
	}

	// Set state of output audio pins.
	pinout.setStreaming(true);

	// Set processing method.
	SET_PROCESS(&StepSequencer::subProcess);

	// Set sleep mode (optional).
	// setSleep(false);
}

