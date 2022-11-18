#include "smf.h"


void reverse32( int& source )
{
	int result;
	char* c = (char*) &source;
	char* c2 = (char*) &result;
	c2[0] = c[3];
	c2[1] = c[2];
	c2[2] = c[1];
	c2[3] = c[0];
	source = result;
}


void MIDIChunkInfo::reverse_bytes() // for storage to disc
{
	reverse32(ckID);
	reverse32(ckSize);
}

void MIDIHeaderChunk::reverse_bytes() // for storage to disc
{
	reverse32(ckID);
	reverse32(ckSize);
}