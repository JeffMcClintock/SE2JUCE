#pragma once

/*
#include "TextEditWidget.h"
*/

#include "Widget.h"
#include "../se_sdk3/mp_sdk_gui2.h"
#include "../shared/ImageMetadata.h"
#include "../shared/FontCache.h"
#include "../se_sdk3/mp_gui.h"

class TextEditWidget : public TextWidgetBase
{
	std::string text;
	gmpi_gui::MpCompletionHandler onPopupMenuCompleteEvent;
	GmpiGui::TextEdit nativeWidget;

public:
	std::function<void(std::string)> OnChangedEvent;

	TextEditWidget(gmpi::IMpUnknown* host = nullptr) :
		onPopupMenuCompleteEvent([this](int32_t result) -> void { this->OnPopupmenuComplete(result); })
	{
		if (host)
			setHost(host);
	}

	TextEditWidget(gmpi::IMpUnknown* host, const char* style, const char* text = "") :
		onPopupMenuCompleteEvent([this](int32_t result) -> void { this->OnPopupmenuComplete(result); })
	{
		setHost(host);
		SetText(text);
		Init(style);
	}

	virtual void OnRender(GmpiDrawing::Graphics& dc) override;
	void Init(const char* style);
	void SetText(std::string utf8String)
	{
		dirty |= utf8String != text;
		text = utf8String;
	}
	const std::string& GetText()
	{
		return text;
	}
	virtual GmpiDrawing::Size getSize() override;
/* must not change on shared textformat
	void setCentered()
	{
		alignment = GmpiDrawing::TextAlignment::Center;
	}
	void setLeftAligned()
	{
		alignment = GmpiDrawing::TextAlignment::Leading;
	}
	void setRightAligned()
	{
		alignment = GmpiDrawing::TextAlignment::Trailing;
	}
*/
	virtual bool onPointerDown(int32_t flags, GmpiDrawing_API::MP1_POINT point) override;
	virtual void onPointerUp(int32_t flags, GmpiDrawing_API::MP1_POINT point) override;
	void OnPopupmenuComplete(int32_t result);
};

