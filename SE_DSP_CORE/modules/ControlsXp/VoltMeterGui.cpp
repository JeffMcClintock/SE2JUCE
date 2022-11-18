#include "./VoltMeterGui.h"
#include <iomanip>
#include <sstream>
#include <memory>
#include "Drawing.h"
#include "../sharedLegacyWidgets/TextWidget.h"

using namespace std;
using namespace gmpi;
using namespace GmpiDrawing;

GMPI_REGISTER_GUI(MP_SUB_TYPE_GUI2, VoltMeterGui, L"SE Volt Meter" );
SE_DECLARE_INIT_STATIC_FILE(VoltMeter_Gui);

VoltMeterGui::VoltMeterGui()
{
	// initialise pins.
	initializePin(6, pinpatchValue, static_cast<MpGuiBaseMemberPtr2>(&VoltMeterGui::onSetpatchValue));
	initializePin(pinTitle, static_cast<MpGuiBaseMemberPtr2>(&VoltMeterGui::onSetTitle));
}

void VoltMeterGui::onSetTitle()
{
	auto w = make_shared<TextWidget>(getHost(), "control_edit");
	w->setBackGroundColor(Color::Black);

	widgets.push_back(w);

	auto title = (std::string) pinTitle;
	auto tw = make_shared<TextWidget>(getHost(), "control_label", title.c_str(), true);
	widgets.push_back(tw);

	onSetpatchValue();
}

// handle pin updates.
void VoltMeterGui::onSetpatchValue()
{
	if (!widgets.empty())
	{
		auto textbox = dynamic_cast<TextWidget*>(widgets[0].get());

		float v = pinpatchValue;
		// inhibit scientific notation.
		if (fabs(v) < 0.001f)
		{
			v = 0.0f;
		}

		std::stringstream oss;
		oss << std::setprecision(4) << v; // since no format specified. indicates 3 digits total. before or after point.

		textbox->SetText(oss.str());

		if (textbox->ClearDirty())
			invalidateRect();
	}
}

int32_t VoltMeterGui::measure(GmpiDrawing_API::MP1_SIZE availableSize, GmpiDrawing_API::MP1_SIZE* returnDesiredSize)
{
	auto s = widgets[0]->getSize();

	returnDesiredSize->width = (std::max)(s.width, availableSize.width);
	returnDesiredSize->height = s.height;// +8; // for border.

	FontMetadata* returnMetadata;
	FontCache::instance()->GetTextFormat(getHost(), getGuiHost(), "control_label", &returnMetadata);
	returnDesiredSize->height += returnMetadata->pixelHeight_;

	return gmpi::MP_OK;
}

int32_t VoltMeterGui::arrange(GmpiDrawing_API::MP1_RECT finalRect_s)
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

	Rect remainingRect(finalRect_s);
	remainingRect.top = y;

	if (widgets.size() > 2) // browse button?
	{
		remainingRect.right -= widgets[1]->getSize().width;
	}

	widgets[0]->setPosition(remainingRect);

	if (widgets.size() > 2) // browse button?
	{
		remainingRect.left = remainingRect.right;
		remainingRect.right += widgets[1]->getSize().width;

		widgets[1]->setPosition(remainingRect);
	}

	return gmpi::MP_OK;
}

