#pragma once

#include "Drawing.h"
#include "mp_sdk_gui2.h"

class PatchcableDiagGui : public gmpi_gui::MpGuiGfxBase
{
public:
	PatchcableDiagGui();

	// overrides.
	virtual int32_t MP_STDCALL OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext ) override;

private:
 	void onSetPatchcableXml();
 	BlobGuiPin pinPatchCableXml;
};


