#include "pch.h"
#include "ULookup.h"
#include <assert.h>
#include <math.h>
#include "modules/shared/xp_simd.h"
#include "./modules/shared/xplatform.h"
#include "ug_base.h"


static const int GuardValue = 123454321;

//extern short m_fp_mode_normal,m_fp_mode_round_down;
LookupTable::LookupTable() :
	init_flag( false )
	,size( 0 )
	,table(0)
{
}

LookupTable::~LookupTable()
{
#if defined( _MSC_VER )
	_aligned_free( table );
#else
	free( table );
#endif
}

void ULookup::SetValue( int index, float val )
{
	assert( index >= 0 && index < size );

	// remove denormals
	if( val < INSIGNIFIGANT_SAMPLE && val > -INSIGNIFIGANT_SAMPLE && val != 0.f )
	{
		//_RPT0(_CRT_WARN, "Denormal blocked from lookup table\n" );
		val = 0.f;
	}
	(( float*) table)[index] = val;
}

void ULookup_Integer::SetValue( int index, int val )
{
	assert( index >= 0 && index < size );
	( (int*) table )[index] = val;
}

void LookupTable::SetSize( int s )
{
#if defined( _MSC_VER )
	_aligned_free( table );
#else
	free( table );
#endif

	table = 0;

#if defined( _DEBUG )
	int bytes = sizeof(int) * (s+1);
#else
	int bytes = sizeof(int)* s;
#endif

	const int allignmentBytes = 32; // for SSE.
#if defined( _WIN32 )
	table = (int*)_aligned_malloc(bytes, allignmentBytes);
#else
//	m_samples = (float*)mal loc(bytes);
	posix_memalign((void**)&table, allignmentBytes, bytes);
#endif
	assert((0x1f & (intptr_t)table) == 0 && "expecting 32-byte alignment");

	if( table != 0 ) // else memory allocation failed.
	{
		size = s;

#if defined( _DEBUG )
		( (float*) table )[size] = GuardValue;
#endif
	}
}

bool LookupTable::CheckOverwrite( )
{
	return ( (float*) table )[size] == GuardValue;
}


SharedOscWaveTable::SharedOscWaveTable(int /*nothing*/) :
	lookup_(0)
	,sampleRate_(-99)
{
}

float ULookup::GetValueIndexed2( int actual_index )
{
	assert( actual_index >= 0 && actual_index < size );
	return ((float*)table)[actual_index];
}

bool SharedOscWaveTable::Create( ug_base* ug, const std::wstring &name, int sampleRate )
{
	ug->CreateSharedLookup2( name, lookup_, sampleRate, (sizeof(float) - 1 + sizeof(OscWaveTable)) / sizeof(float), true, SLS_ALL_MODULES );

	bool needInit = !lookup_->GetInitialised();

	if( needInit )
	{
//		lookup_->SetSize( (sizeof(float) - 1 + sizeof(OscWaveTable)) / sizeof(float) );
		lookup_->SetInitialised(); // anticipating caller will ALWAYS init the data.
		sampleRate_ = sampleRate;
	}

	return needInit;
}
