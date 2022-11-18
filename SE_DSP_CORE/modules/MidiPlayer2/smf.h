#pragma once
/* ----------------------------------------------------------------------- */
/* Title:        Standard MIDI File General Header                         */
/* Disk Ref:     smf.h                                                     */
/* ----------------------------------------------------------------------- */

/* Four character IDentifier builder */
#define MakeID(a,b,c,d)  ( (unsigned int) (a)<<24L | (unsigned int) (b)<<16L | (c)<<8 | (d) )
// store in reverse order for saving

/* Standard MIDI File IDs. SMF files ALWAYS start with an ID_HEADER chunk
which are then followed by one or more ID_TRACK chunks. New chunk types may
be added... so chunk readers must be prepared to skip over chunks any which
they do not understand or need */

#define  ID_HEADER MakeID('M','T','h','d')
#define  ID_TRACK MakeID('M','T','r','k')

/* ----------------------------------------------------------------------- */
/* Chunks start with a type ID and a count of the data bytes that follow */

struct MIDIChunkInfo
{
	int  ckID;
	int  ckSize;
	void reverse_bytes(); // for storage to disc
};

struct MIDIChunk
{
	int  ckID;
	int  ckSize;
	unsigned char ckData[ 1 /* don't forget that this is really ckSize */ ];
};//Chunk;

struct MIDIHeaderChunk
{
	int  ckID;
	int  ckSize;
	unsigned char Format_hi;
	unsigned char Format_lo;
	unsigned char Tracks_hi;
	unsigned char Tracks_lo;
	unsigned char Division_hi;
	unsigned char Division_lo;
	unsigned short GetFormat()
	{
		return ((unsigned short)Format_hi << 8) + Format_lo;
	};
	unsigned short GetTracks()
	{
		return ((unsigned short)Tracks_hi << 8) + Tracks_lo;
	};
	unsigned short GetDivision()
	{
		return ((unsigned short)Division_hi << 8) + Division_lo;
	};
	void reverse_bytes(); // for storage to disc
};//HeaderChunk;

/* ChunkPSize computes total physical size of a chunk from it's data size */

#define ChunkPSize(dataSize)  (dataSize + sizeof(ChunkInfo))
