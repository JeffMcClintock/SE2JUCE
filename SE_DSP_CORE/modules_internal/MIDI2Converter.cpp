/* Copyright (c) 2007-2022 SynthEdit Ltd
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name SEM, nor SynthEdit, nor 'Music Plugin Interface' nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY SynthEdit Ltd ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL SynthEdit Ltd BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "mp_sdk_audio.h"
#include "mp_midi.h"
#include "../shared/xp_simd.h"

SE_DECLARE_INIT_STATIC_FILE(MIDI2Converter)

/*
namespace gmpi
{
namespace midi
{
	enum class MidiStandard {MIDI_1_0, MIDI_2_0};

	class midiEvent
	{
	public:
		midiEvent(const unsigned char* midiMessage, int size)
		{

		}
	};
}
}
*/

using namespace gmpi;
using namespace gmpi::midi;

class MIDI2Converter final : public MpBase2
{
	MidiInPin pinMIDIIn;
	MidiOutPin pinMIDIOut;
	IntInPin pinOutputMode;

	enum class MidiStandard { MIDI_1_0, MIDI_2_0 };

	gmpi::midi_2_0::MidiConverter1 midiConverter_to_1_0;
	gmpi::midi_2_0::MidiConverter2 midiConverter_to_2_0;

public:
	MIDI2Converter() : 
		midiConverter_to_1_0(
			// provide a lambda to accept converted MIDI 2.0 messages
			[this](const midi::message_view& msg, int offset) {
				pinMIDIOut.send((const unsigned char*) msg.begin(), static_cast<int>(msg.size()), offset);
			}
		),
		midiConverter_to_2_0(
			// provide a lambda to accept converted MIDI 2.0 messages
			[this](const midi::message_view& msg, int offset) {
				pinMIDIOut.send((const unsigned char*) msg.begin(), static_cast<int>(msg.size()), offset);
			}
		)
	{
		initializePin(pinMIDIIn);
		initializePin(pinMIDIOut);
		initializePin(pinOutputMode);
	}

	void onMidiMessage(int /*pin*/, const unsigned char* midiMessage, int size) override
	{
		midi::message_view msg(midiMessage, size);

		const auto inputMode = gmpi::midi_2_0::isMidi2Message(msg) ? MidiStandard::MIDI_2_0 : MidiStandard::MIDI_1_0;
		const auto outputMode = pinOutputMode.getValue() ? MidiStandard::MIDI_2_0 : MidiStandard::MIDI_1_0;

		if (inputMode == outputMode)
		{
			pinMIDIOut.send(midiMessage, size);
			return;
		}

		// MIDI 2.0
		if (inputMode == MidiStandard::MIDI_2_0)
		{
			// MIDI 2.0 to MIDI 1.0
			midiConverter_to_1_0.processMidi(msg, -1);
		}
		else
		{
			// MIDI 1.0 to MIDI 2.0
			midiConverter_to_2_0.processMidi(msg, -1);
		}
	}
};

namespace
{
	auto r = Register<MIDI2Converter>::withXml(R"XML(
<?xml version="1.0" ?>
<PluginList>
  <Plugin id="SE MIDI2 Converter" name="MIDI2 Converter" category="MIDI">
    <Audio>
      <Pin name="MIDI In" datatype="midi" />
      <Pin name="MIDI Out" datatype="midi" direction="out" />
      <Pin name="Output Mode" datatype="enum" metadata="MIDI 1.0, MIDI 2.0" />
    </Audio>
  </Plugin>
</PluginList>
)XML");
}