#pragma once

#include "../shared/GraphHelpers.h"
#include "Drawing.h"

struct SpectrumAnalyserBase
{
	int smoothedZoneHigh = 0; // bins below this need 4-point interpolation
	int linearZoneHigh = 0; // bins below this need 2-point interpolation

	std::vector<float> dbs;
	std::vector<bool> dbUsed;

	struct binData
	{
		int index;
		float fraction;
#ifdef _DEBUG
		float hz;
#endif
	};

	std::vector< binData > pixelToBin;
	float pixelToBinDx = 2.0f; // x increment for each entry in pixelToBin.
	std::vector<float> rawSpectrum;
	float sampleRateFft = 0;
	std::vector<GmpiDrawing::Point> graphValues;
	std::vector<GmpiDrawing::Point> graphValuesOptimized;
	std::vector<GmpiDrawing::Point> peakHoldValues;
	std::vector<GmpiDrawing::Point> peakHoldValuesOptimized;

	void updateSpectrumGraph(int width, int height);
	void decayGraph(float dbDecay);
	void clearPeaks();
	virtual void updatePaths(const std::vector<GmpiDrawing::Point>& graphValuesOptimized, const std::vector<GmpiDrawing::Point>& peakHoldValuesOptimized) = 0;
	virtual void InixPixelToBin(std::vector<SpectrumAnalyserBase::binData>& pixelToBin, int graphWidth, int numValues, float sampleRate) = 0;
};
