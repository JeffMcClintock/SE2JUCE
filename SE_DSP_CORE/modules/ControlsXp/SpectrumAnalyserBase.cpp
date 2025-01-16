#include "./SpectrumAnalyserBase.h"

void SpectrumAnalyserBase::updateSpectrumGraph(int width, int height)
{
	if (rawSpectrum.size() < 10)
		return;

	const int spectrumCount = static_cast<int>(rawSpectrum.size());

	constexpr int chunkyness = 2; // how much smoothing to apply to noisy data on the graph.
	if (pixelToBin.empty())
	{
		InixPixelToBin(pixelToBin, width, spectrumCount, sampleRateFft);

		smoothedZoneHigh = linearZoneHigh = pixelToBin.size() - 3;

		float bin_per_pixel = 0;
		for (int i = 10; i < pixelToBin.size() - 4; i++)
		{
			const auto& e1 = pixelToBin[i - 1];
			const auto& e2 = pixelToBin[i];
			const float b1 = e1.index + e1.fraction;
			const float b2 = e2.index + e2.fraction;
			const float distance = b2 - b1;

			if (distance >= bin_per_pixel)
			{
				_RPTN(0, "bin_per_pixel %d: %d\n", (int)bin_per_pixel, i);
				if (bin_per_pixel == 1)
				{
					smoothedZoneHigh = i;
				}
				else if (bin_per_pixel == 2)
				{
					linearZoneHigh = i;
				}

				++bin_per_pixel;
			}
		}

		// mark all bins required in final graph. saves calcing db then discarding it.
		dbUsed.assign(spectrumCount + 1, false);
		for (int i = 1; i < pixelToBin.size() - 3; ++i)
		{
			auto& pb = pixelToBin[i];

			if (pb.index > linearZoneHigh)
			{
				dbUsed[pb.index] = true;
			}
			else if (pb.index > smoothedZoneHigh)
			{
				dbUsed[pb.index] = true;
				dbUsed[pb.index + 1] = true;
			}
			else
			{
				dbUsed[pb.index - 1] = true;
				dbUsed[pb.index + 0] = true;
				dbUsed[pb.index + 1] = true;
				dbUsed[pb.index + 2] = true;
			}
		}
	}

	const float inverseN = 2.0f / spectrumCount;
	const float dbc = 20.0f * log10f(inverseN);

	constexpr float displayDbTop = 6.0f;
	constexpr float displayDbBot = -120.0f;

	constexpr float clipDbAtBottom = displayDbBot - 5.0f; // -5 to have flat grph just off the bottom
	const float safeMinAmp = powf(10.0f, (clipDbAtBottom - dbc) * 0.1f);

	// convert spectrum to dB
	const float* capturedata = rawSpectrum.data();

	if (dbs.size() != spectrumCount + 1)
		dbs.assign(spectrumCount + 1, -300.0f);

	for (int i = 0; i < linearZoneHigh; ++i)
	{
		const int index = i + 1;

		if (!dbUsed[index])
			continue;

		float db;
		if (capturedata[i] <= safeMinAmp)
		{
			// save on expensive log10 call if signal is so quiet that it's off the bottom of the graph anyhow.
			db = clipDbAtBottom;
		}
		else
		{
			// 10 is wrong? should be 20????
			db = 10.f * log10(capturedata[i]) + dbc;
			assert(!isnan(db));
		}

		dbs[index] = (std::max)(db, dbs[index]);
	}

	dbs[0] = dbs[1]; // makes interpolation easier to have a dummy value at left.

	const float dbScaler = -height / (displayDbTop - displayDbBot);

	graphValues.clear();

	// calc the leftmost cubic-smoothed section
	float x = 0.0f; // lmargin;
	constexpr int audioGraphDensity = 1;
	int i = 0;
	for (; i < smoothedZoneHigh; i++)
	{
		const auto& [index
			, fraction
#ifdef _DEBUG
			, hz
#endif
		] = pixelToBin[i];

		assert(index >= 0);
		assert(index + 2 < dbs.size());

		assert(dbUsed[index - 1] && dbUsed[index] && dbUsed[index + 1] && dbUsed[index + 2]);

		const float y0 = dbs[index - 1];
		const float y1 = dbs[index + 0];
		const float y2 = dbs[index + 1];
		const float y3 = dbs[index + 2];

		const auto db = (y1 + 0.5f * fraction * (y2 - y0 + fraction * (2.0f * y0 - 5.0f * y1 + 4.0f * y2 - y3 + fraction * (3.0f * (y1 - y2) + y3 - y0))));

		const auto y = (db - displayDbTop) * dbScaler;

		graphValues.push_back({ x, y });

		x += audioGraphDensity;
	}

#if 0
	for (; i < pixelToBin.size() - 3; i++)
	{
		const auto& [index
			, fraction
#ifdef _DEBUG
			, hz
#endif
		] = pixelToBin[i];

		assert(index >= 0);
		assert(index + 2 < dbs.size());

		assert(dbUsed[index] && dbUsed[index + 1]);

		const auto& y0 = dbs[index + 0];
		const auto& y1 = dbs[index + 1];

		const auto db = y0 + (y1 - y0) * fraction;
		//		const auto y = -db * dbScaler;
		const auto y = (db - displayDbTop) * dbScaler;

		graphValues.push_back({ x, y });

		x += audioGraphDensity;
	}
#else

	// calc the 2-point interpolated section
	for (; i < linearZoneHigh; i++)
	{
		const auto& [index
			, fraction
#ifdef _DEBUG
			, hz
#endif
		] = pixelToBin[i];

		assert(index >= 0);
		assert(index + 2 < dbs.size());

		assert(dbUsed[index] && dbUsed[index + 1]);

		const auto& y0 = dbs[index + 0];
		const auto& y1 = dbs[index + 1];

		const auto db = y0 + (y1 - y0) * fraction;
		const auto y = (db - displayDbTop) * dbScaler;

		graphValues.push_back({ x, y });

		x += audioGraphDensity;
	}

	// calc the 'max' section where we just take the maximum value of several bins.
	{
		int bin = pixelToBin[i - 1].index;

		for (; i < pixelToBin.size() - 3; i += chunkyness)
		{
			const auto toBin = pixelToBin[i].index;
			const float maximumAmp = *std::max_element(capturedata + bin, capturedata + toBin);

			float db;
			if (maximumAmp <= safeMinAmp)
			{
				// save on expensive log10 call if signal is so quiet that it's off the bottom of the graph anyhow.
				db = clipDbAtBottom;
			}
			else
			{
				db = 10.f * log10(maximumAmp) + dbc;
				assert(!isnan(db));
			}

			assert(dbUsed[bin]);
			dbs[bin] = (std::max)(db, dbs[bin]);

			const auto y = (dbs[bin] - displayDbTop) * dbScaler;

			graphValues.push_back({ x, y });

			x += audioGraphDensity * chunkyness;
			bin = toBin;
		}
	}

#endif

	// PEAK HOLD
	if (peakHoldValues.size() != graphValues.size())
	{
		peakHoldValues = graphValues;
	}
	for (int j = 0; j < graphValues.size(); ++j)
	{
		peakHoldValues[j].y = (std::min)(peakHoldValues[j].y, graphValues[j].y);
	}

	SimplifyGraph(peakHoldValues, peakHoldValuesOptimized);
	SimplifyGraph(graphValues, graphValuesOptimized);

	updatePaths(graphValuesOptimized, peakHoldValuesOptimized);
}

void SpectrumAnalyserBase::clearPeaks()
{
	peakHoldValues.clear();
}

void SpectrumAnalyserBase::decayGraph(float dbDecay)
{
	constexpr float minDb = -300.f;
	for (auto& db : dbs)
	{
		db = (std::max)(minDb, db - dbDecay);
	}
}
