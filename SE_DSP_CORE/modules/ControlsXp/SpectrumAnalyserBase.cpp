#include "./SpectrumAnalyserBase.h"

void SpectrumAnalyserBase::updateSpectrumGraph(int width, int height, float displayDbTop, float displayDbBot)
{
	if (rawSpectrum.size() < 10 || sampleRateFft < 1.0f)
		return;

	const int spectrumCount2 = static_cast<int>(rawSpectrum.size()) - 1;

	if (pixelToBin.empty())
	{
		InixPixelToBin(pixelToBin, width, spectrumCount2, sampleRateFft);

		// calc the zones of the graph that need cubic/linear/none interpolation
		smoothedZoneHigh = linearZoneHigh = pixelToBin.size() - 1;

		float bin_per_pixel = 0.8f;
		for (int i = 10; i < pixelToBin.size(); i++)
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

		// for the right side of graph, clump bins together. Assigning each to it's nearest pixel.
		{
			const float bin2Hz = sampleRateFft / (2.0f * spectrumCount2);
			closestPixelToBin.assign(spectrumCount2, 0);

			int closestX = 0;
			for (int bin = 0; bin < spectrumCount2; bin++)
			{
				const float hz = bin * bin2Hz;

				float closest = 1000000;
				for (int x = closestX; x < pixelToBin.size(); ++x)
				{
					const auto dist = fabs(pixelToBin[x].hz - hz);
					if (dist < closest)
					{
						closestX = x;
						closest = dist;
					}
					else if (pixelToBin[x].hz > hz) // break early rather than racing off in the wrong direction to the end of the array
					{
						break;
					}
				}

				// quantize to 2 pixels in x direction.
				const auto closestXQuantized = ((closestX - linearZoneHigh) & ~1) + linearZoneHigh;
				closestPixelToBin[bin] = closestXQuantized;
			}
		}

		// mark all bins required in final graph. saves calcing db then discarding it.
		dbUsed.assign(spectrumCount2 + 2, false);
		for (int i = 0; i < pixelToBin.size(); ++i) // refactor into 3 loops
		{
			auto& index = pixelToBin[i].index;

			dbUsed[index] = true;
			if (index > smoothedZoneHigh)
			{
				dbUsed[index + 1] = true;
			}
			else
			{
				dbUsed[(std::max)(0, index - 1)] = true;
				dbUsed[index + 1] = true;
				dbUsed[index + 2] = true;
			}
		}
	}

	//const float displayDbTop = this->pin 6.0f;
	//const float displayDbBot = -120.0f;
	const float clipDbAtBottom = displayDbBot - 5.0f; // -5 to have flat graph just off the bottom
	const float inverseN = 2.0f / spectrumCount2;
	const float dbc = 20.0f * log10f(inverseN);
	const float safeMinAmp = powf(10.0f, (clipDbAtBottom - dbc) * 0.1f);
	const float* capturedata = rawSpectrum.data();

	if (spectrumUpdateFlag)
	{
		dbToPixel = -height / (displayDbTop - displayDbBot);

		// convert spectrum to dB
		if (dbs_in.size() != spectrumCount2 + 2)
		{
			dbs_in  .assign(spectrumCount2 + 2, -300.0f);
			dbs_disp.assign(spectrumCount2 + 2, -300.0f);
		}

		for (int i = 0; i < pixelToBin[linearZoneHigh].index; ++i)
		{
			if (!dbUsed[i])
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

			dbs_in[i] = db;
			dbs_disp[i] = (std::max)(db, dbs_disp[i]);
		}

		// for the dense part of the graph at right, just find local maxima of small groups. Then do the expensive conversion to dB.
		{
//			int dx = 4;
			float maxAmp = 0.0f;
			int currentX = -1;
			for (int bin = 0 ; bin < closestPixelToBin.size() ; ++bin)
			{
				int x = closestPixelToBin[bin];
				if (x != currentX)
				{
					if (currentX >= linearZoneHigh)
					{
						float db;
						if (maxAmp <= safeMinAmp)
						{
							// save on expensive log10 call if signal is so quiet that it's off the bottom of the graph anyhow.
							db = clipDbAtBottom;
						}
						else
						{
							db = 10.f * log10(maxAmp) + dbc;
							assert(!isnan(db));
						}
						const int sumaryBin = pixelToBin[currentX].index;

						dbs_in[sumaryBin] = db;
						dbs_disp[sumaryBin] = (std::max)(db, dbs_disp[sumaryBin]);
					}
					currentX = x;
					maxAmp = 0.0f;
				}

				maxAmp = (std::max)(maxAmp, capturedata[bin]);
			}
		}
	}

	graphValues.clear();

	// calc the leftmost cubic-smoothed section
	
	int dx = 2;
#ifdef _DEBUG
	dx = 1;
#endif
	for (int x = 0; x < smoothedZoneHigh; x += dx)
	{
		const auto& [index
			, fraction
			, hz
		] = pixelToBin[x];

		assert(index >= 0);
		assert(index + 2 < dbs_in.size());

		assert(dbUsed[(std::max)(0, index - 1)] && dbUsed[index] && dbUsed[index + 1] && dbUsed[index + 2]);

		const float y0 = dbs_disp[(std::max)(0, index - 1)];
		const float y1 = dbs_disp[index + 0];
		const float y2 = dbs_disp[index + 1];
		const float y3 = dbs_disp[index + 2];

		const auto db = (y1 + 0.5f * fraction * (y2 - y0 + fraction * (2.0f * y0 - 5.0f * y1 + 4.0f * y2 - y3 + fraction * (3.0f * (y1 - y2) + y3 - y0))));

		const auto y = (db - displayDbTop) * dbToPixel;

		graphValues.push_back({ static_cast<float>(x), y });
	}

	// calc the 2-point interpolated section
	for (int x = smoothedZoneHigh; x < linearZoneHigh; x += dx)
	{
		const auto& [index
			, fraction
			, hz
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

	// plot the 'max' section where we just take the maximum value of several bins.
	for (int x = linearZoneHigh; x < width; x += 2) // see also closestXQuantized
	{
		const auto dbBin = pixelToBin[x].index;

		const auto db = dbs_disp[dbBin];
		const auto y = (db - displayDbTop) * dbToPixel;

		graphValues.push_back({ static_cast<float>(x), y });
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
