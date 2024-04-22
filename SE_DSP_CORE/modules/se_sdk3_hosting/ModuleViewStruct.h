#pragma once
#include "ModuleView.h"

namespace SynthEdit2
{
class ModuleViewStruct : public ModuleView
{
	std::string lPlugNames;
	std::string rPlugNames;
	GmpiDrawing::PathGeometry outlineGeometry;
	std::vector< pinViewInfo > plugs_;
	const float clientPadding = 2.0f;
	const int plugTextHorizontalPadding = -1; // gap between plug text and plug circle outer radius.
	GmpiDrawing::Rect clipArea;
	bool muted = false;

	static std::chrono::time_point<std::chrono::steady_clock> lastClickedTime;

	std::shared_ptr<sharedGraphicResources_struct> drawingResources;
	static GraphicsResourceCache<sharedGraphicResources_struct> drawingResourcesCache;
	sharedGraphicResources_struct* getDrawingResources(GmpiDrawing::Factory& factory);

	GmpiDrawing::Rect GetCpuRect();
#if defined(SE_EDIT_SUPPORT)
	void RenderCpu(GmpiDrawing::Graphics& g);
	bool showCpu()
	{
		return cpuInfo != nullptr;
	}
#endif
	cpu_accumulator* cpuInfo = {};

public:

	ModuleViewStruct(const wchar_t* typeId, ViewBase* pParent, int handle) : ModuleView(typeId, pParent, handle) {}
	ModuleViewStruct(Json::Value* context, class ViewBase* pParent, std::map<int, class ModuleView*>& guiObjectMap);

	virtual GmpiDrawing::Rect GetClipRect() override;

	GmpiDrawing::PathGeometry CreateModuleOutline(GmpiDrawing::Factory& factory);
	GmpiDrawing::PathGeometry CreateModuleOutline2(GmpiDrawing::Factory& factory);
	GmpiDrawing::Point getConnectionPoint(CableType cableType, int pinIndex) override;
	int getPinDatatype(int pinIndex);
	bool getPinGuiType(int pinIndex);
	bool isMuted() override
	{
		return muted;
	}

	virtual int32_t measure(GmpiDrawing::Size availableSize, GmpiDrawing::Size* returnDesiredSize) override;
	virtual int32_t arrange(GmpiDrawing::Rect finalRect) override;
	virtual void OnRender(GmpiDrawing::Graphics& g) override;
	std::pair<int, int> getPinUnderMouse(GmpiDrawing_API::MP1_POINT point);
	int32_t OnDoubleClicked(int32_t flags, GmpiDrawing_API::MP1_POINT point);
	int32_t onPointerDown(int32_t flags, GmpiDrawing_API::MP1_POINT point) override;

	void OnCableDrag(ConnectorViewBase* dragline, GmpiDrawing::Point dragPoint, float& bestDistance, IViewChild*& bestModule, int& bestPinIndex) override;

	bool EndCableDrag(GmpiDrawing_API::MP1_POINT point, ConnectorViewBase* dragline) override;

	bool isVisable() override
	{
		return true;
	}
	virtual std::unique_ptr<SynthEdit2::IViewChild> createAdorner(ViewBase* pParent) override;
	void OnCpuUpdate(class cpu_accumulator* cpuInfo) override;
	bool isRackModule() override
	{
		return false;
	}
};
}
