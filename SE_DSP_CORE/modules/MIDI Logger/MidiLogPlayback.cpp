// Disable MSCPP "Consider using _wfopen_s instead. " warning.
#define _CRT_SECURE_NO_WARNINGS

#include "./MidiLogPlayback.h"
#include "../shared/unicode_conversion.h"

REGISTER_PLUGIN ( MidiLogPlayback, L"SE MIDI Log Playback" );

MidiLogPlayback::MidiLogPlayback( IMpUnknown* host ) : MpBase( host )
,inputStream(0)
,sampleClock(0)
, timestamp(INT_MAX)
{
	// Register pins.
	initializePin( 0, pinFileName );
	initializePin( 1, pinMidi );

	SET_PROCESS( &MidiLogPlayback::subProcess );
	setSleep( false ); // disable automatic sleeping.
}

MidiLogPlayback::~MidiLogPlayback()
{
	if (inputStream)
		fclose( inputStream );
}

void MidiLogPlayback::subProcess( int bufferOffset, int sampleFrames )
{
	while( timestamp < sampleClock + sampleFrames )
	{
//		_RPT2(_CRT_WARN, "%5d %5d\n", timestamp, timestamp - sampleClock );
		pinMidi.send( midiMessage, byteCount, timestamp - sampleClock );
		readMessage();
	}

	sampleClock += sampleFrames;
}

void MidiLogPlayback::readMessage()
{
	int r;
	char byteString[100];
	r = fscanf( inputStream, "%ld", &timestamp );

	if( r < 0 )
	{
		timestamp = INT_MAX;
		SET_PROCESS( &MidiLogPlayback::subProcessNothing );
		return;
	}

	// get string like - ", FF F7 07 ..."
	fgets( byteString, sizeof(byteString), inputStream );

	// two extra chars at front, one extra space at end.
	byteCount = ( (int) strlen( byteString ) - 3 ) / 3;
	for( int i = 0 ; i < byteCount ; ++i )
	{
		unsigned int temp;
		sscanf( &(byteString[2 + i * 3]), "%x", &temp);
		midiMessage[i] = temp;
	}
}

void MidiLogPlayback::onSetPins()
{
	// Check which pins are updated.
	if( pinFileName.isUpdated() )
	{
		wchar_t fullFilename[500];
		getHost()->resolveFilename( pinFileName.getValue().c_str(), sizeof(fullFilename)/sizeof(fullFilename[0]), fullFilename );

		if( inputStream != 0 )
		{
			fclose( inputStream );
		}
		//inputStream = _wfopen( fullFilename, L"rt");
#ifdef _WIN32
		inputStream = _wfopen( fullFilename, L"rt");
#else
        inputStream = fopen( JmUnicodeConversions::WStringToUtf8(fullFilename).c_str(), "rt");
#endif
		if( inputStream == 0 )
		{
#ifdef _WIN32
			MessageBoxA(0,"MidiLogPlayback: Failed to open input file.", "debug msg", MB_OK );
#endif
			SET_PROCESS(&MidiLogPlayback::subProcessNothing);
			return;
		}

		char byteString[500];
		fgets( byteString, sizeof(byteString), inputStream );
		fgets( byteString, sizeof(byteString), inputStream );
		fgets( byteString, sizeof(byteString), inputStream );

		readMessage();
	}
}

void MidiLogPlayback::onMidiMessage( int pin, unsigned char* midiMessage, int size )
{
	if( inputStream == 0 )
	{
		return;
	}

	fwprintf( inputStream, L"%5d, ", sampleClock + blockPosition() );

	for( int i = 0 ; i < size ; ++i )
	{
		fwprintf( inputStream, L"%02x ", (int) midiMessage[i] );
	}

	fwprintf( inputStream, L"\n" );
}
