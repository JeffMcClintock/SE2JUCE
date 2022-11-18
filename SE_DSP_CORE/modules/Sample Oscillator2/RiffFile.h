// CRiffFile unit generator
// 
#if !defined(_CRiffFile_h_)
#define _CRiffFile_h_

#include <windows.h>
#include <mmsystem.h>
#include <vector>
#include "mp_sdk_common.h"

//CString name, char *type, char *chunk_id, char **chunk_address, int *chunk_count, int chunk_size_check);
#define REG_CHUNK( AA,BB,CC,DD,EE,FF ) AddChunk(CRiffFile::riff_match(AA, BB, CC,((char **) DD), EE, sizeof(FF) ))
// This version provides the file offset of the chunk when passed a pointer
#define REG_CHUNK2( AA,BB,CC,DD,EE,FF,GG ) AddChunk(CRiffFile::riff_match(AA, BB, CC,((char **) DD), EE, sizeof(FF), GG ))

class CRiffFile
{
public:
	// Provides a handy way to list where you want a certain chunk stored
	// just tell it the chunk sect and name, and where to put it, size is
	// there as a check that the the chunk is an exact multiple of 'size' big
	struct riff_match
	{
		riff_match(){};
		riff_match(	std::string n, char *fct, char *cid, char **ad, unsigned int * ca,int size, int *file_offs = NULL) :
			name(n)
			,addr(ad)
			,count_addr(ca)
			,item_size(size)
			,file_offset(file_offs)
			{
				fccType = mmioFOURCC(fct[0], fct[1], fct[2], fct[3]);
				ckid	= mmioFOURCC(cid[0], cid[1], cid[2], cid[3]);
			};

		std::string	name;
		DWORD	fccType;
		DWORD	ckid;
		char **	addr;
		unsigned int		*count_addr;	// *int in which to store item count
		int		item_size;
		int	*file_offset;
	};

	CRiffFile();
	~CRiffFile();
	bool ReadFile(void);
	void AddChunk( riff_match &riff_desc );
	bool Open( const wchar_t* filename, DWORD &riff_type );
	bool Open( gmpi::IProtectedFile* file, DWORD& riff_type );

private:
	bool Open2( DWORD & riff_type);
	HMMIO file_handle;
	std::vector<riff_match> rec_chunks;
	int level; //current indent level
	MMCKINFO cnk[30];
	char* buffer;
};

#endif