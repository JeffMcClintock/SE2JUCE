#include <algorithm>
#include "math.h"
#include "./BpmClock3.h"
#include "../shared/xp_simd.h"
SE_DECLARE_INIT_STATIC_FILE(BpmClock3)

REGISTER_PLUGIN( BpmClock3, L"SE BPM Clock3" );

// #define DEBUG_BPMCLOCK

BpmClock3::BpmClock3( IMpUnknown* host ) : MpBase( host )
,pulseOutVal( 0.0f )
,static_count( 1000000 )
,accumulator(1.0) // Immediate pulse on start.
,resyncInProgress(false)
{
	// Register pins.
	initializePin( 0, pinHostBpm );
	initializePin( 1, pinHostSongPosition );
	initializePin( 2, pinPulseDivide );
	initializePin( 3, pinPulseOut );
	initializePin(4, pinHostTransport);
	initializePin(5, pinNumerator);
	initializePin(6, pinDenominator);
	initializePin(7, pinHostBarStart);
}

void BpmClock3::subProcess2( int bufferOffset, int sampleFrames )
{
	float* pulseOut = bufferOffset + pinPulseOut.getBuffer();

	int s = sampleFrames;
	while(true)
	{
		double samplesTillTransitionD;
		if( pulseOutVal == 0.0f )
		{
			samplesTillTransitionD = ( 1.0 - accumulator ) / increment;
		}
		else
		{
			samplesTillTransitionD = ( 0.5 - accumulator ) / increment;
		}

//		int samplesTillTransition = _mm_cvtsd_si32(_mm_set_sd(samplesTillTransitionD));
		int samplesTillTransition = FastRealToIntTruncateTowardZero(samplesTillTransitionD);

		int todo = (std::max)( 0, (std::min)( samplesTillTransition, s ) );
		if( static_count > 0 )
		{
			for( int i = todo ; i > 0 ; --i )
			{
				*pulseOut++ = pulseOutVal;
			}

			static_count -= todo;
		}
		else
		{
			pulseOut += todo;
		}

		accumulator += increment * todo;
		s -= todo;

//		if( samplesTillTransition >= s ) // no transition yet.
		if( s == 0 ) // no transition yet.
		{
			return;
		}

		if( pulseOutVal == 0.0f )
		{
#ifdef DEBUG_BPMCLOCK
			_RPTW1(_CRT_WARN, L" PROC Transistion HI. Bar %f\n", 0.25f * (float) pinHostSongPosition );
#endif
			accumulator -= 1.0;
			pulseOutVal = 1.0f;
			if( resyncInProgress )
			{
#ifdef DEBUG_BPMCLOCK
			_RPTW0(_CRT_WARN, L" resyncInProgress DONE\n" );
#endif
				// We have just had a short pulse to re-sync. Now reset increment and accumulator to business-as-usual.
				resyncInProgress = false;
				CalcIncrement(pinPulseDivide);
//                accumulator = 0.0; // approximation.
// mistake?				float SongPosition = (float)(ppqPos + incrementPerSample) * (float)(sampleFrames - s);
				float SongPosition = (float)(ppqPos + incrementPerSample * (sampleFrames - s));
				accumulator -= CalcAccumulatorError(SongPosition, accumulator, pinPulseDivide, pinHostBarStart, pinNumerator, pinDenominator);
			}
		}
		else
		{
#ifdef DEBUG_BPMCLOCK
			_RPTW1(_CRT_WARN, L" PROC Transistion LO. Bar %f\n", 0.25f * (float) pinHostSongPosition );
#endif
			pulseOutVal = 0.0f;
		}

		pinPulseOut.setStreaming( false, bufferOffset + sampleFrames - s );
		static_count = getBlockSize();
	}

    ppqPos += incrementPerSample * (double) sampleFrames;
}


float CalculateMuliplier( int PulseDivisionSetting, int numerator, int denominator )
{
	if( PulseDivisionSetting < 0 ) // whole quarter notes.
	{
		if (PulseDivisionSetting < -3)
		{
			// Whole Bars. -4 = 1 Bar, -8 = 2 Bar etc. Regardless of time division.
//			float bars = PulseDivisionSetting / -4.0f;
			numerator = (std::max)(numerator, 1);
			denominator = (std::max)(denominator, 1);
			// Take time signature into account.
			return -PulseDivisionSetting * (float)numerator / denominator;
		}

		// old way. Assume 4/4
		return -(float)PulseDivisionSetting;
	}
	else
	{
		if( PulseDivisionSetting == 0 ) // Lazy-sync to host. (not used).
		{
			return 8.0f;
		}
		else
		{
			return 1.0f / (float) PulseDivisionSetting; // fraction-of-bar.
		}
	}
}

void BpmClock3::CalcIncrement( int pulseDivide )
{
	assert(pinHostBpm.getValue() > 0);
	auto bpm = (std::max)(1.0f, pinHostBpm.getValue());
	increment = bpm / (CalculateMuliplier(pulseDivide, pinNumerator, pinDenominator) * getSampleRate() * 60.0f);
//	_RPTW1(_CRT_WARN, L"    increment %f\n", increment );
	assert(increment > 0.0f);
}

double BpmClock3::CalcAccumulatorError(float HostSongPosition, double accumulator, int pulseDivision, float hostBarStart, int numerator, int denominator)
{
	float multiplier = CalculateMuliplier(pulseDivision, numerator, denominator);

	double accumulatorError = 0.0;

	if( multiplier >= 1.0f )
	{
		// Syncs to multiple of division. e.g 8, 16, 24 bar, etc..
//		double fraction = HostSongPosition / multiplier;
		double fraction = (HostSongPosition - hostBarStart) / multiplier;

		fraction = fraction - floor(fraction);
		accumulatorError = accumulator - fraction;
	}
	else
	{
		float fraction = HostSongPosition - floorf(HostSongPosition); // fraction of QN.
		// What is the expected pulse state?
		float hostPulseWithinBar =  fraction / multiplier;				// number of pulses since bar-start.
		float hostPulseFraction = hostPulseWithinBar - floorf(hostPulseWithinBar);	// position within current pulse 0.0 -> 1.0

		accumulatorError = accumulator - hostPulseFraction;
	}

	if( accumulatorError > 0.5 )
	{
		accumulatorError -= 1.0;
	}
	else
	{
		if( accumulatorError < -0.5 )
		{
			accumulatorError += 1.0;
		}
	}

	return accumulatorError;
}

void BpmClock3::onSetPins()
{
	if (pinPulseDivide.isUpdated())
	{
//		_RPTW1(_CRT_WARN, L"DSP Pulse Divide = %d\n", (int)pinPulseDivide);
	}
	if (pinNumerator.isUpdated() || pinDenominator.isUpdated())
	{
//		_RPTW2(_CRT_WARN, L"BpmClock3 TimeSig = %d/%d\n", (int)pinNumerator, (int)pinDenominator);
	}
	if (pinHostBarStart.isUpdated())
	{
//		_RPTW1(_CRT_WARN, L"BpmClock3 Bar Start = %f\n", (double)pinHostBarStart);
	}

	const int resyncDivisions = 4; // when theres a need to resync, do it on a bar division (4 QN).

	int PulseDivide = pinPulseDivide;

	bool resync = false;

	if( pinHostBpm.isUpdated() )
	{
		const float seconds_per_minute = 60.0;
        incrementPerSample = pinHostBpm / ( getSampleRate() * seconds_per_minute );
	}

	// Special case - When DAW hits 'play' sync at least to next bar, else could be waiting several bars.
	if( pinHostTransport.isUpdated() && pinHostTransport == true ) // && pinPulseDivide <= -resyncDivisions )
	{
//		_RPTW1(_CRT_WARN, L"TRANSPORT START: Bar %f ********************\n", 0.25f * (float) pinHostSongPosition );
		resync = resyncInProgress = true;
	}

	if( resyncInProgress ) // note resyncInProgress may remain true over several calls here while song-pos updates toward the next bar start..
	{
		PulseDivide = (std::max)( PulseDivide, -resyncDivisions ); // force sync to (at latest) next whole bar.
	}

	if( pinHostSongPosition.isUpdated() || resync )
	{
        ppqPos = pinHostSongPosition;

		double accumulatorError = CalcAccumulatorError(pinHostSongPosition, accumulator, PulseDivide, pinHostBarStart, pinNumerator, pinDenominator);
#ifdef DEBUG_BPMCLOCK
            if( resyncInProgress )
            {
                _RPTW2(_CRT_WARN, L"RESYNCING: BAR %f  accumulatorError %f\n", 0.25f * (float) pinHostSongPosition, accumulatorError );
            }
#endif
		// Any sudden jump on timeline causes resync. (aassumed user moved playback).
		const double maxExpectedDrift = 0.01;
		if( !resync && (accumulatorError > maxExpectedDrift || accumulatorError < -maxExpectedDrift) )
		{
#ifdef DEBUG_BPMCLOCK
            _RPTW1(_CRT_WARN, L"TRANSPORT JUMP: Bar %f ********************\n", 0.25f * (float) pinHostSongPosition );
#endif
			resync = resyncInProgress = true;
			PulseDivide = (std::max)( PulseDivide, -resyncDivisions ); // force sync to (at latest) next whole bar.

			// Recalc accumulator error given new PulseDivide.
			accumulatorError = CalcAccumulatorError(pinHostSongPosition, accumulator, PulseDivide, pinHostBarStart, pinNumerator, pinDenominator);
		}

		accumulator -= accumulatorError;
	}

	// If the transport has jumped arround, we may need to reset output to zero ready for clean hi pulse (else it gets missed for one cycle).
	if(	resync )
	{
		// If corrected accumulator < 0, this need to be wrapped 0.0 - 1.0 range else pulse won't happen for 1 and a bit bars.
		accumulator = accumulator - floor(accumulator);

		// Always drop output to zero. Even at exact bar start to force a positive-going transition.
		float newPulseOut = 0.0;
		if( newPulseOut != pulseOutVal )
		{
#ifdef DEBUG_BPMCLOCK
            			_RPTW2(_CRT_WARN, L"    Transition %d: accumulator %f\n", (int) newPulseOut, accumulator );
#endif
			pulseOutVal = newPulseOut;
			pinPulseOut.setUpdated();
			static_count = getBlockSize();
		}

		if( accumulator < 0.00001 ) // DAW is exactly on bar-start? Start next positive transistion ASAP (after a brief LO pulse).
		{
#ifdef DEBUG_BPMCLOCK
            //			_RPTW0(_CRT_WARN, L"    trigger HI transition ASAP\n" );
#endif
			CalcIncrement( PulseDivide );
			accumulator = 1.0 - increment - increment; // trigger HI transition ASAP. (should be in 1-2 samples depending on increment precision.).
		}
	}

	if( pinHostBpm.isUpdated() || pinPulseDivide.isUpdated() || resync )
	{
		CalcIncrement( PulseDivide );
	}

	assert(increment > 0.0f);

	// Set processing method.
	SET_PROCESS(&BpmClock3::subProcess2);

	// Inhibit sleep mode.
	setSleep(false);
}
