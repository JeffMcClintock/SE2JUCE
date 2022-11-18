#include "mp_sdk_gui2.h"
#include "Drawing.h"
#include "it_enum_list.h"
#include "../shared/unicode_conversion.h"
#include "../shared/FontCache.h"

using namespace gmpi;
using namespace GmpiDrawing;
using namespace JmUnicodeConversions;

#error not used

class PatchBrowserGuiGui : public gmpi_gui::MpGuiGfxBase, public FontCacheClient
{
	struct presetInfo
	{
		int category;
		int index;
		std::string name;
	};

	std::vector<std::string> categories;
	std::vector< presetInfo > presets;
	int currentCategoryIndex = -1;
	int currentPresetIndex = -1;

	IntGuiPin pinChoice;
	StringGuiPin pinItemList;
	StringGuiPin pinStyle;
	/*
		BoolGuiPin pinWriteable;
		BoolGuiPin pinGreyed;
		StringGuiPin pinHint;
		StringGuiPin pinMenuItems;
		IntGuiPin pinMenuSelection;
		StringGuiPin pinHeading;
		BoolGuiPin pinMomentary;
		BoolGuiPin pinEnableSpecialStrings;
	*/


 	void onSetChoice()
	{
		// pinChoice changed
	}

 	void onSetItemList()
	{
		presets.clear();
		categories.clear();

		int currentCategory = 0;
		categories.push_back("Other");
		int nestedLevel = 0;

		it_enum_list itr(pinItemList);
		for (itr.First(); !itr.IsDone(); itr.Next())
		{
			int32_t flags = itr.CurrentItem()->value == pinChoice ? gmpi_gui::MP_PLATFORM_MENU_TICKED : 0;

			auto& txt = itr.CurrentItem()->text;

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
							++currentCategory;
							categories.push_back(WStringToUtf8(txt.substr(4)));
						}
						break;

					case L'<':
						--nestedLevel;
						if (nestedLevel == 0)
						{
							currentCategory = 0;
						}
						break;
					}
					continue;
				}
			}

			//// Add "Other" category only if uncategorised items exist
			//if(categories.empty())
			//	categories.push_back(currentCategory);

			presetInfo p{ currentCategory, itr.CurrentItem()->value, WStringToUtf8(txt) };
			presets.push_back(p);
		}
	}

 	void onSetStyle()
	{
		// pinStyle changed
	}

public:
	PatchBrowserGuiGui()
	{
		initializePin( pinChoice, static_cast<MpGuiBaseMemberPtr2>(&PatchBrowserGuiGui::onSetChoice) );
		initializePin( pinItemList, static_cast<MpGuiBaseMemberPtr2>(&PatchBrowserGuiGui::onSetItemList) );
		initializePin( pinStyle, static_cast<MpGuiBaseMemberPtr2>(&PatchBrowserGuiGui::onSetStyle) );
/*
		initializePin( pinWriteable, static_cast<MpGuiBaseMemberPtr2>(&PatchBrowserGuiGui::onSetWriteable) );
		initializePin( pinGreyed, static_cast<MpGuiBaseMemberPtr2>(&PatchBrowserGuiGui::onSetGreyed) );
		initializePin( pinHint, static_cast<MpGuiBaseMemberPtr2>(&PatchBrowserGuiGui::onSetHint) );
		initializePin( pinMenuItems, static_cast<MpGuiBaseMemberPtr2>(&PatchBrowserGuiGui::onSetMenuItems) );
		initializePin( pinMenuSelection, static_cast<MpGuiBaseMemberPtr2>(&PatchBrowserGuiGui::onSetMenuSelection) );
		initializePin( pinHeading, static_cast<MpGuiBaseMemberPtr2>(&PatchBrowserGuiGui::onSetHeading) );
		initializePin( pinMomentary, static_cast<MpGuiBaseMemberPtr2>(&PatchBrowserGuiGui::onSetMomentary) );
		initializePin( pinEnableSpecialStrings, static_cast<MpGuiBaseMemberPtr2>(&PatchBrowserGuiGui::onSetEnableSpecialStrings) );
*/
	}

	int32_t MP_STDCALL arrange(GmpiDrawing_API::MP1_RECT finalRect) override
	{
		auto r = gmpi_gui::MpGuiGfxBase::arrange(finalRect);


		return r;
	}

	int32_t MP_STDCALL OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext ) override
	{
		Graphics g(drawingContext);
		auto r = getRect();

		auto textFormat = GetTextFormat(pinStyle);
		auto brush = g.CreateSolidColorBrush(GetFontMetatdata(pinStyle)->getColor());
		auto directXOffset = GetFontMetatdata(pinStyle)->getLegacyVerticalOffset();

		GmpiDrawing_API::MP1_FONT_METRICS fontMetrics;
		textFormat.GetFontMetrics(&fontMetrics);
		auto rowheight = fontMetrics.lineGap + fontMetrics.ascent + fontMetrics.descent;

		Rect clipRect{0, 0, r.getWidth() * 0.5f, r.getHeight()};
		g.PushAxisAlignedClip(clipRect);

		Rect textRect{0.0f, 0.0f, clipRect.getWidth(), rowheight };
		for (auto& category : categories)
		{
			if (textRect.bottom > r.getHeight())
				break;

			g.DrawTextU(category, textFormat, textRect, brush);

			textRect.Offset(0.0f, rowheight);
		}

		clipRect.left = clipRect.right;
		clipRect.right = r.getWidth();

		g.PopAxisAlignedClip();
		g.PushAxisAlignedClip(clipRect);

		textRect.Offset(textRect.right , -textRect.top);

		for (auto& preset : presets)
		{
			if (textRect.bottom > r.getHeight())
				break;

			g.DrawTextU(preset.name, textFormat, textRect, brush);
			textRect.Offset(0.0f, rowheight);
		}

		g.PopAxisAlignedClip();

		return gmpi::MP_OK;
	}
};

/*
namespace
{
	auto r = Register<PatchBrowserGuiGui>::withId(L"SE Patch Browser GUI");
}

  <Plugin id="SE Patch Browser GUI" name="Patch Browser GUI" category="Experimental">
	<GUI graphicsApi="GmpiGui">
	  <Pin name="Choice" datatype="int" />
	  <Pin name="Item List" datatype="string" />
	  <Pin name="Style" datatype="string" default="normal" />
	  <!--
	  <Pin name="Writeable" datatype="bool" default="1" />
	  <Pin name="Greyed" datatype="bool" />
	  <Pin name="Hint" datatype="string" />
	  <Pin name="Menu Items" datatype="string" />
	  <Pin name="Menu Selection" datatype="int" />
	  <Pin name="Heading" datatype="string" default="Menu" />
	  <Pin name="Momentary" datatype="bool" />
	  <Pin name="Enable Special Strings" datatype="bool" default="1" isMinimised="true" />
	  -->
	</GUI>
  </Plugin>
*/