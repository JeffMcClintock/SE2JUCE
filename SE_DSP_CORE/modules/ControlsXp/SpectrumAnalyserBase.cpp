#include "./SpectrumAnalyserBase.h"

void SpectrumAnalyserBase::updateSpectrumGraph(int width, int height)
{
	if (rawSpectrum.size() < 10 || sampleRateFft< 1.0f)
		return;

	const int spectrumCount2 = static_cast<int>(rawSpectrum.size() - 1);

	if (pixelToBin.empty())
	{
		InixPixelToBin(pixelToBin, width, spectrumCount2, sampleRateFft);

		smoothedZoneHigh = linearZoneHigh = pixelToBin.size() - 3;

		float bin_per_pixel = 0.8f;
		for (int i = 10; i < pixelToBin.size() - 4; i++)
		{
			const auto& e1 = pixelToBin[i - 1];
			const auto& e2 = pixelToBin[i];
			const float b1 = e1.index + e1.fraction;
			const float b2 = e2.index + e2.fraction;
			const float distance = b2 - b1;

			if (distance >= bin_per_pixel)
			{
				// _RPTN(0, "bin_per_pixel %d: %d\n", (int)bin_per_pixel, i);
				if (bin_per_pixel <= 1.0f)
				{
					smoothedZoneHigh = i;
				}
				else
				{
					linearZoneHigh = i;
					break;
				}
				++bin_per_pixel;
			}
		}

		// mark all bins required in final graph. saves calcing db then discarding it.
		dbUsed.assign(spectrumCount2 + 2, false);
		for (int i = 0; i < pixelToBin.size() - 3; ++i)
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

	constexpr float displayDbTop = 6.0f;
	constexpr float displayDbBot = -120.0f;
	constexpr float clipDbAtBottom = displayDbBot - 5.0f; // -5 to have flat grph just off the bottom
	const float inverseN = 2.0f / spectrumCount2;
	const float dbc = 20.0f * log10f(inverseN);
	const float safeMinAmp = powf(10.0f, (clipDbAtBottom - dbc) * 0.1f);
	const float* capturedata = rawSpectrum.data();

	if (spectrumUpdateFlag)
	{
		// convert spectrum to dB
		if (dbs_in.size() != spectrumCount2 + 2)
		{
			dbs_in  .assign(spectrumCount2 + 2, -300.0f);
			dbs_disp.assign(spectrumCount2 + 2, -300.0f);
		}

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

			dbs_in[index] = db;
			dbs_disp[index] = (std::max)(db, dbs_disp[index]);
		}

		// for the dense part of the graph at right, just find local maxima of small groups. Then do the expensive conversion to dB.
		{
			// more smoothing to noisy data on the high end of the graph.
			int dx = 4;
			int i = linearZoneHigh;
			auto fromBin = (pixelToBin[i - 1].index + pixelToBin[i].index) / 2;
			for (; i < pixelToBin.size() - 3; i += dx)
			{
				const auto toBin = (pixelToBin[i].index + pixelToBin[i + 1].index) / 2;
				const float maximumAmp = *std::max_element(capturedata + fromBin, capturedata + toBin);

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

				const auto dbBin = pixelToBin[i].index;
				assert(dbUsed[dbBin]);

				dbs_in[dbBin] = db;
				dbs_disp[dbBin] = (std::max)(db, dbs_disp[dbBin]);

				fromBin = toBin;
			}
		}
		dbs_disp[0] = dbs_disp[1]; // makes interpolation easier to have a dummy value at left.

		dbToPixel = -height / (displayDbTop - displayDbBot);
	}

	graphValues.clear();

	// calc the leftmost cubic-smoothed section
	int x = 0;
	int dx = 2;
#ifdef _DEBUG
	dx = 1;
#endif
	for (; x < smoothedZoneHigh; x += dx)
	{
		const auto& [index
			, fraction
#ifdef _DEBUG
			, hz
#endif
		] = pixelToBin[x];

		assert(index >= 0);
		assert(index + 2 < dbs_in.size());

		assert(dbUsed[index - 1] && dbUsed[index] && dbUsed[index + 1] && dbUsed[index + 2]);

		const float y0 = dbs_disp[index - 1];
		const float y1 = dbs_disp[index + 0];
		const float y2 = dbs_disp[index + 1];
		const float y3 = dbs_disp[index + 2];

		const auto db = (y1 + 0.5f * fraction * (y2 - y0 + fraction * (2.0f * y0 - 5.0f * y1 + 4.0f * y2 - y3 + fraction * (3.0f * (y1 - y2) + y3 - y0))));

		const auto y = (db - displayDbTop) * dbToPixel;

		graphValues.push_back({ static_cast<float>(x), y });
	}

	// calc the 2-point interpolated section
	for (; x < linearZoneHigh; x += dx)
	{
		const auto& [index
			, fraction
#ifdef _DEBUG
			, hz
#endif
		] = pixelToBin[x];

		assert(index >= 0);
		assert(index + 2 < dbs_in.size());

		assert(dbUsed[index] && dbUsed[index + 1]);

		const auto& y0 = dbs_disp[index + 0];
		const auto& y1 = dbs_disp[index + 1];

		const auto db = y0 + (y1 - y0) * fraction;
		const auto y = (db - displayDbTop) * dbToPixel;

		graphValues.push_back({ static_cast<float>(x), y });
	}

	// calc the 'max' section where we just take the maximum value of several bins.
	{
		// more smoothing to noisy data on the high end of the graph.
		dx = 4;
		assert(width <= pixelToBin.size());
		for (; x < width; x += dx)
		{
			const auto dbBin = pixelToBin[x].index;
			assert(dbUsed[dbBin]);

			const auto db = dbs_disp[dbBin];
			const auto y = (db - displayDbTop) * dbToPixel;

			graphValues.push_back({ static_cast<float>(x), y });
		}
	}

	// PEAK HOLD
	if (peakHoldValues.size() != graphValues.size())
	{
		peakHoldValues = graphValues;
	}
	if (spectrumUpdateFlag)
	{
		for (int j = 0; j < graphValues.size(); ++j)
		{
			peakHoldValues[j].y = (std::min)(peakHoldValues[j].y, graphValues[j].y);
		}

		SimplifyGraph(peakHoldValues, peakHoldValuesOptimized);
	}

	SimplifyGraph(graphValues, graphValuesOptimized);

	updatePaths(graphValuesOptimized, peakHoldValuesOptimized);

	spectrumUpdateFlag = false;
	spectrumDecayFlag = false;
}

void SpectrumAnalyserBase::clearPeaks()
{
	peakHoldValues.clear();
}

void SpectrumAnalyserBase::decayGraph(float dbDecay)
{
	for (int i = 0 ; i < dbs_in.size(); ++i)
	{
		dbs_disp[i] = (std::max)(dbs_in[i], dbs_disp[i] - dbDecay);
	}
	spectrumDecayFlag = true;
}
