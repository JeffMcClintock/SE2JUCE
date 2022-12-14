#include "real_fft.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <assert.h>

#define SWAP(a,b) tempr=(a);(a)=(b);(b)=tempr

void four1(float data[], unsigned int nn, int isign)
/*
Replaces data[1..2*nn] by its discrete Fourier transform, if isign is input as 1; or replaces
data[1..2*nn] by nn times its inverse discrete Fourier transform, if isign is input as - 1.
data is a complex array of length nn or, equivalently, a real array of length 2*nn. nn MUST
be an integer power of 2 (this is not checked for!).
*/
{
	unsigned int n,mmax,m,j,istep,i;
	double wtemp,wr,wpr,wpi,wi;//,theta; // Double precision for the trigonometric recurrences.
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

	float* sineTable = SineTables::Instance()->GetTable( nn, isign );

	while (n > mmax) //Outer loop executed log 2 nn times.
	{
		istep=mmax << 1;
		//theta=isign*(2.0*M_PI/mmax); // Initialize the trigonometric recurrence.
		
		//wtemp=sin(0.5*theta);
		//assert( (float)wtemp == *sineTable++ );
		wtemp = *sineTable++;

		wpr = -2.f*wtemp*wtemp;
		//wpi=sin(theta);
		//assert( (float)wpi == *sineTable++ );
		wpi = *sineTable++;

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

void realft(float data[], unsigned int n, int isign)
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
	double wr,wi,wpr,wpi,wtemp,theta; // Double precision for the trigonometric recurrences.
	theta=3.141592653589793/(double) (n>>1); // Initialize the recurrence.

	if (isign == 1)
	{
		c2 = -0.5;
		four1(data,n>>1,1); //The forward transform is here.
	}
	else
	{
		c2=0.5; //Otherwise set up for an inverse transform.
		theta = -theta;
	}

	wtemp=sin(0.5*theta);
	wpr = -2.f*wtemp*wtemp;
	wpi=sin(theta);
	wr=1.0+wpr;
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

SineTables* SineTables::Instance()
{
	static SineTables SineTables_;
	return &SineTables_;
}

SineTables::SineTables()
{
	InitTable( Forward1024, 1024, 1 );
	InitTable( Reverse1024, 1024, -1 );
}

void SineTables::InitTable(float* table, int nn, int isign )
{
	unsigned int n,mmax,istep;
	double theta; // Double precision for the trigonometric recurrences.

	n=nn << 1;

	// Here begins the Danielson-Lanczos section of the routine.
	mmax=2;

	float* sineTable = SineTables::Instance()->GetTable( nn, isign );

	while (n > mmax) //Outer loop executed log 2 nn times.
	{
		istep=mmax << 1;
		theta=isign*(2.0*M_PI/mmax); // Initialize the trigonometric recurrence.
		
		*sineTable++ = sin(0.5*theta);
		*sineTable++ = sin(theta);

		mmax=istep;
	}
}

float* SineTables::GetTable( int fftSize, int fftSign )
{
	if( fftSign == 1 )
		return Forward1024;
	else
		return Reverse1024;
}
