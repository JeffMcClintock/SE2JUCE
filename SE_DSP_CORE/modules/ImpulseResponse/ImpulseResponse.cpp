#include "./ImpulseResponse.h"
#define _USE_MATH_DEFINES
#include <math.h>

const int LOG2_MAXFFTSIZE = 15;
const int MAXFFTSIZE = 1 << LOG2_MAXFFTSIZE;
#define time_delay FFT_SIZE

SE_DECLARE_INIT_STATIC_FILE(ImpulseResponse);

REGISTER_PLUGIN ( ImpulseResponse, L"SE Impulse Response" );

ImpulseResponse::ImpulseResponse( IMpUnknown* host ) : MpBase( host )
	,m_idx(0)
{
	// Register pins.
	initializePin( 0, pinImpulsein );
	initializePin( 1, pinSignalin );
	initializePin( 2, pinFreqScale );
	initializePin( 3, pinResults );
	initializePin( 4, pinSampleRateToGui );
	
	m_spectrum_size = SP_OCTAVES * BINS_PER_OCTAVE;

	// create window function
	float c = (float) M_PI / FFT_SIZE;
	for(int i=0; i < FFT_SIZE; i++)
	{
		// sin squared window
		float t = sinf( i * c );
		window[i] = t * t;
	}
}

// do nothing while UI updates
void ImpulseResponse::subProcessIdle(int bufferOffset, int sampleFrames)
{
	float* impulse = bufferOffset + pinImpulsein.getBuffer();

	int last = bufferOffset + sampleFrames;
	int to_pos = bufferOffset;
	int cur_pos = bufferOffset;

	// Time till next impulse
	while( *impulse < 1.0f && to_pos < last )
	{
		++to_pos;
		++impulse;
	}

	if( to_pos != last )
	{
		subProcess( to_pos, sampleFrames + cur_pos - to_pos );
		SET_PROCESS(&ImpulseResponse::subProcess);
		return;
	}
}

void ImpulseResponse::subProcess( int bufferOffset, int sampleFrames )
{
	// get pointers to in/output buffers.
	float* signal	= bufferOffset + pinSignalin.getBuffer();

    int count = (std::min)( sampleFrames, FFT_SIZE - m_idx );

	assert( m_idx >= 0 && m_idx < FFT_SIZE );

	while( count-- > 0 )
	{
		buffer[m_idx++] = *signal++;
	}

	if( m_idx >= FFT_SIZE )
	{
		printResult();
		SET_PROCESS(&ImpulseResponse::subProcessIdle);
	}
}

void ImpulseResponse::onSetPins()
{
	// Set processing method.
	SET_PROCESS(&ImpulseResponse::subProcess);
}

void ImpulseResponse::printResult()
{
	m_idx = 0;

	pinSampleRateToGui.setValue( getSampleRate(), 0 );

	if( pinFreqScale == 0 ) // Impulse Response
	{
		// send an odd number of samples to signal it's the raw impulse.
		pinResults.setValueRaw( sizeof(buffer[0]) * FFT_SIZE/2 - 1, &buffer );
		pinResults.sendPinUpdate( 0 );
		return;
	}

	// no window (square window).
	// Calculate FFT.
	realft( &(buffer[-1]), FFT_SIZE, 1 );

	// calc power. Get magnitude of real and imaginary vectors.
	float nyquist = buffer[1]; // DC & nyquist combined into 1st 2 entries

	//	realArray[1] = 0.0f;
	for(int i=1; i < FFT_SIZE/2; i++)
	{
		float power = buffer[2*i] * buffer[2*i] + buffer[2*i+1] * buffer[2*i+1];
		// square root
		power = sqrtf(power);
		buffer[i] = power;
	}

	buffer[0] = fabs(buffer[0]) / 2.f; // DC component is divided by 2
	buffer[FFT_SIZE/2] = fabs(nyquist);
	// convert results to log scale
	float octave_step = (float) SP_OCTAVES / (float) m_spectrum_size;
	float octave_center = 0.f;
	float fft_max_khz = getSampleRate() / 2.f;

	// convert level to db
	int m = 0;
	constexpr float db_scale_const = 105.0f / (float) FFT_SIZE;
	float peak = - 100;
	float tot = 0;

	// Send to GUI.
	pinResults.setValueRaw( sizeof(buffer[0]) * FFT_SIZE/2, &buffer);
	pinResults.sendPinUpdate( 0 );
}

void ImpulseResponse::realft(float data[], unsigned int n, int isign)
/*
Calculates the Fourier transform of a set of n real-valued data points. Replaces this data (which
is stored in array data[1..n]) by the positive frequency half of its complex Fourier transform.
The real-valued first and last components of the complex transform are returned as elements
data[1] and data[2], respectively. n must be a power of 2. This routine also calculates the
inverse transform of a complex data array if it is the transform of real data. (Result in this case
must be multiplied by 2/n.)
*/
{
	unsigned int i,i1,i2,i3,i4,np3;
	float c1=0.5,c2,h1r,h1i,h2r,h2i;
	float wr,wi,wpr,wpi,wtemp,theta; // Double precision for the trigonometric recurrences.
	theta=(float) (M_PI/ (n>>1)); // Initialize the recurrence.

	if (isign == 1)
	{
		c2 = -0.5;
		four1(data,n>>1,1); //The forward transform is here.
	}
	else
	{
		c2=0.5; //Otherwise set up for an inverse transform. theta = -theta;
	}

	wtemp = sin(0.5f * theta);
	wpr = -2.f * wtemp * wtemp;
	wpi = sin(theta);
	wr = 1.0f + wpr;
	wi=wpi;
	np3=n+3;

	for (i=2; i<=(n>>2); i++)
	{
		// Case i=1 done separately below.
		i4=1+(i3=np3-(i2=1+(i1=i+i-1)));
		h1r=c1*(data[i1]+data[i3]); // The two separate transforms are separated out of data.
		h1i=c1*(data[i2]-data[i4]);
		h2r = -c2*(data[i2]+data[i4]);
		h2i=c2*(data[i1]-data[i3]);
		data[i1]=h1r+wr*h2r-wi*h2i; // Here they are recombined to form	the true transform of the original real data.
		data[i2]=h1i+wr*h2i+wi*h2r;
		data[i3]=h1r-wr*h2r+wi*h2i;
		data[i4] = -h1i+wr*h2i+wi*h2r;
		wr=(wtemp=wr)*wpr-wi*wpi+wr; // The recurrence.
		wi=wi*wpr+wtemp*wpi+wi;
	}

	if (isign == 1)
	{
		data[1] = (h1r=data[1])+data[2];	// Squeeze the first and last data together
		data[2] = h1r-data[2];				// to get them all within the original array.
	}
	else
	{
		data[1]=c1*((h1r=data[1])+data[2]);
		data[2]=c1*(h1r-data[2]);
		four1(data,n>>1,-1); //This is the inverse transform for the case isign=-1.
	}
}
// --------------------------------------

#define SWAP(a,b) tempr=(a);(a)=(b);(b)=tempr

void ImpulseResponse::four1(float data[], unsigned int nn, int isign)
/*
Replaces data[1..2*nn] by its discrete Fourier transform, if isign is input as 1; or replaces
data[1..2*nn] by nn times its inverse discrete Fourier transform, if isign is input as - 1.
data is a complex array of length nn or, equivalently, a real array of length 2*nn. nn MUST
be an integer power of 2 (this is not checked for!).
*/
{
	unsigned int n,mmax,m,j,istep,i;
	float wtemp,wr,wpr,wpi,wi,theta; // Double precision for the trigonometric recurrences.
	float tempr,tempi;
	n=nn << 1;
	j=1;

	for (i=1; i<n; i+=2) // This is the bit-reversal section of the routine.
	{
		if(j > i)
		{
			SWAP(data[j],data[i]); //Exchange the two complex numbers.
			SWAP(data[j+1],data[i+1]);
		}

		m=n >> 1;

		while (m >= 2 && j > m)
		{
			j -=m;
			m >>= 1;
		}

		j += m;
	}

	// Here begins the Danielson-Lanczos section of the routine.
	mmax=2;

	while (n > mmax) //Outer loop executed log 2 nn times.
	{
		istep=mmax << 1;
		theta=isign*(6.28318530717959f/mmax); // Initialize the trigonometric recurrence.
		wtemp=sin( 0.5f * theta );
		wpr = -2.f*wtemp*wtemp;
		wpi=sin(theta);
		wr=1.0;
		wi=0.0;

		for (m=1; m<mmax; m+=2) // Here are the two nested inner loops.
		{
			for (i=m; i<=n; i+=istep)
			{
				j=i+mmax; // This is the Danielson-Lanczos formula:
				tempr=wr*data[j]-wi*data[j+1];
				tempi=wr*data[j+1]+wi*data[j];
				data[j]=data[i]-tempr;
				data[j+1]=data[i+1]-tempi;
				data[i] += tempr;
				data[i+1] += tempi;
			}

			wr=(wtemp=wr)*wpr-wi*wpi+wr; // Trigonometric recurrence.
			wi=wi*wpr+wtemp*wpi+wi;
		}

		mmax=istep;
	}
}
