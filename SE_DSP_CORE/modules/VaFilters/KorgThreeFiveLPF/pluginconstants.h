#pragma once

// turn off non-critical warnings
#pragma warning(disable : 4244)//double to float
#pragma warning(disable : 4996)//strncpy
#pragma warning(disable : 4305)//double float truncation

// includes for the project
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// this #define enables the following constants form math.h
#define _MATH_DEFINES_DEFINED
/*
#define M_E        2.71828182845904523536
#define M_LOG2E    1.44269504088896340736
#define M_LOG10E   0.434294481903251827651
#define M_LN2      0.693147180559945309417
#define M_LN10     2.30258509299404568402
#define M_PI       3.14159265358979323846
#define M_PI_2     1.57079632679489661923
#define M_PI_4     0.785398163397448309616
#define M_1_PI     0.318309886183790671538
#define M_2_PI     0.636619772367581343076
#define M_2_SQRTPI 1.12837916709551257390
#define M_SQRT2    1.41421356237309504880
#define M_SQRT1_2  0.707106781186547524401
*/
#include <math.h>

// For WIN vs MacOS
// XCode requires these be defined for compatibility
#if defined _WINDOWS || defined _WINDLL
#include <windows.h>
#else // MacOS
typedef unsigned int        UINT;
typedef unsigned long       DWORD;
typedef unsigned char		UCHAR;
typedef unsigned char       BYTE;
#endif

// More #defines for MacOS/XCode
#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#ifndef itoa
#define itoa(value,string,radix)  sprintf(string, "%d", value)
#endif

#ifndef ltoa
#define ltoa(value,string,radix)  sprintf(string, "%u", value)
#endif

// a few more constants from student suggestions
#define  pi 3.1415926535897932384626433832795
#define  sqrt2over2  0.707106781186547524401 // same as M_SQRT1_2

// constants for dealing with overflow or underflow
#define FLT_EPSILON_PLUS      1.192092896e-07         /* smallest such that 1.0+FLT_EPSILON != 1.0 */
#define FLT_EPSILON_MINUS    -1.192092896e-07         /* smallest such that 1.0-FLT_EPSILON != 1.0 */
#define FLT_MIN_PLUS          1.175494351e-38         /* min positive value */
#define FLT_MIN_MINUS        -1.175494351e-38         /* min negative value */

const UINT CURRENT_PLUGIN_API = 61;
const UINT CONTROL_THEME_SIZE = 32;
const UINT PLUGIN_CONTROL_THEME_SIZE = 64;
const UINT CONTROL_THEME = 0;
const UINT PRESET_COUNT = 16; // more than 16 in VST gets sluggish depending on the client

// custom messages
#define SEND_STATUS_WND_MESSAGE		WM_USER + 3000
#define UPDATE_SLIDER_CONTROL		WM_USER + 3001
#define SEND_EDIT_CTRL_STRING		WM_USER + 3002
#define SEND_SLIDER_CTRL_VALUE		WM_USER + 3004
#define SEND_RADIO_CHECK			WM_USER + 3005
#define SEND_ASGN_BUTTON_CLICK		WM_USER + 3006
#define SEND_UPDATE_GUI				WM_USER + 3007
#define SEND_JSPROG_BUTTON_CLICK	WM_USER + 3008

const UINT MAX_JS_PROGRAM_STEPS		= 16;
const UINT MAX_JS_PROGRAM_STEP_VARS	= 7;
const UINT JS_PROGRAM_CHANGE		= 0xFFFFFFFF;

// helper for the 2-Dimensional Joystick Program array
#define JS_PROG_INDEX(x,y) ((x)+(MAX_JS_PROGRAM_STEPS*(y)))

// basic enums
enum {intData, floatData, doubleData, UINTData, nonData};
enum {JS_ONESHOT, JS_LOOP, JS_SUSTAIN, JS_LOOP_BACKANDFORTH};

// SHARED with Client App - do not remove!
#ifndef CLIENT_APP
const UINT FILTER_CONTROL_CONTINUOUSLY_VARIABLE		= 100;
const UINT FILTER_CONTROL_DIRECT_DATA_ENTRY			= 101;
const UINT FILTER_CONTROL_RADIO_SWITCH_VARIABLE		= 102;
const UINT FILTER_CONTROL_LED_METER					= 103; // RackAFX custom LED meters 5/23/11
const UINT FILTER_CONTROL_COMBO_VARIABLE			= 104; // my own Joystick 7/30/11

const UINT DETECT_MODE_PEAK		= 0;
const UINT DETECT_MODE_MS		= 1;
const UINT DETECT_MODE_RMS		= 2;
const UINT DETECT_MODE_NONE		= 3;

/*
	Function:	lagrpol() implements n-order Lagrange Interpolation

	Inputs:		double* x	Pointer to an array containing the x-coordinates of the input values
				double* y	Pointer to an array containing the y-coordinates of the input values
				int n		The order of the interpolator, this is also the length of the x,y input arrays
				double xbar	The x-coorinates whose y-value we want to interpolate

	Returns		The interpolated value y at xbar. xbar ideally is between the middle two values in the input array,
				but can be anywhere within the limits, which is needed for interpolating the first few or last few samples
				in a table with a fixed size.
*/
inline double lagrpol(double* x, double* y, int n, double xbar)
{
    int i,j;
    double fx=0.0;
    double l=1.0;
    for (i=0; i<n; i++)
    {
        l=1.0;
        for (j=0; j<n; j++)
		{
     		 if (j != i)
				 l *= (xbar-x[j])/(x[i]-x[j]);
		}
		fx += l*y[i];
    }
    return (fx);
}

inline float dLinTerp(float x1, float x2, float y1, float y2, float x)
{
	float denom = x2 - x1;
	if(denom == 0)
		return y1; // should not ever happen

	// calculate decimal position of x
	float dx = (x - x1)/(x2 - x1);

	// use weighted sum method of interpolating
	float result = dx*y2 + (1-dx)*y1;

	return result;

}


inline bool normalizeBuffer(double* pInputBuffer, UINT uBufferSize)
{
	double fMax = 0;

	for(UINT j=0; j<uBufferSize; j++)
	{
		if((fabs(pInputBuffer[j])) > fMax)
			fMax = fabs(pInputBuffer[j]);
	}

	if(fMax > 0)
	{
		for(UINT j=0; j<uBufferSize; j++)
			pInputBuffer[j] = pInputBuffer[j]/fMax;
	}

	return true;
}

#endif

// Helper Functions ------------------------------------------------------------- //
// calcLogControl: accepts a float variable from 0.0 to 1.0
// returns a log version from 0.0 to 1.0
/* y = 0.5*log10(x) + 1.0
	|
1.0 |					*
	|		*
	|	*
	| *
	|*
	|*
0.0 ------------------------
	0.0					1.0
*/
inline float calcLogControl(float fVar)
{
	return fVar == 0.0 ? 0.0 : 0.5*log10(fVar) + 1.0;
}
// ----------------------------------------------------------------------------- //

// calcAntiLogControl: accepts a float variable from 0.0 to 1.0
// returns an anti-log version from 0.0 to 1.0
/* y = e^(2x-2)
	|
1.0 |					*
	|					*
	|				  *
	|				*
	|		*
	|*
0.0 ------------------------
	0.0					1.0
*/
inline float calcAntiLogControl(float fVar)
{
	return pow(10.0, 2.0*fVar - 2);
}
// ----------------------------------------------------------------------------- //

// calcInverseLogControl:accepts a float variable from 0.0 t0 1.0
// returns an anti-log version from 1.0 to 0.0
/* y = 0.5(1-x) + 1
	|
1.0 |*					
	|			*					
	|				*	  
	|				  *
	|					*
	|					*
0.0 ------------------------
	0.0					1.0
*/
inline float calcInverseLogControl(float fVar)
{
	return fVar == 1.0 ? 0.0 : 0.5*log10(1.0 - fVar) + 1.0;
}
// ----------------------------------------------------------------------------- //

// calcInverseAntiLogControl: accepts a float variable from 0.0 t0 1.0
// returns an anti-log version from 1.0 to 0.0
/* y = 10^(-2x) -- this is similar to e^(-5x) but clamps value to 0 at 1
	|
1.0 |*					
	|*		
	| *	
	|	*
	|		*
	|					*
0.0 ------------------------
	0.0					1.0
*/
inline float calcInverseAntiLogControl(float fVar)
{
	return pow((float)10.0, (float)-2.0*fVar);
}
// ----------------------------------------------------------------------------- //


// Helpers for advanced users who make their own GUI
inline float calcDisplayVariable(float fMin, float fMax, float fVar)
{
	return (fMax - fMin)*fVar + fMin;
}

// 0->1
inline float calcSliderVariable(float fMin, float fMax, float fVar)
{
	float fDiff = fMax - fMin;
	float fCookedData = (fVar - fMin)/fDiff;
	return fCookedData;
}

//
// String Helpers: these convert to and from strings
//
// user must delete char array when done!
inline char* uintToString(long value)
{
	char* text =  new char[33];
	ltoa (value, text, 10);
	return text;
}

inline UINT stringToUINT(char* p)
{
	return atol (p);
}

// user must delete char array when done!
inline char* intToString(long value)
{
	char* text =  new char[33];
	itoa (value, text, 10);
	return text;
}

inline double stringToDouble(char* p)
{
	return atof (p);
}

inline double stringToFloat(char* p)
{
	return atof (p);
}

inline int stringToInt(char* p)
{
	return atol (p);
}

// user must delete char array when done!
inline char* floatToString(float value, int nSigDigits)
{
	char* text =  new char[64];
	if(nSigDigits > 32)
		nSigDigits = 32;
	// gcvt (value, nSigDigits, text);
	sprintf(text,"%.*f", nSigDigits, value);

	return text;
}

// user must delete char array when done!
inline char* doubleToString(double value, int nSigDigits)
{
	char* text =  new char[64];
	if(nSigDigits > 32)
		nSigDigits = 32;
	// gcvt (value, nSigDigits, text);
	sprintf(text,"%.*f", nSigDigits, value);
	return text;
}


// user must delete the return char* after use
//
// This function returns the contatenation of String1 and String2 as a newly created
// correctly sized string.
//
// example:
//		char* p = addStrings("this", " and that"):
//
//		results in p = "this and that"
//
// example, when called from your DLL you can use your getMyDLLDirectory() method:
// 		char* p = addStrings(getMyDLLDirectory(),"\\test\\this.wav"); <-- note the double \\
//
//		this returns a char* that is the Path to the file: this.wav
//									located in the folder: test
//										 which is located: inside the Directory containing your PlugIn
inline char* addStrings(char* pString1, char* pString2)
{
	int n = strlen(pString1);
	int m = strlen(pString2);

	char* p = new char[n+m+1];
	strcpy(p, pString1);

	return strncat(p, pString2, m);
}

// Helpers for separating control and data information
// advanced use only, see website for details
inline int extractControlID(char* p)
{
	// find :
	char * pColon;
	pColon = strchr(p,':');
	if(!pColon) return -1; // no more

	DWORD dEndIndex = strlen(p) - strlen(pColon);

	char* pID = new char[dEndIndex+1];
	memset(pID,0,dEndIndex);

	strncpy(pID, p, dEndIndex);
	pID[dEndIndex] = '\0';

	int n = stringToInt(pID);
	delete [] pID;

	return n;
}

inline double extractControlValue(char* p)
{
	// find :
	char * pColon;
	pColon = strchr(p,':');
	if(!pColon) return -1; // no more

	DWORD dEndIndex = strlen(pColon);

	char* pVal = new char[dEndIndex+1];
	memset(pVal,0,dEndIndex);

	strncpy(pVal, pColon+1, dEndIndex);
	pVal[dEndIndex] = '\0';
	//delete [] pVal;

	//return 0;

	double d = stringToDouble(pVal);
	delete [] pVal;

	return d;
}

// END Helper Functions --------------------------------------------------------- //


// RackAFX Built-In Objects for use in Plug-Ins; see book for details
//
//
// Note about Helper Objects: DO NOT MODIFY THESE OBJECTS. If you need to do so,
// create a derived class and modify it. These objects may be updated from time to
// time so they need to be left alone.
//
// The Object Implementations are found in PluginObjects.cpp
//
// --- CEnvelopeDetector ---
//
// http://www.musicdsp.org/archive.php?classid=0#205
//
const float DIGITAL_TC = -2.0; // log(1%)
const float ANALOG_TC = -0.43533393574791066201247090699309; // (log(36.7%)
const float METER_UPDATE_INTERVAL_MSEC = 15.0;
const float METER_MIN_DB = -60.0;

class CEnvelopeDetector
{
public:
	CEnvelopeDetector(void);
	~CEnvelopeDetector(void);

public:

	// Call the Init Function to initialize and setup all at once; this can be called as many times
	// as you want
	void init(float samplerate, float attack_in_ms, float release_in_ms, bool bAnalogTC, UINT uDetect, bool bLogDetector);

	// these functions allow you to change modes and attack/release one at a time during
	// realtime operation
	void setTCModeAnalog(bool bAnalogTC); // {m_bAnalogTC = bAnalogTC;}

	// THEN do these after init
	void setAttackTime(float attack_in_ms);
	void setReleaseTime(float release_in_ms);

	// Use these "codes"
	// DETECT PEAK   = 0
	// DETECT MS	 = 1
	// DETECT RMS	 = 2
	//
	void setDetectMode(UINT uDetect) {m_uDetectMode = uDetect;}

	void setSampleRate(float f) {m_fSampleRate = f;}

	void setLogDetect(bool b) {m_bLogDetector = b;}

	// call this to detect; it returns the peak ms or rms value at that instant
	float detect(float fInput);

	// call this from your prepareForPlay() function each time to reset the detector
	void prepareForPlay();

protected:
	int  m_nSample;
	float m_fAttackTime;
	float m_fReleaseTime;
	float m_fAttackTime_mSec;
	float m_fReleaseTime_mSec;
	float m_fSampleRate;
	float m_fEnvelope;
	UINT  m_uDetectMode;
	bool  m_bAnalogTC;
	bool  m_bLogDetector;
};


// --- BiQuad ---
// This class can be used alone or as the base class for a derived object
// Note the virtual destructor, required for proper clean-up of derived
// classes
//
// Implements a single Bi-Quad Structure
class CBiQuad
{
public:
	CBiQuad(void);
	virtual ~CBiQuad(void);

protected:
	float m_f_Xz_1; // x z-1 delay element
	float m_f_Xz_2; // x z-2 delay element
	float m_f_Yz_1; // y z-1 delay element
	float m_f_Yz_2; // y z-2 delay element

	// allow other objects to set these directly since we have no cooking
	// function or intelligence

public:
	float m_f_a0;
	float m_f_a1;
	float m_f_a2;
	float m_f_b1;
	float m_f_b2;

	// flush Delays
	void flushDelays()
	{
		m_f_Xz_1 = 0;
		m_f_Xz_2 = 0;
		m_f_Yz_1 = 0;
		m_f_Yz_2 = 0;
	}

	// Do the filter: given input xn, calculate output yn and return it
	float doBiQuad(float f_xn)
	{
		// just do the difference equation: y(n) = a0x(n) + a1x(n-1) + a2x(n-2) - b1y(n-1) - b2y(n-2)
		float yn = m_f_a0*f_xn + m_f_a1*m_f_Xz_1 + m_f_a2*m_f_Xz_2 - m_f_b1*m_f_Yz_1 - m_f_b2*m_f_Yz_2;

		// underflow check
		if(yn > 0.0 && yn < FLT_MIN_PLUS) yn = 0;
		if(yn < 0.0 && yn > FLT_MIN_MINUS) yn = 0;

		// shuffle delays
		// Y delays
		m_f_Yz_2 = m_f_Yz_1;
		m_f_Yz_1 = yn;

		// X delays
		m_f_Xz_2 = m_f_Xz_1;
		m_f_Xz_1 = f_xn;

		return  yn;
	}
};


// --- CJoystickProgram ---
class CJoystickProgram
{
public:	// Functions
	//
	// (1) Initialize here:
	CJoystickProgram(float* pJSProgramTable, UINT uMode);

	// One Time Destruction
	~CJoystickProgram(void);

	void startProgram();
	void pauseProgram(){m_bRunning = false;}
	void resumeProgram(){m_bRunning = true;}
	void reset(){m_uSampleCount = 0; m_bRunning = false; m_bDirInc = true; m_fTimerDurationMSec = 0; m_nCurrentProgramStep = 0;}
	void setJSMode(UINT uMode);
	int getCurrentStep(){return m_nCurrentProgramStep;}

	// (2) Set the sample rate in prepareForPlay()
	void setSampleRate(int nSampleRate);

	// (3) call this once per sample period
	void incTimer();

	// (4) get the current vector mix ratios
	void getVectorMixValues(float& fA, float& fB, float& fC, float& fD){fA = m_fA_Mix; fB = m_fB_Mix, fC = m_fC_Mix, fD = m_fD_Mix;}

	void getVectorACBDMixes(float& fAB, float& fBD){fAB = m_fAC_Mix; fBD = m_fBD_Mix;}

protected:
	UINT m_uSampleCount;
	UINT m_uJSMode;
	bool m_bRunning;
	bool m_bDirInc;

	float m_fTimerDurationMSec;
	float m_fSampleRate;
	int  m_nTimerDurationSamples;
	int m_nCurrentProgramStep;
	int m_nNumSteps;

public:
	float m_fStartA_Mix;
	float m_fStartB_Mix;
	float m_fStartC_Mix;
	float m_fStartD_Mix;

	float m_fStartAC_Mix;
	float m_fStartBD_Mix;

	float m_fEndA_Mix;
	float m_fEndB_Mix;
	float m_fEndC_Mix;
	float m_fEndD_Mix;

	float m_fEndAC_Mix;
	float m_fEndBD_Mix;

	float m_fA_Mix;
	float m_fB_Mix;
	float m_fC_Mix;
	float m_fD_Mix;

	float m_fAC_Mix;
	float m_fBD_Mix;

	void calculateCurrentVectorMix();
	float* m_pJSProgramTable;
};

// --- CWaveTable ---
// generic WT Oscillator for you to use
class CWaveTable
{
public:	// Functions
	//
	// One Time Initialization
	CWaveTable();

	// One Time Destruction
	~CWaveTable(void);

	// The Prepare For Play Function is called just before audio streams
	bool prepareForPlay();

	// --- MEMBER FUNCTIONS
	//  function to do render one sample of the Oscillator
	//  call this once per sample period
	//
	//	pYn is the normal output
	//	pYqn is the quad phase output
	void doOscillate(float* pYn, float* pYqn);

	// reset the read index
	void reset()
	{
		m_fReadIndex = 0.0;
		m_fQuadPhaseReadIndex = 256.0;	// 1/4 of our 1024 point buffer
	}

	// set the sample rate: NEEDED to calcualte the increment value from frequency
	void setSampleRate(int nSampleRate)
	{
		m_nSampleRate = nSampleRate;
	}

	// our cooking function
	void cookFrequency()
	{
		// inc = L*fd/fs
		m_f_inc = 1024.0*m_fFrequency_Hz/(float)m_nSampleRate;
	}

	// --- MEMBER OBJECTS
	// Array for the Table
	float m_SinArray[1024];			// 1024 Point Sinusoid
	float m_SawtoothArray[1024];    // saw
	float m_TriangleArray[1024];	// tri
	float m_SquareArray[1024];		// sqr

	// band limited to 5 partials
	float m_SawtoothArray_BL5[1024];    // saw, BL = 5
	float m_TriangleArray_BL5[1024];	// tri, BL = 5
	float m_SquareArray_BL5[1024];		// sqr, BL = 5

	// current read location
	float m_fReadIndex;					// NOTE its a FLOAT!
	float m_fQuadPhaseReadIndex;		// NOTE its a FLOAT!

	// our inc value
	float m_f_inc;

	// fs value
	int   m_nSampleRate;

	// user-controlled variables:
	// Frequency
	float m_fFrequency_Hz;

	// Inverted Output
	bool m_bInvert;

	// Type
	UINT m_uOscType;
	enum{sine,saw,tri,square};

	// Mode
	UINT m_uTableMode;
	enum{normal,bandlimit};

	// Polarity
	UINT m_uPolarity;
	enum{bipolar,unipolar};
};

//
// This is a helper object for reading Wave files into floating point buffers.
// It is NOT optimized for speed (yet)
#if defined _WINDOWS || defined _WINDLL
#include <mmsystem.h>

// wave file parser
class CWaveData
{
public:	// Functions
	//
	// One Time Initialization
	// pFilePath is the FULLY qualified file name + additional path info
	// VALID Examples: audio.wav
	//				   //samples//audio.wav
	CWaveData(char* pFilePath = NULL);

	// prompts with file open dialog, returns TRUE if successfuly
	// opened and parsed the file into the member m_pWaveBuffer
	bool initWithUserWAVFile(char* pInitDir = NULL);

	// One Time Destruction
	~CWaveData(void);

	UINT m_uNumChannels;
	UINT m_uSampleRate;
	UINT m_uSampleCount;
	UINT m_uLoopCount;
	UINT m_uLoopStartIndex;
	UINT m_uLoopEndIndex;
	UINT m_uLoopType;
	UINT m_uMIDINote;
	UINT m_uMIDIPitchFraction;
	UINT m_uSMPTEFormat;
	UINT m_uSMPTEOffset;

	bool m_bWaveLoaded;

	// the WAV file converted to floats on range of -1.0 --> +1.0
	float* m_pWaveBuffer;

protected:
	bool readWaveFile(char* pFilePath);
	HANDLE m_hFile;

};
#endif

// --- CUICtrl ---
//
// CUICtrl is the C++ obhect that manages your GUI objects
// like sliders and buttons. Do not ever edit or change this code as the
// objects are shared with the Client and you may break the app.
//
// enum for VU Colors
enum {csVU, csRed, csOrange, csYellow, csGreen, csBlue, csViolet};

class CUICtrl
{
public:
	CUICtrl(void);
	~CUICtrl(void);
	CUICtrl(const CUICtrl& initCUICtrl);

public:
	UINT uControlType;
	UINT uControlId;
	UINT uDetectorMode;

	float fUserDisplayDataLoLimit;
	float fUserDisplayDataHiLimit;

	UINT  uUserDataType;
	float fInitUserIntValue;
	float fInitUserFloatValue;
	float fInitUserDoubleValue;
	float fInitUserUINTValue;

	// FOR VST PRESETS ONLY!
	float fUserCookedIntData;
	float fUserCookedFloatData;
	float fUserCookedDoubleData;
	float fUserCookedUINTData;

	int*	m_pUserCookedIntData;
	float*	m_pUserCookedFloatData;
	double*	m_pUserCookedDoubleData;
	UINT*	m_pUserCookedUINTData;
	float*	m_pCurrentMeterValue;

	char*  cControlName;
	char*  cControlUnits;
	char*  cVariableName;
	char*  cEnumeratedList;
	char*  cMeterVariableName;

	char   cName[1024];
	char   cUnits[1024];
	char   cVName[1024];
	char   cMeterVName[1024];
	char   cVEnumeratedList[1024];

	bool	bOwnerControl;
	bool	bUseMeter;
	bool	bUpsideDownMeter;
	bool	bLogMeter;

	bool	bLogSlider;
	bool	bExpSlider;

	UINT    uMeterColorScheme;
	float   fMeterAttack_ms;
	float   fMeterRelease_ms;

	bool	bMIDIControl;
	UINT	uMIDIControlCommand; // pitchbend or CC
	UINT	uMIDIControlName; // eg Continuous Controller #3
	UINT	uMIDIControlChannel; // eg Continuous Controller #3

	int		nGUIRow;
	int		nGUIColumn;

	UINT	uControlTheme[CONTROL_THEME_SIZE];
	double 	dPresetData[PRESET_COUNT];

	UINT	uFluxCapControl[PLUGIN_CONTROL_THEME_SIZE];
	float	fFluxCapData[PLUGIN_CONTROL_THEME_SIZE];

	void*   pvAddlData;
// ----------------------------------------------------------------------------------------
public:
	CUICtrl& operator=(const CUICtrl& aCUICtrl)	// need this override for collections to work
	{
		if(this == &aCUICtrl)
			return *this;

		this->uControlType				= aCUICtrl.uControlType;
		this->uControlId				= aCUICtrl.uControlId;
		this->bOwnerControl			= aCUICtrl.bOwnerControl;
		this->bUseMeter			= aCUICtrl.bUseMeter;
		this->bUpsideDownMeter			= aCUICtrl.bUpsideDownMeter;
		this->uDetectorMode			= aCUICtrl.uDetectorMode;
		this->bLogMeter			= aCUICtrl.bLogMeter;
		this->bLogSlider			= aCUICtrl.bLogSlider;
		this->bExpSlider			= aCUICtrl.bExpSlider;

		this->uMeterColorScheme			= aCUICtrl.uMeterColorScheme;
		this->fMeterAttack_ms			= aCUICtrl.fMeterAttack_ms;
		this->fMeterRelease_ms			= aCUICtrl.fMeterRelease_ms;

		this->fUserDisplayDataLoLimit				= aCUICtrl.fUserDisplayDataLoLimit;
		this->fUserDisplayDataHiLimit				= aCUICtrl.fUserDisplayDataHiLimit;

		this->uUserDataType				= aCUICtrl.uUserDataType;
		this->fInitUserIntValue				= aCUICtrl.fInitUserIntValue;
		this->fInitUserFloatValue				= aCUICtrl.fInitUserFloatValue;
		this->fInitUserDoubleValue				= aCUICtrl.fInitUserDoubleValue;
		this->fInitUserUINTValue				= aCUICtrl.fInitUserUINTValue;

		// VST PRESETS ONLY
		this->fUserCookedIntData				= aCUICtrl.fUserCookedIntData;
		this->fUserCookedFloatData				= aCUICtrl.fInitUserFloatValue;
		this->fUserCookedDoubleData				= aCUICtrl.fUserCookedDoubleData;
		this->fUserCookedUINTData				= aCUICtrl.fUserCookedUINTData;

		this->m_pUserCookedIntData				= aCUICtrl.m_pUserCookedIntData;
		this->m_pUserCookedFloatData				= aCUICtrl.m_pUserCookedFloatData;
		this->m_pUserCookedDoubleData				= aCUICtrl.m_pUserCookedDoubleData;
		this->m_pUserCookedUINTData				= aCUICtrl.m_pUserCookedUINTData;
		this->m_pCurrentMeterValue				= aCUICtrl.m_pCurrentMeterValue;

		this->bMIDIControl				= aCUICtrl.bMIDIControl;
		this->uMIDIControlCommand		= aCUICtrl.uMIDIControlCommand;
		this->uMIDIControlName			= aCUICtrl.uMIDIControlName;
		this->uMIDIControlChannel			= aCUICtrl.uMIDIControlChannel;

		this->nGUIRow				= aCUICtrl.nGUIRow;
		this->nGUIColumn				= aCUICtrl.nGUIColumn;
		this->pvAddlData			= aCUICtrl.pvAddlData;

		for(int i=0; i<PRESET_COUNT; i++)
		{
			this->dPresetData[i] = aCUICtrl.dPresetData[i];
		}

		for(int i=0; i<CONTROL_THEME_SIZE; i++)
		{
			this->uControlTheme[i] = aCUICtrl.uControlTheme[i];
		}

		for(int i=0; i<PLUGIN_CONTROL_THEME_SIZE; i++)
		{
			this->uFluxCapControl[i] = aCUICtrl.uFluxCapControl[i];
		}

		for(int i=0; i<PLUGIN_CONTROL_THEME_SIZE; i++)
		{
			this->fFluxCapData[i] = aCUICtrl.fFluxCapData[i];
		}

		cControlName = &cName[0];
		cControlUnits = &cUnits[0];
		cVariableName = &cVName[0];
		cMeterVariableName = &cMeterVName[0];
		cEnumeratedList = &cVEnumeratedList[0];

		strncpy(this->cControlName, aCUICtrl.cControlName, 1023);
		cControlName[1023] = '\0';

		strncpy(this->cControlUnits, aCUICtrl.cControlUnits, 1023);
		cControlUnits[1023] = '\0';

		strncpy(this->cVariableName, aCUICtrl.cVariableName, 1023);
		cVariableName[1023] = '\0';

		strncpy(this->cEnumeratedList, aCUICtrl.cEnumeratedList, 1023);
		cEnumeratedList[1023] = '\0';

		strncpy(this->cMeterVariableName, aCUICtrl.cMeterVariableName, 1023);
		cMeterVariableName[1023] = '\0';

		return *this;

	}
};


// -- CUIControlList --
// This is the linked list of control objects
const UINT uMaxVSTProgramNameLen = 24;

class CUIControlList
{
     private:

         struct node
         {
            CUICtrl data;
            node *link;
         }*p;

public:

     CUIControlList();
     void append( CUICtrl data);
     void add_as_first( CUICtrl data );
     void addafter( int c, CUICtrl data);
     void update(CUICtrl data);
     void del( CUICtrl data);
     void display();
     int count();
     int countLegalVSTIF();
     int countLegalCustomVSTGUI();

	 CUICtrl* getAt(int nIndex);

	 // for VST preset storage
	 char name[uMaxVSTProgramNameLen+1];

     ~CUIControlList();

	CUIControlList& operator=(CUIControlList& aCUICtrlList)	// need this override for collections to work
	{
		if(this == &aCUICtrlList)
			return *this;

		strncpy(this->name, aCUICtrlList.name, uMaxVSTProgramNameLen);
		name[uMaxVSTProgramNameLen] = '\0';

		int nCount = aCUICtrlList.count();
		for(int j=0; j<nCount; j++)
		{
			CUICtrl* p = aCUICtrlList.getAt(j);
			CUICtrl* pClone = new CUICtrl(*p);
			this->append(*pClone);
		}

		return *this;
	}
};
