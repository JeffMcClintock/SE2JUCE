#pragma once
#include <vector>
#include <memory>
#include <algorithm>
#include "Drawing.h"
#include "mp_sdk_gui2.h"
#include "Widget.h"

class WidgetHost : public gmpi_gui::MpGuiGfxBase
{
protected:
	std::vector< std::shared_ptr<Widget> > widgets;
	Widget* captureWidget;

public:
	WidgetHost() :
		captureWidget(nullptr)
	{}

	virtual int32_t MP_STDCALL OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext) override
	{
		GmpiDrawing::Graphics dc2(drawingContext);

		for (auto& w : widgets)
		{
			w->OnRender(dc2);
		}
		return gmpi::MP_OK;
	}

	int32_t MP_STDCALL hitTest(GmpiDrawing_API::MP1_POINT point) override
	{
		for (auto& w : widgets)
		{
			if (w->widgetHitTest(point))
			{
				return gmpi::MP_OK;
			}
		}

		return gmpi::MP_UNHANDLED;
	}

	virtual int32_t MP_STDCALL onPointerDown(int32_t flags, GmpiDrawing_API::MP1_POINT point) override
	{
		captureWidget = nullptr;
		int32_t returnHit = gmpi::MP_UNHANDLED;

		for (auto& w : widgets)
		{
			if (w->widgetHitTest(point))
			{
				returnHit = gmpi::MP_OK; // note hit, even if mouse not captured (for context menu to work).

				if (w->onPointerDown(flags, point))
				{
					captureWidget = w.get();
					setCapture();

					if (w->ClearDirty())
						invalidateRect();

					return gmpi::MP_OK;
				}
			}
		}

		return returnHit;
	}

	virtual int32_t MP_STDCALL onPointerMove(int32_t flags, GmpiDrawing_API::MP1_POINT point) override
	{
		if (!getCapture())
		{
			return gmpi::MP_UNHANDLED;
		}

		if (captureWidget)
		{
			captureWidget->onPointerMove(flags, point);

			if (captureWidget->ClearDirty())
				invalidateRect();
		}

		return gmpi::MP_OK;
	}

	virtual int32_t MP_STDCALL onPointerUp(int32_t flags, GmpiDrawing_API::MP1_POINT point) override
	{
		if (!getCapture())
		{
			return gmpi::MP_UNHANDLED;
		}

		releaseCapture();

		if (captureWidget)
		{
			captureWidget->onPointerUp(flags, point);
		}

		return gmpi::MP_OK;
	}

	virtual int32_t MP_STDCALL measure(GmpiDrawing_API::MP1_SIZE availableSize, GmpiDrawing_API::MP1_SIZE* returnDesiredSize) override
	{
		returnDesiredSize->height = 0.0f;
		returnDesiredSize->width = 0.0f;

		for (auto& w : widgets)
		{
			returnDesiredSize->width = (std::max)(returnDesiredSize->height, w->getRect().left);
			returnDesiredSize->height = (std::max)(returnDesiredSize->height, w->getRect().bottom);
		}

		return gmpi::MP_OK;
	}
};

