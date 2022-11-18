
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
#include "mp_sdk_gui2.h"
#include "../se_sdk3/mp_sdk_audio.h"

using namespace gmpi;

/*
class SquareGui final : public SeGuiInvisibleBase
{
 	FloatGuiPin pinSize;
 	BlobGuiPin pinGeometry;

public:
	SquareGui()
	{
		initializePin(pinSize);
		initializePin(pinGeometry);
	}
};
*/

namespace
{
//	auto r = Register<SquareGui>::withId(L"CD Square");
	auto r = Register<SeGuiInvisibleBase>::withId(L"CD Square");
}

// TODO: Don't even seem to need to make a class, just describe it in XML and register it as 'SeGuiInvisibleBase'
namespace
{
	auto r2 = Register<SeGuiInvisibleBase>::withId(L"CD Circle");
}

class Value : public MpBase2
{
public:
	Value()
	{
		initializePin(pinValueHostIn);
		initializePin(pinValueHostOut);
		initializePin(pinLatchInput);
		initializePin(pinValueIn);
		initializePin(pinValueOut);
	}

	void onSetPins(void) override
	{
		float value;
		if (pinLatchInput.getValue())
		{
			value = pinValueIn.getValue();
		}
		else
		{
			value = pinValueHostIn.getValue();
		}

		pinValueOut = value;
		pinValueHostOut = value;

		// When updating patch from DSP, SE does not reflect the value back to pinValueHostIn automatically, fake it.
		pinValueHostIn.setValueRaw(sizeof(value), &value);
	}

private:
	FloatInPin pinValueHostIn;
	FloatOutPin pinValueHostOut;
	BoolInPin pinLatchInput;
	FloatInPin pinValueIn;
	FloatOutPin pinValueOut;
};
namespace
{
		auto r3 = Register<Value>::withId(L"CD Value");
}