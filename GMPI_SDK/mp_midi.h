#pragma once
#include <stdint.h>
#include <algorithm>

/*
#include "../se_sdk3/mp_midi.h"

using namespace GmpiMidi;
using namespace GmpiMidiHdProtocol;

*/

#if defined(__GNUC__)
#pragma pack(push,1)
#else
#pragma pack(push)
#pragma pack(1)
#endif

namespace GmpiMidi
{
	enum MidiStatus //: unsigned char
	{
		MIDI_NoteOff		= 0x80,
		MIDI_NoteOn			= 0x90,
		MIDI_PolyAfterTouch	= 0xA0,
		MIDI_ControlChange	= 0xB0,
		MIDI_ProgramChange	= 0xC0,
		MIDI_ChannelPressue	= 0xD0,
		MIDI_PitchBend		= 0xE0,
		MIDI_SystemMessage	= 0xF0,
		MIDI_SystemMessageEnd = 0xF7,
	};

	enum MidiSysexTimeMessages
	{
		MIDI_Universal_Realtime = 0x7F,
		MIDI_Universal_NonRealtime = 0x7E,
		MIDI_Sub_Id_Tuning_Standard = 0x08,
	};

	enum MidiLimits
	{
		MIDI_KeyCount = 128,
		MIDI_ControllerCount = 128,
		MIDI_7bit_MinimumValue = 0,
		MIDI_7bit_MaximimumValue = 127,
	};

	enum MidiChannels
	{
		MIDI_ChannelCount = 16,
		MIDI_Channel_MinimumValue = 0,
		MIDI_Channel_MaximimumValue = 15,
	};

	enum Controller
	{
		CC_HighResolutionVelocityPrefix = 88,
		CC_AllSoundOff					= 120,
		CC_ResetAllControllers			= 121,
		CC_AllNotesOff					= 123,
	};

	// converts a 14-bit bipolar controler to a normalized value (between 0.0 and 1.0)
	inline float bipoler14bitToNormalized(uint8_t msb, uint8_t lsb)
	{
		constexpr int centerInt = 0x2000;
		constexpr float scale = 0.5f / (centerInt - 1); // -0.5 -> +0.5

		const int controllerInt = (static_cast<int>(lsb) + (static_cast<int>(msb) << 7)) - centerInt;
		// bender range is 0 -> 8192 (center) -> 16383
		// which is lopsided (8192 values negative, 8191 positive). So we scale by the smaller number, then clamp any out-of-range value.
		return (std::max)(0.0f, 0.5f + controllerInt * scale);
	}
}

namespace GmpiMidiHdProtocol
{
	/*
	enum WrappedHdMessage
	{
		size = 20, // bytes.
	};
*/
	/*
	// MIDI HD-PROTOCOL. (hd protocol)

	*The short packet note - on has fields for:
	-specifying one of 14 channel groups and one of 254 channels in the group
	- Specifying one of 256 notes
	- 12 bits of velocity
	- 20 bits of articulation, direct pitch, or timestamp. [0 - 0xFFFFF]
	A long packet has fields for articulation, direct pitch, AND timestamp

	NOTES:
	HD is exclusively based on 32 bits unsigned integer atoms.
	shortest HD message is 3 atoms long (12 bytes...). http://comments.gmane.org/gmane.comp.multimedia.puredata.devel/12921
	*/

	struct Midi2
	{
		unsigned char sysexWrapper[8];
		unsigned char status;	// status (low 4 bits) + chan-group in upper 4 bits (0x0f = All).
		unsigned char channel;	// 254 channels, 0xFF = All.
		unsigned char key;		// 255 keys, 0xFF = All.
		unsigned char unused; // was controllerClass; // 
		uint32_t value;			// upper 12 bits are Velocity/Controller Number, lower 20 bits are direct pitch or articulation.
		unsigned char sysexWrapperEnd[1];

		const unsigned char* data()
		{
			return (unsigned char*) this;
		}
		int size()
		{
			return (int) sizeof(Midi2);
		}
	};

	inline void setMidiMessage(Midi2& msg, int status, int value = 0, int key = 0xff, int velocity = 0, int channel = 0, int channelGroup = 0)
	{
		// Form SYSEX wrapper.
		msg.sysexWrapper[0] = GmpiMidi::MIDI_SystemMessage;
		msg.sysexWrapper[1] = 0x7f;	// universal real-time.
		msg.sysexWrapper[2] = 0x00; // device-ID. "channel"
		msg.sysexWrapper[3] = 0x70; // sub-ID#1. (Jeff's HD-PROTOCOL wrapper)
		msg.sysexWrapper[4] = 0x00; // sub-ID#2. (Version 0)
		msg.sysexWrapper[5] = 0x00; // unused padding for alignment. TODO move to front of struct
		msg.sysexWrapper[6] = 0x00;
		msg.sysexWrapper[7] = 0x00;

		msg.sysexWrapperEnd[0] = GmpiMidi::MIDI_SystemMessageEnd;

		msg.status = ( status & 0xF0 ) | ( channelGroup & 0x0F );
		msg.channel = static_cast<unsigned char>(channel);
		msg.key = static_cast<unsigned char>(key);
		msg.value = ( value & 0x0FFF ) | ( velocity << 20 );
		msg.unused /* was controllerClass */ = 0;
	}

	/* confirmed MIDI 2.0
	16 channels x 16 channel groups
	16 bit velocity
	16 bit articulation, 256 types
	32 bit CCs
	128 CCs
	256 registered per note controllers
	256 assignable per note controllers
	16384 RPNs/NRPNs


	inline void setMidiMessage2(Midi2& msg, int status, int value = 0, int key = 0xff, int velocity = 0, int channel = 0, int channelGroup = 0)
	{
		// Form SYSEX wrapper.
		msg.sysexWrapper[0] = GmpiMidi::MIDI_SystemMessage;
		msg.sysexWrapper[1] = 0x7f;	// universal real-time.
		msg.sysexWrapper[2] = 0x00; // device-ID. "channel"
		msg.sysexWrapper[3] = 0x70; // sub-ID#1. (Jeff's HD-PROTOCOL wrapper)
		msg.sysexWrapper[4] = 0x01; // sub-ID#2. (Version 1)
		msg.sysexWrapper[5] = 0x00; // unused padding for alignment. TODO move to front of struct
		msg.sysexWrapper[6] = 0x00;
		msg.sysexWrapper[7] = 0x00;

		msg.sysexWrapperEnd[0] = GmpiMidi::MIDI_SystemMessageEnd;

		msg.status = (status & 0xF0) | (channelGroup & 0x0F);
		msg.channel = static_cast<unsigned char>(channel);
		msg.key = static_cast<unsigned char>(key);

		msg.value = (velocity & 0x0FFF) | (value << 20);
		msg.unused / * was controllerClass * / = 0;
	}
*/

	inline bool isWrappedHdProtocol(const unsigned char* m, int size)
	{
		return size == sizeof(Midi2) && m[0] == GmpiMidi::MIDI_SystemMessage && m[1] == 0x7f && m[2] == 0x00 && m[3] == 0x70 && m[4] == 0x00;
	}

	inline void DecodeHdMessage(const unsigned char* midiMessage, int /* size */, int& status, int& midiChannel, int& channelGroup, int& keyNumber, int& val_12b, int& val_20b)
	{
		Midi2* m2 = (Midi2*) midiMessage;
		status = m2->status & 0xF0;
		channelGroup = m2->status & 0x0f;
		midiChannel = m2->channel;
		keyNumber = m2->key;
		val_20b = m2->value & 0x0FFF;
		val_12b = m2->value >> 20;
	}
}

#pragma pack(pop)
