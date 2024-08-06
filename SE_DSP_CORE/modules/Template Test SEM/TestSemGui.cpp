#include "./TestSemGui.h"
#include "Drawing.h"

using namespace gmpi;
using namespace GmpiDrawing;

GMPI_REGISTER_GUI(MP_SUB_TYPE_GUI2, TestSemGui, L"SE Test SEM" );

TestSemGui::TestSemGui()
{
	// initialise pins.
	initializePin( pinCaptureDataA, static_cast<MpGuiBaseMemberPtr2>(&TestSemGui::onSetCaptureDataA) );
	initializePin( pinCaptureDataB, static_cast<MpGuiBaseMemberPtr2>(&TestSemGui::onSetCaptureDataB) );
	initializePin( pinVoiceGate, static_cast<MpGuiBaseMemberPtr2>(&TestSemGui::onSetVoiceGate) );
	initializePin( pinpolydetect, static_cast<MpGuiBaseMemberPtr2>(&TestSemGui::onSetpolydetect) );
}

// handle pin updates.
void TestSemGui::onSetCaptureDataA()
{
	// pinCaptureDataA changed
}

void TestSemGui::onSetCaptureDataB()
{
	// pinCaptureDataB changed
}

void TestSemGui::onSetVoiceGate()
{
	// pinVoiceGate changed
}

void TestSemGui::onSetpolydetect()
{
	// pinpolydetect changed
}

int32_t TestSemGui::OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext )
{
	Graphics g(drawingContext);

	auto textFormat = GetGraphicsFactory().CreateTextFormat();
	auto brush = g.CreateSolidColorBrush(Color::Red);

	g.DrawTextU("Hello World!", textFormat, 0.0f, 0.0f, brush);

	return gmpi::MP_OK;
}

