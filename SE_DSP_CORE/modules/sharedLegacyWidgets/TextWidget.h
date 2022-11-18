#pragma once

/*
#include "TextWidget.h"
*/

#include "Widget.h"
#include "../se_sdk3/mp_sdk_gui2.h"
#include "../shared/ImageMetadata.h"
#include "../shared/FontCache.h"

class TextWidget : public TextWidgetBase
{
	std::string text;
	GmpiDrawing_API::MP1_COLOR backgroundColor;

public:
	TextWidget(gmpi::IMpUnknown* host = nullptr)
	{
		if (host)
			setHost(host);
	}

	TextWidget(gmpi::IMpUnknown* host, const char* style, const char* text = "", bool centered = false)
	{
		setHost(host);
		SetText(text);
		Init(style, centered);
	}

	virtual void OnRender(GmpiDrawing::Graphics& dc) override;
	void Init(const char* style, bool centered);
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

	void setBackGroundColor(GmpiDrawing::Color c)
	{
		backgroundColor = c;
	}
};

