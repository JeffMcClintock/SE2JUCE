/*
	RackAFX(TM)
	Applications Programming Interface
	Derived Class Object Definition
	Copyright(c) Tritone Systems Inc. 2006-2012

	Your plug-in must implement the constructor,
	destructor and virtual Plug-In API Functions below.
*/

#pragma once

// base class
#include "plugin.h"
#include "Decimator.h"
#include "Interpolator.h"
#include "TPTMoogLadderFilter.h"
#include "MoogLadderFilter.h"

// abstract base class for RackAFX filters
class CVAFilters : public CPlugIn
{
public:
	// RackAFX Plug-In API Member Methods:
	// The followung 5 methods must be impelemented for a meaningful Plug-In
	//
	// 1. One Time Initialization
	CVAFilters();

	// 2. One Time Destruction
	virtual ~CVAFilters(void);

	// 3. The Prepare For Play Function is called just before audio streams
	virtual bool __stdcall prepareForPlay();

	// 4. processAudioFrame() processes an audio input to create an audio output
	virtual bool __stdcall processAudioFrame(float* pInputBuffer, float* pOutputBuffer, UINT uNumInputChannels, UINT uNumOutputChannels);

	// 5. userInterfaceChange() occurs when the user moves a control.
	virtual bool __stdcall userInterfaceChange(int nControlIndex);


	// OPTIONAL ADVANCED METHODS ------------------------------------------------------------------------------------------------
	// These are more advanced; see the website for more details
	//
	// 6. initialize() is called once just after creation; if you need to use Plug-In -> Host methods
	//				   such as sendUpdateGUI(), you must do them here and NOT in the constructor
	virtual bool __stdcall initialize();

	// 7. joystickControlChange() occurs when the user moves a control.
	virtual bool __stdcall joystickControlChange(float fControlA, float fControlB, float fControlC, float fControlD, float fACMix, float fBDMix);

	// 8. process buffers instead of Frames:
	// NOTE: set m_bWantBuffers = true to use this function
	virtual bool __stdcall processRackAFXAudioBuffer(float* pInputBuffer, float* pOutputBuffer, UINT uNumInputChannels, UINT uNumOutputChannels, UINT uBufferSize);

	// 9. rocess buffers instead of Frames:
	// NOTE: set m_bWantVSTBuffers = true to use this function
	virtual bool __stdcall processVSTAudioBuffer(float** ppInputs, float** ppOutputs, UINT uNumChannels, int uNumFrames);

	// 10. MIDI Note On Event
	virtual bool __stdcall midiNoteOn(UINT uChannel, UINT uMIDINote, UINT uVelocity);

	// 11. MIDI Note Off Event
	virtual bool __stdcall midiNoteOff(UINT uChannel, UINT uMIDINote, UINT uVelocity, bool bAllNotesOff);


	// 12. MIDI Modulation Wheel uModValue = 0 -> 127
	virtual bool __stdcall midiModWheel(UINT uChannel, UINT uModValue);

	// 13. MIDI Pitch Bend
	//					nActualPitchBendValue = -8192 -> 8191, 0 is center, corresponding to the 14-bit MIDI value
	//					fNormalizedPitchBendValue = -1.0 -> +1.0, 0 is at center by using only -8191 -> +8191
	virtual bool __stdcall midiPitchBend(UINT uChannel, int nActualPitchBendValue, float fNormalizedPitchBendValue);

	// 14. MIDI Timing Clock (Sunk to BPM) function called once per clock
	virtual bool __stdcall midiClock();


	// 15. all MIDI messages -
	// NOTE: set m_bWantAllMIDIMessages true to get everything else (other than note on/off)
	virtual bool __stdcall midiMessage(unsigned char cChannel, unsigned char cStatus, unsigned char cData1, unsigned char cData2);

	// 16. initUI() is called only once from the constructor; you do not need to write or call it. Do NOT modify this function
	virtual bool __stdcall initUI();



	// Add your code here: ----------------------------------------------------------- //
	void flushDelays(void);

	CBiQuad m_LeftBiQuad;
	CBiQuad m_RightBiQuad;

	// one for left, one for right
	float m_fZ1[2];

	// for second integrator
	float m_fZ2[2];

	// TPT 1st Order
	void doTPT1PoleFilter(float *pInput, float* pOutput, int numChannels, UINT uFilterType);

	// TPT 2nd Order State Variable Filter
	void doTPTSVF(float *pInput, float* pOutput, int numChannels, UINT uFilterType);

	// BiQuad Bilinear Z-Transform version
	void calculateBZTCoeffs(UINT uFilterType);
	void doBZTFilter(float *pInput, float* pOutput, int numChannels, UINT uFilterType);

	// ladder filters
	CTPTMoogLadderFilter leftTPTMoogLPF;
	CTPTMoogLadderFilter rightTPTMoogLPF;

	// from my earlier Bonus Project
	CMoogLadderFilter leftStilsonMoogLPF;
	CMoogLadderFilter rightStilsonMoogLPF;

	void doLadderFilter(float *pInput, float* pOutput, int numChannels, UINT uFilterType);

	// big function that sorts and calls others
	void doFilters(float *pInput, float* pOutput, int numChannels, UINT uFilterType);

	float m_fFilterSampleRate;

	// oversampling stuff
	int m_nOversamplingRatio;

	CInterpolator m_Interpolator;
	CDecimator m_Decimator;

	float* m_pLeftInterpBuffer;
	float* m_pRightInterpBuffer;

	float* m_pLeftDecipBuffer;
	float* m_pRightDeciBuffer;
	// END OF USER CODE -------------------------------------------------------------- //


	// ADDED BY RACKAFX -- DO NOT EDIT THIS CODE!!! ----------------------------------- //
	//  **--0x07FD--**

	float m_fFc;
	float m_fQ;
	float m_fK;
	UINT m_uFilterType;
	enum{TPT_LPF1,BZT_LPF1,MASS_LPF1,TPT_HPF1,BZT_HPF1,SVF_LPF2,BZT_LPF2,MASS_LPF2,SVF_HPF2,BZT_HPF2,SVF_BPF2,SVF_UBPF2,SVF_BSHELF,SVF_NOTCH2,SVF_PEAK2,SVF_APF2,TPT_LADDER,STILSON_LADDER};
	UINT m_uOversampleMode;
	enum{OFF,ON};

	// **--0x1A7F--**
	// ------------------------------------------------------------------------------- //

};




















