#pragma warning(disable : 4996) // "Call to 'std::transform' with parameters that may be unsafe"

#include "./Inverter.h"
#include <algorithm>

#ifdef _WIN32
// Mac appears to turn on AVX2 in *generic* code when we include the headers, so it crashes on non-AVX2 hardware.
// TODO: disable AVX2 on MAc in gmpi_simd.h so client don't have to worry about it.
//#define USE_EXPERIMENTAL_SIMD
#endif

#ifdef USE_EXPERIMENTAL_SIMD
#include "../shared/gmpi_simd.h"
using namespace simd_gmpi;
#endif

#include "../se_sdk3/mp_sdk_audio.h"

SE_DECLARE_INIT_STATIC_FILE(Inverter)
SE_DECLARE_INIT_STATIC_FILE(Inverter2)

struct Negate
{
	template<typename T>
	inline T operator()(T sample) const { return -sample; }
};


class Inverter : public MpBase2
{
#ifdef USE_EXPERIMENTAL_SIMD
	Negate functor;
#endif

public:
	Inverter();

#ifdef USE_EXPERIMENTAL_SIMD

	template<typename VECTOR_T>
	void subProcess(int sampleFrames)
	{
		/* todo
		need to support double type.

		__m128d test2;
		*/

		processVectors<VECTOR_T>(functor, sampleFrames, getBuffer(pinSignalOut), getBuffer(pinSignalIn));

		/*
		// perhaps.
		// sample buffers as objects (which know how they are aligned, or not).
		SampleVectorType[ getAlignment(getBuffer(pinSignalIn)) ] input(sampleFrames, getBuffer(pinSignalIn));
		SampleVector< getAlignment(getBuffer(pinSignalOut)) > output(sampleFrames, getBuffer(pinSignalOut));

		Negate functor;
		processVectors<VECTOR_T>(functor, getBuffer(pinSignalOut), getBuffer(pinSignalIn));

		and/or

		struct Negate : public UnaryOperator ..


		class Inverter : public Processor<Negate>; // BOOM!


		// maybe just register classes to detect SIMD support from base-class, so decision made only once during instantiation.

		template<int SIMD_VERSION = SIMD_VERSION_SSE2>
		class Inverter : public Processor<Negate, SIMD_VERSION>{}; // BOOM!


		*/
	}

#else
#if 0
	// classic proceedural loop
	void subProcess(int sampleFrames)
	{
		// pure C++ reference code.
		const float* signalIn = getBuffer(pinSignalIn);
		float* signalOut = getBuffer(pinSignalOut);

		for (int s = sampleFrames; s > 0; --s)
		{
			*signalOut++ = -*signalIn++;
		}
	}
#else

	// Modern functional style.
	void subProcess(int sampleFrames)
	{
		const float* signalIn = getBuffer(pinSignalIn);
		float* signalOut = getBuffer(pinSignalOut);

		std::transform(signalIn, signalIn + sampleFrames, signalOut, Negate());
	}

#endif
#endif
    
	virtual void onSetPins();

private:
	AudioInPin pinSignalIn;
	AudioOutPin pinSignalOut;
};

Inverter::Inverter()
{
	// Associate each pin object with it's ID in the XML file.
	initializePin( pinSignalIn );
	initializePin( pinSignalOut );
}

void Inverter::onSetPins()
{
	// Set state of output audio pins.
	pinSignalOut.setStreaming( pinSignalIn.isStreaming() );
    
#ifdef USE_EXPERIMENTAL_SIMD

	// Set processing method ("dynamic dispatch" based on CPU SIMD support).
	SubProcess_ptr2 processMethods[]{
		static_cast <SubProcess_ptr2> (&Inverter::subProcess<simd_gmpi::simd_level0::Value>),
		static_cast <SubProcess_ptr2> (&Inverter::subProcess<simd_gmpi::simd_level1::Value>),
		static_cast <SubProcess_ptr2> (&Inverter::subProcess<simd_gmpi::simd_level2::Value>),
	};

    setSubProcess(processMethods[(int)getCpuSupportLevel()]);

#else
    
    setSubProcess( &Inverter::subProcess );

#endif
    

	// testing only. setSubProcess(processMethods[(int)SimdSupportLevel::Basic]);
}

namespace
{
	auto r = gmpi::Register<Inverter>::withId(L"SynthEdit Inverter2");
}
