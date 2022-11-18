#include "VoltsToMidiCc.h"
#include <algorithm>
#include "mp_midi.h"

REGISTER_PLUGIN ( VoltsToMidiCc, L"SE Volts to MIDI CC" );
SE_DECLARE_INIT_STATIC_FILE(VoltsToMIDICC);

VoltsToMidiCc::VoltsToMidiCc( IMpUnknown* host ) : MpBase( host )
{
	// Register pins.
	initializePin( 0, pinValue );
	initializePin( 1, pinChannel );
	initializePin( 2, pinMidiCc );
	initializePin( 3, pinMIDIOut );
}

void VoltsToMidiCc::subProcess( int bufferOffset, int sampleFrames )
{
	// get pointers to in/output buffers.
	float* val	= bufferOffset + pinValue.getBuffer();

	int bufferPos = bufferOffset + sampleFrames - 1;
	SendValue( bufferPos );
}

void VoltsToMidiCc::onSetPins()
{
	if( pinChannel.isUpdated() || pinMidiCc.isUpdated() )
	{
		currentCcValue = 8;
		SendValue();
	}

	// Check which pins are updated.
	if( pinValue.isStreaming() )
	{
		// Set processing method.
		SET_PROCESS(&VoltsToMidiCc::subProcess);
	}
	else
	{
		SET_PROCESS(&VoltsToMidiCc::subProcessNothing);
		SendValue();
	}

}

void VoltsToMidiCc::SendValue( int bufferPosition )
{
	const float v = pinValue.getValue( bufferPosition );
#if 0
	int newMidiValue = (int) (v * 127.0f);
	newMidiValue = (std::min)(newMidiValue, 127);
	newMidiValue = (std::max)(newMidiValue, 0);

	if( newMidiValue == currentCcValue )
	{
		return;
	}
	
	currentCcValue = newMidiValue;

	const int size = 3;
	unsigned char data[3];

	data[0] = 0xB0; // Control Change.
	data[1] = pinMidiCc;
	data[2] = currentCcValue;

	pinMIDIOut.send( data, size, bufferPosition );
#else

	const uint32_t newMidiValue = gmpi::midi::utils::floatToU32(v);

	if( newMidiValue == currentCcValue)
	{
		return;
	}

	currentCcValue = newMidiValue;

	// MIDI 2.0
	const auto msgout = gmpi::midi_2_0::makeController(
		pinMidiCc.getValue(),
		v,
		pinChannel.getValue()
	);

	pinMIDIOut.send(
		msgout.m,
		static_cast<int>(std::size(msgout.m)),
		bufferPosition
	);
#endif
}
