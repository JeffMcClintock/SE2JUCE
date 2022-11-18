#include "./ClassicControlGuiBase.h"
#include "../sharedLegacyWidgets/TextWidget.h"

void ClassicControlGuiBase::onSetTitle()
{
	if (!widgets.empty())
	{
		auto header = dynamic_cast<TextWidget*>(widgets.back().get());

		header->SetText(pinTitle);

		if (header->ClearDirty())
			invalidateMeasure();
	}
}

int32_t ClassicControlGuiBase::initialize()
{
	auto r = gmpi_gui::MpGuiGfxBase::initialize(); // ensure all pins initialised (so widgets are built).

	dynamic_cast<TextWidget*>(widgets.back().get())->SetText(pinTitle);

	return r;
}

bool ClassicControlGuiBase::useBackwardCompatibleArrangement()
{
	// In SE 1.1 List-Entry had centered headings.
	// In 1.3 everything got left-justified by mistake.Rectify this, but only if (1.4) backward-compatibility switched off.
	if(backwardCompatibleVerticalArrange == -1)
	{
		FontMetadata* fontData{};
		FontCache::instance()->GetTextFormat(getHost(), getGuiHost(), "control_label", &fontData);

		backwardCompatibleVerticalArrange = (int) fontData->verticalSnapBackwardCompatibilityMode;
	}

	return backwardCompatibleVerticalArrange == 1;
}
