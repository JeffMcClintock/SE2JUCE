#include "./FreqAnalyser.h"
#include "../shared/xplatform.h"
#include "../shared/real_fft.h"
#define _USE_MATH_DEFINES
#include <math.h>

SE_DECLARE_INIT_STATIC_FILE(FreqAnalyser2)
REGISTER_PLUGIN2(FreqAnalyser, L"SE Freq Analyser2");

/* TODO !!!
	properties->flags = UGF_VOICE_MON_IGNORE;
*/

FreqAnalyser::FreqAnalyser() :
index_( 0 )
,sleepCount( -1 )
{
	initializePin(pinSamplesA);
	initializePin(pinSignalA);
	initializePin(pinCaptureSize);
}

int32_t FreqAnalyser::open()
{
	MpBase2::open();	// always call the base class

	setSubProcess( &FreqAnalyser::subProcess );

	setSleep( false ); // disable automatic sleeping.

	return gmpi::MP_OK;
}

// wait for waveform to restart.
void FreqAnalyser::waitAwhile(int sampleFrames)
{
	timeoutCount_ -= sampleFrames;
	if(timeoutCount_ < 0)
	{
		index_ = 0;
		setSubProcess(&FreqAnalyser::subProcess);
	}
}

void FreqAnalyser::subProcess(int sampleFrames)
{
	// get pointers to in/output buffers
	auto signala = getBuffer(pinSignalA);

	for (int s = sampleFrames; s > 0 && index_ < captureSamples; s--, ++index_)
	{
		assert(index_ < captureSamples);
		resultsA_[index_] = *signala++;
	}

	if(index_ == captureSamples )
	{
		sendResultToGui(0);
	}
}

void FreqAnalyser::sendResultToGui(int block_offset)
{
	if (captureSamples < 100) // latency may mean this pin is initially zero for a short time.
		return;

	auto FFT_SIZE = captureSamples;

	// apply window
	for (int i = 0; i < FFT_SIZE; i++)
	{
		resultsA_[i] = window[i] * resultsA_[i];
	}

	realft(resultsA_.data() - 1, FFT_SIZE, 1);
	// calc power
	float nyquist = resultsA_[1]; // DC & nyquist combined into 1st 2 entries

	// Always divide FFT results b n/2, also compensate for 50% loss in window function.
	float inverseN = 4.0f / FFT_SIZE;
	for (int i = 1; i < FFT_SIZE / 2; i++)
	{
		float a = resultsA_[2 * i];
		float b = resultsA_[2 * i + 1];
		resultsA_[i] = a * a + b * b; // remainder of dB calculation left to GUI.
	}

//	resultsA_[0] = fabs(resultsA_[0]) * inverseN * 0.5f; // DC component is divided by 2
	resultsA_[0] = fabs(resultsA_[0]) * 0.5f; // DC component is divided by 2
	resultsA_[FFT_SIZE / 2] = fabs(nyquist);

	// GUI does a square root on everything, compensate.
	resultsA_[0] *= resultsA_[0];
	resultsA_[FFT_SIZE / 2] *= resultsA_[FFT_SIZE / 2];

	// FFT produces half the original data.
	const int datasize = (1 + resultsA_.size() / 2 ) * sizeof(resultsA_[0]);

	// Add an extra member communicating sample-rate to GUI.
	// !! overwrites nyquist value?
	resultsA_[resultsA_.size() / 2] = getSampleRate();

	pinSamplesA.setValueRaw( datasize, resultsA_.data() );
	pinSamplesA.sendPinUpdate( block_offset );

	// waste of CPU to send updates more often than GUI can repaint,
	// wait approx 1/10th second between captures.
	timeoutCount_ = (int)getSampleRate() / 10;
	setSubProcess(&FreqAnalyser::waitAwhile);

	// if inputs aren't changing, we can sleep.
	if( sleepCount > 0 )
	{
		if( --sleepCount == 0 )
		{
			setSubProcess(&FreqAnalyser::subProcessNothing);
			setSleep( true );
		}
	}
}

void FreqAnalyser::onSetPins()  // one or more pins_ updated.  Check pin update flags to determine which ones.
{
	setSleep( false );

	if (pinCaptureSize.isUpdated())
	{
		index_ = 0;
		captureSamples = pinCaptureSize;
		resultsA_.assign(captureSamples, 0.0f);

		window.resize(captureSamples);

		// create window function
		for (int i = 0; i < captureSamples; i++)
		{
			// sin squared window
			float t = sin(i * ((float)M_PI) / captureSamples);
			window[i] = t * t;
		}
	}

	if( pinSignalA.isStreaming() )
	{
		sleepCount = -1; // indicates no sleep.
	}
	else
	{
		// need to send at least 2 captures to ensure flat-line signal captured.
		// Current capture may be half done.
		sleepCount = 2;
	}

	// Avoid resetting capture unless module is actually asleep.
	if( getSubProcess() == &FreqAnalyser::subProcessNothing )
	{
		index_ = 0;
		setSubProcess(&FreqAnalyser::subProcess);
	}
}


