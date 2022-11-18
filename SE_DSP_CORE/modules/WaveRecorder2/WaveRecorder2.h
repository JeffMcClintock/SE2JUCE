#ifndef WAVERECORDER2_H_INCLUDED
#define WAVERECORDER2_H_INCLUDED

#include "../se_sdk3/mp_sdk_audio.h"
#include <memory>
#include <vector>

#ifndef _MSC_VER
typedef int32_t LONG;
typedef int16_t SHORT;
#endif


struct wave_file_header2
{
	char chnk1_name[4];
	LONG chnk1_size;
	char chnk2_name[4];
	char chnk3_name[4];
	LONG chnk3_size;
	SHORT wFormatTag;
	SHORT nChannels;
	LONG nSamplesPerSec;
	LONG nAvgBytesPerSec;
	SHORT nBlockAlign;
	SHORT wBitsPerSample;
	char chnk4_name[4];
	LONG chnk4_size;
};

class WaveRecorder2 : public MpBase2
{
public:
	WaveRecorder2();
	~WaveRecorder2();
	void CloseFile();
	virtual int32_t MP_STDCALL open() override;
	void subProcess(int sampleFrames);
	void subProcess16bit(int sampleFrames);
	virtual void onSetPins() override;

private:
	StringInPin pinFileName;
	IntInPin pinFormat;
	FloatInPin pinTimeLimit;

	std::vector< std::unique_ptr<AudioInPin> > AudioIns;
	std::vector< float* > AudioInPtrs;
	std::vector< unsigned char > AudioBuffer;
	FILE* outputStream;
	int64_t sampleFrameCount;

	wave_file_header2 waveHeader;
	int64_t maxFrames = -1;
};

#endif

