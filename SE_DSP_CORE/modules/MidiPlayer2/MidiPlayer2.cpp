#include <math.h>
#include <algorithm>
#include <climits> // INT_MAX
#include "./MidiPlayer2.h"
#include "smf.h"
#include "../shared/unicode_conversion.h"
#include "../se_sdk3/MpString.h"

#define NOTE_OFF				0x80
#define NOTE_OFF_SIZE			2
#define NOTE_ON					0x90
#define NOTE_ON_SIZE			2
#define POLY_AFTERTOUCH			0xA0
#define POLY_AFTERTOUCH_SIZE	2
#define CONTROL_CHANGE			0xB0
#define CONTROL_CHANGE_SIZE		2
#define PROGRAM_CHANGE			0xC0
#define PROGRAM_CHANGE_SIZE		1
#define CHANNEL_PRESSURE		0xD0
#define CHANNEL_PRESSURE_SIZE	1
#define PITCHBEND				0xE0
#define PITCHBEND_SIZE			2
#define SYSTEM_MSG				0xF0	/* F0 -> FF */
#define SYSTEM_EXCLUSIVE		0xF0
#define MIDI_CLOCK				0xF8
#define MIDI_CLOCK_START		0xFA
#define MIDI_CLOCK_CONT			0xFB
#define MIDI_CLOCK_STOP			0xFC
#define ACTIVE_SENSING			0xFE
#define MIDI_CC_ALL_SOUND_OFF	120
#define MIDI_CC_ALL_NOTES_OFF	123

/* some messages */
#define ERROR_MESSAGE_1  L"Cannot Find/Load this file.  Check your Audio Preferences MIDI path setting: "
#define ERROR_MESSAGE_2  L"This is NOT a Standard MIDI File"
#define ERROR_MESSAGE_3  L"file but it doesn't look right\n"
#define ERROR_MESSAGE_4  L"We've hit (and skipped over) an UNKNOWN CHUNK\n"
/////////////////////////////////////////////////////////////////////////////////////////////////

REGISTER_PLUGIN ( MidiPlayer2, L"SE MIDI Player2" );

MidiPlayer2::MidiPlayer2( IMpUnknown* host, bool initializePins ) : MpBase( host )
	,buffer(0)
	,next_event(0)
	,pulse_count(0)
	,pulse_frac(0)
	,next_midi_clock(0)
	,tracks(0)
	,file_bpm (120) /* beats per minute */
	,pulses_per_clock(4)
	,counter(0)
	,gate_state(false)
	,trigger_state(false)
{
	if( initializePins )
	{
		initializePin( 0, pinFileName );
		initializePin( 1, pinTrigger );
		initializePin( 2, pinGate );
		initializePin( 3, pinMIDIOut );
		initializePin( 4, pinHostBpm );
		initializePin( 5, pinTempoFrom );
		initializePin(6, pinLoopMode);
		initializePin(pinSendClocks);
	}
	memset( noteMemory, 0, sizeof(noteMemory) );
}

// gate or trigger pin streaming...
void MidiPlayer2::processWithGate(int start_pos, int sampleframes)
{
	float *gate	= pinGate.getBuffer() + start_pos;
	float *trigger	= pinTrigger.getBuffer() + start_pos;

	int last = start_pos + sampleframes;
	int to_pos = start_pos;
	int cur_pos = start_pos;

	while( cur_pos < last )
	{
		// how long till next gate/trigger change?
		while( (*gate > 0.f ) == gate_state && (*trigger > 0.f ) == trigger_state && to_pos < last )
		{
			to_pos++;
			gate++;
			trigger++;
		}

		if( to_pos > cur_pos && gate_state )
		{
			subProcess( cur_pos, to_pos - cur_pos );
		}

		if( to_pos == last )
		{
			return;
		}

		cur_pos = to_pos;

		OnGateChange( cur_pos );
	}
}

void MidiPlayer2::subProcess( int bufferOffset, int sampleFrames )
{
	assert( counter >= 0 );

	int s = sampleFrames;
	while( s > 0 )
	{
		if( counter >= s )
		{
			counter -= s;
			return;
		}
		else
		{
			s -= counter;
			counter = 0;
			NextMidiEvent( bufferOffset + sampleFrames - s );
		}
	}
}

void MidiPlayer2::onSetPins()
{
	if( pinFileName.isUpdated() )
	{
		if( loadMidiFile() == 0 )
		{
			MidiClockStart(-1);
		}
	}

	// Setting tempo depends on PPQN value from MIDI file, so this MUST be done after pinFileName.
	if( pinTempoFrom.isUpdated() || pinHostBpm.isUpdated() || pinFileName.isUpdated() )
	{
		// NOTE: If MIDI Player is upstream of MIDI-CV, it will be tagged as 'one block latency'.
		// This can result in it getting a BPM of 0.0 at startup, followed by actual BPM one block later.
		// This happens only on first run, on subseqent runs, the parameter will tend to have the correct BPM 'left over' from the previous run.

//		_RPTN(0, "MidiPlayer2 pinHostBpm=%f\n", pinHostBpm.getValue());
		if( pinTempoFrom == 1 ) // Tempo from Host.
		{
			Setbpm( pinHostBpm );
		}
		else
		{
			Setbpm( file_bpm );
		}
	}

	if( pinGate.isUpdated() || pinTrigger.isUpdated() )
	{
		OnGateChange();
	}

	ChooseSubProcess();

	// Set sleep mode off.
	setSleep(false);
}

void MidiPlayer2::OnGateChange( int blockPosition )
{
	bool new_trigger_state = ( pinTrigger.getValue(blockPosition) > 0.f );
	bool new_gate_state = ( pinGate.getValue(blockPosition) > 0.f );

	if( new_trigger_state != trigger_state ) // ignore spurious changes
	{
		trigger_state = new_trigger_state;

		if( trigger_state == true ) // && new_gate_state == true )
		{
			NotesOff(blockPosition);
			// Re-start playback.
			ResetTracks();
		}
	}
	if( new_gate_state != gate_state ) // ignore spurious changes
	{
		gate_state = new_gate_state;

		if( gate_state == false )
		{
			NotesOff(blockPosition);
		}
	}
	
	ChooseSubProcess();
}

void MidiPlayer2::NotesOff(int blockPosition)
{
	// send note-off for any playing note.
	unsigned char midiMessage[3] = {0,0,0};
	for( int chan = 0 ; chan < 16 ; ++chan )
	{
		for( int note = 0 ; note < 128 ; ++note )
		{
			if( noteMemory[chan][note] )
			{
				midiMessage[0] = NOTE_OFF | chan;
				midiMessage[1] = note;
				pinMIDIOut.send( midiMessage, sizeof(midiMessage) );
			}
		}
	}
	memset( noteMemory, 0, sizeof(noteMemory) );
}

void MidiPlayer2::ChooseSubProcess()
{
	if( buffer == 0 )
	{
		SET_PROCESS(&MidiPlayer2::subProcessNothing);
		return;
	}

	if( pinGate.isStreaming() || pinTrigger.isStreaming() )
	{
		SET_PROCESS(&MidiPlayer2::processWithGate);
	}
	else
	{
		if( gate_state )
		{
			SET_PROCESS(&MidiPlayer2::subProcess);
		}
		else
		{
			SET_PROCESS(&MidiPlayer2::subProcessNothing);
		}
	}
}

int MidiPlayer2::loadMidiFile()
{
	memset( trackInfo_, 0, sizeof(trackInfo_) );

	unsigned short format;
	format = 0;
	tracks = 0;
	ppqn = 0;
	counter = 0;
	delete[] buffer;
	buffer = {};

	NotesOff( blockPosition() );

	if( pinFileName.getValue().empty() )
	{
		return 1;
	}

	std::wstring fname = pinFileName;
	if( fname.find( L".mid" ) == std::string::npos && fname.find( L".MID" ) == std::string::npos )
	{
		fname += L".mid";
	}

	const auto fullFilename = host.resolveFilename(fname);
	auto file = host.openUri(fullFilename);
	const int64_t file_size = file.size();
	
	buffer = new unsigned char[file_size + 2];
	file.read( (char*) buffer, file_size );

	// first check that the file loaded is actually a MIDI File
	unsigned char* g_pointer = buffer;

	if( ReadChunkID(g_pointer) != ID_HEADER )
	{
		delete [] buffer;
		buffer = 0;
		message( ERROR_MESSAGE_2);
		return 1;
	}
	
	// This is a Standard MIDI File, so get header details
	MIDIHeaderChunk* head = (MIDIHeaderChunk*)g_pointer;
	format = head->GetFormat();
	tracks = head->GetTracks();
	ppqn = head->GetDivision();
	pulses_per_clock = ppqn / 24; // midi clocks per quarter note
	assert (tracks < 50);  // !!! use dynamic array
	ResetTracks();

	ChooseSubProcess();
	return 0;
}

void MidiPlayer2::ResetTracks()
{
	file_bpm = 120; // Default MIDI BPM.

	/* now deal with each new track chunk in turn
	- at the moment the g_pointer is still at the
	start of the buffer, ie at the start of the
	HEADER chunk.... */
	unsigned char*  g_pointer = buffer;

	for ( int i = 0 ; i < tracks ; i++ )
	{
		g_pointer += ReadChunkSize(g_pointer) + sizeof(MIDIChunkInfo); /* skip to new chunk */

		// Must check that we've got a track chunk
		if( ReadChunkID( g_pointer ) == ID_TRACK )
		{
			int current_datasize=ReadChunkSize(g_pointer);
			trackInfo_[i].start = g_pointer + sizeof(MIDIChunkInfo); /* remember start of track */
			trackInfo_[i].end = trackInfo_[i].start + current_datasize;
		}
		else
		{
			message(ERROR_MESSAGE_4);
			i--; // adjust track counter or we will lose a real track
		}
	}

	for(int i=0; i<tracks; i++)
	{
		trackInfo_[i].status = NULL;
		unsigned char* local_pointer = trackInfo_[i].start;
		int time = ReadVarLen(local_pointer);
		trackInfo_[i].next_event_at = time;
		trackInfo_[i].pointer = local_pointer;
		/*	_RPT0(_CRT_WARN,"track %d first event at clock %d\n",i,trackInfo_[i].next_event_at);
		*/
	}

	next_event = 0;
	pulse_count = 0;
	pulse_frac = 0;
	next_midi_clock = 0;
	g_current_status = g_current_chan = 0;
	counter = 0;
}

/* ----------------------------------------------------------------------- */

void MidiPlayer2::NextMidiEvent(int bufferOffset)
{
	int i,time;
	unsigned char eventType;
	unsigned char midiMessage[3];
	unsigned char* g_local_pointer;

	if( next_event == INT_MAX )
	{
		counter = INT_MAX;
		Done();
		return;
	}

	assert( next_event == pulse_count || next_midi_clock == pulse_count );

	if( next_midi_clock == pulse_count )
	{
		if (pinSendClocks)
		{
			midiMessage[0] = MIDI_CLOCK;
			pinMIDIOut.send(midiMessage, 1, bufferOffset);
		}
		next_midi_clock += pulses_per_clock;
	}

	if( next_event == pulse_count)
	{
		next_event = INT_MAX;

		for(i=0; i<tracks; i++)
		{
			assert( trackInfo_[i].start <= trackInfo_[i].pointer );

			if( trackInfo_[i].end <= trackInfo_[i].pointer )
				continue; // end of track

			while(trackInfo_[i].next_event_at <= pulse_count )
			{
				/* less than or equal in case its missed */
				g_local_pointer = trackInfo_[i].pointer;
				g_current_status= trackInfo_[i].status;
				g_current_chan	= trackInfo_[i].chan;

				/* send event */
				eventType = *g_local_pointer;

				if(eventType < 0x80)	// Data byte
				{
					SendMidiEvent(g_local_pointer, bufferOffset);
				}
				else
				{
					// Status byte
					g_local_pointer++; /* skip identifers for these events */

					switch(eventType)
					{
					case 0xF0:
						SysexEvent(g_local_pointer, bufferOffset);
						g_current_status=NULL;
						break;

					case 0xF7:
						SysexEvent(g_local_pointer, bufferOffset);
						g_current_status=NULL;
						break;

					case 0xFF:
						MetaEvent(g_local_pointer, bufferOffset);
						g_current_status=NULL;
						break;

					default:
						g_current_status=eventType & 0xF0;
						g_current_chan=eventType & 0x0F;
						SendMidiEvent(g_local_pointer, bufferOffset);
						//							sent_midi = true;
					}
				}

				if( trackInfo_[i].end <= g_local_pointer )
				{
					trackInfo_[i].pointer = g_local_pointer;
					trackInfo_[i].next_event_at = INT_MAX;
				}
				else
				{
					/* get next event time offset */
					time = ReadVarLen(g_local_pointer);
					eventType = *g_local_pointer;
					trackInfo_[i].next_event_at += time;
					trackInfo_[i].pointer = g_local_pointer;
					trackInfo_[i].status = g_current_status;
					trackInfo_[i].chan = g_current_chan;
					/*
						_RPT0(_CRT_WARN,"track %d next event at clock %d\n",i,trackInfo_[i].next_event_at);
					*/
				}
			}

			if(trackInfo_[i].next_event_at < next_event)
				next_event = trackInfo_[i].next_event_at;
		}

		if( next_event == INT_MAX )
		{
			if( loopEntireFile() && buffer )
			{
				ResetTracks();
			}
			else
			{
				MidiClockStop(bufferOffset);
				// Stop synth after 0.5 seconds
				counter = ((int) getSampleRate()) >> 1;
				return; // prevent further clock events being sent.
			}
		}
	}

	// calculate delay to next midi event (convert from pulses to samples)
	next_event = (std::min)( next_midi_clock, next_event );
	double samples_to_next_event = ( next_event - pulse_count ) * samp_per_pulse + pulse_frac;
	pulse_frac = samples_to_next_event - floor(samples_to_next_event);
	// RUN_AT( SampleClock() + samples_to_next_event, &MidiPlayer2::NextMidiEvent );
	counter = (int) samples_to_next_event;
	pulse_count = next_event;
}

void MidiPlayer2::MetaEvent( unsigned char *&g_local_pointer, int bufferOffset )
{
	int event_datasize, event_identifier;
	event_identifier = *g_local_pointer;

	switch( event_identifier )
	{
	case 0: // Sequence num 2 byte
	case 01: //	text
	case 02: // copyright
	case 03: // seq/track name
	case 04: // Instrument name
	case 05: // lyric
	case 06: // Marker name
	case 07: // Cue point
	case 0x20: // sysex midi chan
	case 0x21: // midi port
	case 0x2f: // end of track
		break;

	case 0x51: // TEMPO
		if( pinTempoFrom == 0 )
		{
			int us_per_qn = g_local_pointer[2];	// microsec per quarternote
			us_per_qn = (us_per_qn << 8) + g_local_pointer[3];
			us_per_qn = (us_per_qn << 8) + g_local_pointer[4];
			file_bpm = (float) 60000000/us_per_qn;
			Setbpm(file_bpm);
		}
		break;
	};

	g_local_pointer++;

	event_datasize=ReadVarLen(g_local_pointer);

	g_local_pointer+=event_datasize;
}

void MidiPlayer2::SendMidiEvent( unsigned char *&g_local_pointer, int bufferOffset )
{
	short msg_size;

	if( g_current_status == PROGRAM_CHANGE || g_current_status == CHANNEL_PRESSURE )
		msg_size = 1;
	else
		msg_size = 2;

	int output = *g_local_pointer;
	int byte1 = g_local_pointer[0];
	int byte2 = g_local_pointer[1];
	output += byte2 << 8;
	output = (output << 8) + g_current_status + g_current_chan;
	assert(g_current_status != NOTE_ON || ((output >> 8)& 0xff) < 128 ); // illegal note value.
	pinMIDIOut.send( (unsigned char *) &output, msg_size + 1, bufferOffset );
	g_local_pointer += msg_size;

	if( g_current_status == NOTE_ON )
	{
		noteMemory[g_current_chan][byte1] = true;
	}
	if( g_current_status == NOTE_OFF )
	{
		noteMemory[g_current_chan][byte1] = false;
	}
}

void MidiPlayer2::SysexEvent( unsigned char *&g_local_pointer, int bufferOffset )
{
	int event_datasize;
	event_datasize = ReadVarLen(g_local_pointer);
	// format in mem :Sysex-start, Length of sysex, remaining sysex bytes
	unsigned char* midi_bytes = g_local_pointer - 1;
	// overwrite first byte with SYSEX (0xF)
	unsigned char saved_byte = *midi_bytes;
	*midi_bytes = SYSTEM_EXCLUSIVE;
	pinMIDIOut.send( midi_bytes, event_datasize + 1, bufferOffset );
	// restore overwritten val
	*midi_bytes = saved_byte;
	g_local_pointer+=event_datasize;
}

int MidiPlayer2::ReadVarLen( unsigned char *&g_local_pointer )
{
	int value;
	unsigned char c;
	value=(int)*g_local_pointer++;

	if(value&0x80)
	{
		value&=0x7F;

		do
		{
			value=(value<<7)+((c=*g_local_pointer++)&0x7F);
		}
		while(c&0x80);
	}

	return value;
}

int MidiPlayer2::ReadChunkSize(unsigned char* RCS_pointer)
{
	int value;
	unsigned char i;
	RCS_pointer+=4;
	value=*RCS_pointer++;

	for (i=0; i<3; i++)
		value=(value<<8)+(*RCS_pointer++);

	return value;
}

unsigned int MidiPlayer2::ReadChunkID(unsigned char* RCID_pointer)
{
	unsigned int value;
	unsigned char i;
	value=*RCID_pointer++;

	for (i=0; i<3; i++)
		value=(value<<8)+(*RCID_pointer++);

	return value;
}

MidiPlayer2::~MidiPlayer2()
{
	delete [] buffer;
}

void MidiPlayer2::Setbpm(double bpm)
{
	const auto prev_samp_per_pulse = samp_per_pulse;

	bpm = (std::max)(bpm,10.);
	bpm = (std::min)(bpm, 1000.);
	double pp_sec = bpm / 60.0 * ppqn;
	samp_per_pulse = getSampleRate() / pp_sec;
//	_RPT1(_CRT_WARN, "samp_per_pulse %f.\n", samp_per_pulse );

	// adjust time left on next event.
	if (counter > 0)
	{
		counter = static_cast<int>( counter * samp_per_pulse / prev_samp_per_pulse);
	}
}

void MidiPlayer2::MidiClockStart( int bufferOffset )
{
	if (pinSendClocks)
	{
		unsigned char msg = MIDI_CLOCK_START;
		pinMIDIOut.send(&msg, 1, bufferOffset);
	}
}

void MidiPlayer2::MidiClockStop( int bufferOffset )
{
	if (pinSendClocks)
	{
		unsigned char msg = MIDI_CLOCK_STOP;
		pinMIDIOut.send(&msg, 1, bufferOffset);
	}
}

void MidiPlayer2::Done()
{
#if defined( SE_EDIT_SUPPORT )
	// when rendering to disk, stop rendering once MIDI file ends.
	if( !AudioMaster2()->IoManager()->AudioDriver()->RunsRealTime() )
	{
		dynamic_cast<SeAudioMaster*>( AudioMaster() )->end_run();
	}
#endif

	SET_PROCESS(&MidiPlayer2::subProcessNothing);
}

void MidiPlayer2::message( const wchar_t* txt )
{
#ifdef _WIN32
	::MessageBox(0, txt, L"MidiPlayer2", MB_OK );
#else
    assert(false); // TODO.
#endif
}

