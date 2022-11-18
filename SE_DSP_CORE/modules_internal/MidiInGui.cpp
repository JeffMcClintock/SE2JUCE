#include "pch.h"
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
#include "Drawing.h"
#include "MidiIn.h"
#include "../se_sdk3/TimerManager.h"

using namespace gmpi;
using namespace GmpiDrawing;

class MidiInGui final : public gmpi_gui::MpGuiGfxBase, public TimerClient
{
 	void onActivity()
	{
		if (pinActivity > 0)
		{
			if (count < 0)
			{
				invalidateRect();
			}

			StartTimer(10);
			count = 10;
		}
	}

 	IntGuiPin pinActivity;
	int count = -1;

public:
	MidiInGui()
	{
		initializePin(pinActivity, static_cast<MpGuiBaseMemberPtr2>(&MidiInGui::onActivity) );
	}

	int32_t MP_STDCALL OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext ) override
	{
		Graphics g(drawingContext);

		const auto rect = getRect();

		auto brush = g.CreateSolidColorBrush(Color::Gray);
		const auto center = CenterPoint(rect);
		const float radius = 4.5f;

		g.DrawCircle(center, radius, brush);

		brush.SetColor(count > 0 ? Color::Lime : Color::DimGray);

		g.FillCircle(center, radius, brush);

		return gmpi::MP_OK;
	}

	bool OnTimer() override
	{
		auto r = --count >= 0;

		if (!r)
		{
			invalidateRect();
		}

		return r;
	}

	int32_t MP_STDCALL measure(GmpiDrawing_API::MP1_SIZE availableSize, GmpiDrawing_API::MP1_SIZE* returnDesiredSize) override
	{
		returnDesiredSize->width = 8;
		returnDesiredSize->height = 14;

		return gmpi::MP_OK;
	}
};

namespace
{
	auto r = Register<MidiIn>::withXml(R"XML(
<?xml version="1.0" ?>
<PluginList>
  <Plugin id="MIDI In" name="MIDI In" category="MIDI">
    <Parameters>
      <Parameter id="0" datatype="int" persistant="false" />
    </Parameters>
    <GUI>
      <Pin name="Activity" datatype="int" parameterId="0" />
    </GUI>
    <Audio>
      <Pin name="MIDI Data" datatype="midi" direction="out" />
      <Pin name="Activity" datatype="int" direction="out" parameterId="0" />
      <Pin name="MPE Mode" datatype="bool" default="1"/>
    </Audio>
  </Plugin>
</PluginList>
)XML");

	auto r2 = Register<MidiInGui>::withId(L"MIDI In");
}


