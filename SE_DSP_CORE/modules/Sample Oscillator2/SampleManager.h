#pragma once

#include <map>
#include "csoundfont.h"
#include "mp_sdk_common.h"

struct sample_info
{
	int handle = 0;
	std::wstring filename;
	int bank = 0;
	int patch = 0;
	SoundfontPatch* patchData = {};
};

typedef std::map<int , sample_info*> soundfontContainer_t;

class SampleManager
{
public:
	SampleManager();
	~SampleManager();
	static SampleManager* Instance();

	int Load( gmpi::IProtectedFile2* file, const wchar_t* filename, int bank, int patch );
	void GetPlayingZones( int sampleHandle, short p_chan, short p_note, short p_vel, activeZoneListType &returnZones);
	sfSample* GetSampleHeader( int sampleHandle, int id );
	WORD* GetSampleChunk( int sampleHandle );
	int AddRef( int sampleHandle );
	int Release( int sampleHandle );

private:
	soundfontContainer_t soundfonts;
};
