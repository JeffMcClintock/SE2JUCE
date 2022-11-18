#include ".\PitchDetector.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <algorithm>
#include "real_fft.h"

// Prevent stupid 'min' macro overriding std::min
//#define NOMINMAX
#undef min
#undef max

// Golden seach stuff.
#define R 0.61803399f
// The golden ratios.
#define C (1.0f-R)
#define SHFT2(a,b,c) (a)=(b);(b)=(c);
#define SHFT3(a,b,c,d) (a)=(b);(b)=(c);(c)=(d);

using namespace std;

void filter::setHz( float frequencyHz, float sampleRate )
{
	filter_l = expf( (float) -M_PI * 2.0f * frequencyHz / sampleRate );
}

REGISTER_PLUGIN ( PitchDetector, L"SE Pitch Detector" );

PitchDetector::PitchDetector( IMpUnknown* host ) : MpBase( host )
	,PeakDetectorDecay_( 0.9f )
	,currentPeak_( 0.0f )
	,currentNegativePeak_( 0.0f )
	,detectorOut_( false )
	,previousPeakTime_( 0 )
	,time_( 0 )
	,output_( 0.0f )
//	,prevInput_( 0.0f )
//	,prevPeak_( 0.0f )
	,filter_y1n( 0.0f )
	,outputMemoryIdx( 0 )
	,updateCounter_(0)
	,bufferIdx(0)
	,frameNumber(0)
{
	// Register pins.
	initializePin( 0, pinInput );
	initializePin( 1, pinPulseWidth );
	initializePin( 2, pinScale );
	initializePin( 3, pinAudioOut );
}

int32_t PitchDetector::open()
{
	// start the memory off at 440Hz to minimise settling time from a cold start.
	float middle_a = 0.5f;
	float middle_a_period = getSampleRate() / 440.0f;
	for( int i = 0 ; i < outputMemorySize; ++i )
	{
		outputMemoryPeriod[i] = middle_a_period;
	}

	//const float hiPassHz = 10.0f;
	const float loPassHz = 800.0f; // highest Uklulele string 440Hz
	filter_l = expf( (float) -M_PI * 2.0f * loPassHz / getSampleRate());

	buffer.resize(fftSize);
	hiPassFilter_.setHz( 10.0f, getSampleRate() ); // DC-Block filter

	return MpBase::open();
}
/*
// MEDIAN FILTER.
//Returns the kth smallest value in the array arr[1..n]. The input array will be rearranged
//to have this value in location arr[k], with all smaller elements moved to arr[1..k-1] (in
//arbitrary order) and all larger elements in arr[k+1..n] (also in arbitrary order).
#define SWAP( a,b ) temp=(a); (a)=(b); (b)=temp;
float select( unsigned long k, unsigned long n, float arr[] )
{
	unsigned long i,ir,j,l,mid;
	float a,temp;
	l=1;
	ir=n;
	for (;; ) {
		if (ir <= l+1) { //Active partition contains 1 or 2 elements.
			if (ir == l+1 && arr[ir] < arr[l]) { // Case of 2 elements.
				SWAP( arr[l],arr[ir] )
			}
			return arr[k];
		} else {
			mid=(l+ir) >> 1; //Choose median of left, center, and right elements as partitioning element a. Also rearrange so that arr[l]  arr[l+1],arr[ir]  arr[l+1].
			SWAP( arr[mid],arr[l+1] )
			if (arr[l] > arr[ir]) {
				SWAP( arr[l],arr[ir] )
			}
			if (arr[l+1] > arr[ir]) {
				SWAP( arr[l+1],arr[ir] )
			}
			if (arr[l] > arr[l+1]) {
				SWAP( arr[l],arr[l+1] )
			}
			i=l+1; // Initialize pointers for partitioning.
			j=ir;
			a=arr[l+1]; // Partitioning element.
			for (;; ) { // Beginning of innermost loop.
				do i++; while (arr[i] < a); //Scan up to nd element > a.
				do j--; while (arr[j] > a); //Scan down to nd element < a.
				if (j < i) break;  //Pointers crossed. Partitioning complete.
				SWAP( arr[i],arr[j] )
			} //End of innermost loop.
			arr[l+1]=arr[j]; //Insert partitioning element.
			arr[j]=a;
			if (j >= k) ir=j-1;  //Keep active the partition that contains the
			if (j <= k) l=i;  // kth element.
		}
	}
}
*/
void PitchDetector::subProcess( int bufferOffset, int sampleFrames )
{
	// Musical Applications of Microprocessors page 584.

	// get pointers to in/output buffers.
	float* input    = bufferOffset + pinInput.getBuffer();
//	float* pulseWidth= bufferOffset + pinPulseWidth.getBuffer();
	float* audioOut = bufferOffset + pinAudioOut.getBuffer();

	for( int s = sampleFrames; s > 0; --s )
	{
		float i = *input++;

		// 1 pole low pass
		filter_y1n = i + filter_l * ( filter_y1n - i );
		// i = i - filter_y1n; // high pass
		i = filter_y1n;

		// Binary pulse for each peak.
		bool newDetectorOut;

		if( detectorOut_ )
		{
			newDetectorOut = i > currentNegativePeak_;
		}
		else
		{
			newDetectorOut = i > currentPeak_;
		}

		if( detectorOut_ != newDetectorOut )
		{
			detectorOut_ = newDetectorOut;

			if( newDetectorOut )
			{
				time_t t = time_ + sampleFrames - s;
				int period = (int) (t - previousPeakTime_);
				previousPeakTime_ = t;

				outputMemoryPeriod[outputMemoryIdx] = (float) period;
				outputMemoryIdx = (outputMemoryIdx + 1) % outputMemorySize;

				// Peak detectors need to react more quickly than overall pitch detection, else pitch detector takes too long to settle.
				// Take average of last 3 cycles.
				const int peakAverageCount = 3;
				int idx = outputMemoryIdx;
				float averagePeriod = 0.0f;
				for( int c = 0 ; c < peakAverageCount ; ++c )
				{
					idx = (idx + outputMemorySize - 1) % outputMemorySize;
					averagePeriod += outputMemoryPeriod[idx];
				}

				averagePeriod /= (float) peakAverageCount;
				// impose sanity on peak decay rate.
				averagePeriod = min( averagePeriod, getSampleRate() * 0.05f ); // prevent divide-by-zero. 20Hz minimum freq.
				averagePeriod = max( averagePeriod, getSampleRate() * 0.0005f ); // maximum 2KHz.

				//const float decayRate = logf( 0.35f ); // Decay 65% of amplitude over 1 period.
				const float decayRate = logf( 0.50f ); // Guess - need to sytematically optimise this with real guitar samples.
				PeakDetectorDecay_ = expf( decayRate / averagePeriod );

/*
				if( pinScale == 5 ) // unfiltered.
				{
					double Hz = getSampleRate() / period;
					float unfiltered;
					double volts = log( Hz ) / 0.69314718055994529 - 3.7813597135246599;
					unfiltered = (float) volts * 0.1f;
					output_ = unfiltered;
				}
*/
			}

			if( pinScale == 4 ) // pulses.
			{
				output_ = 0.5f * ((float) !newDetectorOut - 0.5f);
			}
		}

		// Peak followers,
		if( i < currentPeak_ )
		{
			currentPeak_ *= PeakDetectorDecay_;
			currentPeak_ = max( currentPeak_, 0.0f );
		}
		currentPeak_ = max( currentPeak_, i );

		if(currentNegativePeak_ < i)
		{
			currentNegativePeak_ *= PeakDetectorDecay_;
			currentNegativePeak_ = min( currentNegativePeak_, 0.0f );
		}
		currentNegativePeak_ = min( currentNegativePeak_, i );

		if( pinScale == 2 ) // Peaks.
		{
			output_ = currentPeak_;
		}
		if( pinScale == 3 ) // -Peaks.
		{
			output_ = currentNegativePeak_;
		}

		*audioOut++ = output_;

		//prevInput = i;
		//prevPeak = currentPeak_;

		if( pinScale == 4 ) // pulses.
		{
			output_ = 0.0f;
		}
	}

	time_ += sampleFrames;

	//prevInput_ = prevInput;
	//prevPeak_ = prevPeak;

	updateCounter_ -= sampleFrames;
	if( updateCounter_ < 0 )
	{
		updateCounter_ += (int) getSampleRate() / GuiUpdateRateHz;
		calcAveragePitch();
	}
}

// - QDSS Windowed Sinc ReSampling subroutine.

// function parameters
// : x      = new sample point location (relative to old indexes)
//            (e.g. every other integer for 0.5x decimation)
// : indat  = original data array
// : alim   = size of data array
// : fmax   = low pass filter cutoff frequency
// : fsr    = sample rate
// : wnwdth = width of windowed Sinc used as the low pass filter

// resamp() returns a filtered new sample point

float resamp( float x, float* indat, int indatSize, float fmax, float fsr, int wnwdth)
{
    float r_g,r_w,r_a,r_snc,r_y; // some local variables
    r_g = 2 * fmax / fsr;            // Calc gain correction factor
    r_y = 0;
    for( int i = -wnwdth/2 ; i < (wnwdth/2)-1 ; ++ i) // For 1 window width
    {
        int j = (int) (x + i);           // Calc input sample index

        // calculate von Hann Window. Scale and calculate Sinc
        r_w     = 0.5f - 0.5f * cosf(2.0f * (float)M_PI * (0.5f + (j - x)/wnwdth) );
        r_a     = 2.0f * (float)M_PI * (j - x) * fmax/fsr;
        r_snc   = 1;

        if( r_a != 0.0 )
        {
            r_snc = sin(r_a) / r_a;
        }

        if( (j >= 0) && (j < indatSize) )
        {
            r_y = r_y + r_g * r_w * r_snc * indat[j];
        }
    }
    return r_y;                   // Return new filtered sample
}

float ExtractPeriod(vector<float>& sample, int autocorrelateto = 0, int slot = 0 )
{
	int alg = 1; // 1-Autocorrelation, 2-Cuberoot Autocorrelation, 3-Enhanced Autocorrelation
	bool sutractTimeDoubled = true;

	const int minimumPeriod = 10; // wave less than 16 samples not much use.
	const int correlateCount = 1024;
    const int n = correlateCount * 2;

#ifdef _DEBUG
	bool debugAutocorrelate = slot == -1;
	float debugtrace[5][n];
	if( debugAutocorrelate )
	{
		memset( debugtrace, 0, sizeof(debugtrace) );
	}
#endif

	// New - FFT based autocorrelate with low-pass filter.

    float realData[correlateCount * 2 +3];

    // Copy wave to temp array for FFT.
	autocorrelateto = max(0, min(autocorrelateto, (int)sample.size() - correlateCount )); // can't correlate too near end of sample.
    int tocopy = min(correlateCount, (int)sample.size() - autocorrelateto);
    for (int s = 0; s < tocopy; s++)
    {
        realData[s + 1] = sample[(int)autocorrelateto + s];

#ifdef _DEBUG
		if( debugAutocorrelate )
		{
			debugtrace[0][s] = realData[s + 1];
		}
#endif

	}

    // Zero-pad.
    for (int s = tocopy; s < n; s++)
    {
        realData[s + 1] = 0.0f;
    }

    // Perform forward FFT.
    realft(realData, n, 1);

    float autoCorrelation[n + 2];
    int no2 = n / 2;

	// Computer power.
    for (int i = 1; i < n ; i+=2)
    {
        autoCorrelation[i] = (realData[i] * realData[i] + realData[i+1] * realData[i+1]) / no2;
        autoCorrelation[i+1] = 0.0f; //  (realData[i] * dum - realData[i - 1] * realData[i]) / no2;
    }

	if (alg == 1)
	{
		for (int i = 0; i < n; i++)
			autoCorrelation[i] = sqrtf(autoCorrelation[i]);
	}
 
	if (alg == 2 || alg == 3) {
		// Tolonen and Karjalainen recommend taking the cube root
		// of the power, instead of the square root
 
		for (int i = 0; i < n; i++)
			autoCorrelation[i] = powf(autoCorrelation[i], 1.0f / 3.0f);
	}
 

	// Determin highest harmonic.
	float maxSpectrum = 0.0f;
    for (int i = 1; i < n; i += 2)
    {
		maxSpectrum = std::max( maxSpectrum, autoCorrelation[i] );
	}

    //low-pass filter in freq domain to deemphasis high-harmonics which confuse pitch-detection, and minimise unwanted 'ringing'
	float noiseGate = maxSpectrum * 0.2f;
	if( alg == 3 )
	{
		noiseGate = 0.0f; // don't apply
	}

	int firstSpectralPeak = n;
    float window = 1.0f;
    for (int i = 1; i < n; i += 2)
    {
		if( firstSpectralPeak == n && autoCorrelation[i] > maxSpectrum * 0.5f )
		{
			firstSpectralPeak = i;
		}
		/* terrible in presence of noise
        const float curvyness = 50.0f;
		int window_peak = 12;
		if( i > window_peak )
		{
			window = (1.0f + 1.0f / curvyness) / (1.0f + (curvyness - 1.0f) * 2.0f * (i-window_peak) / (float)n) - 1.0f / curvyness; // 1/r curve. 2 * i cuts it off at 50%
		}
		else
		{
			window = i / (float) window_peak;
		}
		*/
		/* basic
		int lp = 5;
		int hp = 500;
		window = 0.0f;
		if( i > lp && i < hp )
		{
			window = 1.0f;
		}
		*/
#ifdef _DEBUG
		if( debugAutocorrelate )
		{
            debugtrace[1][i] = autoCorrelation[i];
            debugtrace[1][i+1] = debugtrace[1][i]; // fill in zeros.
            debugtrace[2][i] = window;
            debugtrace[2][i+1] = window;
		}
#endif
        autoCorrelation[i] *= window;
		if( noiseGate > autoCorrelation[i] )
		{
			autoCorrelation[i] = 0.0f;
		}

		if( i > firstSpectralPeak ) // attenuate high-freq rubbish after first signifigant harmonic.
		{
			window *= 0.95f;
		}
    }
    // even indices are zero.

    realft(autoCorrelation, n, -1);

	float maxVal = 0.0f;
	float minVal = numeric_limits<float>::max( ); // double.MaxValue;
	float normalise = autoCorrelation[1];
	for (int j = 1; j < correlateCount; ++j)
	{
//		autoCorrelation[j] = autoCorrelation[j] / normalise; // normalise.
//       autoCorrelation[j] *= correlateCount / max( 50.0f, (float) (correlateCount - j)); // compensated for drop-off, but not are far right, else goes extreme. RAMPS LOUDER TOWARD RIGHT.
		float x = (float) j / (float) correlateCount; // compensated for drop-off.
		float inverseWindow = 1.0f - x * 0.85f; // compensate for linear drop-off, also attenuate correlation toward right to favour lowest harmonic at left.
		autoCorrelation[j] /= (inverseWindow * normalise);

		const float tailoff = 0.6f;
		if( x > tailoff )	// tail-off last part to avoid numberical errors as correllation becomes tiny, and to 'filter' out -ve octave erros a little. 
		{
			float fade = (x-tailoff) / (1.0f - tailoff);
			autoCorrelation[j] *= 1.0f - fade * fade;
		}
        //autoCorrelation[j] *= correlateCount / max( 50.0f, (float) (correlateCount + 200 - j)); // compensated for drop-off, but not at far right, else goes extreme.
//		float window = 0.5f + 0.5f * (float)cos(j * M_PI / correlateCount); // hanning.
//		autoCorrelation[j] = min( 1.5f, autoCorrelation[j]); // limit far right weirdness.

		minVal = min( minVal, autoCorrelation[j] );
		maxVal = max( maxVal, autoCorrelation[j] );
	}

	if( alg == 3 || sutractTimeDoubled ) // Enhanced
	{
		maxVal = 0.0f;
		minVal = numeric_limits<float>::max( ); // double.MaxValue;
		// Subtract a time-doubled signal (linearly interp.) from the original
		// (clipped) signal
  
		for ( int i = correlateCount - 1; i > 1; --i )
		{
			int halfPeriod = 1 + (i-1) / 2; // fix off-by-one of FFT.
			if ((i % 2) == 0)
				autoCorrelation[i] = std::max(-10.0f, autoCorrelation[i]) - autoCorrelation[halfPeriod];
			else
				autoCorrelation[i] = std::max(-10.0f, autoCorrelation[i]) -((autoCorrelation[halfPeriod] + autoCorrelation[halfPeriod + 1]) / 2);

			autoCorrelation[i] = std::max(-10.0f, autoCorrelation[i]);

			minVal = min( minVal, autoCorrelation[i] );
			maxVal = max( maxVal, autoCorrelation[i] );
		}
	}

#ifdef _DEBUG
		if( debugAutocorrelate )
		{
			for (int j = 1; j < correlateCount; ++j)
			{
				debugtrace[3][j] = autoCorrelation[j];
			}
		}
#endif	

    // Now search auto-correlation for first dip.
    float upperThreshhold = maxVal - 0.66f * (maxVal-minVal);

    int state = 0; // 0 - before first peak, 1 - after first peak, 2 
    float bestScore = numeric_limits<float>::max( ); //double.MaxValue;
    float bestPeriod = 0.0f;

	int i = 0;
    for (i = minimumPeriod; i < correlateCount; ++i)
    {
        if (autoCorrelation[i] < upperThreshhold) // found beginning of dip.
        {
			break;
		}
	}
	// crawl down pit.
	while( autoCorrelation[i+1] < autoCorrelation[i] )
	{
		++i;
	}

#ifdef _DEBUG
		if( debugAutocorrelate )
		{
			_RPT2(_CRT_WARN, " 1st dip starts %d (thresh %f)\n", i, upperThreshhold );
		}
#endif
		
	// search for highest peak *after* this first peak (to ignore perfect dip at time zero).
	// gives a more reasonable version for low-val.
	maxVal = 0;
	for (int j = i; j < correlateCount; ++j)
	{
		maxVal = max( maxVal, autoCorrelation[j] );
	}

	// Recalc threshholds for remainder of correlation.
	float lowerThreshhold = minVal + 0.33f * (maxVal-minVal);
    upperThreshhold = maxVal - 0.3f * (maxVal-minVal);

	// Thresholds have shifted, so may need to 'climb' peak a little further (else we might inadvertantly be below new lower threshold and think we are heading down already)..
	for (; i < correlateCount; ++i)
	{
		if (autoCorrelation[i] > upperThreshhold ) // going into a dip. done looking for first peak.
		{
			break;
		}
	}
#ifdef _DEBUG
		if( debugAutocorrelate )
		{
			_RPT2(_CRT_WARN, " 1st peak starts %d (thresh %f)\n", i, upperThreshhold );
		}
#endif

	// search for top of peak.
	float best = 0;
	for (int j = i; j < correlateCount; ++j)
	{
		if (autoCorrelation[j] > best ) // lowest point so far.
		{
			best = autoCorrelation[j];
			bestPeriod = (float) j;
		}
//		if (autoCorrelation[j] < lowerThreshhold ) // end of dip. done.
		// Since this is our best estimate we should be confidant there are no bigger dips nearby (within 50% of our best-guess period).
		// any such dip would be a better guess.
		if ( j > (int) ( bestPeriod + bestPeriod/2) ) // should be safe to search up to half way to next cycle.
		{
			break;
		}
	}


#ifdef _DEBUG
		if( debugAutocorrelate )
		{
			_RPT2(_CRT_WARN, " best peak %f (thresh %f)\n", bestPeriod, lowerThreshhold );
		}
#endif

	// At this point we have found the first low dip after the first big peak. This is usually the correct result (except for piano sample).

/* not so good on noisey samples. great on very CLEAN instrument samples.
	// IMPROVED. Search entire remainder of impulse.
	int	prevBestPeriod = bestPeriod;
	for (int j = i; j < correlateCount; ++j)
	{
		float score = autoCorrelation[j]; // + 0.1f * (maxVal-minVal) * (j-bestPeriod) / bestPeriod; // add penalty of 10% at best * 2.
		if (score < best ) // lowest point so far.
		{
			best = score;
			bestPeriod = j;
		}
	}
#ifdef _DEBUG
		if( debugAutocorrelate && bestPeriod != prevBestPeriod )
		{
			_RPT1(_CRT_WARN, " BETTER dip center %d nearby\n", bestPeriod );
		}
#endif
	*/


/*
        else
        {
            if ( score < bestScore ) // ..now look for lowest dip.
            {
				if ( score < (bestScore - sucessWindow) || abs(bestPeriod-i) < minimumPeriod ) // Dip is better if lower than prev best and close to prev best, or if further right, but at least 5% better.
				{
					bestScore = score;
					bestPeriod = i;
				}
			}
*/
/*			Failed when first dip (at zero) way lower than any subsequent one.
            if (state == 1)
            {
                if (autoCorrelation[i] < lowerThreshhold) // ..now look for valley
                {
                    state = 2;
                }
            }
            else
            {
                if (autoCorrelation[i] > upperThreshhold) // found 2nd peak, done.
                {
                    break;
                }
                if (autoCorrelation[i] < bestScore)
                {
                    bestScore = autoCorrelation[i];
                    bestPeriod = i;
                }
            }
        }
    }
*/
		// now interpolate lowest fractional point in dip.
		// fine-tune. Goldren search.
		{
			// SINC interpolator.
			const int sincSize = 10;
			const float filtering = 0.5f;


			float bx = bestPeriod;
			float ax = bx - 1.0f;
			float cx = bx + 1.0f;
			float tol = 0.1f; // tollerance. How accurate the result needs to be.
			float *xmin = &bestPeriod;
			/*
			float golden(float ax, float bx, float cx, float (*f)(float), float tol, float *xmin)
			Given a function f, and given a bracketing triplet of abscissas ax, bx, cx (such that bx is
			between ax and cx, and f(bx) is less than both f(ax) and f(cx)), this routine performs a
			golden section search for the minimum, isolating it to a fractional precision of about tol. The
			abscissa of the minimum is returned as xmin, and the minimum function value is returned as
			golden, the returned function value.
			*/
			float f1,f2,x0,x1,x2,x3;
			x0=ax; // At any given time we will keep track of four
			x3=cx; // points, x0,x1,x2,x3.
			if (fabs(cx-bx) > fabs(bx-ax)) { //Make x0 to x1 the smaller segment,
				x1=bx;
				x2=bx+C*(cx-bx); //and ll in the new point to be tried.
			} else {
				x2=bx;
				x1=bx-C*(bx-ax);
			}
			//The initial function evaluations. Note that
			// we never need to evaluate the function
			// at the original endpoints.
			//f1=(*f)(x1);
			f1 = -resamp(x1, autoCorrelation, n, filtering, 1.0, sincSize);

			//f2=(*f)(x2);
			f2 = -resamp(x2, autoCorrelation, n, filtering, 1.0, sincSize);

// hmm, seems to depend on magnitude of x. i.e. more accurate near origin?			while (fabs(x3-x0) > tol*(fabs(x1)+fabs(x2))) {
			while (fabs(x3-x0) > tol) {
				if (f2 < f1) { // One possible outcome,
					SHFT3(x0,x1,x2,R*x1+C*x3) // its housekeeping,
					//SHFT2(f1,f2,(*f)(x2)) // and a new function evaluation.
					SHFT2(f1,f2, -resamp(x2, autoCorrelation, n, filtering, 1.0, sincSize) ) // and a new function evaluation.
				} else { // The other outcome,
					SHFT3(x3,x2,x1,R*x2+C*x0)
					//SHFT2(f2,f1,(*f)(x1)) // and its new function evaluation.
					SHFT2(f2,f1,-resamp(x1, autoCorrelation, n, filtering, 1.0, sincSize)) // and its new function evaluation.
				}
//				_RPT0(_CRT_WARN, "*" );
			} // Back to see if we are done.
			if (f1 < f2) { // We are done. Output the best of the two
				*xmin=x1; // current values.
				best = f1;
			} else {
				*xmin=x2;
				best = f2;
			}
//			_RPT2(_CRT_WARN, "\nALT: %f, %f\n", closest, *xmin );
		}

#ifdef _DEBUG
		if( debugAutocorrelate )
		{
			// Normalise spectrum.
			maxVal = 0.001f;
			for (int j = 0; j < correlateCount; ++j)
			{
				maxVal = max( maxVal, debugtrace[1][j] );
			}
			maxVal = 1.0f / maxVal;
			for (int j = 0; j < correlateCount; ++j)
			{
				debugtrace[1][j] *= maxVal;

				debugtrace[3][j] = min(6.0f, debugtrace[3][j]); // clip Correlation
			}
/*
			_RPT0(_CRT_WARN, "Sample, FFT, Window, Correlation \n" );
			for (int j = 1; j < correlateCount; ++j)
			{
				for (int i = 0; i < 4; ++i)
				{
					_RPT1(_CRT_WARN, "%f, ", debugtrace[i][j] );
				}
				_RPT0(_CRT_WARN, "\n" );
			}
//			_RPT1(_CRT_WARN, "Period %f\n", bestPeriod );
*/
		}
#endif

	// Seems to be off-by-1 possibly cause of fft operates off-by-one.
	bestPeriod -= 1.0f;

	//Debug.WriteLine("Period Est: " + bestPeriod);
    //Debug.WriteLine("**************************");
//#ifdef DEBUG_PITCH_DETECT
	_RPT3(_CRT_WARN, "S%4d Period %f, %fHz\n", slot, bestPeriod, 44100.0 / bestPeriod );
//#endif
    return bestPeriod;
}

void PitchDetector::subProcess2( int bufferOffset, int sampleFrames )
{
	float* input    = bufferOffset + pinInput.getBuffer();
	float* audioOut = bufferOffset + pinAudioOut.getBuffer();

	for( int s = sampleFrames; s > 0; --s )
	{
		float i = *input++;
		i = hiPassFilter_.processHiPass( i ); // FFT Autocorrelate gets thrown off by DC compnents when signal is small.

		buffer[bufferIdx++] = i;
		if( bufferIdx >= fftSize )
		{
			bufferIdx = 0;
			float period = ExtractPeriod(buffer, 0, frameNumber++);
			outputMemoryPeriod[outputMemoryIdx] = period;

			outputMemoryIdx = (outputMemoryIdx + 1) % outputMemorySize;

			if( period > 3 )
			{
				medianFilter.process(period);
			}

			if( pinScale == 5 ) // unfiltered.
			{
				if( period < 2 )
				{
					output_ = 0.0f;
				}
				else
				{
					double Hz = getSampleRate() / period;
					float unfiltered;
					double volts = log( Hz ) / 0.69314718055994529 - 3.7813597135246599;
					unfiltered = (float) volts * 0.1f;
					output_ = unfiltered;
				}
			}

			if( pinScale == 7 ) // median.
			{
				double Hz = getSampleRate() / medianFilter.output();
				float unfiltered;
				double volts = log( Hz ) / 0.69314718055994529 - 3.7813597135246599;
				unfiltered = (float) volts * 0.1f;
				output_ = unfiltered;
			}
		}

		*audioOut++ = output_;
	}

	updateCounter_ -= sampleFrames;
	if( updateCounter_ < 0 )
	{
		updateCounter_ += (int) getSampleRate() / GuiUpdateRateHz;
		calcAveragePitch();
	}
}

void PitchDetector::calcAveragePitch( )
{
	// Collect the last 100ms of captured period information.
	PeriodSamples.clear();

	// Want needle updated about 10 times per second.
	// Count number of transitions in last 1/10th second.
	int cycles = 0;
	float time = (getSampleRate() / (float) GuiUpdateRateHz);
	int idx = outputMemoryIdx;
	while( time > 0.0f && cycles < outputMemorySize )
	{
		idx = (idx + outputMemorySize - 1) % outputMemorySize;
		PeriodSamples.push_back( outputMemoryPeriod[idx] );
		time -= outputMemoryPeriod[idx];
		++cycles;
	}

	// Median.
	// float median = select( cycles/2, cycles - 1, PeriodSamples );

	// Sort periods.
	sort( PeriodSamples.begin(), PeriodSamples.end() );

	// Average periods (omitting outliers).
/* misses 'good' outliers
	int from = cycles / 3;
	int to = cycles - from;

	float averagePeriod = 0.0f;
	for( auto it = PeriodSamples.begin() + from ; it < PeriodSamples.begin() + to ; ++it )
	{
		averagePeriod += (float) *it;
	}
	averagePeriod /= (float) (to - from);
	*/

	float averagePeriod = 0.0f;
	float median = (float) PeriodSamples[cycles / 2];
	int count = 0;
	for( auto it = PeriodSamples.begin() ; it < PeriodSamples.end() ; ++it )
	{
		if( *it < median * 1.1f && *it > median * 0.9f )
		{
			averagePeriod += (float) *it;
			++count;
		}
	}

	averagePeriod /= (float) count;

	// Calulate pitch from period.
	double Hz = getSampleRate() / averagePeriod;

	float pitch;
	if( pinScale == 1 ) // Khz.
	{
		pitch = (float) Hz * 0.001f;
	}
	else // Octaves
	{
		double volts = log( Hz ) / 0.69314718055994529 - 3.7813597135246599;
		pitch = (float) volts * 0.1f;
	}

	// Estimate noise.
	// Varience.
	float varience = 0.0f;
	for( auto it = PeriodSamples.begin() ; it < PeriodSamples.end() ; ++it )
	{
		float dif = ((float) *it) - averagePeriod;
		varience += dif * dif; // difference from mean squared.
	}
	varience /= (float) cycles;
	float standard_deviation = sqrtf( varience );

	float noiseEstimate = 1.0f; // normalise est as fraction of center period.
	if( averagePeriod > 4.0f ) // avoid divide-by-zero
	{
		noiseEstimate = standard_deviation / averagePeriod;
	}

	// If detected frequency is less than 2 cycles (40Hz) mark as invalid (noisy).
	if( PeriodSamples.size() < 3 )
	{
		noiseEstimate = 1.0f;
	}
	_RPTW4(_CRT_WARN, L" Cycles %3d, Average %.2f, Noise Est %.2f Pitch %.3f\n", cycles, averagePeriod, noiseEstimate, pitch );

	if( (pinScale == 0 || pinScale == 1) && noiseEstimate < 0.3f ) // Pitch.
	{
		output_ = pitch;
	}

//	_RPTW4(_CRT_WARN, L" Cycles %3d, Average %.2f, Noise Est %.2f Pitch %.3f\n", cycles, averagePeriod, noiseEstimate, pitch );

	// Switch offf output when signal gets noisy.
	if( pinScale == 0 ) // official output.
	{
		if( noiseEstimate > 0.3f ) // official output. pure noise = 0.1
		{
//			output_ = -1.0f;
		}
	}

	if( pinScale == 6 ) // noise estimate.
	{
		output_ = noiseEstimate *0.1f;
	}
	//if( pinScale == 7 ) // Median filter.
	//{
	//	output_ = 0.0f;
	//}
	if( pinScale == 8 ) //Simple Average.
	{
		output_ = 0.0f;
	}
}

void PitchDetector::onSetPins( void )
{
	// Check which pins are updated.
	if( pinInput.isStreaming() )
	{
	}
	if( pinPulseWidth.isStreaming() )
	{
	}
	if( pinScale.isUpdated() )
	{
	}

	// Set state of output audio pins.
	pinAudioOut.setStreaming( true );

	// Set processing method.
	SET_PROCESS( &PitchDetector::subProcess2 );
//old	SET_PROCESS( &PitchDetector::subProcess );
}

