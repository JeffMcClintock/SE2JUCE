
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
#include "../../modules/se_sdk3_hosting/Presenter.h"
#include "../../modules/se_sdk3_hosting/ModuleView.h"

using namespace gmpi;

class PrefabBrowserGui final : public SeGuiInvisibleBase
{
	enum ModuleCategories { IO, Oscillator, LFO, Envelope, Filter };

 	void onSetNameIndex()
	{
	}

 	void onSetCategoryIndex()
	{
		switch (pinCategoryIndex)
		{
		case ModuleCategories::IO:
			pinNamesList = L"Output";
			break;

		case ModuleCategories::Oscillator:
			pinNamesList = L"Sine";
			break;

		case ModuleCategories::LFO:
			pinNamesList = L"";
			break;

		case ModuleCategories::Envelope:
			pinNamesList = L"AR";
			break;

		case ModuleCategories::Filter:
			pinNamesList = L"";
			break;

		default:
			pinNamesList = L"";
			break;
		}
	}

	void onMenu()
	{
		it_enum_list it(pinNamesList);
		if (!it.FindValue(pinNameIndex))
		{
			return;
		}

		dynamic_cast<SynthEdit2::ModuleView*>(getHost())->Presenter()->InsertRackModule(it.CurrentItem()->text);
	}

 	IntGuiPin pinNameIndex;
	StringGuiPin pinNamesList;
	IntGuiPin pinCategoryIndex;
	StringGuiPin pinCategoryList;

	StringGuiPin pinMenuItems;
	IntGuiPin pinMenuSelection;

public:
	PrefabBrowserGui()
	{
		initializePin(pinNameIndex, static_cast<MpGuiBaseMemberPtr2>(&PrefabBrowserGui::onSetNameIndex) );
		initializePin(pinNamesList);
		initializePin(pinCategoryIndex, static_cast<MpGuiBaseMemberPtr2>(&PrefabBrowserGui::onSetCategoryIndex) );
		initializePin(pinCategoryList);
		initializePin(pinMenuItems);
		initializePin(pinMenuSelection, static_cast<MpGuiBaseMemberPtr2>(&PrefabBrowserGui::onMenu));
	}

	int32_t MP_STDCALL initialize() override
	{
		pinMenuItems = L"Insert Module";

		pinCategoryList = L"I/O, Oscillator, LFO, Envelope, Filter";
		onSetCategoryIndex();

		return SeGuiInvisibleBase::initialize();
	}
};

namespace
{
	auto r = Register<PrefabBrowserGui>::withXml(R"XML(
<?xml version="1.0" ?>
<PluginList>
  <Plugin id="SE Prefab Browser" name="Prefab Browser" category="Special">
    <GUI>
      <Pin name="Index" datatype="int" direction="out" />
      <Pin name="Names" datatype="string" direction="out" />
      <Pin name="Index" datatype="int" direction="out" />
      <Pin name="Categories" datatype="string" direction="out" />
      <Pin name="Menu Items" datatype="string" direction="out" />
      <Pin name="Menu Selection" datatype="int" direction="out" />
    </GUI>
  </Plugin>
</PluginList>
)XML");
}
