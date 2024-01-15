#ifndef OSCILLATORNAIVE_H_INCLUDED
#define OSCILLATORNAIVE_H_INCLUDED

#include "../se_sdk3/mp_sdk_audio.h"
#include "../shared/xp_simd.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <algorithm>

class OscillatorNaive : public MpBase2
{
	double accumulator;
	double increment;
	double* pitchTable;
	bool syncState;
	double prevPhase;

public:

	class ModulatedPinPolicy
	{
	public:
		inline static void IncrementPointer(float*& samplePointer)
		{
			++samplePointer;
		}
		inline static float* getBuffer(const MpBase2* module, const AudioInPin& pin)
		{
			return module->getBuffer(pin);
		}
		enum { Active = true };
	};

	class NotModulatedPinPolicy
	{
	public:
		inline static void IncrementPointer(const float* samplePointer)
		{
			// do nothing. Hopefully optimizes away to nothing.
		}
		inline static float* getBuffer(const MpBase2* module, const AudioInPin& pin)
		{
			return 0;
		}
		enum { Active = false };
	};

	class OscPitchFixed : public NotModulatedPinPolicy
	{
	public:
		inline static void Calculate(const double* pitchTable, const float* pitch, double& returnIncrement)
		{
			// do nothing. Hopefully optimizes away to nothing.
		}
	};

	class OscPitchChanging : public ModulatedPinPolicy
	{
	public:
		const static int pitchTableLowVolts = -10; // ~ 1/60 Hz
		const static int pitchTableHiVolts = 11;  // ~ 20kHz.

		inline static double ComputeIncrement2(const double* pitchTable, float pitch)
		{
			//	float index = 12.0f * (pitch * 10.0f - (float)pitchTableLowVolts);
			float index = pitch * 120.0f - (float)(12 * pitchTableLowVolts);
			int table_floor = FastRealToIntTruncateTowardZero(index);
			float fraction = index - (float)table_floor;

			/*
			std:min slow compared to direct branching.
			// not as exact as limiting pitch ( fractional part will be wrong (but harmless)
			table_floor = std::min( table_floor, (pitchTableHiVolts - pitchTableLowVolts) * 12 );
			table_floor = std::max( table_floor, 0 );
			*/
			if (table_floor <= 0) // indicated index *might* be less than zero. e.g. Could be 0.1 which is valid, or -0.1 which is not.
			{
				if (!(index >= 0.0f)) // reverse logic to catch Nans.
				{
					return pitchTable[0];
				}
			}
			else
			{
				const int maxTableIndex = (pitchTableHiVolts - pitchTableLowVolts) * 12;
				if (table_floor >= maxTableIndex)
				{
					return pitchTable[maxTableIndex];
				}
			}

			// Cubic interpolator.
			assert(table_floor >= 0 && table_floor <= (pitchTableHiVolts - pitchTableLowVolts) * 12);

			double y0 = pitchTable[table_floor - 1];
			double y1 = pitchTable[table_floor + 0];
			double y2 = pitchTable[table_floor + 1];
			double y3 = pitchTable[table_floor + 2];

			return y1 + 0.5 * fraction*(y2 - y0 + fraction*(2.0 * y0 - 5.0 * y1 + 4.0 * y2 - y3 + fraction*(3.0 * (y1 - y2) + y3 - y0)));
		}
		inline static void Calculate(const double* pitchTable, const float* pitch, double& returnIncrement)
		{
			returnIncrement = ComputeIncrement2(pitchTable, *pitch);
		}
	};

	class WaveShapeSine : public NotModulatedPinPolicy // NotModulated = PW not relevant.
	{
	public:
		inline static float Calculate(double accumulator, float* ignored)
		{
			float saw = ( (float) M_PI ) * 2.0f * (float)accumulator;
			return 0.5f * sinf(saw);
		}
	};

	class WaveShapeSaw : public NotModulatedPinPolicy // NotModulated = PW not relevant.
	{
	public:
		inline static float Calculate(double accumulator, float* ignored)
		{
			float saw = (float)accumulator;
			int temp = FastRealToIntTruncateTowardZero(saw);
			saw -= 0.5f + temp;
			return saw;
		}
	};

	class WaveShapePulse : public ModulatedPinPolicy
	{
	public:
		inline static float Calculate(double accumulator, float* pulseWidth)
		{
			float tri = 0.25f + (float)accumulator;
			int temp = FastRealToIntTruncateTowardZero(tri);
			tri = 2.0f * (tri - temp);
			if (tri > 1.0f)
				tri = 1.5f - tri;
			else
				tri = tri - 0.5f;

			if (tri > *pulseWidth - 0.5)
				return 0.0f + *pulseWidth;
			else
				return -1.0f + *pulseWidth;
		}
	};

	class WaveShapeTriangle : public NotModulatedPinPolicy
	{
	public:
		inline static float Calculate(double accumulator, float* pulseWidth)
		{
			float tri = 0.25f + (float)accumulator;
			int temp = FastRealToIntTruncateTowardZero(tri);
			tri = 2.0f * (tri - temp);
			if (tri > 1.0f)
				tri = 1.5f - tri;
			else
				tri = tri - 0.5f;

			return tri;
		}
	};

	enum EWaveShape { WS_SINE, WS_SAW, WS_PULSE, WS_TRI	};

	OscillatorNaive( );
	int32_t MP_STDCALL open() override;

	template< class WaveShapePolicy, class PitchModulationPolicy, class phaseModulationPolicy, class SyncModulationPolicy >
	void subProcess(int sampleFrames)
	{
		// get pointers to in/output buffers.
		float* pitch = PitchModulationPolicy::getBuffer(this, pinPitch);
		float* pulseWidth = WaveShapePolicy::getBuffer(this, pinPulseWidth);
		float* sync = SyncModulationPolicy::getBuffer(this, pinSync);
		float* audioOut = getBuffer(pinAudioOut);

		float* phaseMod;
		if constexpr (phaseModulationPolicy::Active || SyncModulationPolicy::Active) // Need phase available for sync also.
			phaseMod = getBuffer(pinPhaseMod);

		for (int s = sampleFrames; s > 0; --s)
		{
			PitchModulationPolicy::Calculate(pitchTable, pitch, increment);

			if constexpr (phaseModulationPolicy::Active)
			{
				double delataPhase = prevPhase - *phaseMod;
				prevPhase = *phaseMod;

				accumulator += delataPhase * 0.5f;
				assert(accumulator > 0.0f);
			}

			// trigger sync?
			if constexpr (SyncModulationPolicy::Active)
			{
				if ((*sync > 0.0f) != syncState)
				{
					syncState = *sync > 0.0f;
					if (syncState)
					{
						accumulator = 5.0 - *phaseMod * 0.5f;
					}
				}
				++sync;
			}

			*audioOut = WaveShapePolicy::Calculate(accumulator, pulseWidth);

			accumulator += increment;

			// Increment buffer pointers.
			PitchModulationPolicy::IncrementPointer(pitch);
			WaveShapePolicy::IncrementPointer(pulseWidth);

			if (SyncModulationPolicy::Active || phaseModulationPolicy::Active)
			{
				++phaseMod;
			}

			++audioOut;
		}

		// Keep accumulator from losing precision due to getting too large, but also keep safely above zero, so FastRealToIntTruncateTowardZero() behaves ok.
		int accumulator_floor = FastRealToIntTruncateTowardZero(accumulator);
		accumulator -= (std::max)( 0, accumulator_floor - 20);
	}

    void onSetPins() override;

private:
	AudioInPin pinPitch;
	AudioInPin pinPulseWidth;
	IntInPin pinWaveform;
	AudioInPin pinSync;
	AudioInPin pinPhaseMod;
	AudioOutPin pinAudioOut;
};

#endif

