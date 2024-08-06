#include "Widget.h"

Widget::Widget() :
 dirty(true)
 , guiHost_(nullptr)
 , patchMemoryHost_(nullptr)
{
}

void TextWidgetBase::InitTextFormat(const char* base_style, bool digitsOnly, bool centered, float overideCellHeight)
{
	const auto paragraphAlignment = GmpiDrawing::ParagraphAlignment::Center;

	dtextFormat = FontCache::instance()->GetCustomTextFormat(
		getHost(),
		getGuiHost(),
		customStyleName(base_style, centered, paragraphAlignment),
		base_style,
		[=](FontMetadata* customFont) -> void
			{
				if (!customFont->verticalSnapBackwardCompatibilityMode)
				{
					if(overideCellHeight > 0.0f)
					{
						customFont->bodyHeight_ = overideCellHeight;
					}
					else
					{
						// Calculate text height to fit nicely in cell. Based on local font-metrics.
						const float cellHeight = static_cast<float>(customFont->pixelHeight_);
						const float padding = cellHeight < 16.0f ? 0.0f : 0.5f; // 1 DIP, on small text 0.5 DIPs
						const float bodyHeight = cellHeight - padding;

						customFont->bodyHeight_ = bodyHeight;
					}

					customFont->bodyHeightDigitsOnly_ = digitsOnly;
					customFont->vst3_vertical_offset_ = 0;
				}

				if (centered)
				{
					customFont->setTextAlignment(GmpiDrawing::TextAlignment::Center);
				}
				else
				{
					customFont->setTextAlignment(GmpiDrawing::TextAlignment::Leading);
				}
				customFont->wordWrapping = GmpiDrawing::WordWrapping::NoWrap;
				customFont->paragraphAlignment = paragraphAlignment;
			},
		&typeface_
	);
}

void TextWidgetBase::VerticalCenterText(bool digitsOnly, float borderShrink)
{
	const auto widgetHeight = (float)typeface_->pixelHeight_;

	const auto boundingBoxSize = dtextFormat.GetTextExtentU("A");
	GmpiDrawing_API::MP1_FONT_METRICS fontMetrics;
	dtextFormat.GetFontMetrics(&fontMetrics);

	const auto topPadding = boundingBoxSize.height - fontMetrics.bodyHeight(); // Will be Zero in latest SE, but non-zero on legacy SE.

	// EditWidget
	const float boxHeight = widgetHeight - 2.0f * borderShrink;
	const float boxX = borderShrink;
	if (digitsOnly)
	{
		const float textHeight = fontMetrics.capHeight;
		const float totalTopPadding = topPadding + (fontMetrics.ascent - fontMetrics.capHeight); // Digits leave a gap between cap height and ascent.
		textY = 0.5f * (boxHeight - textHeight) - totalTopPadding;
	}
	else
	{
		const float textHeight = fontMetrics.bodyHeight();
		textY = 0.5f * (boxHeight - textHeight) - topPadding;
	}

	textY += boxX;
	textHeight = boundingBoxSize.height;
}