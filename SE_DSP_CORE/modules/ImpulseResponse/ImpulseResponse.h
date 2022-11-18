#ifndef IMPULSERESPONSE_H_INCLUDED
#define IMPULSERESPONSE_H_INCLUDED

#include "../se_sdk3/mp_sdk_audio.h"

#define FFT_SIZE 1024
#define LOG2_FFTSIZE 8

#define SP_OCTAVES 11
#define BINS_PER_OCTAVE 48
// lowest Hz
#define SPECTRUM_LO 10

class ImpulseResponse : public MpBase
{
public:
	ImpulseResponse( IMpUnknown* host );
	void subProcess( int bufferOffset, int sampleFrames );
	void subProcessIdle( int bufferOffset, int sampleFrames );
	virtual void onSetPins();

private:
	AudioInPin pinImpulsein;
	AudioInPin pinSignalin;
	IntInPin pinFreqScale;
	BlobOutPin pinResults;
	FloatOutPin pinSampleRateToGui;

	float buffer[FFT_SIZE];

private:
	void realft(float data[], unsigned int n, int isign);
	void four1(float data[], unsigned int nn, int isign);
	short scale_type;
	float* input_ptr;
	int m_idx;
	void printResult();
	float window[FFT_SIZE];
	int m_spectrum_size;
};

#endif

