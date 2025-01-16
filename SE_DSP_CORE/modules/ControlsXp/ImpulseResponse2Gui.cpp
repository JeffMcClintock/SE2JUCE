#include "mp_sdk_gui2.h"
#include "Drawing.h"
#include "../shared/FontCache.h"
#include "../shared/GraphHelpers.h"

using namespace gmpi;
using namespace GmpiDrawing;

SE_DECLARE_INIT_STATIC_FILE(ImpulseResponseGui);

inline PathGeometry DataToGraph(Graphics& g, const std::vector<Point>& inData)
{
	auto geometry = g.GetFactory().CreatePathGeometry();
	auto sink = geometry.Open();
	bool first = true;
	for (const auto& p : inData)
	{
		if (first)
		{
			sink.BeginFigure(p);
			first = false;
		}
		else
		{
			sink.AddLine(p);
		}
	}

	sink.EndFigure(FigureEnd::Open);
	sink.Close();

	return geometry;
}

class ImpulseResponseGui : public gmpi_gui::MpGuiGfxBase, public FontCacheClient
{
	void clearPrecalculatedDimensions()
	{
		// Force recalc of text positions etc.
		labelXCoords.clear();
		labelText.clear();
		pixelToBin.clear();
		responseOptimized.clear();
	}

 	void onSetResults()
	{
		if (pinsampleRate.getValue() != currentSampleRate || pinResults.getValue().getSize() != currentCaptureSize)
		{
			currentSampleRate = pinsampleRate.getValue();
			currentCaptureSize = pinResults.getValue().getSize();
			clearPrecalculatedDimensions();
		}

		responseOptimized.clear();

		if (currentSampleRate < 1.0f)
			return;

		const float sampleRate = (float)pinsampleRate.getValue();
		const float* capturedata = (const float*)pinResults.rawData();
		const int spectrumCount = pinResults.rawSize() / sizeof(float);

		const auto r = getRect();
		const auto width = r.getWidth();
		const auto height = r.getHeight();
		float scale = height * 0.95f;

		const auto left = 0;
		const auto right = width - 8.f;
		const auto graphWidth = right - left;

		const int minIndex = 0;
		const int maxIndex = spectrumCount - 1;
		float x = left;
		const float dbScaler = scale / displayDbRange; // 90db Range

		// precalculate log frequency scale
		if (pixelToBin.empty() && spectrumCount > 0)
		{
			const float max_hz = sampleRate * 0.5f;
			const float lowHz = max_hz / powf(2.0f, 10.0f); // 10 octaves below max hz

			const float xScale2 = displayOctaves / graphWidth;
			float hz2bin = spectrumCount / (0.5f * sampleRate);
			const float xScale = graphWidth / displayOctaves;
			const float inv_log2 = 1.0f / logf(2.0f);

			for (float x = left; x < static_cast<float>(width); x += 0.25f)
			{
				// Octave is measured from right
				const float octave = 10.0f - (graphWidth - x) * xScale2;
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
			const auto db = 20.f * log10((std::max)(safeMinAmp, capturedata[i]));//? +dbc;
			assert(!isnan(db));
			dbs.push_back(db);
		}

		// create 'DC" value by interpolation, since it's way off
		if (spectrumCount > 2)
			dbs[0] = dbs[1] - (dbs[2] - dbs[1]);


		for (const auto& bin : pixelToBin)
		{
			const auto& index = bin.index;
			const auto& fraction = bin.fraction;

			const float y0 = dbs[(std::max)(minIndex, index - 1)];
			const float y1 = dbs[(std::min)(maxIndex, index + 0)];
			const float y2 = dbs[(std::min)(maxIndex, index + 1)];
			const float y3 = dbs[(std::min)(maxIndex, index + 2)];

			const auto db = (y1 + 0.5f * fraction * (y2 - y0 + fraction * (2.0f * y0 - 5.0f * y1 + 4.0f * y2 - y3 + fraction * (3.0f * (y1 - y2) + y3 - y0))));
			const float y = -(db - displayDbMax) * dbScaler;

			graphValues.push_back({ (float)x, y });

			x += 0.25f;
		}

		SimplifyGraph(graphValues, responseOptimized);

		invalidateRect();
	}

 	BlobGuiPin pinResults;
 	FloatGuiPin pinsampleRate;

	const float displayOctaves = 10.0f;
	const float displayDbMax = 30.0f;
	const float displayDbRange = 100.0f;

	float currentSampleRate = {};
	int currentCaptureSize = {};
	std::vector<float> labelXCoords;
	std::vector <std::string> labelText;

	struct binData
	{
		int index;
		float fraction;
	};
	std::vector<binData> pixelToBin;
	std::vector<GmpiDrawing::Point> graphValues;
	std::vector<GmpiDrawing::Point> responseOptimized;

public:
	ImpulseResponseGui()
	{
		initializePin( pinResults, static_cast<MpGuiBaseMemberPtr2>(&ImpulseResponseGui::onSetResults) );
		initializePin( pinsampleRate, static_cast<MpGuiBaseMemberPtr2>(&ImpulseResponseGui::onSetResults) );
	}

	int32_t MP_STDCALL arrange(GmpiDrawing_API::MP1_RECT finalRect) override
	{
		clearPrecalculatedDimensions();
		return MpGuiGfxBase::arrange(finalRect);
	}

	int32_t MP_STDCALL OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext ) override
	{
		Graphics g(drawingContext);

		FontMetadata* typeface_ = {};
		GetTextFormat(getHost(), getGuiHost(), "tty", &typeface_); // get typeface info, like color etc.
		auto dtextFormat = g.GetFactory().CreateTextFormat(10);
		dtextFormat.SetTextAlignment(TextAlignment::Leading); // Left
		dtextFormat.SetParagraphAlignment(ParagraphAlignment::Center);
		dtextFormat.SetWordWrapping(WordWrapping::NoWrap); // prevent word wrapping into two lines that don't fit box.

		// Processor send an odd number of samples when in "Impulse" mode (not "Frequency Response" mode).
		const bool rawImpulseMode = (pinResults.rawSize() & 1) == 1;

		const auto r = getRect();
		const auto width = r.getWidth();
		const auto height = r.getHeight();
		float scale = height * 0.95f;

		const auto left = 0;
		const auto right = width - 8.f;
		const auto graphWidth = right - left;

		//float inverseN = 2.0f / spectrumCount;
		//const float dbc = 20.0f * log10f(inverseN);

		// recalc shortcuts if nesc
		if(!rawImpulseMode)
		{
			if (labelXCoords.empty())
			{
				const float xScale = (right - left) / displayOctaves;
				const float inv_log2 = 1.0f / logf(2.0f);
				const float max_hz = currentSampleRate * 0.5f;
				const float inv_max_hz = 1.f / max_hz;

				labelXCoords.clear();
				labelText.clear();
				if(currentSampleRate > 0.f)
				{
					// Highest mark is rounded to nearest 10kHz.
					float topFrequency = floorf(currentSampleRate / 20000.0f) * 10000.0f;
					float frequencyStep = 1000.0f;
					if (width < 500)
					{
						frequencyStep = 10000.0f;
					}
					float hz = topFrequency;
					float previousTextLeft = 100000.0f;
					int x = INT_MAX;
					do {
						const float octaveFrom20k = logf(hz * inv_max_hz) * inv_log2; // e.e at 44k, -1 octaves = 22k
						const auto x = right + octaveFrom20k * xScale;

						bool extendLine = false;

						// Text.
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
							previousTextLeft = x - (2 * size.width) / 3.0f; // allow for text plus whitepace.
							labelText.push_back(txt);
						}
						else
						{
							labelText.push_back({});
						}

						const float xSnapped = floorf(x + 0.5f) + 0.5f; // whole numbers only, plus 0.5 to center pixel.
						labelXCoords.push_back(xSnapped);

						if (frequencyStep > hz * 0.99)
						{
							frequencyStep *= 0.1;
						}

						hz = hz - frequencyStep;

					} while (x > 25 && hz > 5.0);
				}
			}
		}

		auto brush = g.CreateSolidColorBrush(Color::Red);


		// PAINT. /////////////////////////////////////////////////////////////////////////////////////////////////

		ClipDrawingToBounds x(g, r);

		auto mid_x = width * 0.5f;
		auto mid_y = height * 0.5f;
		float leftBorder = 20.f;

		if (typeface_->backgroundColor_ >= 0xff000000) // don't bother for transparent.
		{
			// Fill in solid background black
			auto background_brush = g.CreateSolidColorBrush(typeface_->backgroundColor_);
			g.FillRectangle(r, background_brush);
		}

		int fontHeight = typeface_->pixelHeight_;
		float penWidth = 1.0f;

		brush.SetColor(typeface_->color_);
		auto brush2 = g.CreateSolidColorBrush(Color::Gray);

		// Frequency labels.
		if (!rawImpulseMode)
		{
			// Highest mark is rounded to nearest 10kHz.
			float topFrequency = floorf(currentSampleRate / 20000.0f) * 10000.0f;
			float frequencyStep = 1000.0f;
			if (width < 500)
			{
				frequencyStep = 10000.0f;
			}

			for(int i = 0 ; i < labelXCoords.size() ; ++i)
			{
				const auto x = labelXCoords[i];

				// Text.
				auto size = dtextFormat.GetTextExtentU(labelText[i].c_str());

				// Ensure text don't overwrite text to it's right.
				bool extendLine = false;
				if (!labelText[i].empty())
				{
					extendLine = true;
					GmpiDrawing::Rect textRect(x - size.width / 2, height - fontHeight, x + size.width / 2, height);
					g.DrawTextU(labelText[i].c_str(), dtextFormat, textRect, brush);
				}

				// Vertical line.
				float lineBottom = height - fontHeight;
				if (!extendLine)
				{
					lineBottom -= 2;
				}
				g.DrawLine(GmpiDrawing::Point(x, 0.f), GmpiDrawing::Point(x, lineBottom), brush2, penWidth);
			}

			// dB labels.
			if (height > 30)
			{
				float db = displayDbMax;
				float y = 0;
				while (y < height)
				{
					y = (displayDbMax - db) * scale / displayDbRange;
					const float ySnapped = floorf(y + 0.5f) + 0.5f; // whole numbers only, plus 0.5 to center pixel.

					g.DrawLine(GmpiDrawing::Point(leftBorder, ySnapped), GmpiDrawing::Point(width, ySnapped), brush2, penWidth);

					char txt[10];
					sprintf(txt, "%2.0f", (float)db);

					GmpiDrawing::Rect textRect(0, y - fontHeight / 2, 30, y + fontHeight / 2);
					g.DrawTextU(txt, dtextFormat, textRect, brush);

					db -= 10.0f;
				}

				// extra line at -3dB. To help check filter cuttoffs.
				db = -3.f;
				y = (displayDbMax - db) * scale / displayDbRange;
				const float ySnapped = floorf(y + 0.5f) + 0.5f; // whole numbers only, plus 0.5 to center pixel.

				g.DrawLine(GmpiDrawing::Point(leftBorder, ySnapped), GmpiDrawing::Point(width, ySnapped), brush2, penWidth);
			}
		}
		else
		{
			// Impulse mode.

			// Zero-line
			g.DrawLine(GmpiDrawing::Point(leftBorder, mid_y), GmpiDrawing::Point(width, mid_y), brush2, penWidth);

			// volt ticks
			for (int v = 10; v >= -10; --v)
			{
				const auto y = floorf(0.5f + mid_y - (scale * v) * 0.1f);
				g.DrawLine(GmpiDrawing::Point(0, y), GmpiDrawing::Point(leftBorder, y), brush2, penWidth);
			}
		}

		if(pinResults.rawSize() < 4)
			return gmpi::MP_OK;

		// trace
		if (!rawImpulseMode)
		{
			if (!responseOptimized.empty())
			{
				brush2.SetColor(Color::FromBytes(0, 255, 0));
				auto traceGeometry2 = DataToGraph(g, responseOptimized);
				g.DrawGeometry(traceGeometry2, brush2, penWidth);
			}
		}
		else
		{
			brush2.SetColor(Color::FromBytes(0, 255, 0)); // bright green

			auto factory = g.GetFactory();

			// lines
			auto lineGeometry = factory.CreatePathGeometry();
			auto lineSink = lineGeometry.Open();

			const float* v = (float*)pinResults.rawData();
			int count = pinResults.rawSize() / sizeof(float);
			float dbScaler = scale;

			for (int i = 0; i < count; ++i)
			{
				float x = 20.f + i;
				float y = mid_y - *v * scale;
				if (i == 0)
				{
					lineSink.BeginFigure(x, y, FigureBegin::Filled);
				}
				else
				{
					lineSink.AddLine(GmpiDrawing::Point(x, y));
				}
				++v;
			}

			lineSink.EndFigure(FigureEnd::Open);
			lineSink.Close();
			g.DrawGeometry(lineGeometry, brush2, penWidth);
		}

		return gmpi::MP_OK;
	}
};

namespace
{
	auto r = Register<ImpulseResponseGui>::withId(L"SE Impulse Response2");
}
