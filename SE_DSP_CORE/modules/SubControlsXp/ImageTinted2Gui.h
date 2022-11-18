#ifndef IMAGETINTED2GUI_H_INCLUDED
#define IMAGETINTED2GUI_H_INCLUDED

#include "./Image2GuiXp.h"

class ImageTinted2Gui : public Image2GuiXp
{
public:
	ImageTinted2Gui();

	void InvalidateTint();
	void onLoaded() override;
	GmpiDrawing_API::IMpBitmap* getDrawBitmap() override
	{
		return bitmapTinted_.Get();
	}
	int32_t MP_STDCALL OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext) override
	{
		if (tint_color[3] == 0)
		{
			HSVtoRGB();
			UpdateImage();
		}

		return Image2GuiXp::OnRender(drawingContext);
	}

private:
	GmpiDrawing::Bitmap bitmapTinted_;
	uint8_t tint_color[4]; // RGB color space
	void HSVtoRGB();
	void UpdateImage();

 	FloatGuiPin pinHue;
 	FloatGuiPin pinSaturation;
 	FloatGuiPin pinBrightness;
 	FloatGuiPin pinHueIn;
 	FloatGuiPin pinSaturationIn;
 	FloatGuiPin pinBrightnessIn;
};

#endif


