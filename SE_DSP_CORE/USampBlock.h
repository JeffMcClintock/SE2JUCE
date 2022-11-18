#if !defined(AFX_USAMPBLOCK_H__30BFCAC0_B968_11D4_93A8_00104B15CCF0__INCLUDED_)

#define AFX_USAMPBLOCK_H__30BFCAC0_B968_11D4_93A8_00104B15CCF0__INCLUDED_

#pragma once

#include <algorithm>

#if defined( _WIN32 )
#include <malloc.h>
#else
#include <stdlib.h>
#endif

#include "sample.h"
#include "assert.h"

class ug_base;

// extra samples on end to check for memory overwrite

#define USAMPLEBLOCK_SAFETY_ZONE_SIZE 8

// leave a few samples empty at end of each block
// as badly behaved plugins may overwrite block end point
#define magic_guard_number 12.12345f


class USampBlock
{

public:
#ifdef _DEBUG
	float custom_guard_number;
#endif

	// inline due to high useage.

	inline float GetAt(int p_index)
	{
#ifdef _DEBUG
		assert( m_samples != NULL );
		assert( p_index >= 0 && p_index < debugBlocksize_ );
#endif
		return m_samples[p_index];
	}

	inline float* GetBlock()
	{
		return m_samples;
	}

	void Reset()
	{
		m_write_pos = 0;
	}

	USampBlock( int blockSize ) :
		m_write_pos(0)
	#if defined( _DEBUG )
		,debugBlocksize_(blockSize)
	#endif
	{
		AllocateBuffer( blockSize );
		SetAll( 0.0f, blockSize );
	}

	void AllocateBuffer( int blockSize )
	{
		// aligned to make life easy for SSE SIMD instructions
		int mem_size = sizeof(float) * (blockSize + USAMPLEBLOCK_SAFETY_ZONE_SIZE);

		const int allignmentBytes = 32; // aligned for SSE/AVX
#if defined( _WIN32 )
        m_samples = (float*) _aligned_malloc( mem_size, allignmentBytes );
#else
        posix_memalign((void**) &m_samples, allignmentBytes, mem_size);
#endif
		assert((0x1f & (intptr_t)m_samples) == 0 && "expecting 32-byte alignment");

		// provide detection of overwrites
	#ifdef _DEBUG
		custom_guard_number = magic_guard_number + (((intptr_t)m_samples >> 10) & 0xff);
		//	_RPT2(_CRT_WARN, "%x custom_guard_number = %f\n", this,custom_guard_number );
		float* past_end = m_samples + blockSize;

		for( int i = USAMPLEBLOCK_SAFETY_ZONE_SIZE ; i > 0 ; --i )
		{
			*past_end++ = custom_guard_number;
		}

	#endif
	}

	int CheckOverwrite()
	{
		// TODO need to work in non-debug.
	#ifdef _DEBUG
		float* past_end = m_samples + debugBlocksize_;

		for( int i = USAMPLEBLOCK_SAFETY_ZONE_SIZE -1 ; i >= 0 ; --i )
		{
			if( past_end[i] != custom_guard_number )
			{
				return i + 1;
			}
		}
	#endif

		return 0;
	}

	virtual ~USampBlock()
	{
	#if defined( _MSC_VER )
		_aligned_free(m_samples);
	#else
		free(m_samples);
	#endif
	}

	void Add(float p_val)
	{
		assert( m_write_pos >= 0 && m_write_pos < debugBlocksize_ );
		m_samples[m_write_pos++] = p_val;
	}

	void SetAt(int p_index, float val)
	{
		assert( p_index >= 0 && p_index < debugBlocksize_ );
		m_samples[p_index] = val;
	}

	void SetAll( float p_val, int blockSize )
	{
		for( int b = blockSize - 1 ; b >= 0 ; b-- )
		{
			m_samples[b] = p_val;
		}
	}

	void SetRange(int from_idx, int count, float value)
	{
		assert( from_idx >= 0 && from_idx < debugBlocksize_ );
		assert( from_idx + count <= debugBlocksize_ );
		assert(count > 0);

		for( int b = from_idx ; b < from_idx + count; b++ )
		{
			m_samples[b] = value;
		}
	}

	#ifdef _DEBUG
	void Copy(USampBlock* from)
	{
		//	_RPT3(_CRT_WARN, "USampBlock::Copy %x->%x, first val=%f\n", from,this, m_samples[0]);
		for( int b = USAMPLEBLOCK_SAFETY_ZONE_SIZE + debugBlocksize_ - 1 ; b >= 0 ; b-- )
		{
			m_samples[b] = from->m_samples[b];
		}
	}

	bool Compare(USampBlock* other, int from, int to)
	{
		int first_corrupt = 10000;
		int last_corrupt = -1;

		for( int b = from ; b < to; b++ )
		{
			if( ((int*)m_samples)[b] != ((int*)other->m_samples)[b]) // bitwise compare (avoids fail on #INF)
			{
				first_corrupt = (std::min)( first_corrupt, b );
				last_corrupt = (std::max)( first_corrupt, b );
				_RPT3(_CRT_WARN, "    buffer[%3d] was %f -> %f\n", b, m_samples[b], other->m_samples[b] );
			}
		}

		if( last_corrupt > -1 )
		{
			_RPT2(_CRT_WARN, "Overwrote %d -> %d\n", first_corrupt, last_corrupt );
			return false;
		}
		else
			return true;
	}
	#endif

	#ifdef _DEBUG
	bool CheckAllSame()
	{
		float val = m_samples[0];

		for( int b = debugBlocksize_ - 1 ; b >= 0 ; b-- )
		{
			if( *(int*) &(m_samples[b]) != *(int*) (&val) ) // raw bitwise comparison to handle infinite/NANs etc
			{
				if( m_samples[b] != val ) // handle +ve and -ve zero (which are not bitwize equal)
				{
	#ifdef _DEBUG
					_RPT0(_CRT_WARN, "CheckAllSame() FAIL !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" );
					/*
										int j = 10;
										for(int i = 0 ; i < debugBlocksize_ ;i++ )
										{
											_RPT1(_CRT_WARN, "%e ", m_samples[i] );
											if(j-- < 0 )
											{
												_RPT0(_CRT_WARN, "\n" );
												j = 10;
											}
										}
										_RPT0(_CRT_WARN, "\n\n" );
					*/
	#endif
					return false;
				}
			}
		}

		return true;
	}
#endif


private:

	int m_write_pos;

	float* m_samples;

#ifdef _DEBUG

	int debugBlocksize_;

#endif

};



#endif // !defined(AFX_USAMPBLOCK_H__30BFCAC0_B968_11D4_93A8_00104B15CCF0__INCLUDED_)

