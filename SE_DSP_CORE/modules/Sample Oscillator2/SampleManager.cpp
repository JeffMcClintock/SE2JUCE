#include "SampleManager.h"
#include "assert.h"
#include "../../mfc_emulation.h"
#include "../shared/unicode_conversion.h"

SampleManager* SampleManager::Instance()
{
	static SampleManager obj; // not dll-safe(static object destructor don't work).
	return &obj;
}

SampleManager::SampleManager( )
{
}

SampleManager::~SampleManager()
{
	for( soundfontContainer_t::iterator it = soundfonts.begin() ; it != soundfonts.end() ; ++it )
	{
		sample_info* si = (*it).second;
		delete si->patchData;
		delete si;
	}
}

int SampleManager::Load( gmpi::IProtectedFile2 *file, const wchar_t* filename, int bank, int patch )
{
	const int channel = 0;
	CSoundFont* soundfontData = 0;

	for( soundfontContainer_t::iterator it = soundfonts.begin() ; it != soundfonts.end() ; ++it )
	{
		sample_info* si = (*it).second;
		if( si )
		{
			if( si->filename == filename )
			{
				// We found the raw sample data.
				soundfontData = si->patchData->GetSoundfont();

				if( si->bank == bank && si->patch == patch )
				{
					// We found the zone-list for that exact patch.
					si->patchData->AddRef();
					return si->handle;
				}
			}
		}
	}

	// Can't find soundfont raw data in memory, load it.
	if( soundfontData == 0 )
	{
		soundfontData = new CSoundFont();
		bool result = soundfontData->Load( file );
		if( result == true ) // failed to load
		{
			delete soundfontData;
			return -1;
		}
	}
	soundfontData->AddRef();

	SoundfontPatch* patchData = new SoundfontPatch( soundfontData );
	patchData->AddRef();
	patchData->SetProgram( channel, patch );
	patchData->SetBank( channel, bank );

	// Random Handle generator
	int freeHandle;
	soundfontContainer_t::iterator it;
	do
	{
		freeHandle = rand() & 0x7fffffff; // generate positive integers only
		it = soundfonts.find(freeHandle);
	}while( it != soundfonts.end() );

	sample_info* si = new sample_info;
	si->handle = freeHandle;
	si->filename = filename;
	si->bank = bank;
	si->patch = patch;
	si->patchData = patchData;

	soundfonts.insert( std::pair<int , sample_info*>(freeHandle, si ) );

#ifdef _DEBUG
{
    auto filenameUtf8 = JmUnicodeConversions::WStringToUtf8(filename);
	_RPT2(_CRT_WARN, "SampleManager::Load( '%s' ) handle=%d\n", filenameUtf8.c_str(), freeHandle );
}
#endif

	return freeHandle;
}

void SampleManager::GetPlayingZones( int sampleHandle, short p_chan, short p_note, short p_vel, activeZoneListType &returnZones)
{
	soundfontContainer_t::iterator it = soundfonts.find( sampleHandle );
	if( it != soundfonts.end() )
	{
		(*it).second->patchData->GetPlayingZones( p_chan, p_note, p_vel, returnZones );
	}
}

sfSample* SampleManager::GetSampleHeader( int sampleHandle, int id )
{
	soundfontContainer_t::iterator it = soundfonts.find( sampleHandle );
	if( it != soundfonts.end() )
	{
		return (*it).second->patchData->GetSampleHeader( id );
	}
	return 0;
}

WORD* SampleManager::GetSampleChunk( int sampleHandle )
{
	soundfontContainer_t::iterator it = soundfonts.find( sampleHandle );
	if( it != soundfonts.end() )
	{
		return (*it).second->patchData->GetSampleChunk();
	}
	return 0;
}

int SampleManager::AddRef( int sampleHandle )
{
	soundfontContainer_t::iterator it = soundfonts.find( sampleHandle );
	assert( it != soundfonts.end() );

	if( it != soundfonts.end() )
	{
		return (*it).second->patchData->AddRef();
	}

	return -1;
}

int SampleManager::Release( int sampleHandle )
{
	if( sampleHandle < 0 )
		return 0;

	_RPT1(_CRT_WARN, "SampleManager::Release() handle=%d\n", sampleHandle );

	soundfontContainer_t::iterator it = soundfonts.find( sampleHandle );
	if( it != soundfonts.end() )
	{
		sample_info* si = (*it).second;
		int refcount = si->patchData->Release();
		if( refcount == 0 )
		{
			_RPT0(_CRT_WARN, "  DELETED\n" );
			delete si;
			soundfonts.erase( it );
		}
		else
		{
			_RPT1(_CRT_WARN, "  Refcount = %d\n", refcount );
		}

		return refcount;
	}

	_RPT0(_CRT_WARN, "  not found!\n" );

	return -1;
}
