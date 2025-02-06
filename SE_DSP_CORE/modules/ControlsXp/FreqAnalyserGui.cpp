// prevent MS CPP - 'swprintf' was declared deprecated warning
#if defined(_MSC_VER)
  #define _CRT_SECURE_NO_DEPRECATE
  #pragma warning(disable : 4996)
#endif

#include <stdio.h>  // for GCC.
#include "FreqAnalyserGui.h"
#include <algorithm>
#include "../shared/xp_simd.h"
#include "../shared/GraphHelpers.h"

using namespace se_sdk;
using namespace gmpi;
using namespace GmpiDrawing;

GMPI_REGISTER_GUI(MP_SUB_TYPE_GUI2, FreqAnalyserGui, L"SE Freq Analyser2");
SE_DECLARE_INIT_STATIC_FILE(FreqAnalyser2_Gui);

FreqAnalyserGui::FreqAnalyserGui()
{
	initializePin(pinSpectrum, static_cast<MpGuiBaseMemberPtr2>(&FreqAnalyserGui::onValueChanged));
	initializePin(pinMode, static_cast<MpGuiBaseMemberPtr2>(&FreqAnalyserGui::onModeChanged));
	initializePin(pinDbHigh, static_cast<MpGuiBaseMemberPtr2>(&FreqAnalyserGui::onModeChanged));
	initializePin(pinDbLow, static_cast<MpGuiBaseMemberPtr2>(&FreqAnalyserGui::onModeChanged));
}

void FreqAnalyserGui::onValueChanged()
{
	geometry = nullptr;
	invalidateRect();
}

void FreqAnalyserGui::onModeChanged()
{
	cachedBackground_ = nullptr;
	onValueChanged();
}

#define USE_CACHED_BACKGROUND 1

float calcTopFrequency(float samplerate)
{
	return floorf(samplerate / 20000.0f) * 10000.0f;
}

int32_t FreqAnalyserGui::OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext)
{
	auto r = getRect();

	GmpiDrawing::Graphics g(drawingContext);
	ClipDrawingToBounds cd(g, r);

	constexpr float displayOctaves = 10;
	float displayDbRange = (float)(std::max)(10, pinDbHigh - pinDbLow);
	float displayDbMax = (float) pinDbHigh;

	const float snapToPixelOffset = 0.5f;

	float width = r.right - r.left;
	float height = r.bottom - r.top;

	float scale = height * 0.46f;
	float mid_x = floorf(0.5f + width * 0.5f);
	float mid_y = floorf(0.5f + height * 0.5f);

	float leftBorder = 22;
	float rightBorder = width - leftBorder * 0.5f;
	float bottomBorder = leftBorder;
	float graphWidth = rightBorder - leftBorder;

	int newSpectrumCount = -1 + pinSpectrum.rawSize() / sizeof(float);

	auto capturedata = (const float*) pinSpectrum.rawData();

	if (newSpectrumCount > 0)
	{
		if (samplerate != capturedata[spectrumCount] || spectrumCount != newSpectrumCount)
		{
			spectrumCount = newSpectrumCount;
			samplerate = capturedata[spectrumCount]; // last entry used for sample-rate.
			pixelToBin.clear();

			//for (int i = 20; i < 25; ++i)
			//	_RPTN(0, "bin %d : %f\n", i, capturedata[i]);
		}
	}

	// Invalidate background if SRT changes.
	if( currentBackgroundSampleRate != samplerate )
		cachedBackground_ = nullptr;

#if USE_CACHED_BACKGROUND
	if (cachedBackground_.isNull())
	{
		auto dc = g.CreateCompatibleRenderTarget(Size(r.getWidth(), r.getHeight()));
		dc.BeginDraw();
#else
	{
		auto& dc = g;
#endif
		currentBackgroundSampleRate = samplerate;

		auto dtextFormat = dc.GetFactory().CreateTextFormat(10); // GetTextFormat(getHost(), getGuiHost(), "tty", &typeface_);

		dtextFormat.SetTextAlignment(TextAlignment::Leading); // Left
		dtextFormat.SetParagraphAlignment(ParagraphAlignment::Center);
		dtextFormat.SetWordWrapping(WordWrapping::NoWrap); // prevent word wrapping into two lines that don't fit box.

		auto gradientBrush = dc.CreateLinearGradientBrush(Color::FromRgb(0x39323A), Color::FromRgb(0x080309), Point(0, 0), Point(0, height) );
		dc.FillRectangle(r, gradientBrush);

		auto fontBrush = dc.CreateSolidColorBrush(Color::Gold);
		auto brush2 = dc.CreateSolidColorBrush(Color::Gray);
		float penWidth = 1.0f;
		auto fontHeight = dtextFormat.GetTextExtentU("M").height;

		// dB labels.
		float lastTextY = -10;
		if (height > 30)
		{
			float db = displayDbMax;
			float y = 0;
			while (true)
			{
				y = (displayDbMax - db) * (height - bottomBorder) / displayDbRange;
				y = snapToPixelOffset + floorf(0.5f + y);

				if (y >= height - fontHeight)
				{
					break;
				}

				GraphXAxisYcoord = y;

				dc.DrawLine(GmpiDrawing::Point(leftBorder, y), GmpiDrawing::Point(rightBorder, y), brush2, penWidth);

				if (y > lastTextY + fontHeight * 1.2)
				{
					lastTextY = y;
					char txt[10];
					sprintf(txt, "%3.0f", (float)db);

					//				TextOut(hDC, 0, y - fontHeight / 2, txt, (int)wcslen(txt));
					GmpiDrawing::Rect textRect(0, y - fontHeight / 2, 30, y + fontHeight / 2);
					dc.DrawTextU(txt, dtextFormat, textRect, fontBrush);
				}

				db -= 10.0f;
			}

			// extra line at -3dB. To help check filter cuttoffs.
			db = -3.f;
			y = (displayDbMax - db) * (height - bottomBorder) / displayDbRange;
			y = snapToPixelOffset + floorf(0.5f + y);

			dc.DrawLine(GmpiDrawing::Point(leftBorder, y), GmpiDrawing::Point(rightBorder, y), brush2, penWidth);
		}

		if (pinMode == 0) // Log
		{
			// FREQUENCY LABELS
			// Highest mark is nyquist rounded to nearest 10kHz.
			const float topFrequency = calcTopFrequency(samplerate);
			float frequencyStep = 1000.0;
			if (width < 500)
			{
				frequencyStep = 10000.0;
			}
			float hz = topFrequency;
			float previousTextLeft = INT_MAX;
			float x = INT_MAX;
			do {
				const float octave = displayOctaves - logf(topFrequency / hz) / logf(2.0f);

				x = leftBorder + graphWidth * octave / displayOctaves;

				// hmm, can be misleading when grid line is one pixel off due to snapping
//				x = snapToPixelOffset + floorf(0.5f + x);

				if (x <= leftBorder || hz < 5.0)
					break;

				bool extendLine = false;

				// Text.
				if (samplerate > 0)
				{
					char txt[10];

					// Large values printed in kHz.
					if (hz > 999.0)
					{
						sprintf(txt, "%.0fk", hz * 0.001);
					}
					else
					{
						sprintf(txt, "%.0f", hz);
					}

					//				int stringLength = strlen(txt);
					//SIZE size;
					//::GetTextExtentPoint32(hDC, txt, stringLength, &size);
					auto size = dtextFormat.GetTextExtentU(txt);
					// Ensure text don't overwrite text to it's right.
					if (x + size.width / 2 < previousTextLeft)
					{
						extendLine = true;
						//					TextOut(hDC, x, height - fontHeight, txt, stringLength);

						GmpiDrawing::Rect textRect(x - size.width / 2, height - fontHeight, x + size.width / 2, height);
						dc.DrawTextU(txt, dtextFormat, textRect, fontBrush);

						previousTextLeft = x - (2 * size.width) / 3; // allow for text plus whitepace.
					}
				}

				// Vertical line.
				float lineBottom = height - fontHeight;
				if (!extendLine)
				{
					lineBottom = GraphXAxisYcoord;
				}
				dc.DrawLine(GmpiDrawing::Point(x, 0), GmpiDrawing::Point(x, lineBottom), brush2, penWidth);

				//if (hz > 950 && hz < 1050)
				//{
				//	_RPTN(0, "1k line @ %f\n", x);
				//}

				if (frequencyStep > hz * 0.99f)
				{
					frequencyStep *= 0.1f;
				}

				hz = hz - frequencyStep;

			} while ( true);
		}
		else
		{
			// FREQUENCY LABELS
			// Highest mark is nyquist rounded to nearest 2kHz.
			float topFrequency = floor(samplerate / 2000.0f) * 1000.0f;
			float frequencyStep = 1000.0;
			float hz = topFrequency;
			float previousTextLeft = INT_MAX;
			float x = INT_MAX;
			do {
				x = leftBorder + (2.0f * hz * graphWidth) / samplerate;
				x = snapToPixelOffset + floorf(0.5f + x);

				if (x <= leftBorder || hz < 5.0)
					break;

				bool extendLine = false;

				// Text.
				if (samplerate > 0)
				{
					char txt[10];

					// Large values printed in kHz.
					if (hz > 999.0)
					{
						sprintf(txt, "%.0fk", hz * 0.001);
					}
					else
					{
						sprintf(txt, "%.0f", hz);
					}

					auto size = dtextFormat.GetTextExtentU(txt);
					// Ensure text don't overwrite text to it's right.
					if (x + size.width / 2 < previousTextLeft)
					{
						extendLine = true;
						//					TextOut(hDC, x, height - fontHeight, txt, stringLength);

						GmpiDrawing::Rect textRect(x - size.width / 2, height - fontHeight, x + size.width / 2, height);
						dc.DrawTextU(txt, dtextFormat, textRect, fontBrush);

						previousTextLeft = x - (2 * size.width) / 3; // allow for text plus whitepace.
					}
				}

				// Vertical line.
				float lineBottom = height - fontHeight;
				if (!extendLine)
				{
					lineBottom = GraphXAxisYcoord;
				}
				dc.DrawLine(GmpiDrawing::Point(x, 0), GmpiDrawing::Point(x, lineBottom), brush2, penWidth);

				hz = hz - frequencyStep;

			} while (true);
		}
#if USE_CACHED_BACKGROUND
		dc.EndDraw();
		cachedBackground_ = dc.GetBitmap();
#endif
	}

#if USE_CACHED_BACKGROUND
	g.DrawBitmap(cachedBackground_, Point(0, 0), r);
#endif

	if (spectrumCount < 1 || newSpectrumCount < 1)
		return MP_OK;

	if(geometry.isNull())
	{
        auto factory = g.GetFactory();
   
		geometry = factory.CreatePathGeometry();
		auto sink = geometry.Open();

		lineGeometry = factory.CreatePathGeometry();
		auto lineSink = lineGeometry.Open();

		float x,y;
		sink.BeginFigure(leftBorder, GraphXAxisYcoord, FigureBegin::Filled);
		float inverseN = 2.0f / spectrumCount;
		const float dbc = 20.0f * log10f(inverseN);
		if (pinMode == 0) // Log
		{
			// precalculate log frequency scale
			if (pixelToBin.empty())
			{
				const float xScale2 = displayOctaves / graphWidth;
				float hz2bin = spectrumCount / (0.5f * samplerate);
				const float xScale = graphWidth / displayOctaves;
				const float inv_log2 = 1.0f / logf(2.0f);

				const float topFrequency = calcTopFrequency(samplerate);

				for (float x = 0.0f; x < graphWidth; x += 0.25f)
				{
					const float octave = x * xScale2;
					const float hz = topFrequency / powf(2.0f, displayOctaves - octave);// *lowHz;
					const float bin = hz * hz2bin;// -1.0f;
					const float safeBin = std::clamp(bin, 0.f, (float)spectrumCount - 1.0f);

					const int index = FastRealToIntTruncateTowardZero(safeBin);
					const float fraction = safeBin - (float)index;
					pixelToBin.push_back({ index, fraction });

					//if (hz > 998 && hz < 1002)
					//{
					//	_RPTN(0, "Hz %f, bin %f\n", hz, safeBin);
					//}
				}
			}

			graphValues.clear();

            graphValues.push_back({leftBorder, 0.0f}); // we will update y once we have it
            
			// convert spectrum to dB
			std::vector<float> dbs;
			dbs.reserve(spectrumCount);

			const float safeMinAmp = powf(10.0f, -300.0f * 0.1f);
			for (int i = 0; i < spectrumCount; ++i)
			{
				// 10 is wrong? should be 20????
				const auto db = 10.f * log10((std::max)(safeMinAmp, capturedata[i])) + dbc;
				assert(!isnan(db));
				dbs.push_back(db);
			}

			const int minIndex = 0;
			const int maxIndex = spectrumCount - 1;
			float x = leftBorder;
			for (const auto& [index, fraction]: pixelToBin)
			{
				const double y0 = dbs[(std::max)(minIndex, index - 1)];
				const double y1 = dbs[(std::min)(maxIndex, index + 0)];
				const double y2 = dbs[(std::min)(maxIndex, index + 1)];
				const double y3 = dbs[(std::min)(maxIndex, index + 2)];

				const auto db = (y1 + 0.5 * fraction * (y2 - y0 + fraction * (2.0 * y0 - 5.0 * y1 + 4.0 * y2 - y3 + fraction * (3.0 * (y1 - y2) + y3 - y0))));

				y = (displayDbMax - db) * (height - bottomBorder) / displayDbRange;
				y = (std::min)(y, GraphXAxisYcoord - 0.01f); // mac can't handle adjacent identical points, ensure no point exactly on axis at ends

				graphValues.push_back({x, y});

				//if (bin.index > 22 && bin.index < 24)
				//{
				//	_RPTN(0, "bin %f: x %f\n", bin.index + bin.fraction, x);
				//}

				x += 0.25f;
			}

            // extend line left to border
            graphValues[0].y = graphValues[1].y;
            
            // extend line to right border
            graphValues.push_back({rightBorder, graphValues.back().y});
            
			SimplifyGraph(graphValues, responseOptimized);

            assert(responseOptimized.size() > 1);
                       
            bool first = true;
			for (auto& p : responseOptimized)
			{
                if(first)
                {
                    lineSink.BeginFigure(responseOptimized[0]);
                    first = false;
                }
                else
                {
       				lineSink.AddLine(p);
                }
                
				sink.AddLine(p);
			}

            // complete filled area down to axis
			sink.AddLine(GmpiDrawing::Point(responseOptimized.back().x, GraphXAxisYcoord));
		}
		else // Linear frequency
		{
			for (int i = 1; i < spectrumCount; i++)
			{
				x = leftBorder + (i * graphWidth) / spectrumCount;
//					float db = 20.f * log10f(sqrtf(*capturedata) * inverseN); // square root same as division by 2 in log space.
//					float db = 20.f * (0.5 * log10f(*capturedata) + log10f(inverseN)); // simplify
//					float db = 10.f * log10f(*capturedata) + 20.0 * log10f(inverseN); // extract constant part.
				float db = 10.f * log10f(*capturedata) + dbc;

				y = (displayDbMax - db) * (height - bottomBorder) / displayDbRange;
				y = (std::min)(y, height - bottomBorder);

                const GmpiDrawing::Point p(x, y);
				if (i == 1)
				{
					lineSink.BeginFigure(p);
				}
                else
                {
                    lineSink.AddLine(p);
                }
				sink.AddLine(p);

				++capturedata;
			}

			sink.AddLine(GmpiDrawing::Point(x, GraphXAxisYcoord));
		}

		lineSink.EndFigure(FigureEnd::Open);
		lineSink.Close();
		sink.EndFigure();
		sink.Close();
	}

	auto graphColor = Color::FromArgb(0xFF65B1D1);
	auto brush2 = g.CreateSolidColorBrush(graphColor);
	const float penWidth = 1;
	Color fill = Color::FromArgb(0xc08BA7BF);

	auto plotClip = r;
	plotClip.left = leftBorder;
	plotClip.bottom = GraphXAxisYcoord;
	ClipDrawingToBounds cd2(g, plotClip);

	g.FillGeometry(geometry, g.CreateSolidColorBrush(fill));
	g.DrawGeometry(lineGeometry, brush2, penWidth);

	return gmpi::MP_OK;
}

int32_t FreqAnalyserGui::measure(GmpiDrawing_API::MP1_SIZE availableSize, GmpiDrawing_API::MP1_SIZE* returnDesiredSize)
{
	const float minSize = 15;

	returnDesiredSize->width = (std::max)(minSize, availableSize.width);
	returnDesiredSize->height = (std::max)(minSize, availableSize.height);

	return gmpi::MP_OK;
}

int32_t FreqAnalyserGui::arrange(GmpiDrawing_API::MP1_RECT finalRect)
{
	cachedBackground_.setNull();
	pixelToBin.clear();

#ifdef DRAW_LINES_ON_BITMAP
	foreground_.setNull();
#endif
	return MpGuiGfxBase::arrange(finalRect);
}
