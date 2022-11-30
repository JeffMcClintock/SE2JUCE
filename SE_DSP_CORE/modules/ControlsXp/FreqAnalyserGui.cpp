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
	float bottomBorder = 22;
	float graphWidth = width - leftBorder;

	int newSpectrumCount = -1 + pinSpectrum.rawSize() / sizeof(float);

	auto capturedata = (const float*) pinSpectrum.rawData();

	if (newSpectrumCount > 0)
	{
		if (samplerate != capturedata[spectrumCount] || spectrumCount != newSpectrumCount)
		{
			spectrumCount = newSpectrumCount;
			samplerate = capturedata[spectrumCount]; // last entry used for sample-rate.
			pixelToBin.clear();
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

		auto dtextFormat = g.GetFactory().CreateTextFormat(10); // GetTextFormat(getHost(), getGuiHost(), "tty", &typeface_);

		dtextFormat.SetTextAlignment(TextAlignment::Leading); // Left
		dtextFormat.SetParagraphAlignment(ParagraphAlignment::Center);
		dtextFormat.SetWordWrapping(WordWrapping::NoWrap); // prevent word wrapping into two lines that don't fit box.

		auto gradientBrush = g.CreateLinearGradientBrush(Color::FromRgb(0x39323A), Color::FromRgb(0x080309), Point(0, 0), Point(0, height) );
//		auto gradientBrush = g.CreateSolidColorBrush(Color::Transparent());
		dc.FillRectangle(r, gradientBrush);

		auto fontBrush = g.CreateSolidColorBrush(Color::Gold);
		auto brush2 = g.CreateSolidColorBrush(Color::Gray);
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

				dc.DrawLine(GmpiDrawing::Point(leftBorder, y), GmpiDrawing::Point(width, y), brush2, penWidth);

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

			dc.DrawLine(GmpiDrawing::Point(leftBorder, y), GmpiDrawing::Point(width, y), brush2, penWidth);
		}

		if (pinMode == 0) // Log
		{
			// FREQUENCY LABELS
			// Highest mark is nyquist rounded to nearest 10kHz.
			float topFrequency = floor(samplerate / 20000.0f) * 10000.0f;
			float frequencyStep = 1000.0;
			if (width < 500)
			{
				frequencyStep = 10000.0;
			}
			float hz = topFrequency;
			float previousTextLeft = INT_MAX;
			float x = INT_MAX;
			do {
				float normalizedFrequency = 2.0f * hz / samplerate;
				float octave = (log(normalizedFrequency) - log(1.0f)) / log(2.0f);
				x = leftBorder + ((octave + displayOctaves) * graphWidth) / displayOctaves;
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

				if (frequencyStep > hz * 0.99)
				{
					frequencyStep *= 0.1;
				}

				hz = hz - frequencyStep;

			} while ( true);
		}
		else
		{
			// FREQUENCY LABELS
			// Highest mark is nyquist rounded to nearest 1kHz.
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

	auto factory = g.GetFactory();

	if(geometry.isNull())
	{
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
				constexpr float lowHz = 20.f;
				constexpr float inv_lowHz = 1.0f / lowHz;
				const float xScale2 = displayOctaves / graphWidth;
				float hz2bin = spectrumCount / (0.5f * samplerate);
				const float xScale = graphWidth / displayOctaves;
				const float inv_log2 = 1.0f / logf(2.0f);
#if 0
				labelCoords.clear();
				for (size_t i = 0; i < std::size(scaleMarkHz); ++i)
				{
					const float octaveFrom20 = logf(scaleMarkHz[i] * inv_lowHz) * inv_log2;
					auto x = left + octaveFrom20 * xScale;
					x = floorf(x + 0.5f) + 0.5f; // snap to pixels
					labelCoords.push_back(x);
				}
#endif
				for (float x = 0.0f; x < static_cast<float>(width - leftBorder); x += 0.25f)
				{
					const float octave = x * xScale2;
					const float hz = powf(2.0f, octave) * lowHz;
					const float bin = hz * hz2bin - 1.0f;
					const float safeBin = std::clamp(bin, 0.f, (float)spectrumCount - 1.0f);

					const int index = FastRealToIntTruncateTowardZero(safeBin);
					const float fraction = safeBin - (float)index;
					pixelToBin.push_back({ index, fraction });
				}
			}

			graphValues.clear();

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
			for (const auto& bin : pixelToBin)
			{
				const auto& index = bin.index;
				const auto& fraction = static_cast<double>(bin.fraction);

				const double y0 = dbs[(std::max)(minIndex, index - 1)];
				const double y1 = dbs[(std::min)(maxIndex, index + 0)];
				const double y2 = dbs[(std::min)(maxIndex, index + 1)];
				const double y3 = dbs[(std::min)(maxIndex, index + 2)];

				const auto db = (y1 + 0.5 * fraction * (y2 - y0 + fraction * (2.0 * y0 - 5.0 * y1 + 4.0 * y2 - y3 + fraction * (3.0 * (y1 - y2) + y3 - y0))));

				y = (displayDbMax - db) * (height - bottomBorder) / displayDbRange;
				y = (std::min)(y, GraphXAxisYcoord);

				graphValues.push_back({ (float)x, y });

				x += 0.25f;
			}

			SimplifyGraph(graphValues, responseOptimized);

#if 0
			for (int i = 1; i < spectrumCount; i++)
			{
				float normalizedFrequency = (float)i / (float)spectrumCount;
				float octave = (log(normalizedFrequency) - log(1.0f)) / log(2.0f);
				x = leftBorder + ((octave + displayOctaves) * graphWidth) / displayOctaves;
				x = (std::max)(x, leftBorder);

				float db = 10.f * log10f(*capturedata) + dbc;

				y = (displayDbMax - db) * (height - bottomBorder) / displayDbRange;
				y = (std::min)(y, GraphXAxisYcoord);

				if (i == 1)
				{
					sink.AddLine(GmpiDrawing::Point(leftBorder, y));
					lineSink.BeginFigure(leftBorder, y);
				}
				sink.AddLine(GmpiDrawing::Point(x, y));
				lineSink.AddLine(GmpiDrawing::Point(x, y));

				++capturedata;
			}
#endif
			sink.AddLine(GmpiDrawing::Point(leftBorder, responseOptimized[0].y));
			lineSink.BeginFigure(leftBorder, responseOptimized[0].y);

			for (auto& p : responseOptimized)
			{
				sink.AddLine(p);
				lineSink.AddLine(p);
			}

			lineSink.AddLine(GmpiDrawing::Point(width + 1, responseOptimized.back().y));
			sink.AddLine(GmpiDrawing::Point(responseOptimized.back().x + 1, responseOptimized.back().y));
			sink.AddLine(GmpiDrawing::Point(responseOptimized.back().x + 1, GraphXAxisYcoord));
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

				if (i == 1)
				{
					sink.AddLine(GmpiDrawing::Point(leftBorder, y));
					lineSink.BeginFigure(leftBorder, y);
				}
				sink.AddLine(GmpiDrawing::Point(x, y));
				lineSink.AddLine(GmpiDrawing::Point(x, y));

				++capturedata;
			}

			lineSink.AddLine(GmpiDrawing::Point(width + 1, y));
			sink.AddLine(GmpiDrawing::Point(x + 1, y));
			sink.AddLine(GmpiDrawing::Point(x + 1, GraphXAxisYcoord));
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

/* test pixel to SRGB conversion
	auto b = g.CreateSolidColorBrush(Color::Red);
	Color c;
	for (int i = 0; i < 256; ++i)
	{
		c.r = c.g = c.b = FastGamma::srgbToLinear(i / 255.0f);
		b.SetColor(c);
		b.SetColor(Color::FromBytes(i,i,i));
		g.FillRectangle(Rect(i, 15, i + 1, 21), b);
		if ((i % 20) == 0)
		{
			b.SetColor(Color::Red);
			g.FillRectangle(Rect(i, 21, i + 1, 22), b);
		}
	}
	for (float l = 0; l < 0.05; l += 0.0001)
	{
		_RPT2(_CRT_WARN, "%f\t%f\n", (float) FastGamma::float_to_sRGB(l), 255.f * (float)FastGamma::linearToSrgb(l));
	}
*/

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
