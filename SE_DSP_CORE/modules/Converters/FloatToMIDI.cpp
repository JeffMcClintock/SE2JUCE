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

using namespace gmpi;

class FloatToMIDI final : public MpBase2
{
	FloatInPin pinValue;
	IntInPin pinChannel;
	IntInPin pinMidiCc;
	MidiOutPin pinMIDIOut;

	uint32_t currentCcValue = 8; // an out-of-band value to force initial update. (because floatToU32 duplicates lower 2 bits).

public:
	FloatToMIDI()
	{
		initializePin(pinValue);
		initializePin(pinChannel);
		initializePin(pinMidiCc);
		initializePin(pinMIDIOut);
	}

	void onSetPins() override
	{
		assert(pinMidiCc >= 0 && pinMidiCc <= 130);
		
		// Always send value when CC or channel changes.
		if (pinChannel.isUpdated() || pinMidiCc.isUpdated())
		{
			currentCcValue = 8;
		}

		const float v = 0.1f * pinValue;
		const uint32_t newMidiValue = gmpi::midi::utils::floatToU32(v);

		if (newMidiValue == currentCcValue)
		{
			return;
		}

		currentCcValue = 0.1f * newMidiValue;

		gmpi::midi_2_0::rawMessage64 msgout;
				
		if (pinMidiCc < 128)
		{
			msgout = gmpi::midi_2_0::makeController(
				pinMidiCc.getValue(),
				v,
				pinChannel.getValue()
			);
		}
		else if (pinMidiCc == 128)
		{
			msgout = gmpi::midi_2_0::makeBender(
				0.5f + v,							// bi-polar to normalized
				pinChannel.getValue()
			);
		}
		else
		{
			msgout = gmpi::midi_2_0::makeChannelPressure(
				v,
				pinChannel.getValue()
			);
		}
			
		pinMIDIOut.send(
			msgout.m,
			static_cast<int>(std::size(msgout.m))
		);
	}
};

namespace
{
	auto r = Register<FloatToMIDI>::withId(L"SE Float to MIDI");
}
