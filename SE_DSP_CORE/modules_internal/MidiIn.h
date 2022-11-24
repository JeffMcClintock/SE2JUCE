
#pragma once
/* Copyright (c) 2007-2021 SynthEdit Ltd
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
#include "../se_sdk3/mp_sdk_audio.h"
#include "ug_base.h"
#include "ug_oversampler.h"
#include "SeAudioMaster.h"
#include "UMidiBuffer2.h"
#include "mp_midi.h"
#include "iseshelldsp.h"

using namespace gmpi;

class MidiIn final : public MpBase2, public ISpecialIoModule
{
	BoolInPin pinMpeMode;
	MidiOutPin pinMidiOut;
	IntOutPin pinActivity;

	MidiBuffer3 MidiInBuffer;
	int HundrendMicroseconds = {};
	ug_base* ugbase = {};
	int maxBufferAhead = {};
	gmpi::midi_2_0::MidiConverter2 midiConverter;
	gmpi::midi_2_0::MpeConverter mpeConverter;

	// MPE
	int lower_zone_size = 0; // both 0 = not MPE mode
	int upper_zone_size = 0;
	bool auto_mpe_mode = false;

	unsigned short incoming_rpn[16];
	// RPNs are 14 bit values, so this value never occurs, represents "no rpn"
	static const unsigned short NULL_RPN = 0xffff;
	void cntrl_update_msb(unsigned short& var, short hb) const
	{
		var = (var & 0x7f) + (hb << 7);	// mask off high bits and replace
	}
	void cntrl_update_lsb(unsigned short& var, short lb) const
	{
		var = (var & 0x3F80) + lb;			// mask off low bits and replace
	}

public:
	MidiIn() :
		midiConverter(
			// provide a lambda to accept converted MIDI 2.0 messages
			[this](const midi::message_view& msg, int offset) {
				pinMidiOut.send((const unsigned char*)msg.begin(), static_cast<int>(msg.size()), offset);
			}
		),
		mpeConverter(
			// provide a lambda to accept converted MPE messages
			[this](const midi::message_view& msg, int offset) {
				pinMidiOut.send((const unsigned char*)msg.begin(), static_cast<int>(msg.size()), offset);
			}
		)
	{
		initializePin(pinMidiOut);
		initializePin(pinActivity);
		initializePin(pinMpeMode);

		std::fill(std::begin(incoming_rpn), std::end(incoming_rpn), NULL_RPN);
	}

	int32_t MP_STDCALL open() override
	{
		// Register to receive MIDI data
		ugbase = dynamic_cast<ug_base*>(getHost());

		if (dynamic_cast<ug_oversampler*>(ugbase->AudioMaster()))
		{
			ugbase->message(L"This module can't be oversampled. Please move it out of oversampler Container.");
			return gmpi::MP_FAIL;
		}

		ugbase->AudioMaster()->RegisterIoModule(this);

		setSubProcess(&MidiIn::subProcess);
		setSleep(false);

		HundrendMicroseconds = static_cast<int>(getSampleRate() * 0.0001f);
		maxBufferAhead = static_cast<int>(getSampleRate() * 0.1f); // MIDI FAR ahead of current time is dues to sync error.

		return MpBase2::open();
	}

	void AddMidiEvent(timestamp_t timestamp, unsigned char* data, int size)
	{
		MidiInBuffer.Add(timestamp, data, size);
	}

	void subProcess( int sampleFrames )
	{
		bool sentMidi = {};

		const auto absoluteBlockStartTime = ugbase->SampleClock();

		// can't send events with 'past' timestamp.
		int nextOffset = 0;

		while (!MidiInBuffer.IsEmpty() && nextOffset < sampleFrames)
		{
			auto offset = static_cast<int>(MidiInBuffer.PeekNextTimestamp() - absoluteBlockStartTime);

#if 0 //defined( _DEBUG )
			// MIDI msgs may arrive out of order.  Especially when MIDI latency changed due to audio overun.
			// If so, adjust clock to enforce "MIDI Data must be in order" rule.
			if (offset < nextOffset)
			{
				_RPT1(_CRT_WARN, "ug_midi_in: MIDI data out-of-order/ too close. Moved forward %dms\n", SampleToMs(nextOffset - offset, (int)getSampleRate()));
			}
#endif
			offset = std::max(offset, nextOffset);

			if (offset >= sampleFrames)
			{
				if (offset >= maxBufferAhead) // MIDI FAR ahead of current time is due to sync error. Simply send now.
				{
					offset = nextOffset;
				}
				else
				{
					break;
				}
			}

			auto e = MidiInBuffer.Current();

			// Handle MPE Configuration messages. (MCM)
			{
				const int status = e->data[0] >> 4;
				const int midiChannel = e->data[0] & 0x0f;

				if (midi_1_0::status_type::ControlChange == status && (midiChannel == 0 || midiChannel == 15))
				{
					// incoming change of NRPN or RPN (registered parameter number)
					switch (e->data[1])
					{
					case 101:
						cntrl_update_msb(incoming_rpn[midiChannel], static_cast<short>(e->data[2]));
						break;

					case 100:
						cntrl_update_lsb(incoming_rpn[midiChannel], static_cast<short>(e->data[2]));
						break;

					case 98:
					case 99:
						incoming_rpn[midiChannel] = NULL_RPN;	// NRPNS are coming, cancel rpn msgs
						break;

					case 6:	// RPN_CONTROLLER
					{
						if (incoming_rpn[midiChannel] == 6) // number of channels in zone
						{
							const int number_of_channels_in_zone = e->data[2];
							if (0 == midiChannel)
							{
								lower_zone_size = number_of_channels_in_zone;
								upper_zone_size = (std::min)(upper_zone_size, 15 - lower_zone_size);
							}
							else
							{
								upper_zone_size = number_of_channels_in_zone;
								lower_zone_size = (std::min)(lower_zone_size, 15 - upper_zone_size);
							}

							// (“MPE Mode”) shall be enabled in a controller or a synthesizer when at least one MPE Zone is configured.
							// Setting both Zones on the Manager Channels to use no Channels, shall deactivate the MPE Mode.
							auto_mpe_mode = lower_zone_size || upper_zone_size;
						}
					}
					break;

					}
				}
			}

			if (pinMpeMode || auto_mpe_mode)
			{
				mpeConverter.processMidi({ e->data, e->size }, offset);
			}
			else
			{
				midiConverter.processMidi({ e->data, e->size }, offset);
			}

			sentMidi = true;

			MidiInBuffer.UpdateReadPos(); // safe for other thread to write to buffer now that data no longer needed.

			// Enforce some minimal spacing between MIDI events to retain their order. Prevents ambiguous note-on/note-off from drum-machine.
			// Normal MIDI note-on is about 1ms, so 0.1ms minimum should not affect genuine timing information.
			nextOffset = offset + HundrendMicroseconds;
		}

		if (sentMidi)
		{
			pinActivity.setValue(pinActivity.getValue() + 1, 0);
		}
	}
};
