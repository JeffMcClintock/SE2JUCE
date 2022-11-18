#pragma once
#include <string>
#include "../se_sdk3/Drawing_API.h"
#include "../se_sdk3/mp_sdk_gui2.h"
#include "../se_sdk3/Drawing.h"
#include "Widget.h"

class EditWidget : public TextWidgetBase
{
	std::string text;
	bool readOnly;
	int m_visable_chars;
	gmpi_sdk::mp_shared_ptr<gmpi_gui::IMpPlatformText> nativeEdit;
	gmpi_gui::MpCompletionHandler onTextEntryCompeteEvent;
	static GmpiDrawing::Size default_text_size;

	void OnTextEnteredComplete(int32_t result);

public:
	std::function<void(const char*)> OnChangedEvent;

	EditWidget();

	virtual void OnRender(GmpiDrawing::Graphics& dc) override;
	void Init(const char* style, bool digitsOnly = false);
	void SetMinVisableChars(int chars)
	{
		m_visable_chars = chars;
	}
	void SetText(std::string utf8String)
	{
		dirty |= utf8String != text;
		text = utf8String;
	}
	void setReadOnly(bool pReadOnly = true)
	{
		readOnly = pReadOnly;
	}
	const std::string& GetText()
	{
		return text;
	}
	virtual GmpiDrawing::Size getSize() override;

	virtual bool onPointerDown(int32_t flags, GmpiDrawing_API::MP1_POINT point) override;
	virtual void onPointerUp(int32_t flags, GmpiDrawing_API::MP1_POINT point) override;
};

