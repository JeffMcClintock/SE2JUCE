#include "../se_sdk3/mp_sdk_gui2.h"
#include <sstream>
#include "../shared/it_enum_list.h"

using namespace std;
using namespace gmpi;

struct presetinfo
{
	int index;
	wstring category;
	wstring name;
};

class PatchBrowserGui : public SeGuiInvisibleBase
{
	IntGuiPin programIn;
	StringGuiPin programNamesListIn;
	StringGuiPin programCategoriesListIn;
	IntGuiPin patchCommandIn;
	StringGuiPin patchCommandListIn;

 	IntGuiPin pinPreset;
 	StringGuiPin pinPresetNamesList;
	IntGuiPin patchCommandOut;
	StringGuiPin patchCommandListOut;
	StringGuiPin programNameIn;
	StringGuiPin programNameOut;
	StringGuiPin CategoryIn;
	StringGuiPin CategoryOut;
	BoolGuiPin ModifiedIn;
	BoolGuiPin ModifiedOut;

	void onSetPreset()
	{
//		_RPT1(_CRT_WARN, "onSetPreset() %d\n", pinPreset.getValue());
		if (pinPreset >= 0)
		{
			programIn = pinPreset;
		}
	}

public:
	PatchBrowserGui()
	{
		// Currently ignoring internal preset list.
		// Might be good to enable it in Edit mode.
		initializePin(programIn, static_cast<MpGuiBaseMemberPtr2>(&PatchBrowserGui::onSetprogramIn));
		initializePin(programNamesListIn, static_cast<MpGuiBaseMemberPtr2>(&PatchBrowserGui::updatePresetList));
		initializePin(patchCommandIn);
		initializePin(patchCommandListIn, static_cast<MpGuiBaseMemberPtr2>(&PatchBrowserGui::onSetPatchCommandListIn));

		initializePin( pinPreset, static_cast<MpGuiBaseMemberPtr2>(&PatchBrowserGui::onSetPreset) );
		initializePin( pinPresetNamesList );
		initializePin(patchCommandOut, static_cast<MpGuiBaseMemberPtr2>(&PatchBrowserGui::onSetpatchCommandOut));
		initializePin(patchCommandListOut);

		initializePin(programCategoriesListIn, static_cast<MpGuiBaseMemberPtr2>(&PatchBrowserGui::updatePresetList));

		initializePin(programNameIn, static_cast<MpGuiBaseMemberPtr2>(&PatchBrowserGui::onSetprogramNameIn));
		initializePin(programNameOut, static_cast<MpGuiBaseMemberPtr2>(&PatchBrowserGui::onSetprogramNameOut));

		initializePin(CategoryIn, static_cast<MpGuiBaseMemberPtr2>(&PatchBrowserGui::onSetCategoryIn));
		initializePin(CategoryOut, static_cast<MpGuiBaseMemberPtr2>(&PatchBrowserGui::onSetCategoryOut));

		initializePin(ModifiedIn, static_cast<MpGuiBaseMemberPtr2>(&PatchBrowserGui::onSetModified));
		initializePin(ModifiedOut);

//		add pins for patch commands, refresh presets on import bank
//			add factory support and bool to enable/disable it
	}

	void onSetprogramNameIn()
	{
		programNameOut = programNameIn;
	}
	void onSetprogramNameOut()
	{
		programNameIn = programNameOut;
	}

	void onSetprogramIn()
	{
//		_RPT1(_CRT_WARN, "onSetprogramIn() %d\n", programIn.getValue());
		pinPreset = programIn;
	}

	void onSetpatchCommandOut()
	{
		patchCommandIn = patchCommandOut;

		if (patchCommandOut == 4) // "Import Bank". magic number.
		{
			// Refresh disk preset list.
			updatePresetList();
		}

		// Reset to Zero after executing command.
		if (patchCommandOut > 0)
		{
			patchCommandOut = 0;
			patchCommandIn = 0;
		}
	}

	void onSetPatchCommandListIn()
	{
		patchCommandListOut = patchCommandListIn;
	}

	void onSetCategoryIn()
	{
		CategoryOut = CategoryIn;
	}
	void onSetModified()
	{
		ModifiedOut = ModifiedIn;
	}
	void onSetCategoryOut()
	{
		CategoryIn = CategoryOut;
	}

	void updatePresetList()
	{
		wstring presetNames;
		wstring categoryNames;

		presetNames = programNamesListIn;
		categoryNames = programCategoriesListIn;

		// category, name
		std::vector< presetinfo > outputItems;

		{
			it_enum_list names(presetNames);
			it_enum_list categories(categoryNames);
			categories.First();
			std::wstring category;
			int index = 0;
			for (names.First(); !names.IsDone(); names.Next())
			{
				if (!categories.IsDone())
				{
					category = categories.CurrentItem()->text;
					categories.Next();
				}
				else
				{
					category = L"";
				}

				presetinfo pi{ index, category, names.CurrentItem()->text };
				outputItems.push_back(pi);

				++index;
			}
		}

		std::sort(outputItems.begin(), outputItems.end(),
			[=](const presetinfo& a, const presetinfo& b) -> bool
			{
				// Sort by category
				if (a.category != b.category)
				{
					// blank category last
					if (a.category.empty() != b.category.empty())
						return a.category.empty() < b.category.empty();

					return a.category < b.category;
				}

				// ..then by name
				return a.name < b.name;
			});

		// Don't bother with sub-menus unless user has specified some actual categories.
		bool usingCategories = !outputItems.empty() && !outputItems.front().category.empty();

		std::wostringstream oss;
		{
			bool first = true;
			bool inSubMenu = false;
			std::wstring category{ L"pRetTyUnlikelyString#$%" };
			for (auto& i : outputItems)
			{
				if (!first)
				{
					oss << L',';
				}

				if (usingCategories && category != i.category)
				{
					category = i.category;
					if (inSubMenu)
					{
						oss << L"<<<<,";
					}
					std::wstring subMenu = category.empty() ? L"Other" : category;
					oss << L">>>>" << subMenu << L",";

					inSubMenu = true;
				}

				oss << i.name << L'=' << i.index;

				first = false;
			}
		}
//		_RPT1(_CRT_WARN, "%s\n", oss.str().c_str());

		pinPresetNamesList = oss.str();

		onSetprogramIn(); // In 32-bit GUI, we might have skipped this due to onSetprogramIn() being called first when 'inSynthEdit' was false.
	}
};

namespace
{
	auto r = Register<PatchBrowserGui>::withId(L"SE Patch Browser");
}
SE_DECLARE_INIT_STATIC_FILE(PatchBrowser_Gui);