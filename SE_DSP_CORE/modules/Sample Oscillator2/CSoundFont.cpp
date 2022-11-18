// CSoundFont.cpp
//
#include <algorithm>
#include <assert.h>
#include "csoundfont.h"
#include "RiffFile2.h"

CSoundFont::CSoundFont() : 
	chunk_inst(NULL)
	,chunk_pgen(NULL)
	,chunk_pbag(NULL)
	,chunk_smpl(NULL)
	,chunk_shdr(NULL)
	,chunk_phdr(NULL)
	,chunk_igen(NULL)
	,chunk_pmod(NULL)
	,chunk_ibag(NULL)
	,chunk_imod(NULL)
	,reference_count(0)
{
//	_RPT1(_CRT_WARN, "NEW CSoundFont() %d\n", this );
}

bool CSoundFont::Load( gmpi::IProtectedFile2* file )
{
	RiffFile2 riff;
	uint32_t riff_type;

	riff.Open( file, riff_type );

	// Create list of chunks we could use
	if( riff_type != MAKEFOURCC('s', 'f', 'b', 'k') )	// sf2 file
		return true;

//	riff.REG_CHUNK( "Sample Data",			"sdta", "smpl", &chunk_smpl, &count_smpl, WORD);
	riff.AddChunk(RiffFile2::riff_match("Sample Data", "sdta", "smpl", ((char**)&chunk_smpl), &count_smpl, sizeof(WORD), 0, padSampleMemorybytes));
	riff.REG_CHUNK( "Preset Headers",		"pdta", "phdr", &chunk_phdr, &count_phdr, sfPresetHeader);
	riff.REG_CHUNK( "Preset Bag Headers",	"pdta", "pbag", &chunk_pbag, &count_pbag, sfPresetBag);
	riff.REG_CHUNK( "Preset Layer Modulators","pdta","pmod", &chunk_pmod, &count_pmod, sfModList);
	riff.REG_CHUNK( "Preset Layers",		"pdta", "pgen", &chunk_pgen, &count_pgen, sfGenList);
	riff.REG_CHUNK( "Instruments",			"pdta", "inst", &chunk_inst, &count_inst, sfInst);
	riff.REG_CHUNK( "Inst Splits",			"pdta", "ibag", &chunk_ibag, &count_ibag, sfInstBag);
	riff.REG_CHUNK( "Inst Split Modulators","pdta", "imod", &chunk_imod, &count_imod, sfModList);
	riff.REG_CHUNK( "Inst Split Split Gens","pdta", "igen", &chunk_igen, &count_igen, sfInstGenList );
	riff.REG_CHUNK( "Sample Headers",		"pdta", "shdr", &chunk_shdr, &count_shdr, sfSample );

	return riff.ReadFile();
}

CSoundFont::~CSoundFont()
{
	delete [] (char*) chunk_inst;
	delete [] (char*) chunk_pgen;
	delete [] (char*) chunk_pbag;
	delete [] (char*) chunk_smpl;
	delete [] (char*) chunk_shdr;
	delete [] (char*) chunk_phdr;
	delete [] (char*) chunk_igen;
	delete [] (char*) chunk_pmod;
	delete [] (char*) chunk_ibag;
	delete [] (char*) chunk_imod;

//	_RPT1(_CRT_WARN, "DEL ~CSoundFont() %d\n", this );
}

#if defined( _DEBUG )
void CSoundFont::Dump(int type)
{
	if( type == 0 ) // dump sample headers
	{
		for( unsigned int id = 0; id < count_shdr; id++ )
		{
			sfSample *s = GetSampleHeader(id);
//			_RPT2(_CRT_WARN, "%d :achSampleName %s\n", id, s->achSampleName );
//			_RPT1(_CRT_WARN, "wSampleLink %d\n", s->wSampleLink );
//			_RPT1(_CRT_WARN, "sfSampleType %d", s->sfSampleType );
			if( (s->sfSampleType & 0x8000) != 0 )
			{
//				_RPT0(_CRT_WARN, " ROM" );
			}
			if( (s->sfSampleType & monoSample) != 0 )
			{
//				_RPT0(_CRT_WARN, " MONO" );
			}
			if( (s->sfSampleType & leftSample) != 0 )
			{
//				_RPT0(_CRT_WARN, " LEFT" );
			}
			if( (s->sfSampleType & rightSample) != 0 )
			{
//				_RPT0(_CRT_WARN, " RIGHT" );
			}
			if( (s->sfSampleType & linkedSample) != 0 )
			{
//				_RPT0(_CRT_WARN, " linked" );
			}
//			_RPT0(_CRT_WARN, "\n" );
//			_RPT1(_CRT_WARN, "dwStart %d\n", s->dwStart );
//			_RPT1(_CRT_WARN, "dwEnd %d\n", s->dwEnd );
//			_RPT0(_CRT_WARN, "\n" );
		}
	}

	if( type == 1 ) // zones
	{

	}
}
#endif
/*
struct sfSample
{
CHAR achSampleName[20];
DWORD dwStart;
DWORD dwEnd;
DWORD dwStartloop;
DWORD dwEndloop;
DWORD dwSampleRate;
BYTE byOriginalKey;
CHAR chCorrection;
WORD wSampleLink;
//SFSampleLink sfSampleType; // this enum being complied as 4 byte not 2
WORD sfSampleType;
};
*/

// load zone list for a given patch
bool CSoundFont::LoadZoneList(short channel, short bank, char patch, std::vector<Jzone> & zone_list)
{
	assert( channel >= 0 && channel < 16 );
	assert( bank >= 0 && bank < 129 );
	assert( patch >= 0 && patch < 128 );

	// delete any zones for this chan
	for( std::vector<Jzone>::iterator it = zone_list.begin() ; it != zone_list.end() ; )
	{
//		if( zone_list->GetAt(i).channel == channel )
//			zone_list->RemoveAt(i);
		if( (*it).channel == channel )
		{
			it = zone_list.erase( it );
		}
		else
		{
			++it;
		}
	}

	// load new zones	// First find current preset
/*	int bank = 0;

	if( channel == 9 ) // special case percussion is bank 128 on chan 10
		bank = 128;
*/
	// get this preset (patch)
	sfPresetHeader *phdr = getPresetHeader(bank, patch);

	// did we find preset?
	if( phdr == 0 )
	{
//		_RPT1(_CRT_WARN,"Sample Play: Unsupported Patch %d\n", patch);
		return false;
	}

	sfPresetBag *pbag = chunk_pbag + phdr->wPresetBagNdx;
	// determin last pbag by peeking ahead to 1st pbag of next preset 
	int num_pbag = phdr[1].wPresetBagNdx - phdr->wPresetBagNdx;

	int instrument_index = -1; // default for slack files

	// for each preset bag ..
	for(int i = 0; i < num_pbag ; i ++ )
	{
		unsigned short gen_ndx = pbag[i].wGenNdx;
		unsigned short gen_ndx2 = pbag[i+1].wGenNdx;
		int last_loaded_zone_lo = 0;

//not using		short mod_ndx = pbag[i].wModNdx;
		// look thru gnerator in reverse,
		// because instrument index is always last generator
		// we can then apply preset-level generators last
		for( int j = gen_ndx2 - 1 ; j >= gen_ndx ; j-- )
		{
			// get the generator
			sfGenList *pgen = chunk_pgen + j;

//			_RPT3(_CRT_WARN," PGEN %3d: sh %d, wd %d\n", (int)pgen->sfGenOper, (int)pgen->genAmount.shAmount, (int)pgen->genAmount.wAmount);
			
			// not used		genAmountType amt = pgen->genAmount;
			switch( pgen->sfGenOper )
			{
			case 41:	// instrument index. Must be last (except in global preset zone, which doesn't have it)
				{
					last_loaded_zone_lo = (int) zone_list.size();
					instrument_index = pgen->genAmount.wAmount;
					LoadZoneList_pt2( channel, instrument_index, zone_list );
//					last_loaded_zone_hi = zone_list->GetSize();
				}
				break;
			case 43:	// key range
				{
					IntersectGenRange( pgen, last_loaded_zone_lo, zone_list);
				}
				break;
			case 44:	// velocity range
				{
					IntersectGenRange( pgen, last_loaded_zone_lo, zone_list);
				}
				break;
			default:
				{
//					_RPT2(_CRT_WARN,"Chan %d : Ignoring global GEN %d\n", channel, pgen->sfGenOper );
				}
			};
		}
	}
/*
	if( instrument_index == -1 ) // none found, assume idx = 0
	{
		LoadZoneList_pt2( channel, 0, zone_list );
	}*/

/*
	for( int i = zone_list->GetUpperBound(); i >= 0 ; i-- )
	{
		Jzone &z = zone_list->GetAt(i);
		_RPT2(_CRT_WARN, "%d : chan %d\n", i,z.channel );
		_RPT1(_CRT_WARN, "Sample Header %d\n",z.Get(53).wAmount );
		_RPT1(_CRT_WARN, "NoteLo %d\n", z.Get(43).ranges.byLo );
		_RPT1(_CRT_WARN, "NoteHi %d\n",  z.Get(43).ranges.byHi );
		_RPT1(_CRT_WARN, "VelLo %d\n", z.Get(44).ranges.byLo);
		_RPT1(_CRT_WARN, "VelHi %d\n",z.Get(44).ranges.byHi );
	}
*/
	return true;
}
 
// if split applied at preset level, it intesects range applied at inst level
void CSoundFont::IntersectGenRange( sfGenList *pgen, int zone_lo,  std::vector<Jzone> & zone_list)
{
	int gen_id = pgen->sfGenOper;
	int range_lo = pgen->genAmount.ranges.byLo;
	int range_hi = pgen->genAmount.ranges.byHi;
	sfInstGenList ig;
	ig.sfGenOper = pgen->sfGenOper;

	for( unsigned int z = zone_lo ; z < zone_list.size() ; ++z )
	{
		ig.genAmount = zone_list[z].Get(gen_id);
        ig.genAmount.ranges.byLo = (std::max)( (int) ig.genAmount.ranges.byLo, range_lo );
        ig.genAmount.ranges.byHi = (std::min)( (int) ig.genAmount.ranges.byHi, range_hi );
		zone_list[z].Set(&ig);
	}
}

void CSoundFont::LoadZoneList_pt2( short channel, int instrument_index, std::vector<Jzone> & zone_list)
{
	unsigned short gen_ndx2;
	unsigned short mod_ndx2;
	Jzone zone;
	sfInst *inst;
	sfInstBag *ibag, *last_ibag;

	// now we have an instrument index to play
	inst = chunk_inst + instrument_index;

	// determin first and last instument split
	ibag = chunk_ibag + inst->wInstBagNdx;
	last_ibag = chunk_ibag + inst[1].wInstBagNdx;

	Jzone global_zone;
	global_zone.SetDefaults();
	while( ibag != last_ibag )
	{
		// preset bag ..
		gen_ndx2 = ibag->wInstGenNdx;
		mod_ndx2 = ibag->wInstModNdx;
		
		zone = global_zone;

		sfInstGenList *igen = 0;

		// store the generators in zone
		for( int kk = gen_ndx2 ; kk < ibag[1].wInstGenNdx ; kk++)
		{
			igen = chunk_igen + kk;
			zone.Set(igen);
//			_RPT3(_CRT_WARN," GEN %3d: sh %d, wd %d\n", (int)igen->sfGenOper, (int)igen->genAmount.shAmount, (int)igen->genAmount.wAmount);
		}

//		if( igen == 0 ) // empty zone
//			_RPT0(_CRT_WARN,"Empty ");

		// Do we have a global zone?
		//( check last generator not sampleid )
		if( igen == 0 || igen->sfGenOper != 53 ) // this is a global zone
		{
//			_RPT0(_CRT_WARN,"^^^ Global Zone\n");
			global_zone = zone;
		}
		else // this is a standard zone
		{
//			_RPT0(_CRT_WARN,"^^^ Local Zone\n");
			zone.channel = channel;
			zone_list.push_back(zone);
		}

		// check out next one
		ibag++;
	}
}

// Class Jzone, helper
sfSample * CSoundFont::GetSampleHeader(int id)
{
	assert( id >= 0 && id < (int) count_shdr );
	return chunk_shdr + id;
}

genAmountType Jzone::Get(int index)
{
	assert( index < NUM_SF_MODULATORS );

	if( index < NUM_SF_MODULATORS )
		return value[index];

	return value[0]; //??what else
}

void Jzone::SetDefaults()
{
	for(int i = 0 ; i < NUM_SF_MODULATORS; i++ )
	{
		switch(i)
		{
		case 8:
			value[i].wAmount = 13500;
			break;
		case 21:
		case 23:
		case 25:
		case 26:
		case 27:
		case 28:
		case 30:
		case 33:
		case 34:
		case 35:
		case 36:
		case 38:
			value[i].wAmount = -12000;
			break;
		case 43:
		case 44:
			value[i].ranges.byLo = 0;
			value[i].ranges.byHi = 127;
			break;
		case 46:
		case 47:
		case 58:
			value[i].wAmount = -1;
			break;
		case 56:
			value[i].wAmount = 100;
			break;
		default:
			value[i].wAmount = 0;
		}
	}
}

void Jzone::Set(sfInstGenList *igen)
{
	if( igen->sfGenOper < NUM_SF_MODULATORS )
		value[igen->sfGenOper] = igen->genAmount;
}

std::string CSoundFont::getPatchName(int bank, int p_patch)
{
	// get this preset (patch)
	sfPresetHeader *phdr = getPresetHeader(bank, p_patch);

	// did we find preset?
	if( phdr != 0 )
	{
		return phdr->achPresetName;
	}

	return "";
}

SF_Patch_Type CSoundFont::PatchType(int bank, int patch)
{
	bool rom_samples = false;

	sfInstGenList *igen;
	unsigned short gen_ndx2;
	unsigned short mod_ndx2;
	sfInstBag *ibag, *last_ibag;

	sfInst *inst;

	// get this preset (patch)
	sfPresetHeader *phdr = getPresetHeader(bank, patch);

	// did we find preset?
	if( phdr == 0 )
	{
//		_RPT1(_CRT_WARN,"Sample Play: Unsupported Patch %d\n", patch);
		return SFP_EMPTY;
	}

	sfPresetBag *pbag = chunk_pbag + phdr->wPresetBagNdx;
	// determin last pbag by peeking ahead to 1st pbag of next preset 
	int num_pbag = phdr[1].wPresetBagNdx - phdr->wPresetBagNdx;

	int instrument_index = 0; // default to zero as some files assume this

	// for each preset bag ..
	for(int i = 0; i < num_pbag ; i ++ )
	{
		short gen_ndx = pbag[i].wGenNdx;
		short mod_ndx = pbag[i].wModNdx;

		// get the generator
		sfGenList *pgen = chunk_pgen + gen_ndx;

		// get the type of generator
		WORD gen = pgen->sfGenOper;
		
		// not used		genAmountType amt = pgen->genAmount;
		switch( gen )
		{
		case 41:	//instrument index
			instrument_index = pgen->genAmount.wAmount;
			break;
		};
	}

	// now we have an instrument index to play
	inst = chunk_inst + instrument_index;

	// determin first and last instument split
	ibag = chunk_ibag + inst->wInstBagNdx;
	last_ibag = chunk_ibag + inst[1].wInstBagNdx;

	while( ibag != last_ibag )
	{
		// preset bag ..
		gen_ndx2 = ibag->wInstGenNdx;
		mod_ndx2 = ibag->wInstModNdx;
		
		// get the generators
		// search for correct zone based on key & vel
		for( int kk = gen_ndx2 ; kk < ibag[1].wInstGenNdx ; kk++)
		{
			igen = chunk_igen + kk;

			if( igen->sfGenOper == 53 ) // sample header number
			{
				sfSample *cur_sample = GetSampleHeader( igen->genAmount.wAmount );
				if( (cur_sample->sfSampleType & 0x8000) == 0 ) // not rom sample?
				{
					// valid sample exists
					return SFP_OK; // patch not empty
				}
				else
				{
					rom_samples = true;
				}
			}
		}

		// check out next one
		ibag++;
	}

	if( rom_samples)
		return SFP_ROM_SAMPLE;
	else
		return SFP_EMPTY;
}

bool CSoundFont::isBankEmpty(int bank)
{
	sfInstGenList *igen;
	unsigned short gen_ndx2;
	unsigned short mod_ndx2;
	sfInstBag *ibag, *last_ibag;

	sfInst *inst;

	if( chunk_phdr == 0 ) // file not loaded or corrupt
		return 0;

	// iterate presets (patches)
	sfPresetHeader *phdr = chunk_phdr;
	sfPresetHeader *last_phdr = chunk_phdr + count_phdr - 1;

	while( phdr < last_phdr )
	{
		if( phdr->wBank == bank )
		{
			sfPresetBag *pbag = chunk_pbag + phdr->wPresetBagNdx;
			// determin last pbag by peeking ahead to 1st pbag of next preset 
			int num_pbag = phdr[1].wPresetBagNdx - phdr->wPresetBagNdx;

			// for each preset bag ..
			for(int i = 0; i < num_pbag ; i ++ )
			{
				short gen_ndx = pbag[i].wGenNdx;
				short mod_ndx = pbag[i].wModNdx;

				// get the generator
				sfGenList *pgen = chunk_pgen + gen_ndx;

				// get the type of generator
				WORD gen = pgen->sfGenOper;
				
				// not used		genAmountType amt = pgen->genAmount;
				switch( gen )
				{
				case 41:	//instrument index
					// now we have an instrument index to play
					inst = chunk_inst + pgen->genAmount.wAmount;

					// determin first and last instument split
					ibag = chunk_ibag + inst->wInstBagNdx;
					last_ibag = chunk_ibag + inst[1].wInstBagNdx;

					while( ibag != last_ibag )
					{
						// preset bag ..
						gen_ndx2 = ibag->wInstGenNdx;
						mod_ndx2 = ibag->wInstModNdx;
						
						// get the generators
						// search for correct zone based on key & vel
						for( int kk = gen_ndx2 ; kk < ibag[1].wInstGenNdx ; kk++)
						{
							igen = chunk_igen + kk;

							if( igen->sfGenOper == 53 ) // sample header number
							{
								sfSample *cur_sample = GetSampleHeader( igen->genAmount.wAmount );
								if( (cur_sample->sfSampleType & 0x8000) == 0 ) // not rom sample?
								{
									// valid sample exists
									return false; // patch not empty
								}
							}
						}

						// check out next instrument split
						ibag++;
					}
					break;
				};
			}
		}
		phdr++;
	}
	return true;
}

sfPresetHeader * CSoundFont::getPresetHeader(int bank, int patch)
{
	if( chunk_phdr == 0 ) // file not loaded or corrupt
		return 0;

	// search for this preset (patch)
	sfPresetHeader *phdr = chunk_phdr;
	sfPresetHeader *last_phdr = chunk_phdr + count_phdr - 1;

	while( phdr < last_phdr && ( phdr->wPreset != patch || phdr->wBank != bank ) )
		phdr++;

	if( phdr == last_phdr)
	{
		return 0;
	}

	assert(phdr->wPreset == patch && phdr->wBank == bank);
	return phdr;
}

SoundfontPatch::SoundfontPatch( CSoundFont *soundfont ) :
reference_count(0)
,soundfont_(soundfont)
{
//	_RPT0(_CRT_WARN, "NEW SoundfontPatch\n" );
	// init patch numbers
	for(int chan = 15; chan >= 0 ; chan -- )
	{
		channels[chan].bank_num = -1;
		channels[chan].patch_num = -1;
		channels[chan].enabled = true;
	}
//	channels[9].bank_num = 128; // special case percussion is bank 128 on chan 10
}


void SoundfontPatch::SetProgram(short chan, WORD patch)
{
	// will be called by each voice, so avoid extra work
	if( channels[chan].patch_num != patch )
	{
		channels[chan].patch_num = (char) patch;

		if( channels[chan].bank_num >= 0 ) // -1 = 'not set'
		{
			soundfont_->LoadZoneList( chan, channels[chan].bank_num, (char) patch, zone_list );
		}
	}
}

void SoundfontPatch::SetBank(short chan, WORD p_bank)
{
	// will be called by each voice, so avoid extra work
	if( channels[chan].bank_num != p_bank )
	{
		channels[chan].bank_num = (unsigned char) p_bank;
		if( channels[chan].patch_num >= 0 ) // it's initially -1
		{
			soundfont_->LoadZoneList( chan, p_bank, channels[chan].patch_num, zone_list );
		}
	}
}

void SoundfontPatch::GetPlayingZones(short p_chan, short p_note, short p_vel, activeZoneListType &returnZones )
{
	returnZones.clear();

	for( std::vector<Jzone>::reverse_iterator it = zone_list.rbegin() ; it != zone_list.rend() ; ++it )
	{
		Jzone &z = *it;
		short NoteLo = z.Get(43).ranges.byLo;
		short NoteHi = z.Get(43).ranges.byHi;
		// vel range
		short VelLo = z.Get(44).ranges.byLo;
		short VelHi = z.Get(44).ranges.byHi;
		if( z.channel == p_chan && p_note <= NoteHi && p_note >= NoteLo && p_vel <= VelHi && p_vel >= VelLo )
		{
			//return &z;
			returnZones.push_back( &z );
		}
	}
/*
	if( zone_list.GetUpperBound() == -1 )
	{
		_RPT0(_CRT_WARN, "Sample Player, EMPTY PATCH\n" );
	}
	else
	{
		_RPT2(_CRT_WARN,"Sample Play:no suitable sample (Note %d, Vel %d)\n",  p_note,  p_vel);
	}
*/
//	return 0;
}
