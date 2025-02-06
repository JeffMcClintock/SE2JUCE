// prevent MS CPP - 'swprintf' was declared deprecated warning
#if defined(_MSC_VER)
  #define _CRT_SECURE_NO_DEPRECATE
  #pragma warning(disable : 4996)
#endif

#include <stdio.h>  // for GCC.
#include "Scope3Gui.h"
#include <algorithm>
#include "../shared/xp_simd.h"
#ifndef _WIN32
#include <sys/time.h>                // for gettimeofday()
#endif

using namespace gmpi;
using namespace gmpi_gui;
using namespace GmpiDrawing;
using namespace GmpiDrawing_API;

#undef DrawText

GMPI_REGISTER_GUI(MP_SUB_TYPE_GUI2, Scope3Gui, L"SE Scope3 XP");
GMPI_REGISTER_GUI(MP_SUB_TYPE_GUI2, Scope3Gui, L"SE TrigScope3 XP");
SE_DECLARE_INIT_STATIC_FILE(Scope3XP_Gui);

#ifndef _WIN32
int32_t timeGetTime() // MacOS version
{
    timeval t1;

    // start timer
    gettimeofday(&t1, NULL);
    return (int32_t)(t1.tv_sec * 1000.0);
}
#endif

Scope3Gui::Scope3Gui() :
newestVoice_( 0 )
{
	initializePin(0, pinSamplesA, static_cast<MpGuiBaseMemberIndexedPtr2>(&Scope3Gui::onValueChanged));
	initializePin(1, pinSamplesB, static_cast<MpGuiBaseMemberIndexedPtr2>(&Scope3Gui::onValueChanged));
	initializePin(3, pinGates, static_cast<MpGuiBaseMemberIndexedPtr2>(&Scope3Gui::onVoicesActiveChanged));
	initializePin(4, pinPolyMode, static_cast<MpGuiBaseMemberPtr2>(&Scope3Gui::onPolyModeChanged));

	for( int i = 0 ; i < MP_VOICE_COUNT ; ++i )
	{
		VoiceLastUpdated[i] = std::chrono::steady_clock::now();
	}
}

void Scope3Gui::onValueChanged( int voiceId )
{
	VoiceLastUpdated[voiceId] = std::chrono::steady_clock::now(); // timeGetTime();

	invalidateRect();
	StartTimer(100);
}

void Scope3Gui::onVoicesActiveChanged( int voiceId )
{
	if( pinPolyMode == true && pinGates.getValue( voiceId ) )
	{
		newestVoice_ = voiceId;
	}
}

void Scope3Gui::onPolyModeChanged()
{
	if( pinPolyMode == false )
	{
		newestVoice_ = 0; // monophonic mode.
	}
}

bool Scope3Gui::OnTimer()
{
	invalidateRect();
	return true;
}

void Scope3Gui::DrawTrace(Factory& factory, Graphics& g, float* capturedata, GmpiDrawing::Brush& brush, float mid_y, float scale, int width)
{
	const float penWidth = 1;
	auto geometry = factory.CreatePathGeometry();
	auto sink = geometry.Open();

#if 0 // not very efficient, draws too much detail on small scopes.
	int step = (std::max)( 1, FastRealToIntTruncateTowardZero( SCOPE_BUFFER_SIZE / width ) );
	float xinc = (width * step) / SCOPE_BUFFER_SIZE;

	float x = 0;
	sink.BeginFigure(x, mid_y - (*capturedata * scale));

	for (int i = 1; i < SCOPE_BUFFER_SIZE; i += step)
	{
		x += xinc;
		sink.AddLine(GmpiDrawing::Point(x, mid_y - (capturedata[i] * scale)));
	}
#else
	// Only draw one point per pixel. i.e. less points on smaller scopes.
	GmpiDrawing::Point p(0, 0);
	float graph[SCOPE_BUFFER_SIZE];
	float xinc;
	int graphSize;

	// If scope is wider in pixels than samples, draw one point per sample, else one point per pixel (saves CPU).
	if (SCOPE_BUFFER_SIZE <= width)
	{
		xinc = width / (float) SCOPE_BUFFER_SIZE;
		graphSize = SCOPE_BUFFER_SIZE;

		for (int index = 0; index < SCOPE_BUFFER_SIZE; ++index)
		{
			float y = capturedata[index];
			graph[index] = mid_y - y * scale;
		}
	}
	else
	{
		xinc = 1.0f;
		graphSize = width;

		float indexf = 0;
		float indexInc = (SCOPE_BUFFER_SIZE - 1) / (float) width;

		for (int i = 0; i < width; ++i)
		{
			int index = FastRealToIntFloor(indexf);
			float frac = indexf - index;

			float y1 = capturedata[index];
			float y2 = capturedata[index + 1];
			float y = y1 + frac * (y2 - y1);
			graph[i] = mid_y - y * scale;

			indexf += indexInc;
		}
	}

	// Simplify points and plot.
	const float tollerance = 0.3f;
	p.x = 0.0f;
	p.y = graph[0];
	sink.BeginFigure(p);

	for (int i = 0; i < graphSize - 2; )
	{
		float dy = graph[i + 1] - graph[i];
		int j = i + 2;
		float predicted = graph[i + 1];
		while (j < graphSize - 2)
		{
			predicted += dy;

			float err = graph[j] - predicted;

			if (err > tollerance || err < -tollerance)
			{
				--j;
				{
					p.y = graph[j];
					p.x = xinc * (float)j;
					sink.AddLine(p);
				}
				break;
			}
			++j;
		}
		i = j;
	}

	// final 2 points.
	p.y = graph[graphSize - 2];
	p.x = xinc * (float)(graphSize - 2);
	sink.AddLine(p);
	p.y = graph[graphSize - 1];
	p.x += xinc;
	sink.AddLine(p);

	//		_RPT1(_CRT_WARN, "eliminated %d\n", eliminated);
/*
	// greate geometry.
	for (int index = 0; index < graphSize; ++index)
	{
		p.y = graph[index];

		if (index == 0)
			sink.BeginFigure(p);
		else
		{
			if (p.y != excludedPointY)
				sink.AddLine(p);
		}
		p.x += xinc;
	}
	*/
#endif

	sink.EndFigure(FigureEnd::Open);
	sink.Close();

	g.DrawGeometry(geometry, brush, penWidth);
}

void line(GmpiDrawing::Point from, GmpiDrawing::Point to, int32_t* image, int width, int32_t color)
{
	int x0 = FastRealToIntTruncateTowardZero(from.x);
	int y0 = FastRealToIntTruncateTowardZero(from.y);
	int x1 = FastRealToIntTruncateTowardZero(to.x);
	int y1 = FastRealToIntTruncateTowardZero(to.y);

	bool steep = false;
	if (std::abs(x0 - x1)<std::abs(y0 - y1)) {
		std::swap(x0, y0);
		std::swap(x1, y1);
		steep = true;
	}
	if (x0>x1) {
		std::swap(x0, x1);
		std::swap(y0, y1);
	}
	int dx = x1 - x0;
	int dy = y1 - y0;
	int derror2 = std::abs(dy) * 2;
	int error2 = 0;
	int y = y0;
	for (int x = x0; x <= x1; x++) {
		if (steep) {
			//			image.set(y, x, color);
			image[x * width + y] = color;
		}
		else {
			//			image.set(x, y, color);
			image[y * width + x] = color;
		}
		error2 += derror2;
		if (error2 > dx) {
			y += (y1>y0 ? 1 : -1);
			error2 -= dx * 2;
		}
	}
}

#if 0
void Scope3Gui::DrawTrace2(int32_t* image, int width, int32_t color, float* capturedata, float mid_y, float scale)
{
	const float penWidth = 1;
	int step = (std::max)(1, SCOPE_BUFFER_SIZE / width);
	float xinc = (width * step) / (float) SCOPE_BUFFER_SIZE;
	float x = 0;

	GmpiDrawing::Point prev(x, mid_y - (*capturedata * scale));

	for (int i = 1; i < SCOPE_BUFFER_SIZE; i += step)
	{
		x += xinc;
		GmpiDrawing::Point to(x, mid_y - (capturedata[i] * scale));

		//g.DrawLine(prev, to, brush, penWidth);
		line(prev, to, image, width, color);
		prev = to;
	}
}
#endif

int32_t Scope3Gui::OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext)
{
#ifdef _DEBUG
	assert(debugInitializeCheck_);
//	assert(debug_IsMeasured);
	assert(debug_IsArranged);
#endif

	GmpiDrawing::Graphics g(drawingContext);

	const float snapToPixelOffset = 0.5f;

	auto r = getRect();

	g.PushAxisAlignedClip(r);
    
	std::chrono::steady_clock::time_point showUpdatesAfter = std::chrono::steady_clock::now() - std::chrono::milliseconds(500); // timeGetTime() - 500; // show any trace updated in last 0.5 seconds.
	float height = r.bottom - r.top;
	float width = r.right - r.left;

	int widthi = FastRealToIntFloor(width + 0.5f);

	float scale = height * 0.46f;;
	float mid_y = floorf(0.5f + height * 0.5f);

#if 1
	if (cachedBackground_.isNull())
	{
		float mid_x = floorf(0.5f + width * 0.5f);

        GmpiDrawing::Size mysize(r.getWidth(), r.getHeight());
		auto dc = g.CreateCompatibleRenderTarget(mysize);
		dc.BeginDraw();

		if (true) // fontInfo_.colorBackground >= 0 ) // -1 indicates transparent background
		{
			// Fill in solid background black
			auto background_brush = dc.CreateSolidColorBrush(typeface_->getBackgroundColor());
			dc.FillRectangle(r, background_brush);
		}

		auto darked_col = typeface_->getColor();
		darked_col.r *= 0.5f;
		darked_col.g *= 0.5f;
		darked_col.b *= 0.5f;

		auto brush2 = dc.CreateSolidColorBrush(darked_col);

		// BACKGROUND LINES
		// horizontal line
		float penWidth = 1.0f;
		dc.DrawLine(GmpiDrawing::Point(0, mid_y + snapToPixelOffset), GmpiDrawing::Point(width, mid_y + snapToPixelOffset), brush2, penWidth);

		// vertical line
		dc.DrawLine(GmpiDrawing::Point(mid_x + snapToPixelOffset, 0), GmpiDrawing::Point(mid_x + snapToPixelOffset, height), brush2, penWidth);

		// voltage ticks
		int tick_width = 2;
		int step = 1;
		if (height < 50)
			step = 4;

		for (int v = -10; v < 11; v += step)
		{
			float y = snapToPixelOffset + floorf(v * scale * 0.1f);

			if (v % 5 == 0)
				tick_width = 4;
			else
				tick_width = 2;

			dc.DrawLine(GmpiDrawing::Point(mid_x - tick_width, mid_y + y), GmpiDrawing::Point(mid_x + tick_width, mid_y + y), brush2, penWidth);
		}

		// labels
		brush2.SetColor(typeface_->getColor());
		if (height > 30)
		{
			float yOffset;
			float fontBoxSize;
			if(typeface_->verticalSnapBackwardCompatibilityMode)
			{
				fontBoxSize = 12;
				yOffset = -fontBoxSize / 2;
			}
			else
			{
				const auto metrics = dtextFormat.GetFontMetrics();
				fontBoxSize = metrics.bodyHeight();
				yOffset = metrics.capHeight * 0.5 - metrics.ascent;
			}

			for (int v = -10; v < 11; v += 5)
			{
				char txt[10];
				float y = v * scale / 10.f;
				sprintf(txt, "%2.0f", (float)v);

				float tx = mid_x + tick_width;
				float ty = mid_y - (int)y + yOffset;
				GmpiDrawing::Rect textRect(tx, ty, tx + 100, ty + fontBoxSize);
				dc.DrawTextU(txt, dtextFormat, textRect, brush2);
			}
		}

		dc.EndDraw();

		cachedBackground_ = dc.GetBitmap();
	}

	g.DrawBitmap(cachedBackground_, GmpiDrawing::Point(0, 0), r);
#else
    auto background_brush = g.CreateSolidColorBrush(typeface_->getBackgroundColor());
    g.FillRectangle(r, background_brush);
#endif
    
	// traces
#ifdef DRAW_LINES_ON_BITMAP
	if (foreground_.isNull())
	{
		foreground_ = g.GetFactory().CreateImage((int32_t)r.getWidth(), (int32_t)r.getHeight());
	}
#endif

	bool voicesStillActive = false;
	{ // RIAA pixel lock.

#ifdef DRAW_LINES_ON_BITMAP
		auto pixels = foreground_.lockPixels(GmpiDrawing_API::MP1_BITMAP_LOCK_WRITE);
	auto imageSize = foreground_.GetSize();
	int widthPixels = pixels.getBytesPerRow() / sizeof(uint32_t);
	int totalPixels = (int)imageSize.height * widthPixels;

	int32_t* sourcePixels = (int32_t*)pixels.getAddress();

	// Clear bitmap
	int i = 0;
	for (int32_t* p = sourcePixels; i < totalPixels; ++i)
	{
		*p++ = 0;
	}
#endif

#ifndef DRAW_LINES_ON_BITMAP
	auto brush2 = g.CreateSolidColorBrush(Color::DarkOrange);
#endif

	auto factory = g.GetFactory();

	// non-main voices drawn dull.
	if( pinPolyMode == true )
	{
		// 'B' trace.
		for(int voice = 0 ; voice < 128 ; ++voice )
		{
			if( pinGates.getValue(voice) > 0.0f || VoiceLastUpdated[voice] > showUpdatesAfter )
			{
				voicesStillActive = true;

				if( voice != newestVoice_ )
				{
					if( pinSamplesB.rawSize(voice) == sizeof(float) * SCOPE_BUFFER_SIZE )
					{
						float* capturedata = (float*) pinSamplesB.rawData(voice);
#ifdef DRAW_LINES_ON_BITMAP
						DrawTrace2(sourcePixels, widthPixels, 0xFF969600, capturedata, mid_y, scale);
#else
						DrawTrace(factory, g, capturedata, brush2, mid_y, scale, widthi);
#endif
					}
				}
			}
		}

		// 'A' trace.
#ifdef DRAW_LINES_ON_BITMAP
		brush2.SetColor(Color::FromBytes(0, 160, 0)); // dark green
#endif

		for(int voice = 0 ; voice < 128 ; ++voice )
		{
			if( pinGates.getValue(voice) > 0.0f || VoiceLastUpdated[voice] > showUpdatesAfter )
			{
				voicesStillActive = true;

				if( voice != newestVoice_ )
				{
					if( pinSamplesA.rawSize(voice) == sizeof(float) * SCOPE_BUFFER_SIZE )
					{
						float* capturedata = (float*) pinSamplesA.rawData(voice);
#ifdef DRAW_LINES_ON_BITMAP
						DrawTrace2(sourcePixels, widthPixels, 0xFF00A000, capturedata, mid_y, scale);
#else
						DrawTrace(factory, g, capturedata, brush2, mid_y, scale, widthi);
#endif
					}
				}
			}
		}
	}

	// main trace drawn bright.
	// Note: Only works when MIDI connected to Patch-Automator. Else Gate host controls not active.
	if( ( newestVoice_ >= 0 && (pinGates.getValue(newestVoice_) || VoiceLastUpdated[newestVoice_] > showUpdatesAfter )) || pinPolyMode == false )
	{
		if( pinSamplesB.rawSize(newestVoice_) == sizeof(float) * SCOPE_BUFFER_SIZE )
		{
			float* capturedata = (float*) pinSamplesB.rawData(newestVoice_);
#ifdef DRAW_LINES_ON_BITMAP
			DrawTrace2(sourcePixels, widthPixels, 0xFFFAFA00, capturedata, mid_y, scale);
#else
			brush2.SetColor(Color::FromBytes(250, 250, 0)); // yellow
			DrawTrace(factory, g, capturedata, brush2, mid_y, scale, widthi);
#endif
		}

		if( pinSamplesA.rawSize(newestVoice_) == sizeof(float) * SCOPE_BUFFER_SIZE )
		{
			float* capturedata = (float*)pinSamplesA.rawData(newestVoice_);
#ifdef DRAW_LINES_ON_BITMAP
			DrawTrace2(sourcePixels, widthPixels, 0xFF00FF00, capturedata, mid_y, scale);
#else
			brush2.SetColor(Color::FromBytes(0, 255, 0)); // green.
			DrawTrace(factory, g, capturedata, brush2, mid_y, scale, widthi);
#endif
		}
	}
	}

#ifdef DRAW_LINES_ON_BITMAP
	g.DrawBitmap(foreground_, Point(0, 0), r);
#endif

#if 0 // def _DEBUG
	// test gradient.
	auto brush = g.CreateSolidColorBrush(Color::DarkOrange);
	for (int x = 0; x < 255; ++x)
	{
		brush.SetColor(Color::FromRgb(x | (x<<8) | (x <<16) | (0xff <<24)));
		g.FillRectangle(Rect(x, 0, x + 1, 100), brush);
	}
#endif

	g.PopAxisAlignedClip();

	if( !voicesStillActive )
	{
		StopTimer();
	}

	return gmpi::MP_OK;
}

int32_t MP_STDCALL Scope3Gui::initialize()
{
	dtextFormat = FontCache::instance()->GetCustomTextFormat(
		getHost(),
		getGuiHost(),
		"Custom:Scope3Gui",
		"tty",
		[=](FontMetadata* customFont) -> void
			{
				if(!customFont->verticalSnapBackwardCompatibilityMode)
				{
					customFont->bodyHeight_ = customFont->pixelHeight_;
					customFont->bodyHeightDigitsOnly_ = false;
					customFont->vst3_vertical_offset_ = 0;
				}

				customFont->faceFamilies_[0] = "Arial";
				customFont->setTextAlignment(GmpiDrawing::TextAlignment::Leading);
				customFont->wordWrapping = GmpiDrawing::WordWrapping::NoWrap;
				customFont->paragraphAlignment = GmpiDrawing::ParagraphAlignment::Center;
			},
		&typeface_
	);

	return MpGuiGfxBase::initialize();
}

int32_t Scope3Gui::measure(GmpiDrawing_API::MP1_SIZE availableSize, GmpiDrawing_API::MP1_SIZE* returnDesiredSize)
{
	const float minSize = 15;

	returnDesiredSize->width = (std::max)(minSize, availableSize.width);
	returnDesiredSize->height = (std::max)(minSize, availableSize.height);

	return gmpi::MP_OK;
}

int32_t Scope3Gui::arrange(GmpiDrawing_API::MP1_RECT finalRect)
{
	cachedBackground_.setNull();
#ifdef DRAW_LINES_ON_BITMAP
	foreground_.setNull();
#endif
	return MpGuiGfxBase::arrange(finalRect);
}
