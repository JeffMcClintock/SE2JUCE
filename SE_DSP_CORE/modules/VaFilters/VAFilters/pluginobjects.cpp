#include "pluginconstants.h"
#include <assert.h>
// This file contains the object implementations for the objects declared in
// "pluginconstants.h"
//
// Note about Helper Objects: DO NOT MODIFY THESE OBJECTS. If you need to do so,
// create a derived class and modify it. These objects may be updated from time to
// time so they need to be left alone.



// CEnvelopeDetector Implementation ----------------------------------------------------------------
//
CEnvelopeDetector::CEnvelopeDetector()
{
	m_fAttackTime_mSec = 0.0;
	m_fReleaseTime_mSec = 0.0;
	m_fAttackTime = 0.0;
	m_fReleaseTime = 0.0;
	m_fSampleRate = 44100;
	m_fEnvelope = 0.0;
	m_uDetectMode = 0;
	m_nSample = 0;
	m_bAnalogTC = false;
	m_bLogDetector = false;
}

CEnvelopeDetector::~CEnvelopeDetector()
{
}

void CEnvelopeDetector::prepareForPlay()
{
	m_fEnvelope = 0.0;
	m_nSample = 0;
}

void CEnvelopeDetector::init(float samplerate, float attack_in_ms, float release_in_ms, bool bAnalogTC, UINT uDetect, bool bLogDetector)
{
	m_fEnvelope = 0.0;
	m_fSampleRate = samplerate;
	m_bAnalogTC = bAnalogTC;
	m_fAttackTime_mSec = attack_in_ms;
	m_fReleaseTime_mSec = release_in_ms;
	m_uDetectMode = uDetect;
	m_bLogDetector = bLogDetector;

	// set themm_uDetectMode = uDetect;
	setAttackTime(attack_in_ms);
	setReleaseTime(release_in_ms);
}

void CEnvelopeDetector::setAttackTime(float attack_in_ms)
{
	m_fAttackTime_mSec = attack_in_ms;

	if(m_bAnalogTC)
		m_fAttackTime = exp(ANALOG_TC/( attack_in_ms * m_fSampleRate * 0.001));
	else
		m_fAttackTime = exp(DIGITAL_TC/( attack_in_ms * m_fSampleRate * 0.001));
}

void CEnvelopeDetector::setReleaseTime(float release_in_ms)
{
	m_fReleaseTime_mSec = release_in_ms;

	if(m_bAnalogTC)
		m_fReleaseTime = exp(ANALOG_TC/( release_in_ms * m_fSampleRate * 0.001));
	else
		m_fReleaseTime = exp(DIGITAL_TC/( release_in_ms * m_fSampleRate * 0.001));
}

void CEnvelopeDetector::setTCModeAnalog(bool bAnalogTC)
{
	m_bAnalogTC = bAnalogTC;
	setAttackTime(m_fAttackTime_mSec);
	setReleaseTime(m_fReleaseTime_mSec);
}


float CEnvelopeDetector::detect(float fInput)
{
	switch(m_uDetectMode)
	{
	case 0:
		fInput = fabs(fInput);
		break;
	case 1:
		fInput = fabs(fInput) * fabs(fInput);
		break;
	case 2:
		fInput = pow((float)fabs(fInput) * (float)fabs(fInput), (float)0.5);
		break;
	default:
		fInput = (float)fabs(fInput);
		break;
	}

	float fOld = m_fEnvelope;
	if(fInput> m_fEnvelope)
		m_fEnvelope = m_fAttackTime * (m_fEnvelope - fInput) + fInput;
	else
		m_fEnvelope = m_fReleaseTime * (m_fEnvelope - fInput) + fInput;

	if(m_fEnvelope > 0.0 && m_fEnvelope < FLT_MIN_PLUS) m_fEnvelope = 0;
	if(m_fEnvelope < 0.0 && m_fEnvelope > FLT_MIN_MINUS) m_fEnvelope = 0;

	// bound them; can happen when using pre-detector gains of more than 1.0
	m_fEnvelope = min(m_fEnvelope, 1.0);
	m_fEnvelope = max(m_fEnvelope, 0.0);

	//16-bit scaling!
	if(m_bLogDetector)
	{
		if(m_fEnvelope <= 0)
			return -96.0; // 16 bit noise floor

		return 20*log10	(m_fEnvelope);
	}

	return m_fEnvelope;
}


// CWaveTable Implementation ----------------------------------------------------------------
//
CWaveTable::CWaveTable()
{
	// non inveting unless user sets it
	m_bInvert = false;

	// slope and y-intercept values for the
	// Triangle Wave
	// rising edge1:
	float mt1 = 1.0/256.0;
	float bt1 = 0.0;

	// rising edge2:
	float mt2 = 1.0/256.0;
	float bt2 = -1.0;

	// falling edge:
	float mtf2 = -2.0/512.0;
	float btf2 = +1.0;

	// Sawtooth
	// rising edge1:
	float ms1 = 1.0/512.0;
	float bs1 = 0.0;

	// rising edge2:
	float ms2 = 1.0/512.0;
	float bs2 = -1.0;


	float fMaxTri = 0;
	float fMaxSaw = 0;
	float fMaxSqr = 0;

	// setup arrays
	for(int i = 0; i < 1024; i++)
	{
		// sample the sinusoid, 1024 points
		// sin(wnT) = sin(2pi*i/1024)
		m_SinArray[i] = sin( ((float)i/1024.0)*(2*pi) );

		// saw, triangle and square are just logic mechanisms
		// can you figure them out?
		// Sawtooth
		m_SawtoothArray[i] =  i < 512 ? ms1*i + bs1 : ms2*(i-511) + bs2;

		// Triangle
		if(i < 256)
			m_TriangleArray[i] = mt1*i + bt1; // mx + b; rising edge 1
		else if (i >= 256 && i < 768)
			m_TriangleArray[i] = mtf2*(i-256) + btf2; // mx + b; falling edge
		else
			m_TriangleArray[i] = mt2*(i-768) + bt2; // mx + b; rising edge 2

		// square
		m_SquareArray[i] = i < 512 ? +1.0 : -1.0;

		// zero to start, then loops build the rest
		m_SawtoothArray_BL5[i] = 0.0;
		m_SquareArray_BL5[i] = 0.0;
		m_TriangleArray_BL5[i] = 0.0;

		// sawtooth: += (-1)^g+1(1/g)sin(wnT)
		for(int g=1; g<=6; g++)
		{
			double n = double(g);
			m_SawtoothArray_BL5[i] += pow((float)-1.0,(float)(g+1))*(1.0/n)*sin(2.0*pi*i*n/1024.0);
			// down saw m_SawtoothArray_BL5[i] += (1.0/n)*sin(2.0*pi*i*n/1024.0);
		}

		// triangle: += (-1)^g(1/(2g+1+^2)sin(w(2n+1)T)
		for(int g=0; g<=3; g++)
		{
			double n = double(g);
			m_TriangleArray_BL5[i] += pow((float)-1.0, (float)n)*(1.0/pow((float)(2*n + 1), (float)2.0))*sin(2.0*pi*(2.0*n + 1)*i/1024.0);
		}

		// square: += (1/g)sin(wnT)
		for(int g=1; g<8; g+=2)
		{
			double n = double(g);
			m_SquareArray_BL5[i] += (1.0/n)*sin(2.0*pi*i*n/1024.0);
		}

		// store the max values
		if(i == 0)
		{
			fMaxSaw = m_SawtoothArray_BL5[i];
			fMaxTri = m_TriangleArray_BL5[i];
			fMaxSqr = m_SquareArray_BL5[i];
		}
		else
		{
			// test and store
			if(m_SawtoothArray_BL5[i] > fMaxSaw)
				fMaxSaw = m_SawtoothArray_BL5[i];

			if(m_TriangleArray_BL5[i] > fMaxTri)
				fMaxTri = m_TriangleArray_BL5[i];

			if(m_SquareArray_BL5[i] > fMaxSqr)
				fMaxSqr = m_SquareArray_BL5[i];
		}
	}

	// normalize the bandlimited tables
	for(int i = 0; i < 1024; i++)
	{
		// normalize it
		m_SawtoothArray_BL5[i] /= fMaxSaw;
		m_TriangleArray_BL5[i] /= fMaxTri;
		m_SquareArray_BL5[i] /= fMaxSqr;
	}

	// clear variables
	m_fReadIndex = 0.0;
	m_fQuadPhaseReadIndex = 0.0;
	m_f_inc = 0.0;

	// initialize inc
	reset();
	m_fFrequency_Hz = 440;
	m_uOscType = 0;
	m_uTableMode = 0;
	m_uPolarity = 0;

	cookFrequency();
}


/* destructor()
	Destroy variables allocated in the contructor()

*/
CWaveTable::~CWaveTable()
{


}

/* prepareForPlay()
	Called by the client after Play() is initiated but before audio streams

    NOTE: if you allocte memory in this function, destroy it in ::destructor() above
*/
bool CWaveTable::prepareForPlay()
{
	// reset the index
	reset();

	// cook curent frequency
	cookFrequency();

	return true;
}

// The Oscillate Function!
void CWaveTable::doOscillate(float* pYn, float* pYqn)
{
	// our output value for this cycle
	float fOutSample = 0;
	float fQuadPhaseOutSample = 0;

	// get INT part
	int nReadIndex = (int)m_fReadIndex;
	int nQuadPhaseReadIndex = (int)m_fQuadPhaseReadIndex;

	// get FRAC part
	float fFrac = m_fReadIndex - nReadIndex;
	float fQuadFrac = m_fQuadPhaseReadIndex - nQuadPhaseReadIndex;

	// setup second index for interpolation; wrap the buffer if needed
	int nReadIndexNext = nReadIndex + 1 > 1023 ? 0 :  nReadIndex + 1;
	int nQuadPhaseReadIndexNext = nQuadPhaseReadIndex + 1 > 1023 ? 0 :  nQuadPhaseReadIndex + 1;

	// interpolate the output
	switch(m_uOscType)
	{
		case sine:
			fOutSample = dLinTerp(0, 1, m_SinArray[nReadIndex], m_SinArray[nReadIndexNext], fFrac);
			fQuadPhaseOutSample = dLinTerp(0, 1, m_SinArray[nQuadPhaseReadIndex], m_SinArray[nQuadPhaseReadIndexNext], fQuadFrac);
			break;

		case saw:
			if(m_uTableMode == normal)	// normal
			{
				fOutSample = dLinTerp(0, 1, m_SawtoothArray[nReadIndex], m_SawtoothArray[nReadIndexNext], fFrac);
				fQuadPhaseOutSample = dLinTerp(0, 1, m_SawtoothArray[nQuadPhaseReadIndex], m_SawtoothArray[nQuadPhaseReadIndexNext], fQuadFrac);
			}
			else						// bandlimited
			{
				fOutSample = dLinTerp(0, 1, m_SawtoothArray_BL5[nReadIndex], m_SawtoothArray_BL5[nReadIndexNext], fFrac);
				fQuadPhaseOutSample = dLinTerp(0, 1, m_SawtoothArray_BL5[nQuadPhaseReadIndex], m_SawtoothArray_BL5[nQuadPhaseReadIndexNext], fQuadFrac);
			}
			break;

		case tri:
			if(m_uTableMode == normal)	// normal
			{
				fOutSample = dLinTerp(0, 1, m_TriangleArray[nReadIndex], m_TriangleArray[nReadIndexNext], fFrac);
				fQuadPhaseOutSample = dLinTerp(0, 1, m_TriangleArray[nQuadPhaseReadIndex], m_TriangleArray[nQuadPhaseReadIndexNext], fQuadFrac);
			}
			else						// bandlimited
			{
				fOutSample = dLinTerp(0, 1, m_TriangleArray_BL5[nReadIndex], m_TriangleArray_BL5[nReadIndexNext], fFrac);
				fQuadPhaseOutSample = dLinTerp(0, 1, m_TriangleArray_BL5[nQuadPhaseReadIndex], m_TriangleArray_BL5[nQuadPhaseReadIndexNext], fQuadFrac);
			}
			break;

		case square:
			if(m_uTableMode == normal)	// normal
			{
				fOutSample = dLinTerp(0, 1, m_SquareArray[nReadIndex], m_SquareArray[nReadIndexNext], fFrac);
				fQuadPhaseOutSample = dLinTerp(0, 1, m_SquareArray[nQuadPhaseReadIndex], m_SquareArray[nQuadPhaseReadIndexNext], fQuadFrac);
			}
			else						// bandlimited
			{
				fOutSample = dLinTerp(0, 1, m_SquareArray_BL5[nReadIndex], m_SquareArray_BL5[nReadIndexNext], fFrac);
				fQuadPhaseOutSample = dLinTerp(0, 1, m_SquareArray_BL5[nQuadPhaseReadIndex], m_SquareArray_BL5[nQuadPhaseReadIndexNext], fQuadFrac);
			}
			break;

		// always need a default
		default:
			fOutSample = dLinTerp(0, 1, m_SinArray[nReadIndex], m_SinArray[nReadIndexNext], fFrac);
			fQuadPhaseOutSample = dLinTerp(0, 1, m_SinArray[nQuadPhaseReadIndex], m_SinArray[nQuadPhaseReadIndexNext], fQuadFrac);
			break;
	}

	// add the increment for next time
	m_fReadIndex += m_f_inc;
	m_fQuadPhaseReadIndex += m_f_inc;

	// check for wrap
	if(m_fReadIndex >= 1024)
		m_fReadIndex = m_fReadIndex - 1024;

	if(m_fQuadPhaseReadIndex >= 1024)
		m_fQuadPhaseReadIndex = m_fQuadPhaseReadIndex - 1024;

	// write out
	*pYn = fOutSample;
	*pYqn = fQuadPhaseOutSample;

	if(m_bInvert)
	{
		*pYn *= -1.0;
		*pYqn *= -1.0;
	}

	if(m_uPolarity == unipolar)
	{
		*pYn /= 2.0;
		*pYn += 0.5;

		*pYqn /= 2.0;
		*pYqn += 0.5;
	}
}

// CBiQuad Implementation ----------------------------------------------------------------
//
CBiQuad::CBiQuad()
{
}

CBiQuad::~CBiQuad()
{
}


// CJoystickProgram Implementation ----------------------------------------------------------------
//
CJoystickProgram::CJoystickProgram(float* pJSProgramTable, UINT uMode)
{
	m_bRunning = false;
	m_uSampleCount = 0;
	m_pJSProgramTable = pJSProgramTable;
	m_fTimerDurationMSec = 0;
	m_nCurrentProgramStep = 0;
	m_fA_Mix = 0.25;
	m_fB_Mix = 0.25;
	m_fC_Mix = 0.25;
	m_fD_Mix = 0.25;
	m_fAC_Mix = 0.0;
	m_fBD_Mix = 0.0;
	m_uJSMode = uMode;
	m_bDirInc = true;
}

void CJoystickProgram::setJSMode(UINT uMode)
{
	m_uJSMode = uMode;
	m_bDirInc = true;
}

void CJoystickProgram::setSampleRate(int nSampleRate)
{
	m_fSampleRate = (float)nSampleRate;
}

CJoystickProgram::~CJoystickProgram()
{

}

void CJoystickProgram::incTimer()
{
	if(!m_bRunning)
		return;

	m_uSampleCount++;

	calculateCurrentVectorMix();

	if(m_uSampleCount > (UINT)m_nTimerDurationSamples)
	{
		// goto the next step
		if(m_bDirInc)
			m_nCurrentProgramStep++;
		else
			m_nCurrentProgramStep--;

		if(m_nCurrentProgramStep > m_nNumSteps || m_nCurrentProgramStep < 0)
		{
			if(m_uJSMode == JS_ONESHOT)
			{
				reset();
				return;
			}
			else if(m_uJSMode == JS_LOOP)
			{
				m_nCurrentProgramStep = 0;
			}
			else if(m_uJSMode == JS_LOOP_BACKANDFORTH)
			{
				m_bDirInc = !m_bDirInc;
				if(m_bDirInc)
					m_nCurrentProgramStep +=2;
				else
					m_nCurrentProgramStep -= 2;
			}
			else if(m_uJSMode == JS_SUSTAIN)
			{
				m_bRunning = false;
				return; // just return
			}
		}
		else if(m_uJSMode == JS_SUSTAIN && m_nCurrentProgramStep == m_nNumSteps)
		{
			pauseProgram(); // until user restarts with a note-off event
		}

		// setup for next step
		m_uSampleCount = 0;

		m_fStartA_Mix = m_fEndA_Mix;
		m_fStartB_Mix = m_fEndB_Mix;
		m_fStartC_Mix = m_fEndC_Mix;
		m_fStartD_Mix = m_fEndD_Mix;
		m_fStartAC_Mix = m_fEndAC_Mix;
		m_fStartBD_Mix = m_fEndBD_Mix;

		m_fEndA_Mix = m_pJSProgramTable[JS_PROG_INDEX(m_nCurrentProgramStep,0)];
		m_fEndB_Mix = m_pJSProgramTable[JS_PROG_INDEX(m_nCurrentProgramStep,1)];
		m_fEndC_Mix = m_pJSProgramTable[JS_PROG_INDEX(m_nCurrentProgramStep,2)];
		m_fEndD_Mix = m_pJSProgramTable[JS_PROG_INDEX(m_nCurrentProgramStep,3)];
		m_fEndAC_Mix = m_pJSProgramTable[JS_PROG_INDEX(m_nCurrentProgramStep,5)];
		m_fEndBD_Mix = m_pJSProgramTable[JS_PROG_INDEX(m_nCurrentProgramStep,6)];

		if(m_bDirInc)
			m_fTimerDurationMSec = m_nCurrentProgramStep > 0 ? m_pJSProgramTable[JS_PROG_INDEX(m_nCurrentProgramStep-1,4)] : m_pJSProgramTable[JS_PROG_INDEX(m_nNumSteps,4)];
		else
			m_fTimerDurationMSec = m_pJSProgramTable[JS_PROG_INDEX(m_nCurrentProgramStep,4)];

		m_nTimerDurationSamples = (int)((m_fTimerDurationMSec/1000.0)*(m_fSampleRate));
	}
}

void CJoystickProgram::calculateCurrentVectorMix()
{
	float m = (m_fEndA_Mix - m_fStartA_Mix)/(float)m_nTimerDurationSamples;
	m_fA_Mix = m*(float)m_uSampleCount + m_fStartA_Mix;

	m = (m_fEndB_Mix - m_fStartB_Mix)/(float)m_nTimerDurationSamples;
	m_fB_Mix = m*(float)m_uSampleCount + m_fStartB_Mix;

	m = (m_fEndC_Mix - m_fStartC_Mix)/(float)m_nTimerDurationSamples;
	m_fC_Mix = m*(float)m_uSampleCount + m_fStartC_Mix;

	m = (m_fEndD_Mix - m_fStartD_Mix)/(float)m_nTimerDurationSamples;
	m_fD_Mix = m*(float)m_uSampleCount + m_fStartD_Mix;

	m = (m_fEndAC_Mix - m_fStartAC_Mix)/(float)m_nTimerDurationSamples;
	m_fAC_Mix = m*(float)m_uSampleCount + m_fStartAC_Mix;

	m = (m_fEndBD_Mix - m_fStartBD_Mix)/(float)m_nTimerDurationSamples;
	m_fBD_Mix = m*(float)m_uSampleCount + m_fStartBD_Mix;
}

void CJoystickProgram::startProgram()
{
	m_nNumSteps = 0;
	for(int i=1; i<MAX_JS_PROGRAM_STEPS; i++)
	{
		if(m_pJSProgramTable[JS_PROG_INDEX(i,4)] > 0)
			m_nNumSteps++;
	}

	if(m_nNumSteps == 0)
		return;

	m_bDirInc = true;
	m_nCurrentProgramStep = 1;

	m_fStartA_Mix = m_pJSProgramTable[JS_PROG_INDEX(0,0)];
	m_fStartB_Mix = m_pJSProgramTable[JS_PROG_INDEX(0,1)];
	m_fStartC_Mix = m_pJSProgramTable[JS_PROG_INDEX(0,2)];
	m_fStartD_Mix = m_pJSProgramTable[JS_PROG_INDEX(0,3)];
	m_fStartAC_Mix = m_pJSProgramTable[JS_PROG_INDEX(0,5)];
	m_fStartBD_Mix = m_pJSProgramTable[JS_PROG_INDEX(0,6)];

	m_fEndA_Mix = m_pJSProgramTable[JS_PROG_INDEX(1,0)];
	m_fEndB_Mix = m_pJSProgramTable[JS_PROG_INDEX(1,1)];
	m_fEndC_Mix = m_pJSProgramTable[JS_PROG_INDEX(1,2)];
	m_fEndD_Mix = m_pJSProgramTable[JS_PROG_INDEX(1,3)];
	m_fEndAC_Mix = m_pJSProgramTable[JS_PROG_INDEX(1,5)];
	m_fEndBD_Mix = m_pJSProgramTable[JS_PROG_INDEX(1,6)];

	m_fTimerDurationMSec = m_pJSProgramTable[JS_PROG_INDEX(0,4)];
	m_nTimerDurationSamples = (int)((m_fTimerDurationMSec/1000.0)*(m_fSampleRate));

	m_bRunning = true;
}

// CWaveData Implementation ----------------------------------------------------------------
//
#if defined _WINDOWS || defined _WINDLL
typedef struct {

    UCHAR IdentifierString[4];
    DWORD dwLength;

} RIFF_CHUNK, *PRIFF_CHUNK;

typedef struct {

    WORD  wFormatTag;         // Format category
    WORD  wChannels;          // Number of channels
    DWORD dwSamplesPerSec;    // Sampling rate
    DWORD dwAvgBytesPerSec;   // For buffer estimation
    WORD  wBlockAlign;        // Data block size
    WORD  wBitsPerSample;


} WAVE_FILE_HEADER, *PWAVE_FILE_HEADER;


typedef struct _wave_sample {

     WAVEFORMATEX WaveFormatEx;
     char *pSampleData;
     char *pFXSampleData;
     UINT Index;
     UINT FXIndex;
     UINT Size;
     DWORD dwId;
     DWORD bPlaying;
     struct _wave_sample *pNext;

} WAVE_SAMPLE, *PWAVE_SAMPLE;

// union for data conversion
union UWaveData
{
	float f;
	double d;
	int n;
	unsigned int u;
	unsigned long long u64;
};
// CWaveData
CWaveData::CWaveData(char* pFilePath)
{
	m_bWaveLoaded = false;

	m_pWaveBuffer = NULL;
	m_uLoopCount = 0;
	m_uLoopStartIndex = 0;
	m_uLoopEndIndex = 0;
	m_uLoopType = 0;
	m_uMIDINote = 0;
	m_uMIDIPitchFraction = 0;
	m_uSMPTEFormat = 0;
	m_uSMPTEOffset = 0;

	if(pFilePath)
		m_bWaveLoaded = readWaveFile(pFilePath);
}

CWaveData::~CWaveData()
{
	if(m_pWaveBuffer)
		delete [] m_pWaveBuffer;

}
// prompts with file open dialog, returns TRUE if successfuly
// opened and parsed the file into the member m_pWaveBuffer
bool CWaveData::initWithUserWAVFile(char* pInitDir)
{
	assert(false);
	/* not unicode
	OPENFILENAME ofn; // common dialog box structure
	char szFile[260]; // buffer for file name

    // open a file name
	ZeroMemory( &ofn , sizeof( ofn));
	ofn.lStructSize = sizeof ( ofn );
	ofn.hwndOwner = NULL  ;
	ofn.lpstrFile = szFile ;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "Wave\0*.WAV\0";
	ofn.nFilterIndex =1;
	ofn.lpstrFileTitle = NULL ;
	ofn.nMaxFileTitle = 0 ;
	ofn.lpstrInitialDir=pInitDir ; // you can default to a directory here
	ofn.Flags = OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST ;

	GetOpenFileName( &ofn );

	if(!ofn.lpstrFile)
		return false;

	m_bWaveLoaded = readWaveFile(ofn.lpstrFile);

	//// Now simply display the file name
	// MessageBox ( NULL , ofn.lpstrFile , "File Name" , MB_OK);
	*/
	return m_bWaveLoaded;
}

// THE FOLLOWING TYPES ARE SUPPORTED:
//
// WAVE_FORMAT_PCM and WAVE_FORMAT_EXTENSIBLE
//
// 16-BIT Signed Integer PCM
// 24-BIT Signed Integer PCM 3-ByteAlign
// 24-BIT Signed Integer PCM 4-ByteAlign
// 32-BIT Signed Integer PCM
// 32-BIT Floating Point
// 64-BIT Floating Point
bool CWaveData::readWaveFile(char* pFilePath)
{
	assert(false); // not unicode.
	/*
    bool bSampleLoaded = false;
    RIFF_CHUNK RiffChunk = {0};
    DWORD dwBytes, dwReturnValue;
    WAVE_FILE_HEADER WaveFileHeader;
    DWORD dwIncrementBytes;
    WAVE_SAMPLE WaveSample;

	m_uNumChannels = 0;
	m_uSampleRate = 0;
	m_uSampleCount = 0;

    if(m_hFile = CreateFile(pFilePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL))
    {
		if(m_hFile == INVALID_HANDLE_VALUE)
		{
			m_uLoopType = 0;
			m_uLoopCount = 0;
			m_uLoopStartIndex = 0;
			m_uLoopEndIndex = 0;
			m_uMIDINote = 0;
			m_uMIDIPitchFraction = 0;
			m_uSMPTEFormat = 0;
			m_uSMPTEOffset = 0;

			char* pMessage = addStrings(pFilePath, " could not be opened for reading!");
			MessageBox (NULL , pMessage , "Error" , MB_OK);
			delete [] pMessage;

			if(m_pWaveBuffer)
				delete [] m_pWaveBuffer;

			return false;
		}

        char szIdentifier[5] = {0};

        SetFilePointer(m_hFile, 12, NULL, FILE_CURRENT);

        ReadFile(m_hFile, &RiffChunk, sizeof(RiffChunk), &dwBytes, NULL);
        ReadFile(m_hFile, &WaveFileHeader, sizeof(WaveFileHeader), &dwBytes, NULL);

        WaveSample.WaveFormatEx.wFormatTag      = WaveFileHeader.wFormatTag;
        WaveSample.WaveFormatEx.nChannels       = WaveFileHeader.wChannels;

		// force eeeet to stereo
		// WaveSample.WaveFormatEx.nChannels = 2;

        WaveSample.WaveFormatEx.nSamplesPerSec  = WaveFileHeader.dwSamplesPerSec;
        WaveSample.WaveFormatEx.nAvgBytesPerSec = WaveFileHeader.dwAvgBytesPerSec;
        WaveSample.WaveFormatEx.nBlockAlign     = WaveFileHeader.wBlockAlign;
        WaveSample.WaveFormatEx.wBitsPerSample  = WaveFileHeader.wBitsPerSample;
        WaveSample.WaveFormatEx.cbSize          = 0;

        dwIncrementBytes = dwBytes;

		do {
			// RiffChunk.dwLength - dwIncrementBytes sets the file pointer to position 0 first time through
             SetFilePointer(m_hFile, RiffChunk.dwLength - dwIncrementBytes, NULL, FILE_CURRENT);

             dwReturnValue = GetLastError();

             if(dwReturnValue == 0)
             {
                 dwBytes = ReadFile(m_hFile, &RiffChunk, sizeof(RiffChunk), &dwBytes, NULL);

				 // this now makes the SetFilePointer advance in RiffChunk.dwLength chunks
				 // which vary with the type of chunk
                 dwIncrementBytes = 0;

                 memcpy(szIdentifier, RiffChunk.IdentifierString, 4);
             }

        } while(_stricmp(szIdentifier, "data") && dwReturnValue == 0) ;

		if(WaveSample.WaveFormatEx.wFormatTag != 1 && WaveSample.WaveFormatEx.wFormatTag != 3)
		{
			CloseHandle(m_hFile);
			return false;
		}

		// AUDIO DATA CHUNK data
		// 16 bit
        if(dwReturnValue == 0 && WaveSample.WaveFormatEx.wBitsPerSample == 16)
        {
            WaveSample.pSampleData = (char *)LocalAlloc(LMEM_ZEROINIT, RiffChunk.dwLength);

            WaveSample.Size = RiffChunk.dwLength;

            ReadFile(m_hFile, WaveSample.pSampleData, RiffChunk.dwLength, &dwBytes, NULL);

			UINT nSampleCount = (float)dwBytes/(float)(WaveSample.WaveFormatEx.wBitsPerSample/8.0);
			m_uSampleCount = nSampleCount;

			m_uNumChannels = WaveSample.WaveFormatEx.nChannels;
			m_uSampleRate = WaveSample.WaveFormatEx.nSamplesPerSec;

			if(m_pWaveBuffer)
				delete [] m_pWaveBuffer;

			m_pWaveBuffer = new float[nSampleCount];
			short* pShorts = new short[nSampleCount];
			memset(pShorts, 0, nSampleCount*sizeof(short));
			int m = 0;
			for(UINT i=0; i<nSampleCount; i++)
			{
				// MSB
				pShorts[i] = (unsigned char)WaveSample.pSampleData[m+1];
				pShorts[i] <<= 8;

				// LSB
				short lsb = (unsigned char)WaveSample.pSampleData[m];
				// in case top of lsb is bad
				lsb &= 0x00FF;
				pShorts[i] |= lsb;
				m+=2;
			}

			// convet to float -1.0 -> +1.0
			for(UINT i = 0; i < nSampleCount; i++)
			{
				m_pWaveBuffer[i] = ((float)pShorts[i])/32768.0;
			}

			delete []pShorts;
			LocalFree(WaveSample.pSampleData);

            bSampleLoaded = true;
        }
		// 24 bits
		else if(dwReturnValue == 0 && WaveSample.WaveFormatEx.wBitsPerSample == 24)
        {
            WaveSample.pSampleData = (char *)LocalAlloc(LMEM_ZEROINIT, RiffChunk.dwLength);

            WaveSample.Size = RiffChunk.dwLength;

            ReadFile(m_hFile, WaveSample.pSampleData, RiffChunk.dwLength, &dwBytes, NULL);

			UINT nSampleCount = (float)dwBytes/(float)(WaveSample.WaveFormatEx.wBitsPerSample/8.0);
			m_uSampleCount = nSampleCount;

			m_uNumChannels = WaveSample.WaveFormatEx.nChannels;
			m_uSampleRate = WaveSample.WaveFormatEx.nSamplesPerSec;

			if(m_pWaveBuffer)
				delete [] m_pWaveBuffer;

			// our buffer gets created
			m_pWaveBuffer = new float[nSampleCount];

			int* pSignedInts = new int[nSampleCount];
			memset(pSignedInts, 0, nSampleCount*sizeof(long));

			int m = 0;
			int mask = 0x000000FF;

			// 24-bits in 3-byte packs
			if(WaveSample.WaveFormatEx.nBlockAlign/WaveSample.WaveFormatEx.nChannels == 3)
			{
				for(UINT i=0; i<nSampleCount; i++)
				{
					// MSB
					pSignedInts[i] = (unsigned char)WaveSample.pSampleData[m+2];
					pSignedInts[i] <<= 24;

					// NSB
					int nsb = (int)WaveSample.pSampleData[m+1];
					// in case top of nsb is bad
					nsb &= mask;
					nsb <<= 16;
					pSignedInts[i] |= nsb;

					// LSB
					int lsb = (int)WaveSample.pSampleData[m];
					// in case top of lsb is bad
					lsb &= mask;
					lsb <<= 8;
					pSignedInts[i] |= lsb;

					m+=3;
				}
			}

			// 24-bits in 4-byte packs
			if(WaveSample.WaveFormatEx.nBlockAlign/WaveSample.WaveFormatEx.nChannels == 4)
			{
				for(UINT i=0; i<nSampleCount; i++)
				{
					// MSB
					pSignedInts[i] = (unsigned char)WaveSample.pSampleData[m+3];
					pSignedInts[i] <<= 24;

					// NSB
					int nsb = (int)WaveSample.pSampleData[m+2];
					// in case top of nsb is bad
					nsb &= mask;
					nsb <<= 16;
					pSignedInts[i] |= nsb;

					// NSB2
					int nsb2 = (int)WaveSample.pSampleData[m+1];
					// in case top of nsb is bad
					nsb2 &= mask;
					nsb2 <<= 8;
					pSignedInts[i] |= nsb2;

					// LSB
					int lsb = (int)WaveSample.pSampleData[m];
					// in case top of lsb is bad
					lsb &= mask;
					pSignedInts[i] |= lsb;

					m+=4;
				}
			}

			// convet to float -1.0 -> +1.0
			for(UINT i = 0; i < nSampleCount; i++)
			{
				m_pWaveBuffer[i] = ((float)pSignedInts[i])/2147483648.0; // 2147483648.0 = 1/2 of 2^32
			}

			delete []pSignedInts;
			LocalFree(WaveSample.pSampleData);

            bSampleLoaded = true;

        }
		// 32 bits
		else if(dwReturnValue == 0 && WaveSample.WaveFormatEx.wBitsPerSample == 32)
        {
            WaveSample.pSampleData = (char *)LocalAlloc(LMEM_ZEROINIT, RiffChunk.dwLength);

            WaveSample.Size = RiffChunk.dwLength;

            ReadFile(m_hFile, WaveSample.pSampleData, RiffChunk.dwLength, &dwBytes, NULL);

			UINT nSampleCount = (float)dwBytes/(float)(WaveSample.WaveFormatEx.wBitsPerSample/8.0);
			m_uSampleCount = nSampleCount;

			m_uNumChannels = WaveSample.WaveFormatEx.nChannels;
			m_uSampleRate = WaveSample.WaveFormatEx.nSamplesPerSec;

			if(m_pWaveBuffer)
				delete [] m_pWaveBuffer;

			// our buffer gets created
			m_pWaveBuffer = new float[nSampleCount];

			if(WaveSample.WaveFormatEx.wFormatTag == 1)
			{
				int* pSignedInts = new int[nSampleCount];
				memset(pSignedInts, 0, nSampleCount*sizeof(int));

				int m = 0;
				int mask = 0x000000FF;

				for(UINT i=0; i<nSampleCount; i++)
				{
					// MSB
					pSignedInts[i] = (unsigned char)WaveSample.pSampleData[m+3];
					pSignedInts[i] <<= 24;

					// NSB
					int nsb = (int)WaveSample.pSampleData[m+2];
					// in case top of nsb is bad
					nsb &= mask;
					nsb <<= 16;
					pSignedInts[i] |= nsb;

					// NSB2
					int nsb2 = (int)WaveSample.pSampleData[m+1];
					// in case top of nsb is bad
					nsb2 &= mask;
					nsb2 <<= 8;
					pSignedInts[i] |= nsb2;

					// LSB
					int lsb = (int)WaveSample.pSampleData[m];
					// in case top of lsb is bad
					lsb &= mask;
					pSignedInts[i] |= lsb;

					m+=4;
				}

				// convet to float -1.0 -> +1.0
				for(UINT i = 0; i < nSampleCount; i++)
				{
					m_pWaveBuffer[i] = ((float)pSignedInts[i])/2147483648.0; // 2147483648.0 = 1/2 of 2^32
				}

				delete []pSignedInts;
				LocalFree(WaveSample.pSampleData);

				bSampleLoaded = true;
			}
			else if(WaveSample.WaveFormatEx.wFormatTag == 3) // float
			{
				unsigned int* pUSignedInts = new unsigned int[nSampleCount];
				memset(pUSignedInts, 0, nSampleCount*sizeof(unsigned int));

				int m = 0;
				int mask = 0x000000FF;

				for(UINT i=0; i<nSampleCount; i++)
				{
					// MSB
					pUSignedInts[i] = (unsigned char)WaveSample.pSampleData[m+3];
					pUSignedInts[i] <<= 24;

					// NSB
					int nsb = (unsigned int)WaveSample.pSampleData[m+2];
					// in case top of nsb is bad
					nsb &= mask;
					nsb <<= 16;
					pUSignedInts[i] |= nsb;

					// NSB2
					int nsb2 = (unsigned int)WaveSample.pSampleData[m+1];
					// in case top of nsb is bad
					nsb2 &= mask;
					nsb2 <<= 8;
					pUSignedInts[i] |= nsb2;

					// LSB
					int lsb = (unsigned int)WaveSample.pSampleData[m];
					// in case top of lsb is bad
					lsb &= mask;
					pUSignedInts[i] |= lsb;

					m+=4;
				}

				// Use the Union trick to re-use same memory location as two different data types
				//
				// see: http://www.cplusplus.com/doc/tutorial/other_data_types/#union
				UWaveData wd;
				for(UINT i = 0; i < nSampleCount; i++)
				{
					// save uint version
					wd.u = pUSignedInts[i];
					m_pWaveBuffer[i] = wd.f;
				}

				delete []pUSignedInts;
				LocalFree(WaveSample.pSampleData);

				bSampleLoaded = true;
			}
        }
		// 64 bits
		else if(dwReturnValue == 0 && WaveSample.WaveFormatEx.wBitsPerSample == 64)
        {
            WaveSample.pSampleData = (char *)LocalAlloc(LMEM_ZEROINIT, RiffChunk.dwLength);

            WaveSample.Size = RiffChunk.dwLength;

            ReadFile(m_hFile, WaveSample.pSampleData, RiffChunk.dwLength, &dwBytes, NULL);

			UINT nSampleCount = (float)dwBytes/(float)(WaveSample.WaveFormatEx.wBitsPerSample/8.0);
			m_uSampleCount = nSampleCount;

			m_uNumChannels = WaveSample.WaveFormatEx.nChannels;
			m_uSampleRate = WaveSample.WaveFormatEx.nSamplesPerSec;

			if(m_pWaveBuffer)
				delete [] m_pWaveBuffer;

			// our buffer gets created
			m_pWaveBuffer = new float[nSampleCount];

			// floating point only
			if(WaveSample.WaveFormatEx.wFormatTag == 3) // float
			{
				unsigned long long* pUSignedLongLongs = new unsigned long long[nSampleCount];
				memset(pUSignedLongLongs, 0, nSampleCount*sizeof(unsigned long long));

				int m = 0;
				unsigned long long mask = 0x00000000000000FF;

				for(UINT i=0; i<nSampleCount; i++)
				{
					// MSB
					pUSignedLongLongs[i] = (unsigned char)WaveSample.pSampleData[m+7];
					pUSignedLongLongs[i] <<= 56;

					// NSB
					unsigned long long nsb = (unsigned long long)WaveSample.pSampleData[m+6];
					// in case top of nsb is bad
					nsb &= mask;
					nsb <<= 48;
					pUSignedLongLongs[i] |= nsb;

					// NSB2
					unsigned long long nsb2 = (unsigned long long)WaveSample.pSampleData[m+5];
					// in case top of nsb is bad
					nsb2 &= mask;
					nsb2 <<= 40;
					pUSignedLongLongs[i] |= nsb2;

					// NSB3
					unsigned long long nsb3 = (unsigned long long)WaveSample.pSampleData[m+4];
					// in case top of nsb is bad
					nsb3 &= mask;
					nsb3 <<= 32;
					pUSignedLongLongs[i] |= nsb3;

					// NSB4
					unsigned long long nsb4 = (unsigned long long)WaveSample.pSampleData[m+3];
					// in case top of nsb is bad
					nsb4 &= mask;
					nsb4 <<= 24;
					pUSignedLongLongs[i] |= nsb4;

					// NSB5
					unsigned long long nsb5 = (unsigned long long)WaveSample.pSampleData[m+2];
					// in case top of nsb is bad
					nsb5 &= mask;
					nsb5 <<= 16;
					pUSignedLongLongs[i] |= nsb5;

					// NSB6
					unsigned long long nsb6 = (unsigned long long)WaveSample.pSampleData[m+1];
					// in case top of nsb is bad
					nsb6 &= mask;
					nsb6 <<= 8;
					pUSignedLongLongs[i] |= nsb6;

					// LSB
					unsigned long long lsb = (unsigned long long)WaveSample.pSampleData[m];
					// in case top of lsb is bad
					lsb &= mask;
					pUSignedLongLongs[i] |= lsb;

					m+=8;
				}

				// Use the Union trick to re-use same memory location as two different data types
				//
				// see: http://www.cplusplus.com/doc/tutorial/other_data_types/#union
				UWaveData wd;
				for(UINT i = 0; i < nSampleCount; i++)
				{
					wd.u64 = pUSignedLongLongs[i];
					m_pWaveBuffer[i] = (float)wd.d; // cast the union's double as a float to chop off bottom
				}

				delete []pUSignedLongLongs;
				LocalFree(WaveSample.pSampleData);

				bSampleLoaded = true;
			}
        }

		// LOOK FOR LOOPS!
		LARGE_INTEGER liSize;
		GetFileSizeEx(m_hFile, &liSize);

		int dInc = 0;
		do {
			// note similar method of inc-ing throuh the Chunks, usig the returned
			// dwLength to advance the pointer
             DWORD d = SetFilePointer(m_hFile, dInc, NULL, FILE_CURRENT);

             dwReturnValue = GetLastError();

			 // at EOF?
			 if(d >= liSize.QuadPart)
				 dwReturnValue = 999;

             if(dwReturnValue == 0)
             {
                 dwBytes = ReadFile(m_hFile, &RiffChunk, sizeof(RiffChunk), &dwBytes, NULL);

                 dInc = RiffChunk.dwLength;

                 memcpy(szIdentifier, RiffChunk.IdentifierString, 4);
             }

        } while(_stricmp(szIdentifier, "smpl") && dwReturnValue == 0) ;

		/* smpl CHUNK format
		Offset	Size	Description			Value
		0x00	4	Chunk ID			"smpl" (0x736D706C)
		0x04	4	Chunk Data Size		36 + (Num Sample Loops * 24) + Sampler Data
		0x08	4	Manufacturer		0 - 0xFFFFFFFF	<------ // SKIPPING THIS //
		0x0C	4	Product				0 - 0xFFFFFFFF	<------ // SKIPPING THIS //
		0x10	4	Sample Period		0 - 0xFFFFFFFF	<------ // SKIPPING THIS (already know it)//
		0x14	4	MIDI Unity Note		0 - 127
		0x18	4	MIDI Pitch Fraction	0 - 0xFFFFFFFF
		0x1C	4	SMPTE Format		0, 24, 25, 29, 30
		0x20	4	SMPTE Offset		0 - 0xFFFFFFFF
		0x24	4	Num Sample Loops	0 - 0xFFFFFFFF
		0x28	4	Sampler Data		0 - 0xFFFFFFFF
		0x2C
		List of Sample Loops ------------------ //

		MIDI Unity Note
			The MIDI unity note value has the same meaning as the instrument chunk's MIDI Unshifted Note field which
			specifies the musical note at which the sample will be played at it's original sample rate
			(the sample rate specified in the format chunk).

		MIDI Pitch Fraction
			The MIDI pitch fraction specifies the fraction of a semitone up from the specified MIDI unity note field.
			A value of 0x80000000 means 1/2 semitone (50 cents) and a value of 0x00000000 means no fine tuning
			between semitones.

		SMPTE Format
			The SMPTE format specifies the Society of Motion Pictures and Television E time format used in the
			following SMPTE Offset field. If a value of 0 is set, SMPTE Offset should also be set to 0.
			Value	SMPTE Format
			0	no SMPTE offset
			24	24 frames per second
			25	25 frames per second
			29	30 frames per second with frame dropping (30 drop)
			30	30 frames per second

		SMPTE Offset
			The SMPTE Offset value specifies the time offset to be used for the synchronization / calibration
			to the first sample in the waveform. This value uses a format of 0xhhmmssff where hh is a signed value
			that specifies the number of hours (-23 to 23), mm is an unsigned value that specifies the
			number of minutes (0 to 59), ss is an unsigned value that specifies the number of seconds
			(0 to 59) and ff is an unsigned value that specifies the number of frames (0 to -1).

		/* Sample Loop Data Struct
		Offset	Size	Description		Value
		0x00	4	Cue Point ID	0 - 0xFFFFFFFF
		0x04	4	Type			0 - 0xFFFFFFFF
		0x08	4	Start			0 - 0xFFFFFFFF
		0x0C	4	End				0 - 0xFFFFFFFF
		0x10	4	Fraction		0 - 0xFFFFFFFF
		0x14	4	Play Count		0 - 0xFFFFFFFF

		Loop type:
			Value	Loop Type
			0	Loop forward (normal)
			1	Alternating loop (forward/backward, also known as Ping Pong)
			2	Loop backward (reverse)
			3 - 31	Reserved for future standard types
			32 - 0xFFFFFFFF	Sampler specific types (defined by manufacturer)* /

		// skipping some stuff; could add later
		if(dwReturnValue == 0)
        {
			 dInc = 12; // 3 don't cares 4 bytes each
			m_uLoopType = 0;
			m_uLoopCount = 0;
			m_uLoopStartIndex = 0;
			m_uLoopEndIndex = 0;
			m_uMIDINote = 0;
			m_uMIDIPitchFraction = 0;
			m_uSMPTEFormat = 0;
			m_uSMPTEOffset = 0;

			// found a loop set; currently only taking the FIRST loop set
			// only need one loop set for potentially sustaining waves
             DWORD d = SetFilePointer(m_hFile, dInc, NULL, FILE_CURRENT);

			 // loop count
			 ReadFile(m_hFile, &m_uMIDINote, 4, &dwBytes, NULL);

			 // loop count
			 ReadFile(m_hFile, &m_uMIDIPitchFraction, 4, &dwBytes, NULL);

			 // loop count
			 ReadFile(m_hFile, &m_uSMPTEFormat, 4, &dwBytes, NULL);

			 // loop count
			 ReadFile(m_hFile, &m_uSMPTEOffset, 4, &dwBytes, NULL);

			 // loop count
			 ReadFile(m_hFile, &m_uLoopCount, 4, &dwBytes, NULL);

			 // skip cuepoint & sampledata
			 d = SetFilePointer(m_hFile, 8, NULL, FILE_CURRENT);

			 // loop type
			 ReadFile(m_hFile, &m_uLoopType, 4, &dwBytes, NULL);

			 // loop start sample
			 ReadFile(m_hFile, &m_uLoopStartIndex, 4, &dwBytes, NULL);
			 m_uLoopStartIndex *= m_uNumChannels;

			 // loop end sample
			 ReadFile(m_hFile, &m_uLoopEndIndex, 4, &dwBytes, NULL);
			 m_uLoopEndIndex *= m_uNumChannels;
		}
		else  // no loops
		{
			m_uLoopType = 0;
			m_uLoopCount = 0;
			m_uLoopStartIndex = 0;
			m_uLoopEndIndex = 0;
			m_uMIDINote = 0;
			m_uMIDIPitchFraction = 0;
			m_uSMPTEFormat = 0;
			m_uSMPTEOffset = 0;
		}

		CloseHandle(m_hFile);
	}
	return bSampleLoaded;
	*/
return false;
}

#endif


// UI CONTROL CLASS -- DO NOT DELETE
CUICtrl::CUICtrl()
{
	m_pUserCookedIntData = NULL;
	m_pUserCookedFloatData = NULL;
	m_pUserCookedDoubleData = NULL;
	m_pUserCookedUINTData = NULL;
	m_pCurrentMeterValue = NULL;

	cControlName = &cName[0];
	cControlUnits = &cUnits[0];
	cVariableName = &cVName[0];
	cEnumeratedList = &cVEnumeratedList[0];
	cMeterVariableName = &cMeterVName[0];
	uUserDataType = nonData;

	memset(&cName[0], 0, 1024*sizeof(BYTE));
	memset(&cVName[0], 0, 1024*sizeof(BYTE));
	memset(&cMeterVName[0], 0, 1024*sizeof(BYTE));
	memset(&cUnits[0], 0, 1024*sizeof(BYTE));
	memset(&cEnumeratedList[0], 0, 1024*sizeof(BYTE));

	memset(&dPresetData[0], 0, PRESET_COUNT*sizeof(double));

	bOwnerControl = false;
	bUseMeter = false;
	bLogMeter = false;
	bUpsideDownMeter = false;
	uMeterColorScheme = csVU;
	fMeterAttack_ms = 10.0;
	fMeterRelease_ms = 500.0;

	bLogSlider = false;
	bExpSlider = false;

	uDetectorMode = DETECT_MODE_RMS;
	nGUIRow					= -1;
	nGUIColumn				= -1;
	pvAddlData				= NULL;
	memset(&uControlTheme[0], 0, CONTROL_THEME_SIZE*sizeof(UINT));
	memset(&uFluxCapControl[0], 0, PLUGIN_CONTROL_THEME_SIZE*sizeof(UINT));
	memset(&fFluxCapData[0], 0, PLUGIN_CONTROL_THEME_SIZE*sizeof(float));

	bMIDIControl			= false;
	uMIDIControlCommand		= 0xB0; // CC
	uMIDIControlName		= 0x03; // default CC #3
	uMIDIControlChannel		= 0;
}

CUICtrl::CUICtrl(const CUICtrl& initCUICtrl)
{
	cControlName = &cName[0];
	cControlUnits = &cUnits[0];
	cVariableName = &cVName[0];
	cEnumeratedList = &cVEnumeratedList[0];
	cMeterVariableName = &cMeterVName[0];

	uControlType				= initCUICtrl.uControlType;
	uControlId				= initCUICtrl.uControlId;
	bOwnerControl			= initCUICtrl.bOwnerControl;
	bUseMeter			= initCUICtrl.bUseMeter;
	bLogMeter			= initCUICtrl.bLogMeter;
	bUpsideDownMeter			= initCUICtrl.bUpsideDownMeter;
	uDetectorMode			= initCUICtrl.uDetectorMode;
	bLogSlider			= initCUICtrl.bLogSlider;
	bExpSlider			= initCUICtrl.bExpSlider;

	uMeterColorScheme			= initCUICtrl.uMeterColorScheme;
	fMeterAttack_ms			= initCUICtrl.fMeterAttack_ms;
	fMeterRelease_ms			= initCUICtrl.fMeterRelease_ms;

	fUserDisplayDataLoLimit				= initCUICtrl.fUserDisplayDataLoLimit;
	fUserDisplayDataHiLimit				= initCUICtrl.fUserDisplayDataHiLimit;

	uUserDataType				= initCUICtrl.uUserDataType;
	fInitUserIntValue				= initCUICtrl.fInitUserIntValue;
	fInitUserFloatValue				= initCUICtrl.fInitUserFloatValue;
	fInitUserDoubleValue				= initCUICtrl.fInitUserDoubleValue;
	fInitUserUINTValue				= initCUICtrl.fInitUserUINTValue;

	// VST PRESETS
	fUserCookedIntData				= initCUICtrl.fUserCookedIntData;
	fUserCookedFloatData				= initCUICtrl.fInitUserFloatValue;
	fUserCookedDoubleData				= initCUICtrl.fUserCookedDoubleData;
	fUserCookedUINTData				= initCUICtrl.fUserCookedUINTData;

	m_pUserCookedIntData				= initCUICtrl.m_pUserCookedIntData;
	m_pUserCookedFloatData				= initCUICtrl.m_pUserCookedFloatData;
	m_pUserCookedDoubleData				= initCUICtrl.m_pUserCookedDoubleData;
	m_pUserCookedUINTData				= initCUICtrl.m_pUserCookedUINTData;
	m_pCurrentMeterValue				= initCUICtrl.m_pCurrentMeterValue;

	bMIDIControl				= initCUICtrl.bMIDIControl;
	uMIDIControlCommand			= initCUICtrl.uMIDIControlCommand;
	uMIDIControlName			= initCUICtrl.uMIDIControlName;
	uMIDIControlChannel = initCUICtrl.uMIDIControlChannel;

	nGUIRow					= initCUICtrl.nGUIRow;
	nGUIColumn				= initCUICtrl.nGUIColumn;
	pvAddlData				= initCUICtrl.pvAddlData;
	for(int i=0; i<PRESET_COUNT; i++)
	{
		dPresetData[i] = initCUICtrl.dPresetData[i];
	}

	for(int i=0; i<CONTROL_THEME_SIZE; i++)
	{
		uControlTheme[i] = initCUICtrl.uControlTheme[i];
	}

	for(int i=0; i<PLUGIN_CONTROL_THEME_SIZE; i++)
	{
		uFluxCapControl[i] = initCUICtrl.uFluxCapControl[i];
	}

	for(int i=0; i<PLUGIN_CONTROL_THEME_SIZE; i++)
	{
		fFluxCapData[i] = initCUICtrl.fFluxCapData[i];
	}

    strncpy(cControlName, initCUICtrl.cControlName, 1023);
    cControlName[1023] = '\0';

    strncpy(cControlUnits, initCUICtrl.cControlUnits, 1023);
    cControlUnits[1023] = '\0';

    strncpy(cVariableName, initCUICtrl.cVariableName, 1023);
    cVariableName[1023] = '\0';

	strncpy(cEnumeratedList, initCUICtrl.cEnumeratedList, 1023);
	cEnumeratedList[1023] = '\0';

	strncpy(cMeterVariableName, initCUICtrl.cMeterVariableName, 1023);
	cMeterVariableName[1023] = '\0';
}

CUICtrl::~CUICtrl()
{

}
CUIControlList::CUIControlList()
{
     p=NULL;
}


CUICtrl* CUIControlList::getAt(int nIndex)
{
	node *q;
	int c=0;
	for( q=p ; q != NULL ; q = q->link )
	{
		if(c == nIndex)
			return &q->data;

		c++;
	}

	return NULL;

}

void CUIControlList::append(CUICtrl num)
{
     node *q,*t;

   if( p == NULL )
   {
        p = new node;
      p->data = num;
      p->link = NULL;
   }
   else
   {
        q = p;
      while( q->link != NULL )
           q = q->link;

      t = new node;
      t->data = num;
      t->link = NULL;
      q->link = t;
   }
}

void CUIControlList::add_as_first(CUICtrl num)
{
     node *q;

   q = new node;
   q->data = num;
   q->link = p;
   p = q;
}

void CUIControlList::addafter( int c, CUICtrl num)
{
     node *q,*t;
   int i;
   for(i=0,q=p;i<c;i++)
   {
        q = q->link;
      if( q == NULL )
      {
           //cout<<"\nThere are less than "<<c<<" elements.";
         return;
      }
   }

   t = new node;
   t->data = num;
   t->link = q->link;
   q->link = t;
}

void CUIControlList::update( CUICtrl num )
{
     node *q,*r;
   q = p;
   if( q->data.uControlId == num.uControlId )
   {
	   q->data = num;
      return;
   }

   r = q;
   while( q!=NULL )
   {
      if( q->data.uControlId == num.uControlId )
      {
		   q->data = num;
		  return;
      }
	  r = q;
	  q = q->link;
    }


   //cout<<"\nElement "<<num<<" not Found.";
}
void CUIControlList::del( CUICtrl num )
{
     node *q,*r;
   q = p;
   if( q->data.uControlId == num.uControlId )
   {
        p = q->link;
      delete q;
      return;
   }

   r = q;
   while( q!=NULL )
   {
        if( q->data.uControlId == num.uControlId )
      {
           r->link = q->link;
         delete q;
         return;
      }

      r = q;
      q = q->link;
   }
   //cout<<"\nElement "<<num<<" not Found.";
}

void CUIControlList::display()
{
//      node *q;
//   cout<<endl;

//   for( q = p ; q != NULL ; q = q->link )
  //      cout<<endl<<q->data;

}
int CUIControlList::countLegalVSTIF()
{
     node *q;
   int c=0;
   for( q=p ; q != NULL ; q = q->link )
   {
	   if(q->data.uControlType == FILTER_CONTROL_CONTINUOUSLY_VARIABLE ||
		   q->data.uControlType == FILTER_CONTROL_RADIO_SWITCH_VARIABLE)
	   {
			c++;
	   }
   }
   return c;
}

int CUIControlList::countLegalCustomVSTGUI()
{
     node *q;
   int c=0;
   for( q=p ; q != NULL ; q = q->link )
   {
		if(q->data.uControlType == FILTER_CONTROL_CONTINUOUSLY_VARIABLE)
		{
			 c++;
		}
		if(q->data.uControlType == FILTER_CONTROL_RADIO_SWITCH_VARIABLE)
		{
				c++;
		}
   }

   return c;
}

int CUIControlList::count()
{
     node *q;
   int c=0;
   for( q=p ; q != NULL ; q = q->link )
        c++;

   return c;
}

CUIControlList::~CUIControlList()
{
    node *q;
   if( p == NULL )
        return;

   while( p != NULL )
   {
        q = p->link;
      delete p;
      p = q;
   }
}