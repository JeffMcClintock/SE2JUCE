#include "../se_sdk3/mp_sdk_audio.h"
#include <vector>

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

public:
	KeyboardMidi()
	{
		initializePin( pinChannel );
		initializePin( pinMIDIOut );
	}

	// 'Backdoor' to UI class.  Not recommended. WARNING: Called outside scope of Process function, you can't set pins or send MIDI during this call.
	virtual int32_t MP_STDCALL receiveMessageFromGui(int32_t id, int32_t size, const void* messageData) override
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
		for( auto msg : buffer)
		{
			pinMIDIOut.send(msg.data, sizeof(msg.data), timeStamp++);

			if (timeStamp >= sampleFrames)
				return;
		}

		buffer.clear();
	}

	virtual void onSetPins() override
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
