// Copyright 2007 Jeff McClintock

#define _USE_MATH_DEFINES
#include <math.h>
#include "../../modules/shared/xp_simd.h"
#include "VoiceMute.h"

SE_DECLARE_INIT_STATIC_FILE(VoiceMute)
REGISTER_PLUGIN( VoiceMute, L"SE Voice Mute" );

namespace {

	int32_t r = RegisterPluginXml(
R"XML(
<?xml version="1.0" ?>
<PluginList>
  <Plugin id="SE Voice Mute" name="Voice Mute" category="Debug" simdOverwritesBufferEnd="true" alwaysExport="true">
    <Audio>
      <Pin id="0" name="Input1" direction="in" datatype="float" rate="audio"/>
      <Pin id="1" name="VoiceActive" direction="in" datatype="float" hostConnect="Voice/Active" isPolyphonic="true" />
      <Pin id="2" name="Output" direction="out" datatype="float" rate="audio"/>
      <Pin id="3" name="legacyMode" direction="in" datatype="bool" />
    </Audio>
  </Plugin>
</PluginList>
)XML"
);

}

// see also DEBUG_VOICE_ALLOCATION
//#define VM_DEBUG_OUTPUT

VoiceMute::VoiceMute(IMpUnknown* host) : MpBase(host)
,previousActiveState(false)
,gainIndex_(0)
#if defined( _DEBUG )
,clock_(0)
#endif
{
	// Associate each pin object with it's ID in the XML file.
	initializePin(0, pinInput1);
	initializePin(1, pinVoiceActive);
	initializePin(2, pinOutput1);
	initializePin(3, pinLegacyMode);
}

int32_t VoiceMute::open()
{
	// subjectively 10ms or more is best, faster ramps 'pop' to various degrees.
	const float fadeOutTimeFastMs = 5.0f; // Voice needed urgently, no spare voices available for held-back note.
	const float fadeOutTimeSlowMs = 20.0f; // Voice stolen, fade out smoothly without artifacts.

	// Create fade curve lookup table.
	fadeSamplesFast = (int) ( getSampleRate() / ( 1000.0f / fadeOutTimeFastMs ) );
	fadeSamplesSlow = (int) ( getSampleRate() / ( 1000.0f / fadeOutTimeSlowMs ) );

	int sizeFast = (fadeSamplesFast + 1) * sizeof(float);
	int sizeSlow = (fadeSamplesSlow + 1) * sizeof(float);
	int32_t needInitialize;
	getHost()->allocateSharedMemory( L"VoiceMute:FadeCurveFast", (void**) &fadeCurveFast, getSampleRate(), sizeFast, needInitialize );
	getHost()->allocateSharedMemory( L"VoiceMute:FadeCurveSlow", (void**) &fadeCurveSlow, getSampleRate(), sizeSlow, needInitialize );

	// initialize sine-shaped gain curve.
	if( needInitialize != 0 )
	{
		for( int i = 0 ; i <= fadeSamplesFast ; ++i )
		{
			float level = (float) i / (float) fadeSamplesFast;
			fadeCurveFast[i] = sinf( (level - 0.5f) * (float)M_PI ) * 0.5f + 0.5f;
//			_RPT2(_CRT_WARN, "fadeCurveFast[%d] = %f\n", i, fadeCurveFast[i] );
		}
		for( int i = 0 ; i <= fadeSamplesSlow ; ++i )
		{
			float level = (float) i / (float) fadeSamplesSlow;
			fadeCurveSlow[i] = sinf( (level - 0.5f) * (float)M_PI ) * 0.5f + 0.5f;
//			_RPT2(_CRT_WARN, "fadeCurveFast[%d] = %f\n", i, fadeCurveFast[i] );
		}
	}

	assert( fadeCurveFast[0] == 0.0f && fadeCurveFast[fadeSamplesFast] == 1.0f );

	SET_PROCESS(&VoiceMute::subProcess);

#if defined( _DEBUG )
	// Determine physical voice number.
	gmpi_sdk::mp_shared_ptr<gmpi::IMpUnknown> com_object;
	int32_t res = getHost()->createCloneIterator( com_object.asIMpUnknownPtr() );

	gmpi_sdk::mp_shared_ptr<gmpi::IMpCloneIterator> cloneIterator;
	res = com_object->queryInterface(gmpi::MP_IID_CLONE_ITERATOR, cloneIterator.asIMpUnknownPtr() );

	gmpi::IMpUnknown* clone;
	cloneIterator->first();
	physicalVoiceNumber = 0;
	while(cloneIterator->next(&clone) == gmpi::MP_OK)
	{
		if(clone == static_cast<gmpi::IMpPlugin2*> (this) )
		{
			break;
		}
		++physicalVoiceNumber;
	}
#endif

	return MpBase::open();
}

void VoiceMute::onSetPins()  // one or more pins_ updated.  Check pin update flags to determin which ones.
{
	if( pinVoiceActive.isUpdated() )
	{
		#if defined(VM_DEBUG_OUTPUT)
			_RPT2(_CRT_WARN, "VoiceMute::::onSetPins() V%d VoiceActive=%f\n", physicalVoiceNumber, pinVoiceActive.getValue() );
		#endif
		bool newActiveState = pinVoiceActive > 0.0f;

		if( newActiveState != previousActiveState )
		{
			if( newActiveState == false )
			{
				// start fade-out.
				if( pinVoiceActive < 0.0f )
				{
					gainIndex_ = fadeSamplesFast;
					currentfadeCurve = fadeCurveFast;
				}
				else
				{
					gainIndex_ = fadeSamplesSlow;
					currentfadeCurve = fadeCurveSlow;
				}

				SET_PROCESS(&VoiceMute::subProcessMuting);
			}
			else
			{
				if( pinLegacyMode == true )
				{
#if defined(VM_DEBUG_OUTPUT)
					_RPT0(_CRT_WARN, "VoiceMute startup (MIDI-CV mode)\n" );
#endif
					startupDelayCount = 4;
					SET_PROCESS(&VoiceMute::subProcess3SamplePreGate);
				}
				else
				{
#if defined(VM_DEBUG_OUTPUT)
					_RPT0(_CRT_WARN, "VoiceMute startup (Keyboard2 mode)\n" );
#endif
					SET_PROCESS(&VoiceMute::subProcess);
				}
			}
		}

		previousActiveState = newActiveState;
	}

	bool OutputIsActive;

	if( getSubProcess() == static_cast <SubProcess_ptr> ( &VoiceMute::subProcess ) || getSubProcess() == static_cast <SubProcess_ptr> ( &VoiceMute::subProcess3SamplePreGate ) )
	{
		// under normal processing, output reflects input state.
		OutputIsActive = pinInput1.isStreaming();
	}
	else
	{
		// while muting output always runs, even if input static.
		OutputIsActive = gainIndex_ > 0;
	}

	#if defined(VM_DEBUG_OUTPUT)
//		_RPT2(_CRT_WARN, "VoiceMute V%d setStreaming = %d\n", physicalVoiceNumber, (int) OutputIsActive );
	#endif

	// transmit new output state to modules 'downstream'.
	pinOutput1.setStreaming(OutputIsActive);
}


// The most important function, processing the audio
void VoiceMute::subProcessMuting(int bufferOffset, int sampleFrames)
{
	assert( fadeCurveFast[0] == 0.0f && fadeCurveFast[fadeSamplesFast] == 1.0f );

	// assign pointers to your in/output buffers. each buffer is an array of float fadeSamplesFast
	float* in1  = bufferOffset + pinInput1.getBuffer();
	float* out1 = bufferOffset + pinOutput1.getBuffer();

	for( int s = sampleFrames ; s > 0 ; --s ) // sampleFrames = how many fadeSamplesFast to process (can vary). repeat (loop) that many times
	{
		--gainIndex_;
		if( gainIndex_ < 0 )
		{
//nonono			gainIndex_ = fadeSamplesFast;
			//_RPT1(_CRT_WARN, "VoiceMute::subProcessMuting setStreaming = 0 [%x]\n", this );

			int blockRelativeSamplePosition = bufferOffset + sampleFrames - s;

			pinOutput1.setStreaming(false, blockRelativeSamplePosition);
			subProcessSilence(blockRelativeSamplePosition, s);

			SET_PROCESS(&VoiceMute::subProcessSilence);
			return;
		}

		*out1++ = *in1++ * currentfadeCurve[gainIndex_];
	}
}

void VoiceMute::subProcess(int bufferOffset, int sampleframes)
{
	float* in = bufferOffset + pinInput1.getBuffer();
	float* __restrict out = bufferOffset + pinOutput1.getBuffer();

#ifdef _DEBUG
	if (!pinInput1.isStreaming())
	{
		for (int s = 1; s < sampleframes; ++s) // consistant?
		{
			assert(in[0] == in[s]);
		}
	}
#endif

	// auto-vectorized copy.
	while (sampleframes > 3)
	{
		out[0] = in[0];
		out[1] = in[1];
		out[2] = in[2];
		out[3] = in[3];

		out += 4;
		in += 4;
		sampleframes -= 4;
	}

	while (sampleframes > 0)
	{
		*out++ = *in++;
		--sampleframes;
	}

#if defined( _DEBUG )
	clock_ += sampleframes;
#endif
}

void VoiceMute::subProcessSilence(int bufferOffset, int sampleframes)
{
	auto* __restrict out = bufferOffset + pinOutput1.getBuffer();

	// auto-vectorized copy.
	while (sampleframes > 3)
	{
		out[0] = 0.0f;
		out[1] = 0.0f;
		out[2] = 0.0f;
		out[3] = 0.0f;

		out += 4;
		sampleframes -= 4;
	}

	while (sampleframes > 0)
	{
		*out++ = 0.0f;
		--sampleframes;
	}

#if defined( _DEBUG )
	clock_ += sampleframes;
#endif
}

// Mute output for 3 samples to allow MIDI-CV gate to reset.
void VoiceMute::subProcess3SamplePreGate(int bufferOffset, int sampleFrames)
{
	float* out1 = bufferOffset + pinOutput1.getBuffer();

	for(int s = sampleFrames ; s > 0 ; --s)
	{
		if( --startupDelayCount == 0 )
		{
			int samplesDone = sampleFrames - s;

			// send new static.
			if( !pinInput1.isStreaming() )
			{
				pinOutput1.setUpdated( bufferOffset + samplesDone );
			}
			else
			{
				assert(pinOutput1.isStreaming());
			}

			// Complete processing of block.
			SET_PROCESS(&VoiceMute::subProcess);
			subProcess( bufferOffset + samplesDone, sampleFrames - samplesDone );

#if defined( _DEBUG )
			clock_ += sampleFrames;
#endif
			return;
		}

		*out1++ = 0.0f;
	}

#if defined( _DEBUG )
	clock_ += sampleFrames;
#endif
}

