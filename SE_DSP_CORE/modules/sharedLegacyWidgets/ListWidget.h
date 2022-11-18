#pragma once

/*
#include "ListWidget.h"
*/

#include "Widget.h"
#include "../se_sdk3/Drawing_API.h"
#include "../shared/ImageMetadata.h"
#include "../shared/FontCache.h"
#include "../se_sdk3/mp_gui.h"

class ListWidget : public TextWidgetBase
{
	std::wstring items;
	int currentValue;
	gmpi_gui::MpCompletionHandler onPopupMenuCompleteEvent;
	GmpiGui::PopupMenu nativeFileDialog;

public:
	std::function<void(int32_t)> OnChangedEvent;

	ListWidget() :
		currentValue(0)
		, onPopupMenuCompleteEvent([this](int32_t result) -> void { this->OnPopupmenuComplete(result); })
	{}

	ListWidget(gmpi::IMpUnknown* host, const char* style, const wchar_t* items = L"") :
		onPopupMenuCompleteEvent([this](int32_t result) -> void { this->OnPopupmenuComplete(result); })
	{
		setHost(host);
		SetItems(items);
		Init(style);
	}

	virtual void OnRender(GmpiDrawing::Graphics& dc) override;
	void Init(const char* style);
	void SetItems(std::wstring utf8String)
	{
		dirty |= utf8String != items;
		items = utf8String;
	}
	int GetValue()
	{
		return currentValue;
	}

	void SetValue(int v)
	{
		dirty |= currentValue != v;
		currentValue = v;
	}

	virtual GmpiDrawing::Size getSize() override;
	virtual bool onPointerDown(int32_t flags, GmpiDrawing_API::MP1_POINT point) override;
	virtual void onPointerUp(int32_t flags, GmpiDrawing_API::MP1_POINT point) override;
	void OnPopupmenuComplete(int32_t result);
};

