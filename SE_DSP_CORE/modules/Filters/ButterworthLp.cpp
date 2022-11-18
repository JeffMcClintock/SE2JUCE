#include "./ButterworthLp.h"

using namespace gmpi;

SE_DECLARE_INIT_STATIC_FILE(ButterworthHp); // old
SE_DECLARE_INIT_STATIC_FILE(ButterworthHs); // old
SE_DECLARE_INIT_STATIC_FILE(ButterworthHP);
SE_DECLARE_INIT_STATIC_FILE(ButterworthHS);
SE_DECLARE_INIT_STATIC_FILE(ButterworthBP);
SE_DECLARE_INIT_STATIC_FILE(ButterworthBS);
SE_DECLARE_INIT_STATIC_FILE(ButterworthLS);

REGISTER_PLUGIN2(ButterworthLp, L"SE Butterworth LP")
REGISTER_PLUGIN2(ButterworthHp, L"SE Butterworth HP")
REGISTER_PLUGIN2(ButterworthBp, L"SE Butterworth BP")
REGISTER_PLUGIN2(ButterworthBr, L"SE Butterworth BR")
REGISTER_PLUGIN2(ButterworthBandShelf, L"SE Butterworth BS")
REGISTER_PLUGIN2(ButterworthLowShelf, L"SE Butterworth LS")
REGISTER_PLUGIN2(ButterworthHighShelf, L"SE Butterworth HS")

/*
//#include <iomanip>
//#include <sstream>
std::ostringstream oss;
oss << std::endl << "Oversample Filter Coefs." << std::endl;
for(int i = 0 ; i < filter.getNumStages() ; ++i )
{
oss << "Stage " << i << std::endl << "A[] = "
<< std::setw(8) << filter[i].getA0 ()  << ", "
<< std::setw(8) << filter[i].getA1 ()  << ", "
<< std::setw(8) << filter[i].getA2 () << std::endl;

oss << "B[] = "
<< std::setw(8) << filter[i].getB0 () << ", "
<< std::setw(8) << filter[i].getB1 () << ", "
<< std::setw(8) << filter[i].getB2 () << std::endl << std::endl
;
}

#if defined(__APPLE__ )
printf( oss.str().c_str() );
#else
_RPT1(_CRT_WARN, "%s\n", oss.str().c_str() );
#endif
*/

/* FIX FOR DSPFILTERS LIBRARY

void AnalogLowShelf::design (int numPoles, double gainDb)
{
  if (m_numPoles != numPoles ||
	  m_gainDb != gainDb)
  {
	m_numPoles = numPoles;
	m_gainDb = gainDb;

	reset ();

	const double n2 = numPoles * 2;
	const double g = pow (pow (10., gainDb/20), 1. / n2);

	double gp = -1. / g;
	double gz = -g;

	// JEFF std::polar no longer accepts negative magnitude
	double flipAngle = 0.0f;
	double gp_safe;
	double gz_safe;
	if( gp < 0 )
	{
		gp_safe = -gp;
		gz_safe = -gz;
		flipAngle = M_PI;
	}
	else
	{
		gp_safe = gp;
		gz_safe = gz;
	}

	const int pairs = numPoles / 2;
	for (int i = 1; i <= pairs; ++i)
	{
	  const double theta = flipAngle + doublePi * (0.5 - (2 * i - 1) / n2);
	  addPoleZeroConjugatePairs (std::polar (gp_safe, theta), std::polar (gz_safe, theta));
	}

	if (numPoles & 1)
	  add (gp, gz);
  }
}
*/

class OctavesToHzPassband : public MpBase2
{
	FloatInPin pinPitch;
	FloatInPin pinWidthOctaves;
	FloatOutPin pinWidthHz;

public:
	OctavesToHzPassband()
	{
		initializePin(pinPitch);
		initializePin(pinWidthOctaves);
		initializePin(pinWidthHz);
	}

	void onSetPins() override
	{
		const float minOctave = 0.001f;
		const float minFreq = 0.001f; // 1Hz
		const float maxFreq = 0.499f * getSampleRate();

		// Width is in Octaves.
		auto centerFreqHz = (std::min)(maxFreq, (std::max)(minFreq, pinPitch.getValue()));
		auto octaves = (std::max)(minOctave, pinWidthOctaves.getValue());

		auto ratio = pow(2.0f, octaves);
		auto widthHz = 2.0f * (centerFreqHz * (ratio - 1)) / (ratio + 1);
		pinWidthHz = widthHz;
	}
};

class OctavesToHzPassband2 : public MpBase2
{
	FloatInPin pinPitch;
	FloatInPin pinWidthOctaves;
	FloatOutPin pinPitchOut;
	FloatOutPin pinWidthHz;

public:
	OctavesToHzPassband2()
	{
		initializePin(pinPitch);
		initializePin(pinWidthOctaves);
		initializePin(pinPitchOut);
		initializePin(pinWidthHz);
	}

	void onSetPins() override
	{
		const float minOctave = 0.001f;
		const float minFreq = 0.001f; // 1Hz
		const float maxFreq = 0.499f * getSampleRate();

		// Width is in Octaves.
		auto centerFreqHz = (std::min)(maxFreq, (std::max)(minFreq, pinPitch.getValue()));
		auto octaves = (std::max)(minOctave, pinWidthOctaves.getValue());

		auto ratio = pow(2.0f, octaves * 0.5f);

		auto highBreak = (std::min)(maxFreq, centerFreqHz * ratio);
		auto lowBreak = (std::max)(minFreq, centerFreqHz / ratio);

		pinWidthHz = highBreak - lowBreak;
		pinPitchOut = (highBreak + lowBreak) * 0.5f;
	}
};

namespace
{
	auto r = Register<OctavesToHzPassband>::withId(L"SE Octaves to Hz (Passband)");
	auto r2 = Register<OctavesToHzPassband2>::withId(L"SE Octaves to Hz (Passband)2");
}

