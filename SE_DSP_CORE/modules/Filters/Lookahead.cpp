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

using namespace gmpi;

class Lookahead final : public MpBase2
{
	FloatInPin pinLatencyMs;
	AudioInPin pinSignalIn;
	AudioOutPin pinSignalOut;

public:
	Lookahead()
	{
		initializePin(pinLatencyMs);
		initializePin(pinSignalIn);
		initializePin( pinSignalOut );
	}

	void subProcess( int sampleFrames )
	{
		// get pointers to in/output buffers.
		auto signalIn = getBuffer(pinSignalIn);
		auto signalOut = getBuffer(pinSignalOut);

		std::copy(signalIn, signalIn + sampleFrames, signalOut);
	}

	void onSetPins() override
	{
		// Check which pins are updated.
		if(pinLatencyMs.isUpdated())
		{
			const int latencySamples = static_cast<int>(0.5f + pinLatencyMs.getValue() * host.getSampleRate() * 0.001f );
//_RPTN(0, "Lookahead %fms (%d samples)\n", pinLatencyMs.getValue(), latencySamples);
			host.SetLatency(latencySamples);
		}

		// Set state of output audio pins.
		pinSignalOut.setStreaming(pinSignalIn.isStreaming());

		// Set processing method.
		setSubProcess(&Lookahead::subProcess);
	}
};

namespace
{
	auto r = Register<Lookahead>::withId(L"SE Lookahead");
}
