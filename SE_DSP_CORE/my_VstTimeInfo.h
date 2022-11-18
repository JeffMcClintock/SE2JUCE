#pragma once

// inline static const int my_VstTimeInfo_TransportPlaying = 1 << 1;

struct my_VstTimeInfo // to avoid including VST header and getting millions of compiler warnings (in every file using this header)
{
	enum
	{
		kVstTransportChanged = 1,
		kVstTransportPlaying = 1 << 1,
		kVstTransportCycleActive = 1 << 2,

		kVstAutomationWriting = 1 << 6,
		kVstAutomationReading = 1 << 7,

		// flags which indicate which of the fields in this VstTimeInfo
		//  are valid; samplePos and sampleRate are always valid
		kVstNanosValid = 1 << 8,
		kVstPpqPosValid = 1 << 9,
		kVstTempoValid = 1 << 10,
		kVstBarsValid = 1 << 11,
		kVstCyclePosValid = 1 << 12,	// start and end
		kVstTimeSigValid = 1 << 13,
		kVstSmpteValid = 1 << 14,
		kVstClockValid = 1 << 15
	};

	double samplePos;			// current location
	double sampleRate;
	double nanoSeconds;			// system time
	double ppqPos;				// 1 ppq
	double tempo;				// in bpm
	double barStartPos;			// last bar start, in 1 ppq
	double cycleStartPos;		// 1 ppq
	double cycleEndPos;			// 1 ppq
	int32_t timeSigNumerator;		// time signature
	int32_t timeSigDenominator;
	int32_t smpteOffset;
	int32_t smpteFrameRate;			// 0:24, 1:25, 2:29.97, 3:30, 4:29.97 df, 5:30 df
	int32_t samplesToNextClock;		// midi clock resolution (24 ppq), can be negative
	int32_t flags;					// see below

	bool TransportRun() const
	{
		return (flags & kVstTransportPlaying) != 0;
	}
};
