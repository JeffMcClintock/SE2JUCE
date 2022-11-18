#include "../se_sdk3/mp_sdk_gui2.h"
#include "it_enum_list.h"
#include "../shared/ListBuilder.h"

using namespace gmpi;

class MenuToListboxesGui : public SeGuiInvisibleBase
{
	struct presetInfo
	{
		int category;
		int index;
		std::wstring name;
	};

	std::vector<std::wstring> categories;
	std::vector< presetInfo > presets;
	int currentCategoryIndex = -1;
	int currentPresetIndex = -1;

	void onSetItemList()
	{
		presets.clear();
		categories.clear();

		int currentCategory = -1;
		int nestedLevel = 0;
		bool hasUncategorized = false;

		it_enum_list itr(pinItemList);
		for (itr.First(); !itr.IsDone(); itr.Next())
		{
			int32_t flags = itr.CurrentItem()->value == pinChoice ? gmpi_gui::MP_PLATFORM_MENU_TICKED : 0;

			auto& txt = itr.CurrentItem()->text;

			// Special commands (sub-menus)
			switch (itr.CurrentItem()->getType())
			{
			case enum_entry_type::Separator:
			case enum_entry_type::Break:
				continue;
				break;

			case enum_entry_type::SubMenu:
				++nestedLevel;
				if (nestedLevel == 1)
				{
					currentCategory = static_cast<int>(categories.size());
					categories.push_back(txt.substr(4));
				}
				continue;
				break;

			case enum_entry_type::SubMenuEnd:
				--nestedLevel;
				if (nestedLevel == 0)
				{
					currentCategory = -1;
				}
				continue;
				break;

			default:
				break;
			}
#if 0
			if (txt.size() > 3)
			{
				int i;
				for (i = 1; i < 4; ++i)
				{
					if (txt[0] != txt[i])
						break;
				}

				if (i == 4) // First 4 chars the same.
				{
					switch (txt[0])
					{
					case L'-':
					case L'|':
						break;

					case L'>':
						++nestedLevel;
						if (nestedLevel == 1)
						{
							currentCategory = static_cast<int>(categories.size());
							categories.push_back(txt.substr(4));
						}
						break;

					case L'<':
						--nestedLevel;
						if (nestedLevel == 0)
						{
							currentCategory = -1;
						}
						break;
					}
					continue;
				}
			}
#endif

			presetInfo p{ currentCategory, itr.CurrentItem()->value, txt };

			if (currentCategory == -1)
				hasUncategorized = true;

			presets.push_back(p);
		}

		// Add "Other" category only if uncategorised items exist
		if (hasUncategorized)
		{
			currentCategory = static_cast<int>(categories.size());
			categories.push_back(L"Other");
			for (auto& p : presets)
			{
				if( p.category == -1 )
					p.category = currentCategory;
			}
		}

		// compose list from categories
		ListBuilder lm;
		for (auto& s : categories)
			lm.Add(s);

		pinItemList1 = lm.list;
		onSetChoice();
	}

	void onSetChoice()
	{
		// selection has changed, update selected category and selected item.
		currentCategoryIndex = -1;
		for (auto& preset : presets)
		{
			if (preset.index == pinChoice)
			{
				currentCategoryIndex = preset.category;
				break;
			}
		}

		pinChoice1 = currentCategoryIndex;

		RefreshSecondaryList();
	}

	void RefreshSecondaryList()
	{
		int presetIndex = -1;

		if (currentCategoryIndex > -1)
		{
			ListBuilder lm;

			// create list of sub-items
			int i = 0;
			for (auto& preset : presets)
			{
				if (preset.category == currentCategoryIndex)
				{
					lm.Add(preset.name);
					if (preset.index == pinChoice)
					{
						presetIndex = i;
					}
					++i;
				}
			}

			pinItemList2 = lm.list;
		}
		else
		{
			pinItemList2 = L"";
		}

		pinChoice2 = presetIndex;
	}

	void onSetChoice1()
	{
		// User has chosen a new category, which may or may not contain the current selected item.
		currentCategoryIndex = pinChoice1;
		RefreshSecondaryList();
	}

	void onSetChoice2()
	{
		int i = 0;
		for (auto& preset : presets)
		{
			if (preset.category == currentCategoryIndex)
			{
				if (i == pinChoice2)
				{
					pinChoice = preset.index;
					return;
				}
				++i;
			}
		}
	}

 	IntGuiPin pinChoice;
	IntGuiPin pinChoice1;
	IntGuiPin pinChoice2;
 	StringGuiPin pinItemList;
	StringGuiPin pinItemList1;
	StringGuiPin pinItemList2;

public:
	MenuToListboxesGui()
	{
		initializePin(pinChoice, static_cast<MpGuiBaseMemberPtr2>(&MenuToListboxesGui::onSetChoice) );
		initializePin(pinItemList, static_cast<MpGuiBaseMemberPtr2>(&MenuToListboxesGui::onSetItemList) );
		initializePin(pinChoice1, static_cast<MpGuiBaseMemberPtr2>(&MenuToListboxesGui::onSetChoice1));
		initializePin(pinItemList1);
		initializePin(pinChoice2, static_cast<MpGuiBaseMemberPtr2>(&MenuToListboxesGui::onSetChoice2));
		initializePin(pinItemList2);
	}

};

namespace
{
	auto r = Register<MenuToListboxesGui>::withId(L"SE Menu to Listboxes");
}
