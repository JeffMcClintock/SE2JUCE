#pragma once
#include "../shared/skinBitmap.h"
#include "Widget.h"

class BitmapWidget : public skinBitmap, public Widget
{
protected:
	float animationPosition;

public:
	std::function<void(float)> OnChangedEvent;

	BitmapWidget();

	BitmapWidget(gmpi::IMpUnknown* host, const char* imageFile) :
		animationPosition(0)
	{
		setHost(host);
		Load(imageFile);
	}

	void Load(const char* imageFile);
	virtual GmpiDrawing::Size getSize() override;

	bool bitmapHitTest2(GmpiDrawing::Point point)
	{
		point.x -= position.left;
		point.y -= position.top;
		return bitmapHitTestLocal(point);
	}

	virtual void OnRender(GmpiDrawing::Graphics& dc) override;
	void SetNormalised(float n);
	float GetNormalised()
	{
		return animationPosition;
	}

	virtual GmpiDrawing::Point getKnobCenter();

	void calcDrawAt();
	virtual bool onPointerDown(int32_t flags, GmpiDrawing_API::MP1_POINT point) override;
	virtual void onPointerMove(int32_t flags, GmpiDrawing_API::MP1_POINT point) override;
	virtual void onPointerUp(int32_t flags, GmpiDrawing_API::MP1_POINT point) override;
};

