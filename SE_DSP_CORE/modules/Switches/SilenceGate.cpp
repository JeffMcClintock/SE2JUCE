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
SE_DECLARE_INIT_STATIC_FILE(SilenceGate)

using namespace gmpi;

class SilenceGate final : public MpBase2
{
	AudioInPin pinSignal;
	AudioOutPin pinOutput;

public:
	SilenceGate()
	{
		initializePin( pinSignal );
		initializePin( pinOutput );
	}

	void subProcess( int sampleFrames )
	{
		// get pointers to in/output buffers.
		auto signal = getBuffer(pinSignal);
		auto output = getBuffer(pinOutput);

		// mute anything that is inaudible in 24-bit precision.
		constexpr float threshold = 0.8f / (float) (1 << 24);

		bool enable = false;
		for (int s = 0 ; s < sampleFrames; ++s)
		{
			if (signal[s] > threshold || signal[s] < -threshold)
			{
				enable = true;
				break;
			}
		}

		if (pinOutput.isStreaming() != enable)
		{
			if(pinSignal.isStreaming())
				pinOutput.setStreaming(enable, getBlockPosition());
		}

		if (enable)
		{
			for (int s = sampleFrames; s > 0; --s)
			{
				*output = *signal;

				// Increment buffer pointers.
				++signal;
				++output;
			}
		}
		else
		{
			// mute.
			for(int s = sampleFrames; s > 0; --s)
			{
				*output = 0.0f;
				++output;
			}
		}
	}

	void onSetPins() override
	{
		// Check which pins are updated.
		if( !pinSignal.isStreaming() )
		{
			pinOutput.setStreaming(false);
		}

		// Set processing method.
		setSubProcess(&SilenceGate::subProcess);
	}
};

namespace
{
	auto r = Register<SilenceGate>::withId(L"SE Silence Gate");
}
