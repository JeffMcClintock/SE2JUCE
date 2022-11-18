#ifndef MIDIMONITORGUI_H_INCLUDED
#define MIDIMONITORGUI_H_INCLUDED

#include "Drawing.h"
#include "mp_sdk_gui2.h"
#include <list>
#include <vector>
#include <string>

class MidiMonitorGui : public gmpi_gui::MpGuiGfxBase
{
	std::list< std::string > messages;
	std::vector< std::string > CONTROLLER_DESCRIPTION;

	void MidiMonitorGui::init_controller_descriptions(void);

public:
	MidiMonitorGui();

	// overrides.
	virtual int32_t MP_STDCALL OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext ) override;
	virtual int32_t MP_STDCALL receiveMessageFromAudio(int32_t id, int32_t size, const void* messageData) override;
};

#endif


