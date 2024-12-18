#pragma once

#include <vector>
#include "mp_sdk_common.h"

#ifndef MAKEFOURCC

#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
(((uint32_t)(uint8_t)(ch0))        | (((uint32_t)(uint8_t)(ch1)) << 8) |   \
(((uint32_t)(uint8_t)(ch2)) << 16) | (((uint32_t)(uint8_t)(ch3)) << 24 ))

#endif // !MAKEFOURCC

//CString name, char *type, char *chunk_id, char **chunk_address, int *chunk_count, int chunk_size_check);
#define REG_CHUNK( AA,BB,CC,DD,EE,FF ) AddChunk(RiffFile2::riff_match(AA, BB, CC,((char **) DD), EE, sizeof(FF) ))
// This version provides the file offset of the chunk when passed a pointer
#define REG_CHUNK2( AA,BB,CC,DD,EE,FF,GG ) AddChunk(RiffFile2::riff_match(AA, BB, CC,((char **) DD), EE, sizeof(FF), GG ))

class RiffFile2
{
public:
	/* RIFF chunk information data structure */
	struct MMCKINFO_SE
	{
		uint32_t   ckid;           /* chunk ID */
		uint32_t   cksize;         /* chunk size */
		uint32_t   fccType;        /* form type or list type */
	};

	// Provides a handy way to list where you want a certain chunk stored
	// just tell it the chunk sect and name, and where to put it, size is
	// there as a check that the the chunk is an exact multiple of 'size' big
	struct riff_match
	{
		riff_match() {}
		riff_match(	std::string n, const char *fct, const char *cid, char **ad, unsigned int * ca,int size, int *file_offs = nullptr, int padding = 0) :
			name(n)
			,addr(ad)
			,count_addr(ca)
			,item_size(size)
			,padMemoryAllocationBytes(padding)
			,fccType(MAKEFOURCC(fct[0], fct[1], fct[2], fct[3]))
			,ckid(MAKEFOURCC(cid[0], cid[1], cid[2], cid[3]))
			{
			};

		std::string	name;
		uint32_t	fccType = 0;
		uint32_t	ckid = 0;
		char** addr = {};
		uint32_t* count_addr = {};	// *int in which to store item count
		int		item_size = 0;
		int padMemoryAllocationBytes = 0; // add extra blank data before and after for safety.
	};

	RiffFile2();
	bool ReadFile();
	void AddChunk( riff_match riff_desc );
	bool Open( gmpi::IProtectedFile2* file, uint32_t& riff_type );

private:
	bool Open2(uint32_t& riff_type);
	gmpi::IProtectedFile2* fileHandle;
	int32_t filePos = 0;
	std::vector<riff_match> rec_chunks;
	int level; //current indent level
	MMCKINFO_SE cnk[30] = {};
};
