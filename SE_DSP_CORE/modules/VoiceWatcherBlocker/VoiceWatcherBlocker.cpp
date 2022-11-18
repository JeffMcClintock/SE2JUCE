#include "./VoiceWatcherBlocker.h"

REGISTER_PLUGIN( VoiceWatcherBlocker, L"SynthEdit VoiceWatcherBlocker" );

VoiceWatcherBlocker::VoiceWatcherBlocker( IMpUnknown* host ) : MpBase( host )
{
	// Associate each pin object with it's ID in the XML file.
	initializePin( 0, pinSignalIn );
	initializePin( 1, pinSignalOut );
}

void VoiceWatcherBlocker::subProcess( int bufferOffset, int sampleFrames )
{
	// get pointers to in/output buffers.
	float* signalIn	= bufferOffset + pinSignalIn.getBuffer();
	float* signalOut= bufferOffset + pinSignalOut.getBuffer();

	if( !pinSignalIn.isStreaming() )
	{
		assert( signalIn[0] == signalIn[sampleFrames-1] );
	}

	for( int s = sampleFrames; s > 0; --s )
	{
		*signalOut++ = *signalIn++;
	}

}

void VoiceWatcherBlocker::onSetPins()
{
	// Set state of output audio pins.
	pinSignalOut.setStreaming( pinSignalIn.isStreaming() );

	SET_PROCESS( &VoiceWatcherBlocker::subProcess );
}

