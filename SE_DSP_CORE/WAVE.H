
// wave file header
// wave.h

#ifndef _WIN32
#include <cstdint>
typedef uint32_t LONG;
typedef int16_t SHORT;
#endif

struct wave_file_header
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
