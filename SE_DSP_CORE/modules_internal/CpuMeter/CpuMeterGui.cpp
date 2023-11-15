#include "pch.h"
#include "mp_sdk_gui2.h"
#include "Drawing.h"
#include "TimerManager.h"

#ifdef SE_EDIT_SUPPORT
#include "../se_sdk3_hosting/ModuleView.h"
#include "../se_sdk3_hosting/ViewBase.h"
#include "MfcDocPresenter.h"
#include "SynthEditDocBase.h"
#include "SynthEdit.h"
#endif

SE_DECLARE_INIT_STATIC_FILE(CpuMeterGui);
using namespace gmpi;
using namespace GmpiDrawing;

class CpuMeterGui : public gmpi_gui::MpGuiGfxBase, public TimerClient
{
	float peakCpu = 0.0f;

public:
	CpuMeterGui()
	{
		StartTimer(20);
	}

	bool OnTimer() override
	{
		invalidateRect();
		return true; // continue timer.
	}

	int32_t MP_STDCALL OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext ) override
	{
		auto r = getRect();
		Graphics g(drawingContext);

		ClipDrawingToBounds cd(g, r);

		auto textFormat = GetGraphicsFactory().CreateTextFormat();
		auto brush = g.CreateSolidColorBrush(Color::Green);

#ifdef SE_EDIT_SUPPORT
		// Access SynthEdit internals.
		CSynthEditApp* application = nullptr;
		auto modview = dynamic_cast<SynthEdit2::ModuleView*>(getHost());
		assert(modview != nullptr);
		{
			auto view = modview->parent;
			if (view == nullptr)
			{
				return gmpi::MP_OK;
			}
			auto container = dynamic_cast<MfcDocPresenter*>(view->Presenter())->getContainer();
			application = dynamic_cast<CSynthEditApp*>(container->Document()->Application());
		}
		auto cpu = application->GetCpuLoad();
#else
		auto cpu = std::make_pair(0.0f, 0.0f); // not supported in plugin
		const int CPU_BATCH_SIZE = 1;
#endif
		auto median = std::get<0>(cpu);
		auto peak = std::get<1>(cpu);
		peakCpu = (std::min)(peakCpu, 2.0f); // limit time required to fall from startup peak.
		peakCpu -= 0.001f * static_cast<float>(CPU_BATCH_SIZE);
		peakCpu = (std::max)(peakCpu - 0.01f, peak);

		if (peakCpu > 1.0f)
		{
			peakCpu = 1.0f;
			brush.SetColor(Color::Red);
		}
		else
		{
			brush.SetColor(Color::Green);
		}

		g.FillRectangle(Rect(0, (1.0f - peakCpu) * r.getHeight(), r.getWidth(), r.getHeight()), brush);

		brush.SetColor(Color::Lime);
		g.FillRectangle(Rect(0, (1.0f - median) * r.getHeight(), r.getWidth(), r.getHeight()), brush);

		brush.SetColor(Color::Black);
		char text[20];
		float cpuPercent = std::get<0>(cpu) * 100.0f;
		if(cpuPercent < 10.0f)
			sprintf(text, "%0.1f", cpuPercent);
		else
			sprintf(text, "%0.0f", cpuPercent);

		g.DrawTextU(text, textFormat, 10.0f, r.getHeight() - 20.0f, brush);

		return gmpi::MP_OK;
	}
};

namespace
{
	auto r = gmpi::Register<CpuMeterGui>::withXml(
R"XML(
<?xml version="1.0" ?>
<PluginList>
  <Plugin id="SE CPU Meter" name="CPU Meter" category="Diagnostic" >
    <GUI graphicsApi="GmpiGui"/>
  </Plugin>
</PluginList>
)XML");

}
