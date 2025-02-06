#include "./UnitConverter.h"
#include <math.h>
#include <algorithm>
#include "../shared/fastmaths.h"
#include "../shared/xp_simd.h"

#if GMPI_USE_SSE
#include "fastmath/fmath.h"
#endif

SE_DECLARE_INIT_STATIC_FILE(UnitConverter)
SE_DECLARE_INIT_STATIC_FILE(UnitConverterVolts)

using namespace std;

REGISTER_PLUGIN ( UnitConverter, L"SE Unit Converter" );
REGISTER_PLUGIN2(UnitConverterVolts, L"SE Unit Converter (Volts)");

UnitConverter::UnitConverter( IMpUnknown* host ) : MpBase( host )
{
	// Register pins.
	initializePin( 0, pinA );
	initializePin( 1, pinB );
	initializePin( 2, pinMode );
}

void UnitConverter::onSetPins()
{
	const float verySmallNumber = 1E-6f;

	switch( pinMode )
	{
	case 0: // Hz to Octave (Volts).
		{
			// Convert Hz to Octaves. middle-A (440Hz) being a value of 5 Volts.
			if( pinA < verySmallNumber )
			{
				pinB = 0.0f; // prevent numeric overflow.
			}
			else
			{
				pinB = logf( pinA / 440.0f ) / logf(2.0f) + 5.0f;
			}
		}
		break;

	case 1: // dB to VCA (dB).
		{
            if( pinA > -35.0f )
            {
                pinB = 10.0f + pinA * 9.0f / 35.0f;
            }
            else
            {
                // Slope steepens below -35dB.
                pinB = 49.393f * expf( pinA * 0.114f );
            }
		}
		break;

	case 2: // dB to VCA (exponential).
        {
			float timeconstants = 3.0f;
		    const float c1 = 1.0f / ( 1.0f - expf( -timeconstants ));      //  scale factor truncated curve
			float db = pinA;
			float gain = powf( 10.0f, db * 0.05f );

			pinB = 10.0f * ( 1.0f + logf( 1.0f - (1.0f - gain) / c1) / timeconstants );
        }
        break;

	case 3: // ms to ADSR2.
        {
			if( pinA < verySmallNumber )
			{
				pinB = 0.0f; // prevent numeric overflow.
			}
			else
			{
	            pinB = ( log10f( pinA * 0.001f ) + 3.0f ) * 2.5f;
			}
        }
		break;

	case 4:  // ms to portamento.
		{
			// Time in ms ranges from 2ms to 2500ms.
			// This maps to the DSP range 0 - 10.
			if( pinA > 1.0f )
			{
				pinB = logf( pinA * 0.001f ) / logf( 2.0f ) + 8.666666f;
			}
			else
			{
				// Values less than 2ms represent OFF (no portamento).
				pinB = 0.0f;
			}
		}
        break;

	// VOLTS TO REAL-WORLD UNITS. ****************************************************
	case 100: // Octaves to Volts:
		{
			float value = max( pinA.getValue(), -20.0f );
			value = min( value, 20.0f );

			// Convert Octaves to Hz. middle-A (440Hz) being a value of 5.
			pinB = 440.0f * powf(2.0f, value - 5.0f );
			break;
		}

	case 101: // VCA (dB) to dB.
		{
			// convert to dB. -80 - 0dB
			float value = max( pinA.getValue(), 0.0f );

			if( value > 1.0f )
			{
				pinB = (value - 10.0f) * 35.0f / 9.0f;
			}
			else
			{
				// accurate, but not nesc.
				//double x = (1.1 - v) * 10.0;
				//v = -0.2793*x*x + 1.1091*x -36.515;
				// (very Approximate, but no patches are this quiet anyhow).
				pinB = value * 35.0f - 70.0f;
			}
		}
		break;

	case 102: // VCA (exponential) to dB..
        {
		    const float c1 = 1.0f / ( 1.0f - expf( -3.0f ));      //  scale factor truncated curve
		    float gain = 1.0f - c1 * (1.0f - exp( 3.0f * (pinA * 0.1f - 1.0f)));

			// Avoid overflow on zero gain.
			const float minimumDb = -60.0f;
			if( gain <= powf( 10.0f, minimumDb * 0.5f ) )
			{
				pinB = minimumDb;
			}
			else
			{
				pinB = 20.0f * log10f(gain); // dB
			}
        }
        break;

	case 103: // ADSR2 to ms.
		{
			// Convert to ms.
			pinB = 1000.0f * powf( 10.0f, (pinA * 0.4f ) - 3.0f );
		}
		break;

	case 104: // Portamento to ms.
        // Time in ms ranges from 2ms to 2500ms.
        // This maps to the DSP range 0 - 10.
		if( pinA > 0.0f ) // Zero is special, means OFF or 0 ms.
		{
			// Convert to ms.
			pinB = 1000.0f * pow( 2.0f, pinA - 8.666666f );
		}
		else
		{
			pinB = 0.0f;
		}
        break;
	}
}

UnitConverterVolts::UnitConverterVolts()
{
	// Register pins.
	initializePin(0, pinSignalIn);
	initializePin(1, pinSignalOut);
	initializePin(2, pinMode);
}
/*
// http://fastapprox.googlecode.com/svn/trunk/fastapprox/src/fastonebigheader.h
sta tic inline float
fastlog2(float x)
{
	// !!! INTEL COMPILER BUG, AVOID !!!!!! (aliased pointers, use code below instead)
	union { float f; uint32_t i; } vx = { x };
	union { uint32_t i; float f; } mx = { ( vx.i & 0x007FFFFF ) | 0x3f000000 };
	float y = (float) vx.i;
	y *= 1.1920928955078125e-7f;

	return y - 124.22551499f
		- 1.498030302f * mx.f
		- 1.72587999f / ( 0.3520887068f + mx.f );
}

inline float
fastlog2j(float x) // log base-2
{
	uint32_t vxi = *reinterpret_cast<uint32_t*>(&x);
	uint32_t mxi = ( vxi & 0x007FFFFF ) | 0x3f000000;

	float mxf = *reinterpret_cast<float*>( &mxi );
	float y = (float) vxi;
	y *= 1.1920928955078125e-7f;

	return y - 124.22551499f
		- 1.498030302f * mxf
		- 1.72587999f / ( 0.3520887068f + mxf );
}

sta tic inline float
fastlog(float x)
{
	return 0.69314718f * fastlog2j(x);
}

sta tic inline float
fasterlog2(float x)
{
	union { float f; uint32_t i; } vx = { x };
	float y = (float) vx.i;
	y *= 1.1920928955078125e-7f;
	return y - 126.94269504f;
}

sta tic inline float
fasterlog(float x)
{
	//  return 0.69314718f * fasterlog2 (x);

	union { float f; uint32_t i; } vx = { x };
	float y = (float) vx.i;
	y *= 8.2629582881927490e-8f;
	return y - 87.989971088f;
}
*/

void UnitConverterVolts::subProcessHzToVolts(int sampleFrames)
{
    const float HzMin = 0.00000001f;

#if !GMPI_USE_SSE
    auto signalIn = getBuffer(pinSignalIn);
    auto signalOut = getBuffer(pinSignalOut);

    while (sampleFrames-- > 0)
	{
		*signalOut++ = hz_to_volts_const2 * (log((std::max)(HzMin, *signalIn++)) + hz_to_volts_const1);
	}

#else // SSE Version.

	// get pointers to in/output buffers.
	__m128* signalIn = reinterpret_cast<__m128*>(getBuffer(pinSignalIn));
	__m128* signalOut = reinterpret_cast<__m128*>(getBuffer(pinSignalOut));

	// Process samples in groups of 4.
	const __m128 const1_SSE = _mm_set_ps1(hz_to_volts_const1);
	const __m128 const2_SSE = _mm_set_ps1(hz_to_volts_const2);
	const __m128 HzMin_SSE = _mm_set_ps1(HzMin);

	// process non-sse-aligned prequel.
	if (((intptr_t)signalOut) & 0x0f)
	{
		_mm_storeu_ps( (float*) signalOut, _mm_mul_ps(const2_SSE, _mm_add_ps(const1_SSE, fmath::log_ps(_mm_max_ps(_mm_loadu_ps(reinterpret_cast<float*>(signalIn)), HzMin_SSE)))));

		auto processed = 4 - (((intptr_t)signalIn >> 2) & 3);
		signalIn = (__m128*)(((float*)signalIn) + processed); // go to next aligned SE location.
		signalOut = (__m128*)(((float*)signalOut) + processed);
		sampleFrames -= processed;
	}

	while (sampleFrames > 0)
	{
		*signalOut++ = _mm_mul_ps(const2_SSE, _mm_add_ps(const1_SSE, fmath::log_ps( _mm_max_ps(*signalIn++, HzMin_SSE) )));
		sampleFrames -= 4;
	}

#endif
}

void UnitConverterVolts::subProcessVoltsToHz(int sampleFrames)
{
	// get pointers to in/output buffers.
	float* signalIn = getBuffer(pinSignalIn);
	float* signalOut = getBuffer(pinSignalOut);

	for (int s = sampleFrames; s > 0; --s)
	{
		float value = max(*signalIn * 10.0f, -20.0f);
		value = min(value, 20.0f);

		// Convert Octaves to Hz. middle-A (440Hz) being a value of 5.
//		*signalOut = 0.0440f * powf(2.0f, value - 5.0f);
		*signalOut = 0.0440f * fastpow2(value - 5.0f);
		
		// Increment buffer pointers.
		++signalIn;
		++signalOut;
	}
}

/*
if( pinA > -35.0f )
{
pinB = 10.0f + pinA * 9.0f / 35.0f;
}
else
{
// Slope steepens below -35dB.
pinB = 49.393f * expf( pinA * 0.114f );
}
*/
void UnitConverterVolts::subProcessDbToVca(int sampleFrames)
{
	// get pointers to in/output buffers.
	float* signalIn = getBuffer(pinSignalIn);
	float* signalOut = getBuffer(pinSignalOut);

	for (int s = sampleFrames; s > 0; --s)
	{
		float value = (std::max)(*signalIn * 10.0f, -100.0f);
		value = (std::min)(value, 300.0f);

		if (value > -35.0f)
		{
			*signalOut = 0.1f * (10.0f + value * 9.0f / 35.0f);
		}
		else
		{
			// Slope steepens below -35dB.
			*signalOut = (std::max)(0.0f, 0.1f * (49.393f * fasterexp(value * 0.114f)));
		}

		// Increment buffer pointers.
		++signalIn;
		++signalOut;
	}
}

/*
float timeconstants = 3.0f;
const float c1 = 1.0f / ( 1.0f - expf( -timeconstants ));      //  scale factor truncated curve
float db = pinA;
float gain = powf( 10.0f, db * 0.05f );

pinB = 10.0f * ( 1.0f + logf( 1.0f - (1.0f - gain) / c1) / timeconstants );
*/
void UnitConverterVolts::subProcessDbToExp(int sampleFrames)
{
	// get pointers to in/output buffers.
	float* signalIn = getBuffer(pinSignalIn);
	float* signalOut = getBuffer(pinSignalOut);

	const float timeconstants = 3.0f;
	const float overc1 = 1.0f - expf(-timeconstants);      //  scale factor truncated curve

	for (int s = sampleFrames; s > 0; --s)
	{
		float db = max(*signalIn * 10.0f, -100.0f);
		db = min(db, 300.0f);

		float gain = fasterpow(10.0f, db * 0.05f);

		*signalOut = (std::max)(0.0f, 0.1f * (10.0f * (1.0f + fasterlog(1.0f - (1.0f - gain) * overc1) / timeconstants)));

		/*
		const float c1 = 1.0f / (1.0f - expf(-timeconstants));      //  scale factor truncated curve
		float gain2 = powf(10.0f, db * 0.05f);

		auto xx = 10.0f * (1.0f + logf(1.0f - (1.0f - gain2) / c1) / timeconstants);
		*/

		// Increment buffer pointers.
		++signalIn;
		++signalOut;
	}
}

/*
if( value > 1.0f )
{
	pinB = (value - 10.0f) * 35.0f / 9.0f;
}
else
{
	// accurate, but not nesc.
	//double x = (1.1 - v) * 10.0;
	//v = -0.2793*x*x + 1.1091*x -36.515;
	// (very Approximate, but no patches are this quiet anyhow).
	pinB = value * 35.0f - 70.0f;
}
*/
void UnitConverterVolts::subProcessVcaToDb(int sampleFrames)
{
	// get pointers to in/output buffers.
	float* signalIn = getBuffer(pinSignalIn);
	float* signalOut = getBuffer(pinSignalOut);

	const float timeconstants = 3.0f;
	constexpr float thrtyfiveovernine = 35.0f / 9.0f;

	for (int s = sampleFrames; s > 0; --s)
	{
		// convert to dB. -80 - 0dB
		float value = (std::max)(*signalIn, 0.0f);
		if (value > 0.1f)
		{
			*signalOut = ((value - 1.0f) * thrtyfiveovernine);
		}
		else
		{
			// accurate, but not nesc.
			//double x = (1.1 - v) * 10.0;
			//v = -0.2793*x*x + 1.1091*x -36.515;
			// (very Approximate, but no patches are this quiet anyhow).
			*signalOut = value * 35.f - 7.0f;
		}

		// Increment buffer pointers.
		++signalIn;
		++signalOut;
	}
}

/*
const float c1 = 1.0f / ( 1.0f - expf( -3.0f ));      //  scale factor truncated curve
float gain = 1.0f - c1 * (1.0f - exp( 3.0f * (pinA * 0.1f - 1.0f)));

// Avoid overflow on zero gain.
const float minimumDb = -60.0f;
if( gain <= powf( 10.0f, minimumDb * 0.5f ) )
{
pinB = minimumDb;
}
else
{
pinB = 20.0f * log10f(gain); // dB
}
*/
void UnitConverterVolts::subProcessExpToDb(int sampleFrames)
{
	// get pointers to in/output buffers.
	float* signalIn = getBuffer(pinSignalIn);
	float* signalOut = getBuffer(pinSignalOut);

	const float c1 = 1.0f / (1.0f - expf(-3.0f));      //  scale factor truncated curve
	const float overLog10 = 1.0f / log2(10.0f);

	for (int s = sampleFrames; s > 0; --s)
	{
		float v = (std::max)(*signalIn, 0.0f);
		float gain = 1.0f - c1 * (1.0f - fasterexp(3.0f * (v - 1.0f)));

		// Avoid overflow on zero gain.
		const float minimumDb = -60.0f;
		if (gain < 0.01f ) //powf(10.0f, minimumDb * 0.5f))
		{
			*signalOut = 0.1f * minimumDb;
		}
		else
		{
			*signalOut = 0.1f * (20.0f * overLog10 * fasterlog2(gain)); // dB
		}

		// Increment buffer pointers.
		++signalIn;
		++signalOut;
	}
}

void UnitConverterVolts::onSetPins()
{
	pinSignalOut.setStreaming(pinSignalIn.isStreaming());

	// Set processing method.
	switch (pinMode)
	{
	case UCV_HZ_TO_VOLTS:
		SET_PROCESS2(&UnitConverterVolts::subProcessHzToVolts);
		break;

	case UCV_VOLTS_TO_HZ:
		SET_PROCESS2(&UnitConverterVolts::subProcessVoltsToHz);
		break;

	case UCV_DB_TO_VCA:
		SET_PROCESS2(&UnitConverterVolts::subProcessDbToVca);
		break;

	case UCV_DB_TO_EXP:
		SET_PROCESS2(&UnitConverterVolts::subProcessDbToExp);
		break;

	case UCV_VCA_TO_DB:
		SET_PROCESS2(&UnitConverterVolts::subProcessVcaToDb);
		break;

	case UCV_EXP_TO_DB:
		SET_PROCESS2(&UnitConverterVolts::subProcessExpToDb);
		break;
	}
}

int32_t UnitConverterVolts::open()
{
#ifdef UNITCONVERTER_VOLTS_USE_LOOKUP
	// fix for race conditions.
	static std::mutex safeInit;
	std::lock_guard<std::mutex> lock(safeInit);

	// table method, not good.
	int32_t needInitialize;
	const int tableSize = LOOKUP_SIZE + 4; // extras for interpolator.
	float* temp;
	getHost()->allocateSharedMemory(L"UnitConverterVolts:KhzToVolts", (void**)&temp, -1, sizeof(float) * tableSize, needInitialize);
	lookup_ = temp + 1;
	if( needInitialize != 0 )
	{
		const float c = 0.1f / logf(2.0f);
		constexpr float c2 = 1000.0f / 440.f;

		for( int i = 0; i < LOOKUP_SIZE + 3; ++i )
		{
			float kHz = MAX_KHZ * (float)(i) / (float)LOOKUP_SIZE;
			kHz = ( std::max )( kHz, 0.000001f ); // to prevent divide-by-zero approximate 0kHz as 0.001 Hz.
			// lookup_[i] = 0.1f * ( logf(kHz / 0.440f) / logf(2.0f)) + 0.5f; // full formular. Constansts represent 440 Hz and 5 Volts.
			lookup_[i] = c * logf(kHz * c2) + 0.5f; // fast version.
		}

		for( int i = -1; i < LOOKUP_SIZE + 3; ++i )
		{
			float kHz = MAX_KHZ * (float)( i ) / (float)LOOKUP_SIZE;
			kHz = ( std::max )( kHz, 0.000001f ); // to prevent divide-by-zero approximate 0kHz as 0.001 Hz.
			_RPT3(_CRT_WARN, "KhzToVolts, %f, %f, %f\n", kHz, c * logf(kHz * c2) + 0.5f, c * fastlog(kHz * c2) + 0.5f);
		}
	}

//	lookup_[-1] = lookup_[0] = lookup_[1]; // Zero Hz not possible so approximate.
//	lookup_[0] = lookup_[1] + ( lookup_[1] - lookup_[2] ); // linear extrapolate.
	lookup_[-1] = lookup_[0] + ( lookup_[0] - lookup_[1] ); // linear extrapolate.
#endif

	//	const float const1 = log(10.0f * 1000.0f / 440.0f) + 5.0 * log(2.0); // sample-to-volts, kHz to Hz, middle-A, Middle-Volts.
	hz_to_volts_const1 = static_cast<float>(log((float)(1 << 5) * 10.0f * 1000.0f / 440.0f)); // sample-to-volts, kHz to Hz, middle-A, Middle-Volts.
	hz_to_volts_const2 = static_cast<float>(0.1 / log(2.0));

	return MpBase2::open();
}

