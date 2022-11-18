#include "../se_sdk3/mp_sdk_audio.h"
#include "../se_sdk3/mp_midi.h"
#include <sstream>
using namespace gmpi;

class MidiMessage : public MpBase2
{
	StringInPin pinHexBytes;
	BoolInPin pinTrigger;
	MidiOutPin pinMidiOut;

public:
	MidiMessage()
	{
		initializePin( pinHexBytes );
		initializePin( pinTrigger );
		initializePin( pinMidiOut );
	}

	void onSetPins() override
	{
		if(pinTrigger.isUpdated() && pinTrigger.getValue())
		{
			std::wstringstream ss(pinHexBytes.getValue());
			
			int idx{};
			const int maxSize = 1000;
			unsigned char midiMessage[maxSize];
			while(ss.good() && idx < maxSize)
			{
				std::wstring substr;
				std::getline(ss, substr, L',');
				
				unsigned char byte{};
				int nybble{};
				for(auto c : substr)
				{
					c = tolower(c);

					if(c >= L'0' && c <= L'9')
					{
						byte = (byte << 4) | (c - L'0');
						++nybble;
					}
					if(c >= L'a' && c <= L'f')
					{
						byte = (byte << 4) | (c - L'a' + 10);
						++nybble;
					}
					if(nybble == 2)
					{
						nybble = 0;
						midiMessage[idx++] = byte;
					}
				}
				if (nybble == 1) // trailing byte was 1 char only, pass it out.
				{
					midiMessage[idx++] = byte;
				}
			}

			if(idx > 0)
			{
				pinMidiOut.send(midiMessage, idx);
			}
		}
	}
};

namespace
{
	auto r = Register<MidiMessage>::withId(L"SE MidiMessage");
}
