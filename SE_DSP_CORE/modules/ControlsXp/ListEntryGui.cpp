#include "./ListEntryGui.h"
#include <algorithm>
#define _USE_MATH_DEFINES
#include <math.h>
#include "../se_sdk3/it_enum_list.h"
#include "../shared/unicode_conversion.h"
#include "../sharedLegacyWidgets/TextWidget.h"
#include "../sharedLegacyWidgets/ListWidget.h"
#include "../sharedLegacyWidgets/BitmapWidget.h"
#include "../sharedLegacyWidgets/RotarySwitchWidget.h"

using namespace gmpi_sdk;

using namespace std;
using namespace gmpi;
using namespace gmpi_gui;
using namespace JmUnicodeConversions;
using namespace GmpiDrawing;
using namespace GmpiDrawing_API;

GMPI_REGISTER_GUI(MP_SUB_TYPE_GUI2, ListEntryGui, L"SE List Entry" );
SE_DECLARE_INIT_STATIC_FILE(ListEntry_Gui);

ListEntryGui::ListEntryGui() :
 isarranged(false)
#ifdef _DEBUG
, ismeasured(false)
#endif
, listEntryCount(0)
, currentAppearance(-2)
{
	// initialise pins.
	initializePin( 10, pinValueIn, static_cast<MpGuiBaseMemberPtr2>(&ListEntryGui::onSetValueIn));
	initializePin(pinHint, static_cast<MpGuiBaseMemberPtr2>(&ListEntryGui::onSetValueIn));
	initializePin(pinItemList, static_cast<MpGuiBaseMemberPtr2>(&ListEntryGui::onSetItems));
	initializePin(pinAppearance, static_cast<MpGuiBaseMemberPtr2>(&ListEntryGui::onSetAppearance));
	initializePin(pinTitle, static_cast<MpGuiBaseMemberPtr2>(&ListEntryGui::onSetTitle));

	initializePin(pinMenuItems);
	initializePin(pinMenuSelection);
}

void ListEntryGui::OnWidgetUpdate(int32_t newvalue)
{
	pinValueIn = newvalue;
	onSetValueIn();
}

void ListEntryGui::onSetAppearance()
{
	if (currentAppearance == pinAppearance && (currentAppearance == ACM_PLAIN || currentAppearance == ACM_BUTTON_SELECTOR || currentAppearance == ACM_UP_DOWN_SELECTOR))
		return;

	currentAppearance = pinAppearance;

	// Reset defaults.
	const char* imageFile = nullptr;
	
	switch (pinAppearance)
	{
	case ACM_LED_STACK:	// LED Stack
	case ACM_LED_STACK_LABELED: // LED stack (labled)
	case ACM_BUTTON_SELECTOR:
		imageFile = "button_sm";
		break;

	case ACM_ROTARY_SWITCH_LABELED:	// rotary switch
	case ACM_ROTARY_SWITCH:	// rotary switch (no text)
		imageFile = "switch_rotary";
		break;
	}

	widgets.clear();

	if (imageFile && imageFile[0] != 0) // not empty?
	{
		if (pinAppearance == ACM_ROTARY_SWITCH_LABELED || pinAppearance == ACM_ROTARY_SWITCH)
		{
			auto bm = make_shared<RotarySwitchWidget>(getHost(), imageFile);
			widgets.push_back(bm);
			bm->init(pinItemList, pinAppearance == ACM_ROTARY_SWITCH_LABELED);
			bm->OnChangedEvent = [this](int32_t newvalue) -> void { this->OnWidgetUpdate(newvalue); };
		}
		else
		{
			auto bm = make_shared<BitmapWidget>(getHost(), imageFile);
			widgets.push_back(bm);

			if (pinAppearance == ACM_LED_STACK || pinAppearance == ACM_LED_STACK_LABELED || pinAppearance == ACM_BUTTON_SELECTOR)
			{
				//bm->SetToggle(false);
				bm->toggleMode2 = skinBitmap::ToggleMode::Momentary;
			}
		}
	}

	switch (pinAppearance)
	{
	case ACM_PLAIN:
	{
		auto w = make_shared<ListWidget>(getHost(), "control_edit");
		w->OnChangedEvent = [this](int32_t newvalue) -> void { this->OnWidgetUpdate(newvalue); };

		widgets.push_back(w);
		onSetValueIn();
		onSetItems();
	}
		break;

	case ACM_UP_DOWN_SELECTOR:
	{
		shared_ptr<BitmapWidget> bm = make_shared<BitmapWidget>(getHost(), "arrow_left");
//		bm->SetToggle(false);
		widgets.push_back(bm);

		bm = make_shared<BitmapWidget>(getHost(), "arrow_right");
//		bm->SetToggle(false);
		widgets.push_back(bm);
	}
	// deliberate fall-thru.
	case ACM_BUTTON_SELECTOR:
	{
		widgets.push_back(make_shared<TextWidget>(getHost(), "selector_text"));
	}
	break;

	case ACM_LED_STACK:
	case ACM_LED_STACK_LABELED:
	{
		int ledCount = (std::max)(1, listEntryCount);
		for (int i = 0; i < ledCount; ++i)
		{
			widgets.push_back(make_shared<BitmapWidget>(getHost(), "led"));
		}

		if (ACM_LED_STACK_LABELED == pinAppearance)
		{
			// Labels.
			const char* style = "selector_text";
			it_enum_list itr(pinItemList);
			if (itr.size() == 0)
			{
				widgets.push_back(make_shared<TextWidget>(getHost(), style, ""));
			}
			else
			{
				for (itr.First(); !itr.IsDone(); ++itr)
				{
					widgets.push_back(make_shared<TextWidget>(getHost(), style, WStringToUtf8((*itr)->text).c_str()));
				}
			}
		}
	}
	break;

	case ACM_BUTTON_STACK:
	{
		it_enum_list itr(pinItemList);

		// Buttons.
		for (itr.First(); !itr.IsDone(); ++itr)
		{
			auto bm2 = make_shared<BitmapWidget>(getHost(), "button_sm");
			widgets.push_back(bm2);
			bm2->toggleMode2 = skinBitmap::ToggleMode::OnOnly;
		}

		// Labels.
		const char* style = "selector_text";
		if (itr.size() == 0)
		{
			widgets.push_back(make_shared<TextWidget>(getHost(), style, ""));
		}
		else
		{
			for (itr.First(); !itr.IsDone(); ++itr)
			{
				widgets.push_back(make_shared<TextWidget>(getHost(), style, WStringToUtf8((*itr)->text).c_str()));
			}
		}
	}
	break;
	}

	auto title = (std::string) pinTitle;
	auto tw = make_shared<TextWidget>(getHost(), "control_label", title.c_str(), !useBackwardCompatibleArrangement());
	widgets.push_back(tw);

	onSetValueIn();

	if( isarranged )
		getGuiHost()->invalidateMeasure();
}

void ListEntryGui::onSetItems()
{
	it_enum_list itr(pinItemList);
	listEntryCount = itr.size();

	if (pinAppearance == ACM_PLAIN )
	{
		if (!widgets.empty())
			dynamic_cast<ListWidget*> (widgets[0].get())->SetItems(pinItemList);
		return;
	}
	
	if (pinAppearance == ACM_BUTTON_SELECTOR || pinAppearance == ACM_PLAIN)
	{
		onSetValueIn();
	}
	else
	{
		onSetAppearance();
	}
}

void ListEntryGui::onSetValueIn()
{
	int selectedIndex = 0;
	it_enum_list itr(pinItemList);
	itr.FindValue(pinValueIn);

	if (!itr.IsDone())
	{
		selectedIndex = (*itr)->index;
	}

	switch (pinAppearance)
	{
	case ACM_PLAIN:
		if (!widgets.empty())
			dynamic_cast<ListWidget*> (widgets[0].get())->SetValue(pinValueIn);
		break;

	case ACM_BUTTON_STACK:
		for (int i = 0; i < listEntryCount; ++i)
		{
			dynamic_cast<BitmapWidget*> (widgets[i].get())->SetNormalised(i == selectedIndex ? 1.0f : 0.0f);
		}
		break;

	case ACM_LED_STACK:
	case ACM_LED_STACK_LABELED:
		for (int i = 0; i < listEntryCount; ++i)
		{
			dynamic_cast<BitmapWidget*> (widgets[i + 1].get())->SetNormalised(i == selectedIndex ? 1.0f : 0.0f);
		}
		break;

	case ACM_BUTTON_SELECTOR:
	case ACM_UP_DOWN_SELECTOR:
	{
		if (!itr.IsDone())
		{
			auto last = widgets.size() - 2; // last (not counting header widget)
			dynamic_cast<TextWidget*>(widgets[last].get())->SetText(WStringToUtf8(itr.CurrentItem()->text).c_str());
		}
	}
	break;

	case ACM_ROTARY_SWITCH:
	case ACM_ROTARY_SWITCH_LABELED:
	{
		dynamic_cast<RotarySwitchWidget*>(widgets[0].get())->setValueInt(pinValueIn);
	}
	break;
	};

	bool invalidate = false;

	for (auto& w : widgets)
		invalidate |= w->ClearDirty();

	if (invalidate)
		invalidateRect();
}

int32_t ListEntryGui::onPointerUp(int32_t flags, GmpiDrawing_API::MP1_POINT point)
{
	if (!getCapture())
	{
		return gmpi::MP_UNHANDLED;
	}

	releaseCapture();

	if( captureWidget )
	{
		it_enum_list itr(pinItemList);
		int itemCount = itr.size();

		captureWidget->onPointerUp(flags, point);

		if ((pinAppearance == ACM_BUTTON_SELECTOR || pinAppearance == ACM_LED_STACK || pinAppearance == ACM_LED_STACK_LABELED)
			&& captureWidget == widgets[0].get()) // Clicked increment button.
		{
			Increment();
		}

		if (pinAppearance == ACM_UP_DOWN_SELECTOR) // Clicked increment button.
		{
			if(captureWidget == widgets[1].get())
				Increment();
			else
				Increment(-1);
		}

		if (pinAppearance == ACM_BUTTON_STACK)
		{
			auto bm = dynamic_cast<BitmapWidget*> (captureWidget);
			if (bm && bm->GetNormalised() > 0.0f)
			{
				for (int i = 0; i < (int)widgets.size() / 2; ++i)
				{
					if (widgets[i].get() == bm)
					{
						it_enum_list itr(pinItemList);
						if (itr.FindIndex(i))
						{
							// pinValueIn = (*itr)->value;
							OnWidgetUpdate((*itr)->value);
						}
					}
					else
					{
						dynamic_cast<BitmapWidget*> (widgets[i].get())->SetNormalised(0.0f);
					}
				}
			}
		}

		if (pinAppearance == ACM_LED_STACK || pinAppearance == ACM_LED_STACK_LABELED)
		{
			onSetValueIn();
		}

		for( auto& w : widgets )
		{
			if( w->ClearDirty() )
			{
				invalidateRect();
				break;
			}
		}
	}

	captureWidget = nullptr;

	return gmpi::MP_OK;
}

void ListEntryGui::Increment(int inc)
{
	it_enum_list itr(pinItemList);

	int idx = 0;
	if (itr.FindValue(pinValueIn))
	{
		idx = itr.CurrentItem()->index + inc;
		if (idx >= itr.size())
		{
			idx = 0;
		}
		else
		{
			if (idx < 0)
			{
				idx = itr.size() - 1;
			}
		}
	}

	if (itr.FindIndex(idx))
	{
		OnWidgetUpdate(itr.CurrentItem()->value);
	}
}

int32_t ListEntryGui::measure(GmpiDrawing_API::MP1_SIZE availableSize, GmpiDrawing_API::MP1_SIZE* returnDesiredSize)
{
	it_enum_list itr(pinItemList);

	switch (pinAppearance)
	{
		case ACM_ROTARY_SWITCH:
		case ACM_ROTARY_SWITCH_LABELED:
		{
			*returnDesiredSize = widgets[0]->getSize();
		}
		break;

		case ACM_BUTTON_SELECTOR:
		{
			// Width is sizeable. TODO buggy with inf.
			returnDesiredSize->width = (std::min)(4000.f, availableSize.width);

			auto s = widgets[0]->getSize();
			auto s2 = widgets[1]->getSize();

			auto totalHeight = (std::max)(s.height, s2.height);
			returnDesiredSize->height = totalHeight;

			auto textx = returnDesiredSize->width = s.width;

			auto ew = dynamic_cast<TextWidget*>(widgets[1].get());
			auto temp = ew->GetText();

			for (itr.First(); !itr.IsDone(); ++itr)
			{
				ew->SetText(WStringToUtf8((*itr)->text));
				returnDesiredSize->width = (std::max)(returnDesiredSize->width, textx + ew->getSize().width);
			}
			ew->SetText(temp);
		}
		break;

		case ACM_UP_DOWN_SELECTOR:
		{
			returnDesiredSize->height = 0;

			// left-to-right stack.
			float x = 0;
			for (int i = 0; i < 2; ++i)
			{
				auto s = widgets[i]->getSize();
				returnDesiredSize->height = (std::max)(returnDesiredSize->height, s.height);
				x += s.width;
			}

			// determine max text width.
			auto ew = dynamic_cast<TextWidget*>(widgets[2].get());
			auto temp = ew->GetText();
			returnDesiredSize->width = x;

			for (itr.First(); !itr.IsDone(); ++itr)
			{
				ew->SetText(WStringToUtf8((*itr)->text));
				auto s = ew->getSize();
				returnDesiredSize->width = (std::max)(returnDesiredSize->width, x + s.width);
				returnDesiredSize->height = (std::max)(returnDesiredSize->height, s.height);
			}
			ew->SetText(temp);
		}
		break;

		case ACM_LED_STACK:
		{
			int ledCount = (std::max)(1, listEntryCount);
			auto s = widgets[0]->getSize(); // Button.

			if (widgets.size() > 1)
			{
				// has LEDs.
				auto s2 = widgets[1]->getSize(); // LED.
				returnDesiredSize->width = s.width + s2.width;
				returnDesiredSize->height = (std::max)(s.height, s2.height * (widgets.size() - 1) + rowSpacing * (ledCount - 2));
			}
			else
			{
				// no LEDs.
				returnDesiredSize->width = s.width;
				returnDesiredSize->height = s.height;
			}
		}
		break;

		case ACM_LED_STACK_LABELED:
		{
			int ledCount = (std::max)(1, listEntryCount);

			// Button centered at left.
			auto sb = widgets[0]->getSize(); // Button.
			auto sl = widgets[1]->getSize(); // LED
			auto st = widgets[1 + ledCount]->getSize(); // Text

			float collumnHeight = (std::max)(sl.height, st.height);

			returnDesiredSize->width = sb.width;
			returnDesiredSize->height = sb.height;

			// LEDs stacked vertical
			// LED.
			returnDesiredSize->width += sl.width;
			returnDesiredSize->height = collumnHeight * ledCount + rowSpacing * (ledCount - 1);

			float textx = returnDesiredSize->width;
			// Text.
			for (int i = 0; i < ledCount; ++i)
			{
				float textWidth = ceil(widgets[1 + i + ledCount]->getSize().width);
				returnDesiredSize->width = (std::max)(returnDesiredSize->width, textx + textWidth + 2);
			}
		}
		break;

		case ACM_BUTTON_STACK:
		{
			GmpiDrawing::Size sb, s2;
			if(listEntryCount == 0) // odd case
			{
				// no button.
				s2 = widgets[0]->getSize(); // Text
			}
			else
			{
				sb = widgets[0]->getSize(); // Button.
				s2 = widgets[1]->getSize(); // Text
			}

			sb.height = ceil(sb.height);
			s2.height = ceil(s2.height);
			float dy = (std::max)(ceil(sb.height), ceil(s2.height)) + rowSpacing;
			returnDesiredSize->height = dy * listEntryCount;

			// stacked vertical
			float textx = returnDesiredSize->width = sb.width;
			// Text.
			for (int i = 0; i < listEntryCount; ++i)
			{
				float textWidth = ceil(widgets[i + listEntryCount]->getSize().width);
				returnDesiredSize->width = (std::max)(returnDesiredSize->width, textx + textWidth);
			}
		}
		break;

		default:
		case ACM_PLAIN:
		{
			auto s = widgets[0]->getSize();

			returnDesiredSize->width = (std::max)(s.width, availableSize.width);
			returnDesiredSize->height = s.height;// +8; // for border.
		}
		break;

	}

	FontMetadata* returnMetadata;
	FontCache::instance()->GetTextFormat( getHost(), getGuiHost(), "control_label", &returnMetadata);
	returnDesiredSize->height += returnMetadata->pixelHeight_; // FontCache::instance()->getOriginalPixelHeight("control_label"); // Header size

#ifdef _DEBUG
	ismeasured = true;
#endif

	return gmpi::MP_OK;
}

int32_t ListEntryGui::arrange(GmpiDrawing_API::MP1_RECT finalRect_s)
{
	Rect finalRect(finalRect_s);
	//	_RPT0(_CRT_WARN, "ListEntryGui::arrange\n");
	MpGuiGfxBase::arrange(finalRect);

#ifdef _DEBUG
	assert(ismeasured);
#endif
	isarranged = true;

	float y = 0;

	// Header.
	{
		auto s = widgets.back()->getSize();
		// At top.
		widgets.back()->setPosition(Rect(finalRect.left, y, finalRect.right, y + s.height));

		y += s.height;
	}

	Rect remainingRect(finalRect_s);
	remainingRect.top = y;

	switch( pinAppearance )
	{
	case ACM_ROTARY_SWITCH:
	case ACM_ROTARY_SWITCH_LABELED:
	{
		// Position knob bitmap.
		auto s = widgets[0]->getSize();
		Rect r(0, 0, s.width, s.height);
		r.Offset(0, y);

		widgets[0]->setPosition(r);
	}
	break;

	case ACM_BUTTON_SELECTOR:
	{
		// Button.
		auto s = widgets[0]->getSize();
		auto headerSize = widgets.back()->getSize();

		auto totalHeight = (std::max)(s.height,headerSize.height);

		float y = remainingRect.top + (totalHeight - s.height) / 2;
		widgets[0]->setPosition(Rect(0, y, s.width, y + s.height));

		// Text
		y = remainingRect.top + (totalHeight - headerSize.height) / 2;
		widgets[1]->setPosition(Rect(s.width, y, remainingRect.right, y + headerSize.height));
	}
	break;

	case ACM_UP_DOWN_SELECTOR:
	{
		// left-to-right stack.
		float x = 0;
		for (int i = 0 ; i < 3 ;++i)
		{
			auto& w = widgets[i];
			auto s2 = w->getSize();
			y = remainingRect.top + (remainingRect.getHeight() - s2.height) / 2;
			float right = i == 2 ? finalRect.getWidth() : x + s2.width; // last widget stretches to fill.
			w->setPosition(Rect(x, y, right, y + s2.height));
			x += s2.width;
		}
	}
	break;

	case ACM_LED_STACK:
	case ACM_LED_STACK_LABELED:
	{
		int ledCount = (std::max)(1, listEntryCount);

		// Button centered at left.
		auto sb = widgets[0]->getSize(); // Button.
		auto sl = widgets[1]->getSize(); // LED
		Size st;

		if(ACM_LED_STACK_LABELED == pinAppearance)
			st = widgets[1 + ledCount]->getSize(); // Text

		float collumnHeight = (std::max)(sl.height, st.height);

		float totalHeight = collumnHeight * ledCount + rowSpacing * (ledCount - 1);

		auto yb = y + (totalHeight - sb.height) / 2;
		widgets[0]->setPosition(Rect(0, yb, sb.width, yb + sb.height)); // Button Centered.

		// LEDs stacked vertical
		float xl = sb.width;
		float xt = xl + sl.width;
		for (int i = 0; i < ledCount; ++i)
		{
			float centeredY = y + (collumnHeight - sl.height) * 0.5f;
			widgets[1 + i]->setPosition(Rect(xl, centeredY, xl + sl.width, centeredY + sl.height));
			centeredY = y + (collumnHeight - st.height) * 0.5f;
			if(useBackwardCompatibleArrangement())
			{
				centeredY -= 2.0f; // -2 is hack to raise text a little.
			}

			if (ACM_LED_STACK_LABELED == pinAppearance)
				widgets[1 + i + ledCount]->setPosition(Rect(xt, centeredY, finalRect.getWidth(), centeredY + st.height)); // text.

			y += collumnHeight + rowSpacing;
		}
	}
	break;

	case ACM_BUTTON_STACK:
	{
		int itemCount = listEntryCount;
		GmpiDrawing::Size s, s2;
		if(listEntryCount == 0) // odd case
		{
			// no button.
			s2 = widgets[0]->getSize(); // Text
			widgets[0]->setPosition(Rect(0, 0, s2.width,s2.height));
		}
		else
		{
			s = widgets[0]->getSize(); // Button.
			s2 = widgets[1]->getSize(); // Text
		}

		s.height = ceil(s.height);
		s2.height = ceil(s2.height);
		float x = 0;
		float x2 = s.width;
		int i = 0;
		float dy = (std::max)(ceil(s.height), ceil(s2.height)) + rowSpacing;
		float y1;
		if(useBackwardCompatibleArrangement())
		{
			y1 = y; // Buttons align to top.
		}
		else
		{
			y1 = y + floor(0.5f + (dy - s.height) * 0.5f); // Buttons align to vertical center
		}

		float y2 = y + floor(0.5f + (dy - s2.height) * 0.5f); // center
		for (int i = 0; i < itemCount; ++i )
		{
			widgets[i]->setPosition(Rect(x, y1, x + s.width, y1 + s.height));
			widgets[i+ itemCount]->setPosition(Rect(x2, y2, finalRect.getWidth() + 1, y2 + s2.height));
			y1 += dy;
			y2 += dy;
		}
	}
	break;

	case ACM_PLAIN:
		if(!widgets.empty())
			widgets[0]->setPosition(remainingRect);
		break;

	default:
	break;
	}
	
	return gmpi::MP_OK;
}

int32_t ListEntryGui::initialize()
{
	auto r = ClassicControlGuiBase::initialize(); // ensure all pins initialised (so widgets are built).

	onSetValueIn();

	return r;
}

int32_t ListEntryGui::populateContextMenu(float x, float y, gmpi::IMpUnknown* contextMenuItemsSink)
{
	// TODO: hit-test.

	gmpi::IMpContextItemSink* sink;
	contextMenuItemsSink->queryInterface(gmpi::MP_IID_CONTEXT_ITEMS_SINK, reinterpret_cast<void**>(&sink));

	it_enum_list itr(pinMenuItems);

	for (itr.First(); !itr.IsDone(); ++itr)
	{
		int32_t flags = 0;

		// Special commands (sub-menus)
		switch (itr.CurrentItem()->getType())
		{
			case enum_entry_type::Separator:
			case enum_entry_type::SubMenu:
				flags |= gmpi_gui::MP_PLATFORM_MENU_SEPARATOR;
				break;

			case enum_entry_type::SubMenuEnd:
			case enum_entry_type::Break:
				continue;

			default:
				break;
		}

		sink->AddItem(WStringToUtf8(itr.CurrentItem()->text).c_str(), itr.CurrentItem()->value, flags);
	}

	return gmpi::MP_OK;
}

int32_t ListEntryGui::onContextMenu(int32_t selection)
{
	pinMenuSelection = selection; // send menu momentarily, then reset.
	pinMenuSelection = -1;

	return gmpi::MP_OK;
}
