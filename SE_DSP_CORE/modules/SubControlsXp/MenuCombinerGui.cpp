#include "mp_sdk_gui2.h"
#include "it_enum_list.h"

using namespace gmpi;

class MenuCombinerGui : public SeGuiInvisibleBase
{
  	StringGuiPin pinAItemList;
 	IntGuiPin pinAChoice;
 	StringGuiPin pinBItemList;
 	IntGuiPin pinBChoice;
 	StringGuiPin pinItemListOut;
 	IntGuiPin pinChoiceOut;
 	BoolGuiPin pinASupportMomentary;
 	BoolGuiPin pinBSuportMomentary;
	StringGuiPin pinASubmenuTitle;
	StringGuiPin pinBSubmenuTitle;

	int firstListStartIndex = 0;
	int secondListStartIndex = 0;

public:
	MenuCombinerGui()
	{
		initializePin( pinAChoice );
		initializePin( pinAItemList, static_cast<MpGuiBaseMemberPtr2>(&MenuCombinerGui::onSetItemListIn) );
		initializePin( pinBChoice );
		initializePin( pinBItemList, static_cast<MpGuiBaseMemberPtr2>(&MenuCombinerGui::onSetItemListIn) );
		initializePin( pinChoiceOut, static_cast<MpGuiBaseMemberPtr2>(&MenuCombinerGui::onSetChoice) );
		initializePin( pinItemListOut);
		initializePin(pinASupportMomentary);
		initializePin(pinBSuportMomentary);
		initializePin(pinASubmenuTitle, static_cast<MpGuiBaseMemberPtr2>(&MenuCombinerGui::onSetItemListIn));
		initializePin(pinBSubmenuTitle, static_cast<MpGuiBaseMemberPtr2>(&MenuCombinerGui::onSetItemListIn));
	}

	void onSetItemListIn()
	{
		std::wstring combinedList;

		firstListStartIndex = 0;

		if (!pinASubmenuTitle.getValue().empty())
		{
			combinedList.append(L">>>>");
			combinedList.append(pinASubmenuTitle.getValue());
			++firstListStartIndex;
		}

		secondListStartIndex = firstListStartIndex;

		it_enum_list it(pinAItemList);
		for (it.First(); !it.IsDone(); it.Next())
		{
			enum_entry* e = it.CurrentItem();

			if (!combinedList.empty())
			{
				combinedList.append(L",");
			}
			combinedList.append(e->text);
			++secondListStartIndex;
		}
		if (!pinASubmenuTitle.getValue().empty())
		{
			if (!combinedList.empty())
			{
				combinedList.append(L",");
			}

			combinedList.append(L"<<<<");
			++secondListStartIndex;
		}

		// 2nd list
		if (!pinBSubmenuTitle.getValue().empty())
		{
			if (!combinedList.empty())
			{
				combinedList.append(L",");
			}
			combinedList.append(L">>>>");
			combinedList.append(pinBSubmenuTitle.getValue());
			++secondListStartIndex;
		}
		it_enum_list it2(pinBItemList);
		for (it2.First(); !it2.IsDone(); it2.Next())
		{
			enum_entry* e = it2.CurrentItem();
			if (!combinedList.empty())
			{
				combinedList.append(L",");
			}
			combinedList.append(e->text);
		}
		if (!pinBSubmenuTitle.getValue().empty())
		{
			if (!combinedList.empty())
			{
				combinedList.append(L",");
			}
			combinedList.append(L"<<<<");
		}

		pinItemListOut = combinedList;
	}

	void onSetChoice()
	{
		if (pinChoiceOut == -1)
		{
			if (pinASupportMomentary)
			{
				pinAChoice = -1;
			}
			if (pinBSuportMomentary)
			{
				pinBChoice = -1;
			}
			return;
		}

		int i = pinChoiceOut; // output index starts at one.

		it_enum_list it(pinAItemList);
		if (it.FindIndex(i - firstListStartIndex))
		{
			pinAChoice = it.CurrentItem()->value;
		}
		else
		{
			it_enum_list it2(pinBItemList);
			if (it2.FindIndex(i - secondListStartIndex))
			{
				pinBChoice = it2.CurrentItem()->value;
			}
		}
	}
};

namespace
{
	auto r = Register<MenuCombinerGui>::withId(L"SE Menu Combiner");
}
