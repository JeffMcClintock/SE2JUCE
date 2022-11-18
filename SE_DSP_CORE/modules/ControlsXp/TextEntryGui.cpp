#include "./TextEntryGui.h"
#include <memory>
#include "Drawing.h"
#include "../sharedLegacyWidgets/TextWidget.h"
#include "../sharedLegacyWidgets/TextEditWidget.h"
#include "../sharedLegacyWidgets/BitmapWidget.h"

using namespace std;
using namespace gmpi;
using namespace GmpiDrawing;

GMPI_REGISTER_GUI(MP_SUB_TYPE_GUI2, TextEntryGui, L"SE Text Entry" );

TextEntryGui::TextEntryGui() :
	browseButtonState(0)
{
	// initialise pins.
	initializePin( 9, pinpatchValue, static_cast<MpGuiBaseMemberPtr2>(&TextEntryGui::onSetpatchValue) );
	initializePin(pinExtension, static_cast<MpGuiBaseMemberPtr2>(&TextEntryGui::onSetExtension));
	initializePin( pinHint );
	initializePin(pinTitle, static_cast<MpGuiBaseMemberPtr2>(&TextEntryGui::onSetTitle));
}

void TextEntryGui::OnWidgetUpdate(const std::string& newvalue)
{
	pinpatchValue = newvalue;
	onSetpatchValue();
}

void TextEntryGui::OnPopupmenuComplete(int32_t result)
{
	if (result == gmpi::MP_OK)
	{
		OnWidgetUpdate(nativeFileDialog.GetSelectedFilename());
	}

	nativeFileDialog.setNull(); // release it.
}

void TextEntryGui::OnBrowseButton(float newvalue)
{
	if (browseButtonState == 1 && newvalue == 0)
	{
		GmpiGui::GraphicsHost host(getGuiHost());
		nativeFileDialog = host.createFileDialog();

		nativeFileDialog.AddExtensionList(pinExtension);
		nativeFileDialog.SetInitialFullPath(pinpatchValue);

		nativeFileDialog.ShowAsync( [this](int32_t result) -> void { this->OnPopupmenuComplete(result); } );
	}

	browseButtonState = newvalue;
}

// handle pin updates.
void TextEntryGui::onSetpatchValue()
{
	if (!widgets.empty())
	{
		auto textbox = dynamic_cast<TextEditWidget*>(widgets[0].get());

		textbox->SetText(pinpatchValue);
		
		if (textbox->ClearDirty())
			invalidateRect();
	}
}

void TextEntryGui::onSetExtension()
{
	widgets.clear();

	auto w = make_shared<TextEditWidget>(getHost(), "control_edit");
	w->OnChangedEvent = [this](const std::string& newvalue) -> void { this->OnWidgetUpdate(newvalue); };

	widgets.push_back(w);

	if (!pinExtension.getValue().empty())
	{
		auto bm = make_shared<BitmapWidget>(getHost(), "browse");
		bm->OnChangedEvent = [this](float newvalue) -> void { this->OnBrowseButton(newvalue); };

		widgets.push_back(bm);
	}

	auto title = (std::string) pinTitle;
	auto tw = make_shared<TextWidget>(getHost(), "control_label", title.c_str(), !useBackwardCompatibleArrangement());
	widgets.push_back(tw);

	onSetpatchValue();

	getGuiHost()->invalidateMeasure();
}

int32_t TextEntryGui::measure(GmpiDrawing_API::MP1_SIZE availableSize, GmpiDrawing_API::MP1_SIZE* returnDesiredSize)
{
	auto s = widgets[0]->getSize();

	returnDesiredSize->width = (std::max)(s.width, availableSize.width);
	returnDesiredSize->height = s.height;// +8; // for border.

	FontMetadata* returnMetadata;
	FontCache::instance()->GetTextFormat(getHost(), getGuiHost(), "control_label", &returnMetadata);
	returnDesiredSize->height += returnMetadata->pixelHeight_;

	return gmpi::MP_OK;
}

int32_t TextEntryGui::arrange(GmpiDrawing_API::MP1_RECT finalRect_s)
{
	Rect finalRect(finalRect_s);

	MpGuiGfxBase::arrange(finalRect);

	float y = 0;

	// Header.
	{
		auto s = widgets.back()->getSize();
		// At top.
		widgets.back()->setPosition(Rect(finalRect.left, y, finalRect.right, y + s.height));

		y += s.height;
	}

	float remainingHeight = widgets[0]->getSize().height;
	if (widgets.size() > 2) // Text Widget. Shorten if Browse button.
	{
		remainingHeight = (std::max)(remainingHeight, widgets[1]->getSize().height);
	}

	Rect remainingRect(finalRect_s);
	remainingRect.top = y;

	if (widgets.size() > 2) // Text Widget. Shorten if Browse button.
	{
		remainingRect.right -= widgets[1]->getSize().width;
	}

	// Centered in remaining vertical space.
	remainingRect.top = y + floorf((remainingHeight - widgets[0]->getSize().height) * 0.5f);
	remainingRect.bottom = remainingRect.top + widgets[0]->getSize().height;
	widgets[0]->setPosition(remainingRect);

	if (widgets.size() > 2) // browse button.
	{
		remainingRect.left = remainingRect.right;
		remainingRect.right += widgets[1]->getSize().width;

		remainingRect.top = y + floorf((remainingHeight - widgets[1]->getSize().height) * 0.5f);
		remainingRect.bottom = remainingRect.top + widgets[1]->getSize().height;

		widgets[1]->setPosition(remainingRect);
	}

	return gmpi::MP_OK;
}