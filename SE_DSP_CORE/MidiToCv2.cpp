#include "pch.h"
#include <math.h>
#include "./MidiToCv2.h"
#include "./modules/shared/voice_allocation_modes.h"

SE_DECLARE_INIT_STATIC_FILE(MidiToCv2)
REGISTER_PLUGIN2 ( MidiToCv2, L"SE MIDI to CV 2" );

// New. 0v = 0.001s, 10V = 10s
#define VoltageToTime(v) ( powf( 10.0f,((v) * 0.4f ) - 3.0f ) )

namespace{
	int32_t r = RegisterPluginXml(
#if 1
R"XML(
<?xml version="1.0" encoding="UTF-8"?>
<PluginList>
	<Plugin id="SE MIDI to CV 2" name="MIDI-CV 2" category="MIDI" polyphonicSource="true" cloned="true">
		<Audio>
			<Pin name="MIDI In" datatype="midi"/>
			<Pin name="Channel" datatype="enum" default="-1" ignorePatchChange="true" metadata="All=-1,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16"/>
			<Pin name="Trigger" datatype="float" rate="audio" direction="out"/>
			<Pin name="Gate" datatype="float" rate="audio" direction="out"/>
			<Pin name="Pitch" datatype="float" rate="audio" direction="out"/>
			<Pin name="Velocity" datatype="float" rate="audio" direction="out"/>
			<Pin name="Aftertouch" datatype="float" rate="audio" direction="out"/>
			<Pin name="Voice/Active" datatype="float" private="true" isPolyphonic="true" hostConnect="Voice/Active"/>
			<Pin name="Voice/Gate" datatype="float" private="true" isPolyphonic="true" hostConnect="Voice/Gate"/>
			<Pin name="Voice/Trigger" datatype="float" private="true" isPolyphonic="true" hostConnect="Voice/Trigger"/>
			<Pin name="Voice/VelocityKeyOn" datatype="float" private="true" isPolyphonic="true" hostConnect="Voice/VelocityKeyOn"/>
			<Pin name="Voice/Pitch" datatype="float" private="true" isPolyphonic="true" hostConnect="Voice/Pitch"/>
			<Pin name="Voice/VirtualVoiceId" datatype="int" private="true" isPolyphonic="true" hostConnect="Voice/VirtualVoiceId"/>
			<Pin name="Bender" datatype="float" private="true" hostConnect="Bender"/>
			<Pin name="HoldPedal" datatype="float" private="true" hostConnect="HoldPedal"/>
			<Pin name="GlideStartPitch" datatype="float" private="true" isPolyphonic="true" hostConnect="Voice/GlideStartPitch"/>
			<Pin name="VoiceAllocationMode" datatype="int" private="true" hostConnect="VoiceAllocationMode"/>
			<Pin name="Portamento" datatype="float" private="true" hostConnect="Portamento"/>
			<Pin name="BenderRange" datatype="float" private="true" hostConnect="BenderRange"/>
			<Pin name="Voice/Aftertouch" datatype="float" private="true" isPolyphonic="true" hostConnect="Voice/Aftertouch"/>
			<Pin name="ChannelPressure" datatype="float" private="true" isPolyphonic="true" hostConnect="Channel Pressure"/>
			<Pin name="Voice/Bender" datatype="float" private="true" isPolyphonic="true" hostConnect="Voice/Bender"/>
		</Audio>
	</Plugin>
</PluginList>
)XML");
#else
		"<?xml version=\"1.0\" ?>"
		"<PluginList>"
		  "<Plugin id=\"SE MIDI to CV 2\" name=\"MIDI-CV 2\" category=\"MIDI\" cloned=\"true\" polyphonicSource=\"true\" >"
		    "<Audio>"
		      "<Pin name=\"MIDI In\" datatype=\"midi\" />"
		      "<Pin name=\"Channel\" datatype=\"enum\" default=\"-1\" ignorePatchChange=\"true\" metadata=\"All=-1,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16\" />"
			  "<Pin name=\"Trigger\" datatype=\"float\" rate=\"audio\" direction=\"out\" />"
			  "<Pin name=\"Gate\" datatype=\"float\" rate=\"audio\" direction=\"out\" />"
			  "<Pin name=\"Pitch\" datatype=\"float\" rate=\"audio\" direction=\"out\" />"
		      "<Pin name=\"Velocity\" datatype=\"float\" rate=\"audio\" direction=\"out\" />"
		      "<Pin name=\"Aftertouch\" datatype=\"float\" rate=\"audio\" direction=\"out\" />"

		      "<Pin name=\"Voice/Active\" datatype=\"float\" private=\"true\" isPolyphonic=\"true\" hostConnect=\"Voice/Active\" />"
		      "<Pin name=\"Voice/Gate\" datatype=\"float\" private=\"true\" isPolyphonic=\"true\" hostConnect=\"Voice/Gate\" />"
		      "<Pin name=\"Voice/Trigger\" datatype=\"float\" private=\"true\" isPolyphonic=\"true\" hostConnect=\"Voice/Trigger\" />"
		      "<Pin name=\"Voice/VelocityKeyOn\" datatype=\"float\" private=\"true\" isPolyphonic=\"true\" hostConnect=\"Voice/VelocityKeyOn\" />"
		      "<Pin name=\"Voice/Pitch\" datatype=\"float\" private=\"true\" isPolyphonic=\"true\" hostConnect=\"Voice/Pitch\" />"
		      "<Pin name=\"Voice/VirtualVoiceId\" datatype=\"int\" private=\"true\" isPolyphonic=\"true\" hostConnect=\"Voice/VirtualVoiceId\" />"
		      "<Pin name=\"Bender\" datatype=\"float\" private=\"true\" hostConnect=\"Bender\" />"
		      "<Pin name=\"HoldPedal\" datatype=\"float\" private=\"true\" hostConnect=\"HoldPedal\" />"
			  "<Pin name=\"GlideStartPitch\" datatype=\"float\" private=\"true\" isPolyphonic=\"true\" hostConnect=\"Voice/GlideStartPitch\" />"
			  "<Pin name=\"VoiceAllocationMode\" datatype=\"int\" private=\"true\" hostConnect=\"VoiceAllocationMode\" />"
			  "<Pin name=\"Portamento\" datatype=\"float\" private=\"true\" hostConnect=\"Portamento\" />"
			  "<Pin name=\"BenderRange\" datatype=\"float\" private=\"true\" hostConnect=\"BenderRange\" />"
			  "<Pin name=\"Voice/Aftertouch\" datatype=\"float\" private=\"true\" isPolyphonic=\"true\" hostConnect=\"Voice/Aftertouch\" />"
			  "<Pin name=\"ChannelPressure\" datatype=\"float\" private=\"true\" isPolyphonic=\"true\" hostConnect=\"Channel Pressure\" />"
		      "<Pin name=\"Voice/Bender\" datatype=\"float\" private=\"true\" isPolyphonic=\"true\" hostConnect=\"Voice/Bender\" />"
			  "</Audio>"
		  "</Plugin>"
		"</PluginList>"
		);
#endif
}

MidiToCv2::MidiToCv2( ) :
previousGate_(0.0f)
, currentGate_(0.0f)
{
	// Register pins.
	initializePin(pinMIDIIn);
	initializePin(pinChannel);
	initializePin(pinTrigger);
	initializePin(pinGate);
	initializePin(pinPitch);
	initializePin(pinVelocity);
	initializePin(pinAftertouchOut);
	initializePin(pinVoiceActive);
	initializePin(pinVoiceGate);
	initializePin(pinVoiceTrigger);
	initializePin(pinVoiceVelocityKeyOn);
	initializePin(pinVoicePitch);
	initializePin(pinVoiceVirtualVoiceId);
	initializePin(pinBender);
	initializePin(pinHoldPedal);
	initializePin(pinGlideStartPitch);
	initializePin(pinVoiceAllocationMode);
	initializePin(pinPortamento);
	initializePin(pinBenderRange);
	initializePin(pinVoiceAftertouch);
	initializePin(pinChannelPressure);
	initializePin(pinVoiceBender);
}

int32_t MidiToCv2::open()
{
	MpBase2::open();	// always call the base class

	pinVelocity.setCurveType(SmartAudioPin::Curved);

	SET_PROCESS2(&MidiToCv2::subProcess);

	pinTrigger.setTransitionTime(getSampleRate() * 0.0005f); // 0.5 ms trigger pulse.
	benderInterpolator_.Init(getSampleRate());
	pinAftertouchOut.setCurveType(SmartAudioPin::LinearAdaptive);

#ifdef _DEBUG
	static int temp = 0; // not in release
	debugVoice = temp++;
#endif

	return gmpi::MP_OK;
}

void MidiToCv2::subProcess( int sampleFrames )
{
	auto bufferOffset = getBlockPosition();
	if( pinPitch.isStreaming() && pitchInterpolator_.isDone() && benderInterpolator_.isDone() )
	{
		pinPitch.setStreaming(false, bufferOffset);
	}

	float* gate = getBuffer(pinGate);
	float* pitch = getBuffer(pinPitch);
	
	for( int s = sampleFrames; s > 0; s-- )
	{
		*gate++ = currentGate_;
		*pitch++ = pitchInterpolator_.getNext() + benderInterpolator_.getNext();
	}

	bool canSleep = true;
	pinVelocity.subProcess(bufferOffset, sampleFrames, canSleep);
	pinTrigger.subProcess(bufferOffset, sampleFrames, canSleep);
	pinAftertouchOut.subProcess(bufferOffset, sampleFrames, canSleep);
}

void MidiToCv2::CleanVelocityAndAftertouch()
{
	// Voice needs clean start, no Velocity Smoothing.
	// assume velocity changes on exact same sample as reset.
	pinVelocity.setValueInstant(0.1f * pinVoiceVelocityKeyOn); // 0.1 to convert to Volts.

	if(usePolyAftertouch())
	{
		pinAftertouchOut.setValueInstant(0.1f * pinVoiceAftertouch);
	}
	else
	{
		pinAftertouchOut.setValueInstant(pinChannelPressure);
	}
}

void MidiToCv2::onSetPins()
{
	// Actual value of trigger signal is garbage. All that matters is that it updated.
	const bool triggered = pinVoiceTrigger.isUpdated() && pinVoiceActive == 1.0f; // pinVoiceActive is checked to avoid a spurious pulse on first sample.

	if( triggered )
	{
		// _RPT1(_CRT_WARN, "MidiToCv2::triggered Voice/Bender=%f\n", (float) pinVoiceBender );
		//		_RPT1(_CRT_WARN, "MidiToCv2::onSetPins Trigger=%f\n", (double) pinTrigger );
		pinTrigger.pulse(1.0f);

		/* seems redundant?
		// On hard Note-on, Velocity jumps.
		if( !pinVoiceActive.isUpdated() || pinVoiceActive <= 0.0f )
		{
			hardReset = true;
			CleanVelocityAndAftertouch();
		}
		*/
	}

	// Voice reset happens at hard note ons, unless mono mode is on and a voice is already sounding. (not suspended)
	bool hardReset = false;
	if( pinVoiceActive.isUpdated() )
	{
		_RPTN(_CRT_WARN, "V%2d MidiToCv2::onSetPins pinVoiceActive=%f\n", debugVoice, pinVoiceActive.getValue() );
		if( pinVoiceActive > 0.0f )
		{
			if (pinVoiceActive < 1.0f) // voiceActive of 0.5 indicates overlap voice, don't sustain.
			{
				if (currentGate_ != 0.0f)
				{
					currentGate_ = 0.0f;
					pinGate.setUpdated();
				}
			}
			else
			{
				hardReset = true;
				CleanVelocityAndAftertouch();

				// Gate does notr nesc change on stolen voice when poly=1. Force non-glide.
				previousGate_ = 0;
			}
		}
	}

	/*
	if (pinVoiceBender.isUpdated())
	{
		_RPT1(_CRT_WARN, "MidiToCv2::onSetPins Voice/Bender=%f\n", (float) pinVoiceBender );
	}
	*/

	if( pinHoldPedal.isUpdated() )
	{
		//		_RPT1(_CRT_WARN, "MidiToCv2::onSetPins pinHoldPedal=%f\n", (double) pinHoldPedal );
		if (currentGate_ != 0.0f && pinVoiceGate == 0.0f && (pinHoldPedal < 5.0f || pinVoiceActive < 1.0f))
		{
			currentGate_ = 0.0f;
			pinGate.setUpdated();
		}
	}

	// PITCH.
	bool pitchUpdated = false;
	if (pinVoiceBender.isUpdated() || pinBender.isUpdated() || pinBenderRange.isUpdated())
	{
#if 0 //_DEBUG
		_RPTN(_CRT_WARN, "V%2d MidiToCv2::onSetPins Bender=%f\n\n", debugVoice, pinBender.getValue());

		if(pinBenderRange.isUpdated())
		{
			_RPTN(_CRT_WARN, "MidiToCv2 BenderRange=%f\n\n", pinBenderRange.getValue());
		}
#endif

		constexpr float benderRangeScale = 1.0f / 120.0f;
		// voice bender is hard-coded to 48 semitones (for MPE)
		const float totalBend = pinVoiceBender * 0.05f + pinBender * pinBenderRange * benderRangeScale;
		benderInterpolator_.setTarget(totalBend);
		pitchUpdated = true;
	}

	if( pinPortamento.isUpdated() || pinVoiceAllocationMode.isUpdated() )
	{
		const bool constantTimeGlide = 0 != voice_allocation::extractBits(pinVoiceAllocationMode, voice_allocation::bits::GlideRate_startbit, 1);

		//_RPT2(_CRT_WARN, "MidiToCv2:Glidetime %fs, inc=%f\n", VoltageToTime(pinPortamento), 1.0f / getSampleRate() * VoltageToTime(pinPortamento));
		if( constantTimeGlide )
		{
			pitchInterpolator_.setTransitionTime(getSampleRate() * VoltageToTime(pinPortamento));
		}
	}

	// glide start is only needed on fresh voice activations.
	// else it's a soft-steal and current pitch should already be correct.
	if (hardReset)
	{
		// _RPTN(_CRT_WARN, "V%2d MidiToCv2.setValueInstant(GlideStartPitch:%f)\n", debugVoice, 0.1f * pinGlideStartPitch);
		pitchInterpolator_.setValueInstant(0.1f * pinGlideStartPitch);

		// fix issue of MPE pitch blip on note-on due to benderInterpolator not done from previous note.
		benderInterpolator_.jumpToTarget();
	}

	// If pinVoicePitch updated, glide to that value.
	if (pinVoicePitch.isUpdated() || hardReset)
	{
		// for constant RATE glide, first recalc glide time.
		const bool constantRateGlide = GT_CONST_RATE == voice_allocation::extractBits(pinVoiceAllocationMode, voice_allocation::bits::GlideRate_startbit, 1);
		if (constantRateGlide)
		{
			const float deltaPitch = fabsf(pinGlideStartPitch - pinVoicePitch);
			pitchInterpolator_.setTransitionTime(getSampleRate() * VoltageToTime(pinPortamento) * deltaPitch);
		}

		pitchInterpolator_.setTarget(0.1f * pinVoicePitch);
		pitchUpdated = true;
	}

	// isFirstSample prevents pitch gliding gradually from zero (pinGlideStartPitch) at startup.
	if (isFirstSample)
	{
		pitchInterpolator_.jumpToTarget();
		pitchUpdated = true;
	}

#if 0
	// For samplers it's important to have no glide whatsoever, else wrong key layer is selected.
	if (pinVoicePitch.getValue() == pinGlideStartPitch.getValue() || pinPortamento <= 0.0f || isFirstSample)
	{
		pitchUpdated = pitchInterpolator_.getInstantValue() != 0.1f * pinVoicePitch;
		pitchInterpolator_.setValueInstant(0.1f * pinVoicePitch);

		_RPTN(_CRT_WARN, "V%2d pitchInterpolator_.setTarget(%f) (instant)\n", debugVoice, 0.1f * pinVoicePitch);
	}
	else
	{
		// glide start is only needed on fresh voice activations.
		// else it's a soft-steal and current pitch should already be correct.
		if (hardReset)
		{
			_RPTN(_CRT_WARN, "V%2d MidiToCv2.setValueInstant(GlideStartPitch:%f)\n", debugVoice, 0.1f * pinGlideStartPitch);
			pitchInterpolator_.setValueInstant(0.1f * pinGlideStartPitch);
		}
		_RPTN(_CRT_WARN, "V%2d pinVoicePitch =%f\n", debugVoice, pinVoicePitch.getValue());

		const bool constantRateGlide = GT_CONST_RATE == voice_allocation::extractBits(pinVoiceAllocationMode, voice_allocation::bits::GlideRate_startbit, 1);
		//			_RPT1(_CRT_WARN, "constantRateGlide =%ds\n", (int) constantRateGlide );
		if (constantRateGlide)
		{
			// recalculate glide time.
			const float deltaPitch = fabsf(pinGlideStartPitch - pinVoicePitch);
			//_RPT1(_CRT_WARN, "Glide Time =%fs\n", (double) VoltageToTime( pinPortamento ) * deltaPitch );
			pitchInterpolator_.setTransitionTime(getSampleRate() * VoltageToTime(pinPortamento) * deltaPitch);
		}

		_RPTN(_CRT_WARN, "V%2d pitchInterpolator_.setTarget(%f) (glide)\n", debugVoice, 0.1f * pinVoicePitch);
		pitchInterpolator_.setTarget(0.1f * pinVoicePitch);
		pitchUpdated = true;
	}
#endif

	if (pitchUpdated)
	{
//		_RPTN(_CRT_WARN, "V%2d pinPitch.setStreaming(%d))\n", debugVoice, (int)(!pitchInterpolator_.isDone() || !benderInterpolator_.isDone()));
		pinPitch.setStreaming(!pitchInterpolator_.isDone() || !benderInterpolator_.isDone());
	}
	
	bool legato = true;

	if( pinVoiceGate.isUpdated() )
	{
		if( pinVoiceGate != 0.0f )
		{
			currentGate_ = 1.0f;
		}
		else
		{
			if (pinHoldPedal < 5.0f || pinVoiceActive < 1.0f) // voiceActive of 0.5 indicates overlap voice, don't sustain.
			{
				currentGate_ = 0.0f;
			}
		}

		//_RPT1(_CRT_WARN, "MidiToCv2::onSetPins gate = %f\n", (double) pinVoiceGate );
		pinGate.setStreaming(false);

		// gate transition to HI indicates note-on (not legato.
		if( previousGate_ == 0.0f && pinVoiceGate != 0.0f )
		{
			legato = false;
		}

		previousGate_ = pinVoiceGate;
	}

	if( pinVoiceVelocityKeyOn.isUpdated() )
	{
		// no point setting it again if we just did.
		if( !pinVoiceActive.isUpdated() || pinVoiceActive <= 0.0f )
		{
			float newVelocity = 0.1f * pinVoiceVelocityKeyOn; // convert to Volts.

			if( legato ) // glide smoothly to new velocity.
			{
				pinVelocity.setTransitionTime(getSampleRate() * 0.008f); // 8ms
			}
			else // re-play decaying note, move to new velocity as quickly as pos to keep attack snappy.
			{
				pinVelocity.setTransitionTime(getSampleRate() * 0.0015f); // 1.5ms
			}

			pinVelocity = newVelocity;
		}
	}

	if( pinVoiceAftertouch.isUpdated())
	{
//		_RPT1(_CRT_WARN, "MidiToCv2::onSetPins pinVoiceAftertouch = %f\n", (float) pinVoiceAftertouch );
		polyAftertouchDetected |= (pinVoiceAftertouch > 0.0f);

		if( usePolyAftertouch() )
		{
			pinAftertouchOut.setValue(0.1f * pinVoiceAftertouch); // voice aftertouch range is 0 - 10
		}
	}

	if( pinChannelPressure.isUpdated() )
	{
		monoAftertouchDetected |= (pinChannelPressure > 0.0f);
		if(!usePolyAftertouch() )
		{
			pinAftertouchOut.setValue(pinChannelPressure); // channel aftertouch range is 0 - 1
		}
	}
#if 0
	if (triggered)
	{
		_RPT0(_CRT_WARN, "                   MidiToCv2: Triggered\n");
		_RPTN(_CRT_WARN, "                              Pitch     %f\n", pitchInterpolator_.getInstantValue());
		_RPTN(_CRT_WARN, "                              target    %f\n", pitchInterpolator_.getTargetValue());
		_RPTN(_CRT_WARN, "                              Bend      %f\n", benderInterpolator_.getInstantValue());
		_RPTN(_CRT_WARN, "                              target    %f\n", benderInterpolator_.getTargetValue());
	}
#endif

	isFirstSample = false;
}

