#pragma once

#include <string>
#include <vector>
#include "mp_sdk_common.h"

#ifdef _WIN32
typedef unsigned long       DWORD;
#else
typedef uint32_t           DWORD;
#endif
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef short SHORT;

#define NUM_SF_MODULATORS 60
/*
The SFSampleLink is an enumeration type which describes both the type of sample (mono, stereo left,
etc.) and the whether the sample is located in RAM or ROM memory:
*/
#pragma pack( 1 ) // force correct alignment (default is 8, leaves padding gaps)
typedef enum
{
monoSample = 1,
rightSample = 2,
leftSample = 4,
linkedSample = 8,
RomMonoSample = 0x8001,
RomRightSample = 0x8002,
RomLeftSample = 0x8004,
RomLinkedSample = 0x8008
} SFSampleLink;

struct sfSample
{
char achSampleName[20];
DWORD dwStart;
DWORD dwEnd;
DWORD dwStartloop;
DWORD dwEndloop;
DWORD dwSampleRate;
BYTE byOriginalKey;
char chCorrection;
WORD wSampleLink;
//SFSampleLink sfSampleType; // this enum being complied as 4 byte not 2
WORD sfSampleType;
};

struct sfPresetHeader
{
char achPresetName[20];
WORD wPreset;
WORD wBank;
WORD wPresetBagNdx;
DWORD dwLibrary;
DWORD dwGenre;
DWORD dwMorphology;
};

struct sfPresetBag
{
WORD wGenNdx;
WORD wModNdx;
};

// enum types
typedef WORD SFModulator;
typedef WORD SFGenerator;
typedef WORD SFTransform;

//instrument zone modulators
struct sfModList
{
SFModulator sfModSrcOper;
SFGenerator sfModDestOper;
SHORT modAmount;
SFModulator sfModAmtSrcOper;
SFTransform sfModTransOper;
};

typedef struct
{
BYTE byLo;
BYTE byHi;
} rangesType;

typedef union
{
rangesType ranges;
SHORT shAmount;
WORD wAmount;
} genAmountType;

struct sfInstGenList
{
SFGenerator sfGenOper;
genAmountType genAmount;
};

struct sfInst
{
char achInstName[20];
WORD wInstBagNdx;
};

struct sfInstBag
{
WORD wInstGenNdx;
WORD wInstModNdx;
};

//The genAmountType is a union which allows signed 16 bit, unsigned 16 bit, and two unsigned 8 bit
//fields:

struct sfGenList
{
SFGenerator sfGenOper;
genAmountType genAmount;
};

// back to regular packing
#pragma pack ()

// helper class to hold a zone
class Jzone
{
public:
	genAmountType Get(int index);
	void Set( sfInstGenList *igen );
	void SetDefaults();

	short channel;
	genAmountType value[NUM_SF_MODULATORS];
};

namespace ZoneGenerator
{
	enum Generators{
		PAN=17,
		SAMPLE_MODES=54
	};
};

enum SF_Patch_Type{SFP_OK,SFP_EMPTY,SFP_ROM_SAMPLE};

class CSoundFont
{
	static const int padSampleMemorybytes = 8; // 4 2-byte samples of padding for interpolator
public:
	CSoundFont();
	 ~CSoundFont();
	sfPresetHeader * getPresetHeader( int bank, int patch );
	SF_Patch_Type PatchType(int bank, int patch);
	bool isBankEmpty(int bank);
	std::string getPatchName(int bank,int p_patch);

	bool LoadZoneList(short channel, short bank, char patch, std::vector<Jzone> & zone_list);
	void LoadZoneList_pt2(short channel, int instrument_index, std::vector<Jzone> & zone_list);
	void IntersectGenRange( sfGenList *pgen, int zone_lo,  std::vector<Jzone> & zone_list);

	sfSample * GetSampleHeader(int id);
	WORD* GetSampleChunk(){return chunk_smpl + padSampleMemorybytes / sizeof(*chunk_smpl);}
	bool Load( gmpi::IProtectedFile2* file );
	int get_count_shdr(){return count_shdr;}
#if defined( _DEBUG )
	void Dump(int type);
#endif

	// reference counting;
	int AddRef()
	{
		return ++reference_count;
	}
	int Release()
	{
		int r = --reference_count;
		if( reference_count == 0 )
			delete this;
		return r;
	}

private:
	int reference_count;

	unsigned int count_imod = 0;
	unsigned int count_ibag = 0;
	unsigned int count_inst = 0;
	unsigned int count_pgen = 0;
	unsigned int count_pbag = 0;
	unsigned int count_pmod = 0;
	unsigned int count_phdr = 0;
	unsigned int count_shdr = 0;
	unsigned int count_smpl = 0;
	unsigned int count_igen = 0;
	sfModList		* chunk_pmod = {};
	sfInstBag		* chunk_ibag = {};
	sfInst			* chunk_inst = {};
	sfGenList		* chunk_pgen = {};
	sfModList		* chunk_imod = {};
	sfPresetBag		* chunk_pbag = {};
	WORD			* chunk_smpl = {};
	sfSample		* chunk_shdr = {};
	sfPresetHeader	* chunk_phdr = {};
	sfInstGenList	* chunk_igen = {};
};

struct sf_chan_info
{
	char patch_num;
	char bank_num;
	bool enabled;
};

typedef std::vector<Jzone*> activeZoneListType;

// reference counter pointer to CSoundFont
//TODO: SF to only load samples in use, else it holds megabytes in mem, multiplied sometimes by several SF Oscs
class SoundfontPatch //: public CSoundFont
{
public:
	SoundfontPatch( CSoundFont *soundfont );
	~SoundfontPatch( )
	{
//		_RPT0(_CRT_WARN, "DEL SoundfontPatch\n" );
		soundfont_->Release();
	};
	void GetPlayingZones( short p_chan, short p_note, short p_vel, activeZoneListType &returnZones);
	void SetProgram(short chan, WORD patch);
	void SetBank(short chan, WORD p_bank);
	sf_chan_info * ChannelInfo( int p_chan ){ return channels + p_chan;};
	sfSample* GetSampleHeader( int id )
	{
		return soundfont_->GetSampleHeader( id );
	};
	WORD* GetSampleChunk()
	{
		return soundfont_->GetSampleChunk();
	};
	CSoundFont* GetSoundfont()
	{
		return soundfont_;
	};

	// reference counting;
	int AddRef()
	{
		return ++reference_count;
	};
	int Release()
	{
		int r = --reference_count;
		if( reference_count == 0 )
			delete this;
		return r;
	};

private:
	int reference_count;

	sf_chan_info channels[16];
	std::vector<Jzone> zone_list;
	CSoundFont* soundfont_;
};

