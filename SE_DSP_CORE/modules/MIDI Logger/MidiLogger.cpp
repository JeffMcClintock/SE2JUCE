// Disable MSCPP "Consider using _wfopen_s instead. " warning.
#define _CRT_SECURE_NO_WARNINGS

#include "./MidiLogger.h"
#include "../shared/xplatform.h"

REGISTER_PLUGIN ( MidiLogger, L"SE MIDI Logger" );

MidiLogger::MidiLogger( IMpUnknown* host ) : MpBase( host )
,outputStream(0)
,sampleClock(0)
{
	// Register pins.
	initializePin( 0, pinFileName );
	initializePin( 1, pinMidi );
}

MidiLogger::~MidiLogger()
{
	if (outputStream)
	{
		fflush(outputStream);
		fclose(outputStream);
	}
}

void MidiLogger::subProcess( int bufferOffset, int sampleFrames )
{
	sampleClock += sampleFrames;
}

void MidiLogger::onSetPins()
{
	// Check which pins are updated.
	if( pinFileName.isUpdated() )
	{
		const auto fullFilename = host.resolveFilename_old(pinFileName);

		if( outputStream != 0 )
		{
			fclose( outputStream );
		}
#ifdef _WIN32
		outputStream = _wfopen( fullFilename.c_str(), L"wt");
#else
        outputStream = fopen( JmUnicodeConversions::WStringToUtf8(fullFilename).c_str(), "wt");
#endif
		if( outputStream == 0 )
		{
#ifdef _WIN32
			MessageBoxA(0,"MidiLogger: Failed to open output file.", "debug msg", MB_OK );
#endif
			return;
		}

		fwprintf( outputStream, L"SynthEdit MIDI Event log\n" );
		fwprintf( outputStream, L"Samplerate:%f\n", getSampleRate() );
		fwprintf( outputStream, L"Block Size:%d\n", getBlockSize() );
	}

	SET_PROCESS(&MidiLogger::subProcess);
	setSleep(false); // disable automatic sleeping.
}

void MidiLogger::onMidiMessage( int pin, unsigned char* midiMessage, int size )
{
	if( outputStream == 0 )
	{
		return;
	}

	fwprintf( outputStream, L"%8d, ", (int) sampleClock /* + blockPosition()*/ );

	for( int i = 0 ; i < size ; ++i )
	{
		fwprintf( outputStream, L"%02x ", (int) midiMessage[i] );
	}

	fwprintf( outputStream, L"\n" );
}
