// prevent MS CPP - 'swprintf' was declared deprecated warning
#define _CRT_SECURE_NO_DEPRECATE 
#pragma warning(disable : 4996)

#include <math.h>
#include "./keyboard.h"

// New. 0v = 0.001s, 10V = 10s
#define VoltageToTime(v) ( powf( 10.0f,((v) * 0.4f ) - 3.0f ) )

REGISTER_PLUGIN(Keyboard, L"SynthEdit Keyboard2");	// GDI Version.
REGISTER_PLUGIN(Keyboard, L"SE Keyboard2");			// GMPI-GUI Version

Keyboard::Keyboard(IMpUnknown* host) : MpBase(host)
,previousGate_(0.0f)
,currentGate_(0.0f)
{
	// Associate each pin object with it's ID in the XML file.
	//_RPT1(_CRT_WARN, "Keyboard constructor %x\n", this );
	int i = 0;
	// Inputs.
	initializePin( i++, pinPitch );
	initializePin( i++, pinTrigger );
	initializePin( i++, pinGate );
	initializePin( i++, pinVelocity );
	initializePin( i++, pinVelocityOff );
	initializePin( i++, pinAftertouch );
//	initializePin( i++, pinUnused ); // Not used. was pinVoiceAllocationMode
	++i;
	initializePin( i++, pinVoiceActive );
	initializePin( i++, Bender );
	
	initializePin( i++, GlideType );
	initializePin( i++, GlideRate );
	initializePin( i++, BenderRange );

	// Outputs.
	initializePin( i++, pinPitchOut );
	initializePin( i++, pinTriggerOut );
	initializePin( i++, pinGateOut );
	initializePin( i++, pinVelocityOut );
	initializePin( i++, pinAftertouchOut );
	initializePin( i++, pinRawPitchOut );
	initializePin( i++, pinRawVelocityOut );
	initializePin( i++, pinRawVelocityOffOut );

	initializePin( i++, pinHoldPedal );

}

int32_t Keyboard::open()
{
	MpBase::open();	// always call the base class

	pinVelocityOut.setCurveType( SmartAudioPin::Curved );

	SET_PROCESS( &Keyboard::subProcess );

	pinTriggerOut.setTransitionTime( getSampleRate() * 0.0005f ); // 0.5 ms trigger pulse.
	benderInterpolator_.setTransitionTime( getSampleRate() * 0.005f ); // 5 ms bender smoothing.

	return gmpi::MP_OK;
}

void Keyboard::onSetPins()  // one or more pins_ updated.  Check pin update flags to determin which ones.
{
//	_RPT1(_CRT_WARN, "Keyboard::onSetPins pinGate=%f\n", (double) pinGate );

	// testing.
	if( pinPitch.isUpdated() )
	{
//		_RPT1(_CRT_WARN, "Keyboard::onSetPins pitch=%f\n", (double) pinPitch );
	}

	//_RPT1(_CRT_WARN, "Keyboard::onSetPins() this=%x\n", this );
	if( GlideRate.isUpdated() || GlideType.isUpdated() )
	{
		if( GlideType == 0 ) // Constant time.
		{
			pitchInterpolator_.setTransitionTime( getSampleRate() * VoltageToTime( 10.0f * GlideRate ) );
		}
	}

	if( pinTrigger.isUpdated() && pinTrigger != 0.0f )
	{
		// currently sends a pulse on first sample, not really needed.

//		_RPT1(_CRT_WARN, "Keyboard::onSetPins Trigger=%f\n", (double) pinTrigger );
		pinTriggerOut.pulse( 10.0f );

		//// assume mono-mode retrigger.
		//VelocitySmoothing_ = true;
	}

	// Voice reset always happens at note on, unless mono mode is on and a voice is already sounding. (not suspended)
	if( pinVoiceActive.isUpdated() )
	{
//		_RPT1(_CRT_WARN, "Keyboard::onSetPins pinVoiceActive=%f\n", (double) pinVoiceActive );
		if( pinVoiceActive > 0.0f )
		{
			// Voice needs clean start, no Velocity Smoothing.
			// assume velocity changes on exact same sample as reset.
			float newVelocity = 0.1f * pinVelocity; // convert to Volts.
			pinVelocityOut.setValueInstant( newVelocity );

			// Gate does notr nesc change on stolen voice when poly=1. Force non-glide.
			previousGate_ = 0;
		}
	}

	if( Bender.isUpdated() || BenderRange.isUpdated() )
	{
//		_RPT1(_CRT_WARN, "Keyboard::onSetPins Bender=%f\n", (double) Bender );
		benderInterpolator_.setTarget( Bender * BenderRange / 12.0f ); // 12 semitones .
	}
	
	// When hold pedal is released, or voice is overlapped by newer voice, lower gate.
	if( pinHoldPedal.isUpdated() || pinVoiceActive.isUpdated())
	{
//		_RPT1(_CRT_WARN, "Keyboard::onSetPins pinHoldPedal=%f\n", (double) pinHoldPedal );
		if(currentGate_ != 0.0f && pinGate == 0.0f && (pinHoldPedal < 5.0f || pinVoiceActive < 1.0f) )
		{
			currentGate_ = 0.0f;
			pinGateOut.setStreaming(false);
		}
	}

	if( pinPitch.isUpdated() )
	{
//		_RPT1(_CRT_WARN, "Keyboard::onSetPins pitch=%f\n", (double) pinPitch );
		pinRawPitchOut = pinPitch;

		if( GlideType == 1 ) // Constant rate.
		{
			// recalculate glide time.
			float deltaPitch = fabsf(10.0f * pitchInterpolator_.getInstantValue() - pinPitch);
			//_RPT1(_CRT_WARN, "Glide Time =%f\n", (double) VoltageToTime( 10.0f * GlideRate ) * deltaPitch );
			pitchInterpolator_.setTransitionTime( getSampleRate() * VoltageToTime( 10.0f * GlideRate ) * deltaPitch );
		}

		pitchInterpolator_.setTarget( 0.1f * pinPitch );
	}

	bool legato = true;

	if( pinGate.isUpdated() )
	{
		if( pinGate != 0.0f )
		{
			currentGate_ = 1.0f;
		}
		else
		{
			if( pinHoldPedal < 5.0f || pinVoiceActive < 1.0f ) // voiceActive of 0.5 indicates overlap voice, don't sustain.
			{
				currentGate_ = 0.0f;
			}
		}

		//_RPT1(_CRT_WARN, "Keyboard::onSetPins gate = %f\n", (double) pinGate );
		pinGateOut.setStreaming(false);

		// gate transition to HI indicates note-on (not legato.
		if( previousGate_ == 0.0f && pinGate != 0.0f )
		{
			legato = false;

			// no portamento when new note not legato. Jump to new pitch.
			pitchInterpolator_.setValueInstant( 0.1f * pinPitch );

			// Send stat update for non-gliding updates.
			if( pitchInterpolator_.isDone() && benderInterpolator_.isDone() )
			{
				pinPitchOut.setStreaming(false);
			}
		}

		previousGate_ = pinGate;
	}

	if( pinVelocity.isUpdated() )
	{
		// no point setting it again if we just did.
		if( !pinVoiceActive.isUpdated() || pinVoiceActive <= 0.0f )
		{
			float newVelocity = 0.1f * pinVelocity; // convert to Volts.

			if( legato ) // glide smoothly to new velocity.
			{
				pinVelocityOut.setTransitionTime( getSampleRate() * 0.008f ); // 8ms
			}
			else // re-play decaying note, move to new velocity as quickly as pos to keep attack snappy.
			{
				pinVelocityOut.setTransitionTime( getSampleRate() * 0.0015f ); // 1.5ms
			}

			pinVelocityOut = newVelocity;
		}

		pinRawVelocityOut = pinVelocity;
	}

	if( pinVelocityOff.isUpdated() )
	{
		pinRawVelocityOffOut = pinVelocityOff;
	}

	if( !pitchInterpolator_.isDone() || !benderInterpolator_.isDone() )
	{
		pinPitchOut.setStreaming(true);
	}

	if( pinAftertouch.isUpdated() )
	{
		pinAftertouchOut.setStreaming(false);
	}
}


void Keyboard::subProcess( int bufferOffset, int sampleFrames )
{
//	_RPT2(_CRT_WARN, "Keyboard::subProcess( bufferOffset=%3d, sampleFrames=%3d )\n", bufferOffset, sampleFrames );

	if( pinPitchOut.isStreaming() && pitchInterpolator_.isDone() && benderInterpolator_.isDone() )
	{
		pinPitchOut.setStreaming( false, bufferOffset );
	}

	float* gate			= bufferOffset + pinGateOut.getBuffer();
	float* aftertouch	= bufferOffset + pinAftertouchOut.getBuffer();
	float* pitch		= bufferOffset + pinPitchOut.getBuffer();

	float currentAftertouch	= 0.1f * pinAftertouch;

	for( int s = sampleFrames ; s > 0 ; s-- )
	{
		*gate++ = currentGate_;
		*aftertouch++ = currentAftertouch;
		*pitch++ = pitchInterpolator_.getNext() + benderInterpolator_.getNext();
	}

	bool canSleep = true;
	pinVelocityOut.subProcess( bufferOffset, sampleFrames, canSleep );
	pinTriggerOut.subProcess( bufferOffset, sampleFrames, canSleep );
}







