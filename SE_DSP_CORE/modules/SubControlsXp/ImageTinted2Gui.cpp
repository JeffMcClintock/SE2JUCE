#include "./ImageTinted2Gui.h"
#include "../shared/fast_gamma.h"

using namespace gmpi;
using namespace gmpi_gui;
using namespace GmpiDrawing;
using namespace GmpiDrawing_API;
using namespace se_sdk;

GMPI_REGISTER_GUI(MP_SUB_TYPE_GUI2, ImageTinted2Gui, L"SE ImageTinted XP" );
SE_DECLARE_INIT_STATIC_FILE(ImageTinted2Gui);

inline void clamp_normalised(float& f)
{
	if( f < 0.f ) f = 0.f;

	if( f > 1.f ) f = 1.f;
}

ImageTinted2Gui::ImageTinted2Gui() : Image2GuiXp(false)
{
	/* base does these.
	initializePin(0, pinAnimationPosition, static_cast<MpGuiBaseMemberPtr2>( &Image2GuiXp::calcDrawAt ));
	initializePin(1, pinFilename, static_cast<MpGuiBaseMemberPtr2>( &ImageTinted2Gui::onSetFilenameOverride ));
	initializePin(2, pinHint);
	initializePin(3, pinMenuItems);
	initializePin(4, pinMenuSelection);
	initializePin(5, pinMouseDown);
	initializePin(6, pinMouseDownLegacy);
	initializePin(7, pinFrameCount);
	initializePin(8, pinFrameCountLegacy);
	*/
	initializePin(pinHue, static_cast<MpGuiBaseMemberPtr2>(&ImageTinted2Gui::InvalidateTint) );
	initializePin(pinSaturation, static_cast<MpGuiBaseMemberPtr2>( &ImageTinted2Gui::InvalidateTint ));
	initializePin(pinBrightness, static_cast<MpGuiBaseMemberPtr2>( &ImageTinted2Gui::InvalidateTint ));
	initializePin(pinHueIn, static_cast<MpGuiBaseMemberPtr2>( &ImageTinted2Gui::InvalidateTint ));
	initializePin(pinSaturationIn, static_cast<MpGuiBaseMemberPtr2>( &ImageTinted2Gui::InvalidateTint ));
	initializePin(pinBrightnessIn, static_cast<MpGuiBaseMemberPtr2>( &ImageTinted2Gui::InvalidateTint ));
}

// handle pin updates.
void ImageTinted2Gui::InvalidateTint()
{
	tint_color[3] = 0; // flag need for recalc.
	invalidateRect();
}

inline uint8_t fast8bitScale(uint8_t a, uint8_t b)
{
	int t = (int)a * (int)b;
	return (uint8_t)((t + 1 + (t >> 8)) >> 8); // fast way to divide by 255
}

void ImageTinted2Gui::UpdateImage()
{
	HSVtoRGB();

	// colour existing bitmap.
	if( !bitmap_.isNull() && !bitmapTinted_.isNull())
	{
		auto pixelsSource = bitmap_.lockPixels();
		auto pixelsDest = bitmapTinted_.lockPixels(GmpiDrawing_API::MP1_BITMAP_LOCK_WRITE);
		auto imageSize = bitmap_.GetSize();

        uint8_t tint_color_native[4];
        tint_color_native[3] = tint_color[3]; // Alpha
        if( pixelsSource.getPixelFormat() == IMpBitmapPixels::kRGBA )
        {
            tint_color_native[0] = tint_color[2];
            tint_color_native[1] = tint_color[1];
            tint_color_native[2] = tint_color[0];
        }
        else
        {
            tint_color_native[0] = tint_color[0];
            tint_color_native[1] = tint_color[1];
            tint_color_native[2] = tint_color[2];
        }
        
		int totalPixels = (int)imageSize.height * pixelsSource.getBytesPerRow() / sizeof(uint32_t);

		uint8_t* sourcePixels = pixelsSource.getAddress();
		uint8_t* destPixels = pixelsDest.getAddress();

		if (true) // i.e. GDI+
		{
			for (int i = 0; i < totalPixels; ++i)
			{
				// SE converted 3-byte pixels to monochrome using first byte, 4-byte using second. Totally unpredictable here.
				// Just average the two.
				int alpha = (sourcePixels[1] + sourcePixels[0]) / 2;

				// un-premultiply if needed.
				if (alpha != 0 && alpha != 255)
				{
					alpha = (uint32_t)(alpha * 255) / sourcePixels[3]; //  <- orig alpha
				}

				destPixels[3] = alpha;

				if (alpha == 255)
				{
					destPixels[0] = tint_color_native[0];
					destPixels[1] = tint_color_native[1];
					destPixels[2] = tint_color_native[2];
				}
				else
				{
					// Pre-multiply alpha.
					for (int c = 0; c < 3; ++c)
					{
						/*
						// SRGB Premultiply
						float l = se_sdk::FastGamma::sRGB_to_float(tint_color[c]);
						l *= alpha / 255.0;
						destPixels[c] = se_sdk::FastGamma::float_to_sRGB(l);
						*/

						// linear. Works correct on GDI+	
						{
							int t = (int)tint_color_native[c] * (int)alpha;
							destPixels[c] = (uint8_t)((t + 1 + (t >> 8)) >> 8); // fast way to divide by 255
						}
					}
				}

				sourcePixels += sizeof(uint32_t);
				destPixels += sizeof(uint32_t);
			}
		}
		else // Direct-2D, totally lame and non-sensical. Image does not render correctly.
		{
			// Guy says this is worse.

			/*
			unsigned char table[256];
			for (int i = 0; i < 256; ++i)
			{
				// Fixup alpha hack. No idea why it works.
				float lin = FastGamma::pixelToNormalised(i);
				float test = lin + (lin - FastGamma::srgbToLinear(lin));
				table[i] = (std::min)(255, (int)(0.5f + test * 255.0f));
			}
			*/

			for (int i = 0; i < totalPixels; ++i)
			{
				// SE converted 3-byte pixels to monochrome using first byte, 4-byte using second. Totally unpredictable here.
				// Just average the two.
				int dest_alpha = (sourcePixels[1] + sourcePixels[0]) / 2;

				// un-premultiply if needed.
				if (sourcePixels[3] != 0 && sourcePixels[3] != 255 && dest_alpha != 0)
				{
// with gamma					dest_alpha = (int)(FastGamma::sRGB_to_float(dest_alpha) * 255.0f * 255.0f) / (int)sourcePixels[3];
					dest_alpha = (int)(dest_alpha * 255) / (int)sourcePixels[3];
					dest_alpha = (std::min)(255, dest_alpha);
				}
/*
				destPixels[0] = tint_color[0];
				destPixels[1] = tint_color[1];
				destPixels[2] = tint_color[2];
				destPixels[3] = table[dest_alpha];
*/

				destPixels[0] = fast8bitScale(tint_color[0], dest_alpha);
				destPixels[1] = fast8bitScale(tint_color[1], dest_alpha);
				destPixels[2] = fast8bitScale(tint_color[2], dest_alpha);
				destPixels[3] = dest_alpha;

				sourcePixels += sizeof(uint32_t);
				destPixels += sizeof(uint32_t);
			}
		}

//		bitmapTinted_.ApplyAlphaCorrection();
	}

	invalidateRect();
}

void ImageTinted2Gui::onLoaded()
{
	bitmapTinted_ = nullptr;

	if( !bitmap_.isNull() )
	{
		auto imageSize = bitmap_.GetSize();
		bitmapTinted_ = GetGraphicsFactory().CreateImage(imageSize.width, imageSize.height);
		InvalidateTint();
	}
}

void ImageTinted2Gui::HSVtoRGB()
{
	float hue = pinHueIn;
	float saturation = pinSaturationIn;
	float brightness = pinBrightnessIn;

	/*
	//' backward compatible pins take precedence if connected to anything.
	if( pinHue.isConnected() )
	{
		hue = pinHue;
	}

	if( pinSaturation.isConnected() )
	{
		saturation = pinSaturation;
	}

	if( pinBrightness.isConnected() )
	{
		brightness = pinBrightness;
	}
	*/

	//	_RPT3(_CRT_WARN, "%f  %f  %f\n", hue, saturation, brightness);
	clamp_normalised(hue);
	clamp_normalised(saturation);
	clamp_normalised(brightness);
	float r, g, b;

	if( saturation == 0 )
	{
		// achromatic (grey)
		r = g = b = brightness;
	}
	else
	{
		int i;
		float f, p, q, t;
		float h = hue * 6.f;			// sector 0 to 5
		i = (int)floorf(h);

		if( i > 5 ) // 6.0 handled as 5.9999999
			i = 5;

		f = h - i;			// factorial part of hue
		p = brightness * ( 1 - saturation );
		q = brightness * ( 1 - saturation * f );
		t = brightness * ( 1 - saturation * ( 1 - f ) );

		switch( i )
		{
		case 0:
			r = brightness;
			g = t;
			b = p;
			break;

		case 1:
			r = q;
			g = brightness;
			b = p;
			break;

		case 2:
			r = p;
			g = brightness;
			b = t;
			break;

		case 3:
			r = p;
			g = q;
			b = brightness;
			break;

		case 4:
			r = t;
			g = p;
			b = brightness;
			break;

		default:		// case 5:
			r = brightness;
			g = p;
			b = q;
			break;
		}
	}

	// gamma correction
	constexpr float ASSUMED_GAMMA = 2.2f;
	constexpr float inverse_gamma = 1.f / ASSUMED_GAMMA;
	r = powf(r, inverse_gamma);
	g = powf(g, inverse_gamma);
	b = powf(b, inverse_gamma);

	tint_color[0] = (uint8_t) (b * 255 + 0.5);
	tint_color[1] = (uint8_t) (g * 255 + 0.5);
	tint_color[2] = (uint8_t) (r * 255 + 0.5);
	tint_color[3] = 255; // alpha channel.
}
