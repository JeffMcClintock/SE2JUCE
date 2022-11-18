#include "RiffFile.h"
#include <assert.h>


#include "csoundfont.h"

bool CRiffFile::Open( gmpi::IProtectedFile* file, DWORD& riff_type )
{
	riff_type = 0;
	
	int32_t size;
	file->getSize( size );

	delete [] buffer;
	buffer = new char[size];

	file->read( buffer, size );

	MMIOINFO inf;
	memset(&inf,0,sizeof(inf)); // clear
	inf.fccIOProc = FOURCC_MEM;
	inf.pchBuffer = (HPSTR) buffer;
	inf.cchBuffer = size;

	file_handle = mmioOpen( 0, &inf, MMIO_READ); 

	if(file_handle == NULL)
		return true;

	return Open2(riff_type);
}


bool CRiffFile::Open( const wchar_t* filename, DWORD & riff_type )
{
	riff_type = 0;

	file_handle = mmioOpen( (LPWSTR) filename, NULL, MMIO_READ); 

	if(file_handle == NULL)
		return true;

	return Open2(riff_type);
}

bool CRiffFile::Open2( DWORD & riff_type)
{
	MMCKINFO chnk;
	
	level = 0;

	// Get first chunk (describes file)
	int res = mmioDescend(file_handle, &chnk, NULL,NULL);
	cnk[level] = chnk;//  header_chnk;
	
	// Check this is a RIFF file
	if( chnk.ckid != mmioFOURCC('R', 'I', 'F', 'F') )
		return true;

	riff_type = chnk.fccType;

//	char *j1 = (char*) &(chnk.fccType);
//	char *j2 = (char*) &(chnk.ckid);
//	_RPT4(_CRT_WARN, "'%c%c%c%c' \n",j2[0],j2[1],j2[2],j2[3]);
//	_RPT4(_CRT_WARN, "'%c%c%c%c' \n",j1[0],j1[1],j1[2],j1[3]);

	return false;
}

void CRiffFile::AddChunk( riff_match &riff_desc )
{
	rec_chunks.push_back(riff_desc);
}

#define TP_WAVE 0x45564157
#define TP_WAVEDATA 0x61746164

bool CRiffFile::ReadFile()
{
//	CString m;
	int res = 0;
	MMCKINFO chnk;

	// cnk[level] keeps track of depth of current chunk (usually no more than 1)
	while(res == 0){
		if ((res = mmioDescend(file_handle, &chnk, &(cnk[level]),NULL))) { 
			if(level > 0){
				res = 0;
				mmioAscend(file_handle, &(cnk[level--]), 0);
			}
		}else{
			if( chnk.ckid == mmioFOURCC('L', 'I', 'S', 'T') ){
				cnk[++level] = chnk;
			}else{
/*
// print file chunk tag
char *j1 = (char*) &(cnk[level].fccType);
char *j2 = (char*) &(chnk.ckid);
_RPT4(_CRT_WARN, "Current >%c%c%c%c< ",j1[0],j1[1],j1[2],j1[3]);
_RPT4(_CRT_WARN, ">%c%c%c%c< \n",j2[0],j2[1],j2[2],j2[3]);
*/
				// Handle specific chunk types
				for( int j = (int)rec_chunks.size() - 1; j >= 0 ;j--)
				{
/*
char *j3 = (char*) &(rec_chunks[j].fccType);
char *j4 = (char*) &(rec_chunks[j].ckid);
_RPT4(_CRT_WARN, "TEST - >%c%c%c%c< ",j3[0],j3[1],j3[2],j3[3]);
_RPT4(_CRT_WARN, ">%c%c%c%c< \n",j4[0],j4[1],j4[2],j4[3]);
*/
					if( rec_chunks[j].fccType == cnk[level].fccType && rec_chunks[j].ckid == chnk.ckid )
					{
						// found one....
/*
char *j3 = (char*) &(rec_chunks[j].fccType);
char *j4 = (char*) &(rec_chunks[j].ckid);
_RPT4(_CRT_WARN, "TEST - >%c%c%c%c< ",j3[0],j3[1],j3[2],j3[3]);
_RPT4(_CRT_WARN, ">%c%c%c%c< \n",j4[0],j4[1],j4[2],j4[3]);
*/

						if( rec_chunks[j].count_addr )	// caller wants chunk size (in specific units)
						{
							*(rec_chunks[j].count_addr) = chnk.cksize/rec_chunks[j].item_size;
							assert( chnk.cksize % rec_chunks[j].item_size == 0 );	// CHUNK SIZE ERROR!
						}

						if( rec_chunks[j].file_offset ) // caller wants file offset of chunk
						{
							MMIOINFO file_info;
							res = mmioGetInfo( file_handle, &file_info, 0 );
							if( file_info.fccIOProc == FOURCC_MEM ) //reading from mem
							{
								*(rec_chunks[j].file_offset) = (int) file_info.pchNext - (int) file_info.pchBuffer;
							}
							else // reading from disk
							{
								*(rec_chunks[j].file_offset) = file_info.lDiskOffset;
							} 
						}

						if( rec_chunks[j].addr )	// caller wants chunk read into memory
						{
							assert( *(rec_chunks[j].addr) == 0 ); // overwriting existing pointer!!
							*(rec_chunks[j].addr) = new char[chnk.cksize];
							res = mmioRead( file_handle, (HPSTR) *(rec_chunks[j].addr), chnk.cksize );
							assert( res == chnk.cksize );
/*
							// debug output of result
							int print_words = chnk.cksize / 4;
							print_words = min(print_words,20);
							DWORD *ptr = (DWORD*) *(rec_chunks[j].addr);
							for(int i = 0 ; i < print_words ; i++ )
							{
								_RPT1(_CRT_WARN, "%08x ", *ptr++ );
							}
							_RPT0(_CRT_WARN, "\n" );
*/
						}
						else
						{
/* no coledit files have loopinfo last
							// avoid thrashing disk if loading large wavefile
							// assumes DATA chunk is last chunk in file
							if( cnk[level].fccType == TP_WAVE && chnk.ckid == TP_WAVEDATA )
							{
								mmio Close(file_handle, 0);
								return 0;
							}
*/
							res = mmioSeek( file_handle, chnk.cksize, SEEK_CUR );
						}

						break;
					}
				}
				res = mmioAscend(file_handle, &chnk, 0);
			}
		}
	}

	// MMIO_FHOPEN leaves CFile object open
	mmioClose(file_handle, MMIO_FHOPEN ); 
	
	return 0;
}

CRiffFile::CRiffFile() :
file_handle(NULL)
,level(0)
,rec_chunks()
,buffer(0)
{
}

CRiffFile::~CRiffFile()
{
	delete [] buffer;
}
