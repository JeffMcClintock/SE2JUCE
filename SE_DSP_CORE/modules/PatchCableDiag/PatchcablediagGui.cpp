#include "./PatchcablediagGui.h"
#include "Drawing.h"
#include "../shared/PatchCables.h"
#include "../shared/RawView.h"

using namespace GmpiDrawing;
using namespace gmpi;
GMPI_REGISTER_GUI(MP_SUB_TYPE_GUI2, PatchcableDiagGui, L"SE PatchCableDiag" );

PatchcableDiagGui::PatchcableDiagGui()
{
	// initialise pins.
	initializePin( pinPatchCableXml, static_cast<MpGuiBaseMemberPtr2>(&PatchcableDiagGui::onSetPatchcableXml) );
}

// handle pin updates.
void PatchcableDiagGui::onSetPatchcableXml()
{
	// get the XML describing the patch cables
	RawView raw(pinPatchCableXml.rawData(), (size_t) pinPatchCableXml.rawSize());

#if 0
	// HOW TO MODIFY PATCH CABLES
	{
		// Convert to a handy vector
		SynthEdit2::PatchCables pc(raw);

		// insert or erase any patch-cables you want here.
		// pc.push_back(, , , );
		// pc.cables.erase(it);

		// Save modifications back into XML, update patch-pin
		auto modifiedRaw = pc.Serialise();
		MpBlob newValue(modifiedRaw.size(), modifiedRaw.data());
		pinPatchCableXml = newValue;
	}
#endif

	invalidateRect();
}

int32_t PatchcableDiagGui::OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext )
{
	Graphics g(drawingContext);

	auto textFormat = GetGraphicsFactory().CreateTextFormat();
	auto brush = g.CreateSolidColorBrush(Color::Black);

	auto blob = pinPatchCableXml.getValue();
	if (blob.getSize() > 0)
	{
		std::string xml((const char*)blob.getData(), (size_t) blob.getSize());
		g.DrawTextU(xml, textFormat, getRect(), brush);
	}

	return gmpi::MP_OK;
}

