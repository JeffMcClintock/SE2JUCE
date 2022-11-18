#ifndef MIDIPLAYER2_H_INCLUDED
#define MIDIPLAYER2_H_INCLUDED

#include "mp_sdk_audio.h"

struct track_info
{
	unsigned char* pointer;
	unsigned char	status; /* running status */
	unsigned char	chan; /* running chan ?? combine?? */
	unsigned char*	start;
	unsigned char*	end;
	int next_event_at;
};

class MidiPlayer2 : public MpBase
{
public:
	MidiPlayer2( IMpUnknown* host, bool initializePins = true );
	~MidiPlayer2();

	void processWithGate( int start_pos, int sampleframes );
	void subProcess( int bufferOffset, int sampleFrames );
	virtual void onSetPins();

	void NextMidiEvent(int bufferOffset);
	int loadMidiFile();
	void ResetTracks();
	void Done();

	void Setbpm(double newbpm);

	int ReadChunkSize(unsigned char* RCS_pointer);
	unsigned int ReadChunkID(unsigned char* RCID_pointer);
	int ReadVarLen(unsigned char *&g_local_pointer);

	void MidiClockStart( int bufferOffset );
	void MidiClockStop( int bufferOffset );
	void SysexEvent( unsigned char *&g_local_pointer, int bufferOffset );
	void SendMidiEvent( unsigned char *&g_local_pointer, int bufferOffset );
	void MetaEvent( unsigned char *&g_local_pointer, int bufferOffset );
	void message( const wchar_t* txt );
	virtual bool loopEntireFile()
	{
		return pinLoopMode;
	};
	void OnGateChange( int blockPosition = -1 );
	void ChooseSubProcess();
	void NotesOff(int blockPosition);

protected:
	// Pins.
	StringInPin pinFileName;
	AudioInPin pinGate;
	AudioInPin pinTrigger;
	MidiOutPin pinMIDIOut;
	FloatInPin pinHostBpm;
	IntInPin pinTempoFrom;
	BoolInPin pinLoopMode;

	float file_bpm; /* beats per minute */
	bool gate_state;
	bool trigger_state;

private:
	track_info trackInfo_[50];
	unsigned char*   buffer, g_current_status,g_current_chan;

	int ppqn; /* pulses per quater note */
	double samp_per_pulse;
	double pulse_frac; // fractional part of pulse calc
	int next_event;
	unsigned short tracks;
	int next_midi_clock;
	int pulses_per_clock;
	int pulse_count;
	int counter;
	bool noteMemory[16][128];
};

#endif

