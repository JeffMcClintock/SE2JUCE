#include <vector>
#include "../se_sdk3/mp_sdk_audio.h"
#include "../se_sdk3/mp_midi.h"

using namespace gmpi;

class KeyboardMidi : public MpBase2
{
	IntInPin pinChannel;
	MidiOutPin pinMIDIOut;

	struct SimpleMidiMessage
	{
		unsigned char data[3];
	};

	std::vector< SimpleMidiMessage > buffer;
	gmpi::midi_2_0::MidiConverter2 midiConverter;

public:
	KeyboardMidi() :
		// init the midi converter
		midiConverter(
			// provide a lambda to accept converted MIDI 2.0 messages
			[this](const midi::message_view& msg, int offset)
			{
				pinMIDIOut.send(msg.begin(), msg.size(), offset);
			}
		)
	{
		initializePin( pinChannel );
		initializePin( pinMIDIOut );

		buffer.reserve(20);
	}

	// 'Backdoor' to UI class.  Not recommended. WARNING: Called outside scope of Process function, you can't set pins or send MIDI during this call.
	int32_t MP_STDCALL receiveMessageFromGui(int32_t id, int32_t size, const void* messageData) override
	{
		if (size == 3)
		{
			auto message = reinterpret_cast<const unsigned char*>(messageData);

			// Apply channel.
			SimpleMidiMessage midi;
			midi.data[0] = message[0] | static_cast<unsigned char>(pinChannel.getValue());
			midi.data[1] = message[1];
			midi.data[2] = message[2];
			buffer.push_back(midi);
		}

		return gmpi::MP_OK;
	}

	void subProcess(int sampleFrames)
	{
		int timeStamp = 0;
		for (auto msg : buffer)
		{
			midiConverter.processMidi({msg.data, sizeof(msg.data)}, getBlockPosition() + timeStamp++);

			if (timeStamp >= sampleFrames)
				return;
		}

		buffer.clear();
	}

	void onSetPins() override
	{
		setSubProcess(&KeyboardMidi::subProcess);

		// Set sleep mode (optional).
		setSleep(false);
	}
};

namespace
{
	auto r = Register<KeyboardMidi>::withId(L"SE Keyboard (MIDI)");
}
