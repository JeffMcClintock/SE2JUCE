#include <algorithm>
#include "math.h"
#include "./BpmClock4.h"
#include "../shared/xp_simd.h"

REGISTER_PLUGIN2(BpmClock4, L"SE BPM Clock4");

//#define DEBUG_BPMCLOCK

BpmClock4::BpmClock4() :
pulseOutVal( 0.0f )
,static_count( 1000000 )
,accumulator(1.0) // Immediate pulse on start.
, qnPos(0.0f)
, qnPosIncPerSample(0.0f)
, samplesSinceTransition(1)
, currentState(3)
{
	// Register pins.
	initializePin( pinHostBpm );
	initializePin( pinHostSongPosition );
	initializePin( pinHostTransport);
	initializePin( pinNumerator);
	initializePin( pinDenominator);
	initializePin( pinHostBarStart);
	initializePin( pinPulseDivide );
	initializePin(pinSwing);
	initializePin(pinPulseWidth);
	initializePin( pinPulseOut );

	nextTransition = 1.0;
}

void BpmClock4::subProcess2(int sampleFrames)
{
	auto pulseOut = getBuffer(pinPulseOut);

	int s = sampleFrames;
	while(true)
	{
		double samplesTillTransitionD = (nextTransition - accumulator) / increment;

		int samplesTillTransition = FastRealToIntTruncateTowardZero(samplesTillTransitionD);

		int todo = (std::max)( 0, (std::min)( samplesTillTransition, s ) );

		// Enforce at least one sample between transitions.
		if (samplesSinceTransition == 0)
		{
			todo = (std::max)(1, todo);
		}

		if( static_count > 0 )
		{
			for( int i = todo ; i > 0 ; --i )
			{
				*pulseOut++ = pulseOutVal;
			}
			samplesSinceTransition += todo;

			static_count -= todo;
		}
		else
		{
			pulseOut += todo;
		}

		accumulator += increment * todo;
		qnPos += qnPosIncPerSample * (double)todo;
		s -= todo;

		if( s == 0 ) // no transition yet.
		{
			break;
		}

		samplesSinceTransition = 0;

		// next state.
		currentState = (currentState + 1) & 0x03;

		// States cover one odd and one even pulse (for swing). Hi and Low transitions.
		if (currentState & 0x01)
		{
			pulseOutVal = 0.0f;
		}
		else
		{
			pulseOutVal = 1.0f;
		}

		switch (currentState)
		{
		case 0:
			nextTransition = swingAmnt + (1.0 - swingAmnt) * PulseWidthAmnt;
			nextTransition = (std::min) (nextTransition, 1.0 + swingAmnt);

			accumulator -= 1.0; // reset accumulator.
			break;

		case 1:
			nextTransition = 1.0 + swingAmnt;
			break;

		case 2:
			nextTransition = 1.0 + swingAmnt + (1.0 - swingAmnt) * PulseWidthAmnt;
			break;

		case 3:
			nextTransition = 2.0;
			break;
		}

		nextTransition *= 0.5;

		pinPulseOut.setStreaming( false, getBlockPosition() + sampleFrames - s );
		static_count = getBlockSize();
	}
}

void BpmClock4::CalcIncrement(float overmultiplier)
{
	assert(pinHostBpm.getValue() > 0);

//	increment = pinHostBpm / (multiplier * getSampleRate() * 60.0f);
	auto bpm = (std::max)(1.0f, pinHostBpm.getValue());
	increment = overmultiplier * bpm / (getSampleRate() * 60.0f);
	//	_RPTW1(_CRT_WARN, L"    increment %f\n", increment );
}

double BpmClock4::CalcAccumulatorError(double HostSongPosition, double accumulator, float overmultiplier, float hostBarStart) const
{
	double accumulatorError = 0.0;

	double syncBarStart = hostBarStart;

	// Calulate where we are in respect to sync marker (e.g. every 3rd bar).
	auto estimatedHostBarIndex = FastRealToIntFloor( 0.5 + hostBarStart / quarterNotesPerBar); // assume all bars within song are same length, can get confused with mixed time-signatures but that only matters when syning period more than one bar.
	auto barWithinSyncBars = estimatedHostBarIndex % syncBars; // e.g. 2nd bar out of 3.
	syncBarStart -= barWithinSyncBars * quarterNotesPerBar;

	double hostPositionWithinBar = HostSongPosition - syncBarStart;
	double fraction = overmultiplier * hostPositionWithinBar; // / multiplier;

	accumulatorError = accumulator - fraction;

	if( accumulatorError > 0.5 )
	{
		while (accumulatorError > 0.5)
		{
			accumulatorError -= 1.0;
		}
	}
	else
	{
		while( accumulatorError < -0.5 )
		{
			accumulatorError += 1.0;
		}
	}

	return accumulatorError;
}

void BpmClock4::onSetPins()
{
	if (pinPulseDivide.isUpdated())
	{
//		_RPTW1(_CRT_WARN, L"DSP Pulse Divide = %d\n", (int)pinPulseDivide);

		// Determine how often to sync.

		// SE enums only 16-bit, correct for overflow.
		int PulseDivisionSetting = pinPulseDivide;
		if (PulseDivisionSetting < 0)
			PulseDivisionSetting += 0x10000;

		auto numerator = (std::max)(pinNumerator.getValue(), 1);
		auto denominator = (std::max)(pinDenominator.getValue(), 1);

		quarterNotesPerBar = (4 * denominator) / numerator;
		auto pulsesPerBar = (PPQN * quarterNotesPerBar) / 2; // 2 pulses per step.

		syncBars = 1;
		while( (pulsesPerBar * syncBars) % PulseDivisionSetting != 0) // does note divide nicely into bar?
		{
			++syncBars;
		}
//		_RPTW1(_CRT_WARN, L"Sync period %d bars\n", syncBars);
	}

	if (pinSwing.isUpdated())
	{
		double Swingsafe = (std::min)(10.0f, (std::max)(0.0f, (float) pinSwing));
		swingAmnt = (Swingsafe - 5.0) * 0.2; // 5 = 50% - straight playing.
	}
	if (pinPulseWidth.isUpdated())
	{
		double PulseWidthSafe = (std::min)(10.0f, (std::max)(0.0f, (float) pinPulseWidth));
		PulseWidthAmnt = 0.1 * PulseWidthSafe;
	}

	if( pinHostBpm.isUpdated() )
	{
		const double seconds_per_minute = 60.0;
        qnPosIncPerSample = pinHostBpm / ( getSampleRate() * seconds_per_minute );
	}

	bool resync = false;
	// Special case - When DAW hits 'play' sync at least to next bar, else could be waiting several bars.
	if( pinHostTransport.isUpdated() && pinHostTransport == true ) // && pinPulseDivide <= -resyncDivisions )
	{
//		_RPTW1(_CRT_WARN, L"TRANSPORT START: Bar %f ********************\n", 0.25f * (float) pinHostSongPosition );
		resync = true;
	}

	auto overmultiplier = CalculateMultiplier(pinPulseDivide, pinNumerator, pinDenominator);

	if (pinHostSongPosition.isUpdated())
	{
#ifdef DEBUG_BPMCLOCK
		_RPTW2(_CRT_WARN, L"Host QN %f My QN %f\n", (float)pinHostSongPosition, qnPos);
#endif

		qnPos = pinHostSongPosition;
	}

	if (pinHostSongPosition.isUpdated() || resync)
	{
		double accumulatorError = CalcAccumulatorError(pinHostSongPosition, accumulator, overmultiplier, pinHostBarStart);
#if 0 // def DEBUG_BPMCLOCK
		if( resyncInProgress )
            {
                _RPTW2(_CRT_WARN, L"RESYNCING: BAR %f  accumulatorError %f\n", 0.25f * (float) pinHostSongPosition, accumulatorError );
            }
#endif
		// Any sudden jump on timeline causes resync. (assumed user moved playback).
		const double maxExpectedDrift = 0.01;
		if( !resync && (accumulatorError > maxExpectedDrift || accumulatorError < -maxExpectedDrift) )
		{
#ifdef DEBUG_BPMCLOCK
            _RPTW1(_CRT_WARN, L"TRANSPORT JUMP: Bar %f ********************\n", 0.25f * (float) pinHostSongPosition );
#endif
			resync = /*resyncInProgress =*/ true;
//			multi plier = (std::min)(multi plier, resyncMultiplier);

			// Recalc accumulator error given new PulseDivide.
//			accumulatorError = CalcAccumulatorError(pinHostSongPosition, accumulator, multi plier, pinHostBarStart);
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
			CalcIncrement(overmultiplier);
			accumulator = 1.0 - increment - increment; // trigger HI transition ASAP. (should be in 1-2 samples depending on increment precision.).
			currentState = 3;
		}
	}

	if( pinHostBpm.isUpdated() || pinPulseDivide.isUpdated() || resync )
	{
		CalcIncrement(overmultiplier);
	}

	assert(increment > 0.0f);

	// Set processing method.
	SET_PROCESS2(&BpmClock4::subProcess2);

	// Inhibit sleep mode.
	setSleep(false);
}
