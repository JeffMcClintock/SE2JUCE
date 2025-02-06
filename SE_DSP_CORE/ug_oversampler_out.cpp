#include "pch.h"
#include <mutex>
#include "ug_oversampler_out.h"
#include "ug_oversampler_in.h"
#include "ug_oversampler.h"

using namespace std;

#ifdef _DEBUG
// #define DEBUG_OVERSAMPLER_IN
// #define DEBUG_OVERSAMPLER_OUT
#endif

// ug_oversampler_io combined into this file.
///////////////////////////////////////////////////////////////////////

ug_oversampler_io::ug_oversampler_io() :
	oversamplerBlockPos(0)
	,subSample(0)
{
	SetFlag(UGF_POLYPHONIC_AGREGATOR);
}

UPlug* ug_oversampler_io::AddProxyPlug(UPlug* p)
{
	// add proxy output plug to oversampler_in.
	UPlug* op = new UPlug( this, p );
	if( p->Direction == DR_IN )
		op->Direction = DR_OUT;
	else
		op->Direction = DR_IN;
	op->CreateBuffer();
	AddPlug( op );
	OutsidePlugs.push_back(p);
	return op;
}

UPlug* ug_oversampler_io::GetProxyPlug(int i, UPlug* p)
{
	UPlug* op = plugs[i];

	OutsidePlugs.push_back(p);
	return op;
}

int ug_oversampler_out::calcFirPredelay(int tapCount, int oversampleFactor)
{
	int firLatency = tapCount / 2;
	int currentLatency = firLatency + oversampler_->oversampler_in->latencySamples;
	// Calulate how many extra samples needed to round-up to multiple of oversample factor.
	//return 1 + oversampleFactor_ - (currentLatency % oversampleFactor_) % oversampleFactor_;
// second % seems redundant
assert(oversampleFactor_ - (currentLatency % oversampleFactor_) % oversampleFactor_ == oversampleFactor_ - (currentLatency % oversampleFactor_));
	return oversampleFactor_ - (currentLatency % oversampleFactor_) % oversampleFactor_;
}

void ug_oversampler_out::calcLatency(int filterType, int oversampleFactor)
{
	// total latency of oversampler.
	if (filterType > 10) // FIR Filter on output.
	{
		const auto taps = calcTapCount2(filterType, oversampleFactor);
		const int predelay = calcFirPredelay(taps, oversampleFactor);
		latencySamples = predelay + taps / 2; // downsampler latency.
		staticSettleSamples = predelay + taps + oversampleFactor;

		assert(latencySamples % oversampleFactor == 0);

		//_RPT3(_CRT_WARN, "taps/2 %d predelay %d = latencySamples %d\n", taps / 2, predelay, latencySamples);
	}
	else
	{
		// IIR Filter on output.
		latencySamples = oversampleFactor; // not sure why output has this latency.
	}
}

ug_base* ug_oversampler_out::Clone(CUGLookupList& UGLookupList)
{
	auto clone = dynamic_cast<ug_oversampler_out*>(ug_oversampler_io::Clone(UGLookupList));

	clone->staticSettleSamples = staticSettleSamples;

	return clone;
}

int ug_oversampler_out::Open()
{
	// fix for race conditions.
	static std::mutex safeInit;
	std::lock_guard<std::mutex> lock(safeInit);

	const int filterSetting = oversampler_->filterSetting();

	firMode = filterSetting > 10; // implies FIR mode.

	if( firMode )
	{
		const int tapCount = calcTapCount2(filterSetting, oversampleFactor_);
		sincTaps.numCoefs_ = tapCount;
		gausTaps.numCoefs_ = tapCount;

		wchar_t name[40];
		swprintf( name, 40, L"Oversampler-SINC %d taps x%d", tapCount, oversampleFactor_ );

		int32_t needInitialize;
		allocateSharedMemory( name, (void**) &sincTaps.coefs_, -1, tapCount * sizeof(float), needInitialize, SLS_ALL_MODULES );

		swprintf( name, 40, L"Oversampler-GAUS %d taps x%d", tapCount, oversampleFactor_ );
		allocateSharedMemory( name, (void**) &gausTaps.coefs_, -1, tapCount * sizeof(float), needInitialize, SLS_ALL_MODULES );

		// Fill lookup tables if not done already
		if( needInitialize )
		{
			const double transitionBand = (std::min)(10, (std::max)(1, (17 - filterSetting))) * 0.01;
			const double transitionAdjust = 1.0 - transitionBand;
			auto cuttoff = transitionAdjust * 0.5 / oversampleFactor_;

			sincTaps.InitCoefs(cuttoff);

			gausTaps.InitCoefsGausian(cuttoff);
		}

		// To avoid fractional latency, ensure total latency of downsampling filter is multiple of oversampleFactor.
		// This assume even number of taps, which is slightly asymetrical, hence +1 in formula.
		int predelay = calcFirPredelay(tapCount, oversampleFactor_);

		Filters3_.resize(plugs.size());

		int i = 0;
		for( auto& filter : Filters3_)
		{
			const bool isControlSignal = plugs[i]->GetFlag(PF_CV_HINT);
			const SincFilterCoefs* coefs = isControlSignal ? &gausTaps : &sincTaps;

			filter.Init(tapCount, oversampleFactor_, AudioMaster()->BlockSize(), predelay, coefs);

			++i;
		}
	}
	else
	{
		Filters2_.resize( plugs.size() );

// Filter design code fails on Mac, not sure why. Use pre-calculated coefs instead.
#if defined( __APPLE__ )

	double elipticFilterCoefs_3pole[5][2][2][3] = {

		// 2x oversampling, 3 poles.
		{
			{ { 1, -0.192343,  0.833669, },{ 0.340002, 0.0145516,  0.340002, } },
			{ { 1, -0.153665,         0, },{ 1,         1,         0, } },
		},

		// 4x oversampling, 3 poles.
		{
			{ { 1,  -1.39629,  0.884555, },{ 0.181491, -0.246181,  0.181491, } },
			{ { 1, -0.521573,         0, },{ 1,         1,         0, } },
		},

		// 8x oversampling, 3 poles.
		{
			{ { 1,  -1.80624,   0.93635, },{ 0.100097,  -0.18303,  0.100097, } },
			{ { 1, -0.736155,         0, },{ 1,         1,         0, } },
		},

		// 16x oversampling, 3 poles.
		{
			{ { 1,  -1.93375,  0.967101, },{ 0.0536312, -0.104925, 0.0536312, } },
			{ { 1, -0.859839,         0, },{ 1,         1,         0, } },
		},

		// 32x oversampling, 3 poles.
		{
			{ { 1,  -1.97492,  0.983343, },{ 0.0279125, -0.0555197, 0.0279125, } },
			{ { 1, -0.927518,         0, },{ 1,         1,         0, } },
		},
	};

	double elipticFilterCoefs_5pole[5][3][2][3] = {

		// 2x oversampling, 5 poles.
		{
			{ { 1,  -0.24094,  0.936528, },{ 0.113756, 0.0559434,  0.113756, } },
			{ { 1,  -0.48179,  0.628371, },{ 1, -0.0385096,         1, } },
			{ { 1, -0.428028,         0, },{ 1,         1,         0, } },
		},

		// 4x oversampling, 5 poles.
		{
			{ { 1,  -1.46733,  0.957215, },{ 0.0409441, -0.0437547, 0.0409441, } },
			{ { 1,  -1.43939,  0.758604, },{ 1,  -1.39915,         1, } },
			{ { 1, -0.706963,         0, },{ 1,         1,         0, } },
		},

		// 8x oversampling, 5 poles.
		{
			{ { 1,  -1.84905,  0.976897, },{ 0.0198395, -0.0344251, 0.0198395, } },
			{ { 1,  -1.78181,  0.866885, },{ 1,  -1.84138,         1, } },
			{ { 1, -0.846769,         0, },{ 1,         1,         0, } },
		},

		// 16x oversampling, 5 poles.
		{
			{ { 1,  -1.95578,   0.98819, },{ 0.0102303, -0.0197593, 0.0102303, } },
			{ { 1,  -1.90853,  0.930544, },{ 1,  -1.95979,         1, } },
			{ { 1,  -0.92096,         0, },{ 1,         1,         0, } },
		},

		// 32x oversampling, 5 poles.
		{
			{ { 1,  -1.98591,  0.994053, },{ 0.00526184, -0.0104327, 0.00526184, } },
			{ { 1,  -1.95898,   0.96458, },{ 1,  -1.98991,         1, } },
			{ { 1, -0.959764,         0, },{ 1,         1,         0, } },
		},
	};

	double elipticFilterCoefs_7pole[5][4][2][3] = {

		// 2x oversampling, 7 poles.
		{
			{ { 1, -0.247611,  0.967324, },{ 0.0362602, 0.0325931, 0.0362602, } },
			{ { 1, -0.384983,  0.850946, },{ 1,  0.145542,         1, } },
			{ { 1, -0.773131,  0.572053, },{ 1, -0.0578566,         1, } },
			{ { 1, -0.565066,         0, },{ 1,         1,         0, } },
		},

		// 4x oversampling, 7 poles.
		{
			{ { 1,  -1.48426,  0.978107, },{ 0.00817263, -0.00568898, 0.00817263, } },
			{ { 1,  -1.49383,  0.902946, },{ 1,  -1.29886,         1, } },
			{ { 1,  -1.53801,  0.743358, },{ 1,  -1.40897,         1, } },
			{ { 1, -0.787133,         0, },{ 1,         1,         0, } },
		},

		// 8x oversampling, 7 poles.
		{
			{ { 1,  -1.86002,  0.988239, },{ 0.0033365, -0.00531828, 0.0033365, } },
			{ { 1,  -1.84108,  0.947678, },{ 1,   -1.8108,         1, } },
			{ { 1,  -1.80767,  0.861307, },{ 1,   -1.8443,         1, } },
			{ { 1,  -0.89113,         0, },{ 1,         1,         0, } },
		},

		// 16x oversampling, 7 poles.
		{
			{ { 1,  -1.96159,  0.994005, },{ 0.00163738, -0.00309771, 0.00163738, } },
			{ { 1,  -1.94604,   0.97316, },{ 1,  -1.95175,         1, } },
			{ { 1,  -1.91416,  0.927976, },{ 1,  -1.96055,         1, } },
			{ { 1, -0.944488,         0, },{ 1,         1,         0, } },
		},

		// 32x oversampling, 7 poles.
		{
			{ { 1,  -1.98885,  0.996986, },{ 0.000831285, -0.00163973, 0.000831285, } },
			{ { 1,  -1.97962,  0.986448, },{ 1,  -1.98788,         1, } },
			{ { 1,  -1.95979,  0.963304, },{ 1,  -1.99011,         1, } },
			{ { 1,  -0.97191,         0, },{ 1,         1,         0, } },
		},
	};

	double elipticFilterCoefs_9pole[5][5][2][3] = {

		// 2x oversampling, 9 poles.
		{
			{ { 1, -0.249304,  0.980177, },{ 0.0114956, 0.0137158, 0.0114956, } },
			{ { 1, -0.333152,   0.92172, },{ 1,  0.375932,         1, } },
			{ { 1, -0.565289,   0.79704, },{ 1, 0.0427986,         1, } },
			{ { 1, -0.996018,  0.574142, },{ 1, -0.0654589,         1, } },
			{ { 1, -0.647952,         0, },{ 1,         1,         0, } },
		},

		// 4x oversampling, 9 poles.
		{
			{ { 1,  -1.49078,  0.986748, },{ 0.00157699, -0.000498578, 0.00157699, } },
			{ { 1,  -1.50206,  0.948655, },{ 1,  -1.15271,         1, } },
			{ { 1,  -1.54641,  0.874046, },{ 1,  -1.35644,         1, } },
			{ { 1,  -1.62027,  0.759946, },{ 1,  -1.41279,         1, } },
			{ { 1, -0.832236,         0, },{ 1,         1,         0, } },
		},

		// 8x oversampling, 9 poles.
		{
			{ { 1,  -1.86439,  0.992895, },{ 0.000525171, -0.000745379, 0.000525171, } },
			{ { 1,   -1.8567,  0.972462, },{ 1,  -1.76371,         1, } },
			{ { 1,  -1.84811,   0.93271, },{ 1,  -1.82852,         1, } },
			{ { 1,  -1.83687,  0.872673, },{ 1,  -1.84544,         1, } },
			{ { 1, -0.915243,         0, },{ 1,         1,         0, } },
		},

		// 16x oversampling, 9 poles.
		{
			{ { 1,  -1.96394,  0.996383, },{ 0.000241924, -0.000445054, 0.000241924, } },
			{ { 1,  -1.95663,   0.98594, },{ 1,  -1.93918,         1, } },
			{ { 1,  -1.94398,  0.965495, },{ 1,  -1.95642,         1, } },
			{ { 1,  -1.92515,  0.934307, },{ 1,  -1.96085,         1, } },
			{ { 1, -0.957051,         0, },{ 1,         1,         0, } },
		},

		// 32x oversampling, 9 poles.
		{
			{ { 1,  -1.99005,  0.998182, },{ 0.000120759, -0.000236547, 0.000120759, } },
			{ { 1,  -1.98556,   0.99292, },{ 1,  -1.98468,         1, } },
			{ { 1,  -1.97714,  0.982562, },{ 1,  -1.98906,         1, } },
			{ { 1,  -1.96429,  0.966614, },{ 1,  -1.99018,         1, } },
			{ { 1, -0.978336,         0, },{ 1,         1,         0, } },
		},
	};

	int cuttoffIndex = 0;
	if (oversampleFactor_ < 4)
	{
		cuttoffIndex = 0;
	}
	else
	{
		if (oversampleFactor_ < 8)
		{
			cuttoffIndex = 1;
		}
		else
		{
			if (oversampleFactor_ < 16)
			{
				cuttoffIndex = 2;
			}
			else
			{
				if (oversampleFactor_ < 32)
				{
					cuttoffIndex = 3;
				}
				else
				{
					cuttoffIndex = 4;
				}
			}
		}
	}

	typedef double filterCoefs[2][3];

	filterCoefs* coefs = nullptr;

	switch (filterSetting)
	{
	case 3:
		coefs = &(elipticFilterCoefs_3pole[cuttoffIndex][0]);
		break;

	default:
	case 5:
		coefs = &(elipticFilterCoefs_5pole[cuttoffIndex][0]);
		break;

	case 7:
		coefs = &(elipticFilterCoefs_7pole[cuttoffIndex][0]);
		break;

	case 9:
		coefs = &(elipticFilterCoefs_9pole[cuttoffIndex][0]);
		break;
	}

	for (auto& f : Filters2_)
	{
		f.Init((filterSetting + 1) / 2, coefs);
	}

#else

	const double rolloff = -3.0; // -4.0 - steep (weak stopband). 1.0 less steep, more stopband.
	// Slope takes about 20% of the passband to drop -40Db.
	const double cutoff = 0.46 / (double) oversampleFactor_; // fraction of samplerate. i.e. 0.25 = 10K at 44k SR.
	const double rippleDb = 1.0;
	int order = (std::max)(3, filterSetting); // Flowmotion has oversampleing filter set to "0"
	FilterDesignPolymorphicBase* filterDesign = new FilterDesignPolymorphic< Dsp::Elliptic::LowPass<10> >(order, cutoff, rippleDb, rolloff);

	// Print out coefs. Handy.
#if 0
	{
		int oversampleFactors[] = { 2, 4, 8, 16, 32 };
		int poles[] = { 3, 5, 7, 9 };
		for (auto pole : poles)
		{
			std::ostringstream oss;
			oss << "elipticFilterCoefs elipticFilterCoefs_" << pole << "pole[] = {" << std::endl;
			for (auto oversampleFactor : oversampleFactors)
			{
				double cutoff2 = 0.46 / (double)oversampleFactor; // fraction of samplerate. i.e. 0.25 = 10K at 44k SR.
				FilterDesignPolymorphicBase* filterDesign2 = new FilterDesignPolymorphic< Dsp::Elliptic::LowPass<10> >(pole, cutoff2, rippleDb, rolloff);

				// double coefs[10][2][3]; // [stage][A/B][0,1,2].

				oss << std::endl << "// " << oversampleFactor << "x oversampling, " << pole << " poles." << std::endl;
				oss << "{" << std::endl;
				for (int i = 0; i < filterDesign2->getNumStages(); ++i)
				{
					oss << "  {{ ";
					for (int j = 0; j < 3; ++j)
					{
						oss << std::setw(9) << filterDesign2->GetCoefA(i, j) << ", ";
					}
					oss << "}, { ";
					for (int j = 0; j < 3; ++j)
					{
						oss << std::setw(9) << filterDesign2->GetCoefB(i, j) << ", ";
					}
					oss << "}}, " << std::endl;
				}
				oss << "}," << std::endl;
			}
			oss << "};" << std::endl;

#if defined(__APPLE__ )
				printf(oss.str().c_str());
#else
				_RPT1(_CRT_WARN, "%s\n", oss.str().c_str());
#endif // __APPLE__
		}
	}
#endif

	for (auto& f : Filters2_)
	{
		f.Init(filterDesign);
	}

	delete filterDesign;

#endif
	}

	staticCount.assign( plugs.size(), -1 );
	states.assign(plugs.size(), OSP_Streaming);

	return ug_base::Open();
}

///////////////////////////////////////////////////////////////////////

ug_oversampler_out::ug_oversampler_out()
{
	SetFlag(UGF_POLYPHONIC_AGREGATOR);
}

void ug_oversampler_out::OnFirstSample()
{
	if (firMode)
	{
		SET_CUR_FUNC(&ug_oversampler_out::subProcessFirFilter);
	}
	else
	{
		SET_CUR_FUNC( &ug_oversampler_out::subProcessDownsample );
	}

	for (auto p : OutsidePlugs)
	{
		// Send initial pin value (0).
		int64_t data = 0;
		int32_t size = 0;

		switch (p->DataType)
		{
		case DT_FSAMPLE:
		case DT_MIDI:
			continue;
			break;

		case DT_STRING_UTF8:
		case DT_TEXT:
		case DT_BLOB:
		case DT_BLOB2:
			size = 0;
			break;

		case DT_BOOL:
			size = sizeof(bool);
			break;

		case DT_DOUBLE:
		case DT_INT64:
			size = sizeof(int64_t);
			break;

		case DT_ENUM:
		case DT_FLOAT:
		case DT_INT:
			size = sizeof(int32_t);
			break;

		default:
			assert(false); // TODO!
			break;
		}

		assert(p->DataType != DT_MIDI);
		assert(p->Direction == DR_OUT);
		{
			p->Transmit(0, size, &data);
		}
	}
}

void ug_oversampler_out::HandleEvent(SynthEditEvent* e)
{
	switch (e->eventType)
	{
		case UET_EVENT_STREAMING_STOP:
		{
			const int i = e->parm1; // plug index.
			states[i] = OSP_FilterSettling;
			if (firMode)
			{
				staticCount[i] = staticSettleSamples;
			}
			// Because filter may take time to settle. Transmit RUN state.
			e->eventType = UET_EVENT_STREAMING_START;
			oversampler_->RelayOutGoingEvent(OutsidePlugs[e->parm1], e);
			//_RPT0(0, "OUT SETTLING\n");
		}
		break;

		case UET_EVENT_STREAMING_START:
		{
			int i = e->parm1; // plug index.
			states[i] = OSP_Streaming;
			oversampler_->RelayOutGoingEvent(OutsidePlugs[e->parm1], e);
			//_RPT0(0, "OUT STREAMING\n");
		}
		break;

		case UET_EVENT_SETPIN:
		case UET_EVENT_MIDI:
		{
			e->timeStamp += latencySamples; // not completly safe.
			oversampler_->RelayOutGoingEvent(OutsidePlugs[e->parm1], e);
		}
		break;

		case UET_GRAPH_START:
			OnFirstSample();
			break;

		default:
			ug_base::HandleEvent(e);
			break;
	}
}

void ug_oversampler_out::subProcessDownsample(int start_pos, int sampleframes)
{
	assert(!firMode);

	const int downsampledSampleframes = (sampleframes + subSample) / oversampleFactor_;
//	const int downsampledSampleframes = (sampleframes + subSample + oversampleFactor_ - 1) / oversampleFactor_;
//	const int downsampledSampleframes = (sampleframes + (subSample + oversampleFactor_ - 1) % oversampleFactor_) / oversampleFactor_;

	for( int x = (int) plugs.size() - 1; x >= 0 ; --x )
	{
		auto fromplug = plugs[x];
		assert( fromplug->Direction == DR_IN );

		if (fromplug->DataType != DT_FSAMPLE || states[x] == OSP_Sleep)
			continue;

		auto toplug = OutsidePlugs[x];
		float* from = fromplug->GetSamplePtr() + start_pos;
		float* to = toplug->GetSamplePtr() + oversamplerBlockPos;

		int lsubSample = subSample;

		if (states[x] == OSP_StaticCount)
		{
			const int todo = (std::min)(downsampledSampleframes, staticCount[x]);

			const float target = *from;
			for (int s = todo; s > 0; --s)
			{
				*to++ = target;
			}

			staticCount[x] -= todo;

			if (0 == staticCount[x])
			{
				states[x] = OSP_Sleep;
				const timestamp_t clock = SampleClock() + /* ???start_pos*/ + todo;
				oversampler_->RelayOutGoingState(clock, OutsidePlugs[x], ST_STATIC);
			}
		}
		else
		{
			for( int s = sampleframes ; s > 0 ; --s )
			{
				float r = Filters2_[x].ProcessIISingle( *from++ );
				if(lsubSample == 0 )
				{
					*to = r;
				}

				if (++lsubSample == oversampleFactor_)
				{
					lsubSample = 0;
					++to;
				}
			}

			// input silent. Watching for filter output to settle.
			if (states[x] == OSP_FilterSettling) // non silent.
			{
				// 6.0 E-8 is 1 bit of a 24-bit sample. i.e. inaudible threshold.
				to = toplug->GetSamplePtr() + oversamplerBlockPos;
				const float target = *( fromplug->GetSamplePtr() + start_pos );
				const float smallValue = 1.0E-6f;
				const float hi_threshhold = target + smallValue;
				const float lo_threshhold = target - smallValue;

				// _RPT1(_CRT_WARN, "%g\n", (double) *to );
				int settleCount = 0;
				for( int s = downsampledSampleframes; s > 0 ; --s )
				{
					if( *to < hi_threshhold && *to > lo_threshhold )
					{
						--settleCount;
					}
					else
					{
						settleCount = 20;
					}
				}
				if (settleCount < 0)
				{
					states[x] = OSP_StaticCount;
					staticCount[x] = AudioMaster()->BlockSize();
				}
/* not any better
				auto static_output = staticOutputs[x];
				auto o = toplug->GetSamplePtr() + oversamplerBlockPos;
				for (int i = 0; i < downsampledSampleframes; ++i)
				{
					const float INSIGNIFICANT_SAMPLE = 0.000001f;
					float energy = fabs(static_output - *o);
					if (energy > INSIGNIFICANT_SAMPLE)
					{
						// filter still not settled.
						historyIdx = historyCount;
						static_output = *o;
					}
					--historyIdx;
					++o;
				}
*/
			}
		}
	}

	oversamplerBlockPos += downsampledSampleframes;
	subSample = (subSample + sampleframes) % oversampleFactor_;
}

void ug_oversampler_out::subProcessFirFilter(int start_pos, int sampleframes)
{
//	_RPTN(0, "oversamplerBlockPos=%d start_pos=%d\n", oversamplerBlockPos, start_pos);
	assert(firMode);

	const int downsampledSampleframes = (sampleframes + subSample) / oversampleFactor_;

	for (int x = (int)plugs.size() - 1; x >= 0; --x)
	{
		auto fromplug = plugs[x];
		assert(fromplug->Direction == DR_IN);

		if (fromplug->DataType != DT_FSAMPLE)
			continue;

		if (states[x] != OSP_Sleep)
		{
			auto toplug = OutsidePlugs[x];
			float* from = fromplug->GetSamplePtr() + start_pos;
			float* to = toplug->GetSamplePtr() + oversamplerBlockPos;

#ifdef _DEBUG
			if (OSP_Streaming != states[x])
			{
				// if we're settling, input must be static. check buffer consistency.
				for (int i = 0; i < sampleframes; ++i)
				{
					if (from[0] != from[i])
					{
						assert(false);
						break;
					}
//					assert(0.f == from[i]); // synths only.
				}
			}
#endif

			//_RPT4(_CRT_WARN, "r%3d w%3d offset%3d/%3d ", Filters3_[x].readIndex_, Filters3_[x].writeIndex_, (Filters3_[x].hist_.size() + Filters3_[x].writeIndex_ - Filters3_[x].readIndex_ ) % Filters3_[x].hist_.size(), Filters3_[x].hist_.size());
			//_RPT2(_CRT_WARN, "sampleframes %d downsampledSampleframes %3d\n", sampleframes, downsampledSampleframes);
			Filters3_[x].pushHistory(from, sampleframes);

			for (int s = downsampledSampleframes; s > 0; --s)
			{
				*to++ = Filters3_[x].ProcessIISingle(oversampleFactor_);
			}

#ifdef _DEBUG
			if (OSP_StaticCount == states[x])
			{
				const float* v = fromplug->GetSamplePtr() + start_pos;
				// if we're static, output must be static. check buffer consistency.
				for (int i = 0; i < sampleframes; ++i)
				{
					if (v[0] != v[i])
					{
						assert(false);
						break;
					}
//					assert(0.f == v[i]); // synths only.
				}
				// check outer plug
				const float* op = toplug->GetSamplePtr() + oversamplerBlockPos;
				for (int i = 0; i < downsampledSampleframes; ++i)
				{
					if (op[0] != op[i])
					{
						assert(false);
						break;
					}
//					assert(0.f == op[i]); // synths only.
				}
//				_RPTN(0, " out %d samples at offset %d -> %d\n", downsampledSampleframes, oversamplerBlockPos, oversamplerBlockPos + downsampledSampleframes);
			}
#endif

			// when we are processing the *next* sample after static count expired (i.e. staticCount < 0 not staticCount == 0)
			// .. switch to next mode
			if (OSP_Streaming != states[x])
			{
				assert(staticCount[x] >= 0);

				staticCount[x] -= sampleframes;

				if (staticCount[x] < 0)
				{
					assert(sampleframes + staticCount[x] >= 0); // ensure we are sending state *during* current block

					if (states[x] == OSP_FilterSettling)
					{
						// filter has settled. sent STATIC message.
						const timestamp_t clock = SampleClock() /* + start_pos */ + sampleframes + staticCount[x];
						oversampler_->RelayOutGoingState(clock, OutsidePlugs[x], ST_STATIC);

						states[x] = OSP_StaticCount;
						staticCount[x] += (AudioMaster()->BlockSize() + 1) * oversampleFactor_;
//						_RPT0(0, "OUT STATIC\n");
					}
					else
					{
						assert(states[x] == OSP_StaticCount);
						states[x] = OSP_Sleep;
//						_RPT0(0, "OUT SLEEP\n");

#ifdef _DEBUG
						// check outer plug
						const float* op = toplug->GetSamplePtr();
						auto bs = AudioMaster()->BlockSize();
						for (int i = 0; i < bs; ++i)
						{
							assert(op[0] == op[i]);
						}
#endif
					}
				}
			}
		}
		else
		{
			Filters3_[x].Skip(sampleframes, downsampledSampleframes, oversampleFactor_);
		}
	}

	oversamplerBlockPos += downsampledSampleframes;
	subSample = (subSample + sampleframes) % oversampleFactor_;
}
///////////////////////////////////////////////////////////

int ug_oversampler_in::Open()
{
	auto r = ug_oversampler_io::Open();

	// Init upsampling interpolators.
	{
		int memsize = InterpolatorType::calcCoefsMemoryBytes(oversampleFactor_);

		if (memsize > 0)
		{
			{
				wchar_t name[40];
				swprintf(name, 40, L"OversamplerIn-SINC %d fac x%d", sincSize, oversampleFactor_);

				int32_t needInitialize;
				allocateSharedMemory(name, (void**)&InterpolatorCoefs, -1, memsize, needInitialize, SLS_ALL_MODULES);
				if (needInitialize)
				{
					InterpolatorType::initCoefs(oversampleFactor_, InterpolatorCoefs);
				}
			}

			// gausian smoother for control signals.
			{
				wchar_t name[40];
				swprintf(name, 40, L"OversamplerIn-GAUS %d fac x%d", sincSize, oversampleFactor_);

				int32_t needInitialize;
				allocateSharedMemory(name, (void**)&InterpolatorCoefs_gaussian, -1, memsize, needInitialize, SLS_ALL_MODULES);
				if (needInitialize)
				{
					InterpolatorType::initCoefs_gaussian(oversampleFactor_, InterpolatorCoefs_gaussian);
				}
			}

#if 0
			// Print filters.
			_RPT0(_CRT_WARN, "SINC");
			for (int i = 0; i < sincSize; ++i)
			{
				for (int j = 0; j < oversampleFactor_ ; ++j)
				{
					_RPT1(_CRT_WARN, ", %f", InterpolatorCoefs[i + (oversampleFactor_ - 1 - j) * sincSize]);
				}
			}
			
			_RPT0(_CRT_WARN, "\n");

			_RPT0(_CRT_WARN, "GAUS");
			for (int i = 0; i < sincSize; ++i)
			{
				for (int j = 0; j < oversampleFactor_ ; ++j)
				{
					_RPT1(_CRT_WARN, ", %f", InterpolatorCoefs_gaussian[i + (oversampleFactor_ - 1 - j) * sincSize]);
				}
			}
			
			_RPT0(_CRT_WARN, "\n");
#endif
		}
	}

	RUN_AT( SampleClock(), &ug_oversampler_in::OnFirstSample );
	interpolators_.resize(plugs.size());
	staticCount.assign(plugs.size(), -1);
	states.assign(plugs.size(), OSP_Streaming);

	int filterThroughDelay = 0;
	int i = 0;
	for( auto& filter : interpolators_)
	{
		const bool isControlSignal = plugs[i]->GetFlag(PF_CV_HINT);
		const float* coefs = isControlSignal ? InterpolatorCoefs_gaussian : InterpolatorCoefs;

		filterThroughDelay = filter.Init(sincSize, oversampler_->BlockSize(), coefs);
		++i;
	}

	settleSampleCount = (filterThroughDelay + 1 ) * oversampleFactor_;// +AudioMaster()->BlockSize();

	return r;
}

void ug_oversampler_in::OnFirstSample()
{
	SET_CUR_FUNC( &ug_oversampler_in::subProcessUpsample );
}

void ug_oversampler_in::HandleEvent(SynthEditEvent* e)
{
	switch( e->eventType )
	{
	case UET_GRAPH_START:
		ug_base::HandleEvent(e);
		TransmitInitialPinValues();
		break;

	case UET_EVENT_STREAMING_START:
		{
			const int plugIdx = e->parm1;
			plugs[plugIdx]->TransmitState( e->timeStamp, ST_RUN );
			states[plugIdx] = OSP_Streaming;

#ifdef DEBUG_OVERSAMPLER_IN
			_RPTN(_CRT_WARN, "ug_oversampler_in: STREAMING_START pin %2d time %d\n", plugIdx, (int)e->timeStamp);
			_RPT2(_CRT_WARN, "                   staticCount = %d\n", staticCount[plugIdx]);
#endif
		}
		break;

		case UET_EVENT_STREAMING_STOP:
		{
			// input 'jumped' to new value. Filter takes time to settle so transmit RUN state.
			const int plugIdx = e->parm1;
			plugs[plugIdx]->TransmitState( e->timeStamp, ST_RUN );
			staticCount[plugIdx] = latencySamples * 2;// settleSampleCount;
			states[plugIdx] = OSP_FilterSettling;

#ifdef DEBUG_OVERSAMPLER_IN
			_RPTN(_CRT_WARN, "ug_oversampler_in: STREAMING_STOP pin %2d time %d\n", plugIdx, (int)e->timeStamp);
			_RPT2(_CRT_WARN, "                   staticCount reset to %d\n", staticCount[plugIdx]);
#endif
		}
		break;

		case UET_EVENT_SETPIN:
		case UET_EVENT_MIDI:
		{
			plugs[e->parm1]->Transmit( e->timeStamp + latencySamples, e->parm2, e->Data() );
		}
		break;

	default:
		ug_base::HandleEvent(e);
	}
}

// Event based pins won't recieve anything for 4 samples (due to input latency), send a zero in the mean time.
void ug_oversampler_in::TransmitInitialPinValues()
{
	for (auto p : plugs)
	{
		// Send initial pin value (0).
		int64_t data = 0;
		int32_t size = 0;

		switch (p->DataType)
		{
		case DT_FSAMPLE:
		case DT_MIDI:
			continue;
			break;

		case DT_STRING_UTF8:
		case DT_TEXT:
		case DT_BLOB:
		case DT_BLOB2:
			size = 0;
			break;

		case DT_BOOL:
			size = sizeof(bool);
			break;

		case DT_DOUBLE:
		case DT_INT64:
			size = sizeof(int64_t);
			break;

		case DT_ENUM:
		case DT_FLOAT:
		case DT_INT:
			size = sizeof(int32_t);
			break;

		default:
			assert(false); // TODO!
			break;
		}

		assert(p->DataType != DT_MIDI);
		assert(p->Direction == DR_OUT);
		{
			p->Transmit(0, size, &data);
		}
	}
}

void ug_oversampler_in::OnOversamplerResume()
{
#ifdef DEBUG_OVERSAMPLER_IN
	_RPT0(_CRT_WARN, "ug_oversampler_in::OnOversamplerResume()\n");
#endif
	subSample = 0;

	for (int x = (int) plugs.size() - 1; x >= 0; --x)
	{
		if (staticCount[x] > 0)
		{
			staticCount[x] = settleSampleCount;
#ifdef DEBUG_OVERSAMPLER_IN
	_RPT3(_CRT_WARN, "ug_oversampler staticCount[%d] = %d at time %d\n", x, staticCount[x], (int)this->SampleClock());
#endif
		}
	}

}

int ug_oversampler_in::calcDelayCompensation()
{
	return latencySamples;
}

void ug_oversampler_in::subProcessUpsample(int start_pos, int sampleframes)
{
	for (int x = (int) plugs.size() - 1; x >= 0; --x)
	{
		auto myPlug = plugs[x];
		assert(myPlug->Direction == DR_OUT);

		if (myPlug->DataType != DT_FSAMPLE)
			continue;

		auto outerPlug = OutsidePlugs[x];

		if (!outerPlug->InUse()) // unusual case. Line connected to container outside pin only.
			continue;

		if (OSP_Sleep == states[x])
			continue;

		auto localSampleframes = sampleframes;
		float* from = outerPlug->GetSamplePtr() + oversamplerBlockPos;
		float* to = myPlug->GetSamplePtr() + start_pos;

		if (OSP_Streaming == states[x])
		{
			interpolators_[x].process(localSampleframes, from, to, subSample, oversampleFactor_);
			continue; // nothing else to do.
		}

		if (states[x] == OSP_FilterSettling)
		{
			// process the minimum needed to get to static state, leave any leftover for next case.
			const auto todo = (std::min)(localSampleframes, staticCount[x]);

			interpolators_[x].process(todo, from, to, subSample, oversampleFactor_);

			localSampleframes -= todo;
			staticCount[x] -= todo;

			if (localSampleframes > 0)
			{
#ifdef DEBUG_OVERSAMPLER_IN
				_RPTN(_CRT_WARN, "ug_oversampler_in: V%d pin %2d OSP_FilterSettling (%f)\n", pp_voice_num, x, to[todo - 1]);
#endif

				const timestamp_t clock = SampleClock() + todo;
				myPlug->TransmitState(clock, ST_ONE_OFF);

				states[x] = OSP_StaticCount;
				staticCount[x] = AudioMaster()->BlockSize();

				to += todo;
			}
		}

		// pass input directly to output to avoid small numerical errors from filter.
		if (states[x] == OSP_StaticCount)
		{
			const auto todo = (std::min)(localSampleframes, staticCount[x]);

			// filter may produce *slightly* different output on different sub-samples - even with the same DC input.
			// to prevent this, just pass-through the input.
			for (int s = 0; s < todo; ++s)
			{
				*to++ = *from; // no need to increment 'from', signal is static.
			}

			staticCount[x] -= todo;

			if (localSampleframes > todo)
			{
				states[x] = OSP_Sleep;

#ifdef DEBUG_OVERSAMPLER_IN
				_RPTN(_CRT_WARN, "ug_oversampler_in: V%d pin %2d OSP_Sleep (%f)\n", pp_voice_num, x, *myPlug->GetSamplePtr());
				assert(myPlug->CheckAllSame());
#endif
			}
		}
	}

	oversamplerBlockPos += (subSample + sampleframes) / oversampleFactor_;
	subSample = (subSample + sampleframes) % oversampleFactor_;
}


