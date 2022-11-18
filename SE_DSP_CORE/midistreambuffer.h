#pragma once

#include <mmsystem.h>
#include "se_types.h"

struct MidiEvent;

class MidiStreamBuffer
{
public:
	MIDIHDR MidiHeader;
	int last_event_time; // ms

	MidiStreamBuffer(int nothing = 0);
	// copy constructor. supports use in standard containers.
	MidiStreamBuffer( const MidiStreamBuffer& other );
	~MidiStreamBuffer();
	void SetInput()
	{
		is_output = false;
	}
	bool IsEmpty()
	{
		return MidiHeader.dwBytesRecorded == 0;
	}
#if defined( _DEBUG )
	void debug();
#endif
	//	void Reset(HMIDISTRM MidiOutStream);
	int SendToInputDriver(HMIDIIN Stream);
	int OutStream(HMIDISTRM Stream);
	int PrepareHeader(HMIDISTRM MidiOutStream );
	int UnPrepareHeader(HMIDISTRM MidiOutStream );
	void Add(unsigned int msg, int time); // time must be int to allow conversion to relative (signed)
	void Add(int time, const unsigned char* midi_bytes, int size);
	void AddNull(int time);
	bool inQue()
	{
		return (MidiHeader.dwFlags & MHDR_INQUEUE) != 0;
	};
private:
	char* events;
	bool is_output; // false = input buffer
};

