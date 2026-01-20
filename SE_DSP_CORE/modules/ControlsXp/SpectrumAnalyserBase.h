#pragma once

#include "../shared/GraphHelpers.h"
#include "Drawing.h"

struct SpectrumAnalyserBase
{
	int smoothedZoneHigh = 0; // bins below this need 4-point interpolation
	int linearZoneHigh = 0; // bins below this need 2-point interpolation

	std::vector<float> dbs_in;   // latest values from the Processor
	std::vector<float> dbs_disp; // values to display, will decay over time
	std::vector<bool> dbUsed;

	struct binData
	{
		int index;
		float fraction;
		float hz;
	};

	std::vector< binData > pixelToBin;
	std::vector<int> closestPixelToBin;

	float dbToPixel = 1.0f;
	std::vector<float> rawSpectrum;
	float sampleRateFft = 0;
	bool spectrumUpdateFlag = true;
	bool spectrumDecayFlag = true;

	std::vector<GmpiDrawing::Point> graphValues;
	std::vector<GmpiDrawing::Point> graphValuesOptimized;
	std::vector<GmpiDrawing::Point> peakHoldValues;
	std::vector<GmpiDrawing::Point> peakHoldValuesOptimized;

	void updateSpectrumGraph(int width, int height, float displayDbTop, float displayDbBot);
	void decayGraph(float dbDecay);
	void clearPeaks();
	virtual void updatePaths(const std::vector<GmpiDrawing::Point>& graphValuesOptimized, const std::vector<GmpiDrawing::Point>& peakHoldValuesOptimized) = 0;
	virtual void InixPixelToBin(std::vector<SpectrumAnalyserBase::binData>& pixelToBin, int graphWidth, int numValues, float sampleRate) = 0;
};
