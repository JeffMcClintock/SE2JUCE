#include "./QueLoader.h"

REGISTER_PLUGIN( QueLoader, L"SynthEdit QueLoader" );

QueLoader::QueLoader(IMpUnknown* host) : MpBase(host)
,messageDone( false )
,count( 0 )
{
	// Associate each pin object with it's ID in the XML file
	initializePin(0, pinSamples);
	initializePin(1, pinFromGui);
	initializePin(2, pinRate);
}

int32_t QueLoader::open()
{
	MpBase::open();	// always call the base class

	SET_PROCESS(&QueLoader::subProcess);


	// Module must transmit an initial value on all output pins.
	pinSamples.sendPinUpdate();

//	sendResultToGui(23);
	setSleep( false );

	return gmpi::MP_OK;
}

void QueLoader::subProcess( int bufferOffset, int sampleFrames )
{
	count -= sampleFrames;

	if( count < 0 )
	{
		count += getBlockSize() + (int) getSampleRate() / 30; // 30 Hz

		// Send data to GUI.
		int target = (int) ( pinRate * 1000.0f );
		int sent = 0;

		while( sent < target )
		{
			// send random message.
			const int maxSize = 1000;
			unsigned char buffer[maxSize];

			int size = rand() % maxSize;

			if( size > (target - sent) )
				size = (target - sent);

			unsigned char c = size;

			for( int i = 0 ; i < size ; ++i )
			{
				buffer[i] = c;
				++c;
			}

			getHost()->sendMessageToGui( 1234, size, buffer );

			sent += size;
		}
	}
}

void QueLoader::sendResultToGui(int block_offset)
{
	getHost()->sendMessageToGui( 33, 5, "12345" );

	const int dataSize = 1000030; // 1 MB approx
	char* data = new char[dataSize]; // large dataset
	pinSamples.setValueRaw( dataSize, data);
	pinSamples.sendPinUpdate(block_offset);
	delete [] data;
}

int32_t QueLoader::receiveMessageFromGui( int32_t id, int32_t size, void* messageData )
{
	if( messageDone )
		 return gmpi::MP_OK;

	unsigned char* buffer = (unsigned char*) messageData;

	// check for corruption;
	unsigned char c = *buffer;
	if( id == 123 )
	{
		for( int i = 0 ; i < size ; ++i )
		{
			if( buffer[i] != c )
			{
				messageDone = true;
				MessageBoxA(0,"GUI->DSP Que corruption!!!", "debug msg", MB_OK );
				break;
			}
			++c;
		}
	}

	return gmpi::MP_OK;
};

void QueLoader::onSetPins(void)  // one or more pins_ updated.  Check pin update flags to determin which ones.
{
	if( pinFromGui.isUpdated() )
	{
		int test = pinFromGui.rawSize() ;
	}
}


