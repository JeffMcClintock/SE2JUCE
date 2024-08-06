#pragma once

// Important, this is not the same as a MIDIEVENT (which includes extra dword)
struct MidiEvent
{
	MidiEvent(int delata_time = 0, int event = 0) : dwDeltaTime(delata_time),dwStreamID(0),dwEvent(event) {}
	unsigned int       dwDeltaTime;          // Ticks since last event
	unsigned int       dwStreamID;           // Reserved; must be zero
	unsigned int       dwEvent;              // Event type and parameters
};
