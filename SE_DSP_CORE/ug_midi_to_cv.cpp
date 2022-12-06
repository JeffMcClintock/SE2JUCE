#include "pch.h"
#include "ug_midi_to_cv.h"
#include <math.h>

#include "midi_defs.h"
#include "ug_container.h"
#include "SeAudioMaster.h"
#include "module_register.h"
#include "ug_event.h"
#include "modules/shared/voice_allocation_modes.h"
#include "iseshelldsp.h"

SE_DECLARE_INIT_STATIC_FILE(ug_midi_to_cv)

#define PN_CHAN 1

// Fill an array of InterfaceObjects with plugs and parameters
void ug_notesource::ListInterface2(InterfaceObjectArray& PList)
{
	// IO Var, Direction, Datatype, NoteSource, ppactive, Default, defid (index into unit_gen::PlugFormats)
	LIST_VAR3N( L"MIDI In", DR_IN, DT_MIDI2 , L"0", L"", 0, L"");
	LIST_VAR3( L"Channel", midi_channel/*midi_in.channel()*/, DR_IN, DT_ENUM , L"-1", MIDI_CHAN_LIST,IO_IGNORE_PATCH_CHANGE|IO_POLYPHONIC_ACTIVE, L"MIDI Channel");
}

ug_notesource::ug_notesource() :
	retrigger_old(0)
	,cur_channel(-1)
	,mono_mode_old(0)
	,m_poly_mode_old(VA_POLY_HARD) // hard steal.
{
	SetFlag(UGF_POLYPHONIC | UGF_POLYPHONIC_GENERATOR /* moved to redirector| UGF_UPSTREAM_PATCH_MGR*/ /* | UGF_NEVER_SUSPEND */);
	SET_CUR_FUNC( &ug_notesource::sub_process );
}

void ug_notesource::onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state )
{
	if( CloneOf() == this && p_to_plug == GetPlug( PN_CHAN ) )
	{
		if( midi_channel != cur_channel ) // changin channel
		{
//			AllNotesOff();
			OnChangeChannel();
			cur_channel = midi_channel;
		}
	}

	ug_base::onSetPin( p_clock, p_to_plug, p_state );
}

void ug_notesource::updateVoiceAllocationMode()
{
	int voiceAllocationMode = m_poly_mode_old;
	if( voiceAllocationMode != -1 ) // Ignore local, use Patch-Automator's voice allocation settings
	{
		if( mono_mode_old )
		{
			if( retrigger_old )
			{
				voiceAllocationMode = VA_MONO_RETRIGGER_DEPRECATED;
			}
			else
			{
				voiceAllocationMode = VA_MONO_DEPRECATED;
			}

			voiceAllocationMode += ( m_mono_note_priority << 8 );
		}

		if( !m_auto_glide )
		{
			voiceAllocationMode |= ( 0x01 >> 16 ); // Glide mode = Always.
		}

		if( m_portamento_constant_rate )
		{
			voiceAllocationMode |= ( 0x01 >> 18 );
		}
	}

	parent_container->overridingVoiceAllocationMode_ = voiceAllocationMode;
}

int ug_notesource::getVoiceAllocationMode()
{
	int voiceAllocationMode = m_poly_mode_old;
	if (voiceAllocationMode == -1) // Ignore local, use Patch-Automator's voice allocation settings
	{
		voiceAllocationMode = parent_container->voiceAllocationMode_;// patch_control_container->get_patch_manager()->getVoiceAllocationMode();
	}
	else
	{
		if (mono_mode_old)
		{
			if (retrigger_old)
			{
				voiceAllocationMode = VA_MONO_RETRIGGER_DEPRECATED;
			}
			else
			{
				voiceAllocationMode = VA_MONO_DEPRECATED;
			}

			voiceAllocationMode += (m_mono_note_priority << 8);

			if( m_portamento_constant_rate )
			{
				voiceAllocationMode |= ( 1 << 18 );
			}
		}
	}
	return voiceAllocationMode;
}

namespace
{
	REGISTER_MODULE_1(L"Midi to CV", IDS_MN_MIDI_TO_CV,IDS_MG_OLD,ug_midi_to_cv ,CF_NOTESOURCE|CF_DONT_EXPAND_CONTAINER|CF_STRUCTURE_VIEW,L"This module converts MIDI notes into control voltages.  The midi note number is converted into a pitch voltage ( 1Volt per octave).  Connect this to an Oscillator or Filter to control frequency.  The Trigger output goes high while the key is depressed.  Connect the trigger output to an ADSR gate to trigger an envelope.  This module plays an important part in making SynthEdit Polyphonic. You must put each MIDI to CV module in it's own Container.  See <a href=signals.htm>Signal Levels and Conversions</a> for Voltage to pitch conversion info");

	auto r2 = internalSdk::Register<ug_midi_to_cv_redirect>::withXml(
		(R"XML(
<?xml version="1.0" ?>
<PluginList>
  <Plugin id="SE MIDICVRedirect" name="MIDI-CV Redirect" category="Debug" >
    <Audio>
		<Pin name="MIDI In" datatype="midi" />
		<Pin name="Channel" datatype="enum" default="-1" ignorePatchChange="true" metadata="All=-1,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16" />
        <Pin name="MIDI Out" datatype="midi" direction="out" private="true" />
    </Audio>
  </Plugin>
</PluginList>
)XML")
);
}

#define PLG_BEND_RANGE 2
#define PLG_GATE 3
#define PLG_PITCH 4
#define PLG_VELOCITY 5
#define PLG_AFTERTOUCH 6
#define PN_MONO_MODE		7
#define PLG_PORTAMENTO		9
#define PLG_NOTE_PRIORITY	10

#define PLG_POLY_MODE		13

#define PLG_VOICE_ACTIVE	14
#define PLG_VOICE_GATE		18
#define PLG_VOICE_TRIGGER		19
#define PLG_VOICE_VELOCITY		20
#define PLG_VOICE_PITCH			21
#define PLG_VOICE_AFTERTOUCH	22
#define PLG_BENDER				24
#define PLG_PORATAMENTO_ENABLE	25
#define PLG_HOLD_PEDAL			26

// Fill an array of InterfaceObjects with plugs and parameters
void ug_midi_to_cv::ListInterface2(InterfaceObjectArray& PList)
{
	ug_notesource::ListInterface2(PList);	// Call base class
	//           ("Plug Name",variable, direction,datatype,"Default","enum list/range", flags, "comment");
	LIST_VAR3( L"Bend Range",BendRange, DR_IN, DT_ENUM , L"2", L"range 0,12", 0, L"Maximum MIDI bender range in semitones");
	LIST_PIN( L"Gate", DR_OUT, L"0", L"", 0, L"Use this to control an envelope (ADSR)");
	LIST_PIN( L"Pitch", DR_OUT, L"0", L"", 0, L"Use this to control an Oscillator or Filter Pitch");
	LIST_PIN( L"Velocity", DR_OUT, L"0", L"", 0, L"Provides MIDI velocity level (How fast you hit the key)");
	LIST_PIN( L"Aftertouch", DR_OUT, L"0", L"", 0, L"Provides MIDI Aftertouch level");
	LIST_VAR3( L"Mono Mode", mono_mode_old, DR_IN, DT_ENUM , L"0", L"On=1,Off=0",IO_IGNORE_PATCH_CHANGE, L"Limit sound to one voice. Enables the Retrigger option");
	LIST_VAR3( L"Retrigger", retrigger_old, DR_IN, DT_ENUM , L"1", L"On=1,Off=0",IO_IGNORE_PATCH_CHANGE, L"Retrigger Envelopes when playing legato (you hit a note before releasing previous). Used with Mono Mode.");
	LIST_PIN( L"Portamento Time", DR_IN, L"0", L"",IO_IGNORE_PATCH_CHANGE|IO_LINEAR_INPUT, L"Time for pitch to slide to a new note (When you play legato)");
	LIST_VAR3( L"Mono Note Priority", m_mono_note_priority, DR_IN, DT_ENUM, L"3", L"Off,Low,High,Last",IO_IGNORE_PATCH_CHANGE, L"");
	LIST_VAR3( L"Polyphony", trash_sample_ptr, DR_IN, DT_INT, L"6", L"", IO_DISABLE_IF_POS|IO_PARAMETER_SCREEN_ONLY, L"");
	LIST_VAR3( L"Polyphony Reserve", trash_sample_ptr, DR_IN, DT_INT, L"3", L"", IO_DISABLE_IF_POS|IO_PARAMETER_SCREEN_ONLY, L"");
	LIST_VAR3(L"Polyphony Mode", m_poly_mode_old, DR_IN, DT_ENUM, L"-1", L"Overriden =-1,Soft Steal, Hard Steal, Overlap", /*IO_DISABLE_IF_POS |*/ IO_PARAMETER_SCREEN_ONLY, L"");
	//	LIST_VAR3( L"ToPPS",midi_in, DR_OUT, DT_MIDI2 , L"", L"", IO_DISABLE_IF_POS, L"");
	// TODO !!! Note-Off-Velocity !!
	// detect voice reset, so velocity glide/jump selection works.
	LIST_VAR3( L"Voice/Active", m_voice_active, DR_IN, DT_FLOAT , L"", L"", IO_HOST_CONTROL|IO_HIDE_PIN|IO_PAR_POLYPHONIC, L"");
	LIST_VAR3( L"Portamento Rate", m_portamento_constant_rate, DR_IN, DT_ENUM, L"0", L"Constant Time,Constant Rate", 0, L"");
	LIST_VAR3( L"Auto Glide", m_auto_glide, DR_IN, DT_ENUM, L"0", L"Off,On", 0, L"");

	// With new sort-order setup, need to ensure master MIDI-CV executed before slaves, else note-on don't work.
	// This dummy MIDI-Out can be connected to the clone's MIDI-In.
	LIST_VAR3N(L"MIDI Out", DR_OUT, DT_MIDI2, L"0", L"", IO_HIDE_PIN, L""); // Obsolete.

	LIST_VAR3(L"Voice/Gate", m_voice_gate, DR_IN, DT_FLOAT, L"", L"", IO_HOST_CONTROL | IO_HIDE_PIN | IO_PAR_POLYPHONIC, L"");
	LIST_VAR3(L"Voice/Trigger", m_voice_trigger, DR_IN, DT_FLOAT, L"", L"", IO_HOST_CONTROL | IO_HIDE_PIN | IO_PAR_POLYPHONIC, L"");
	LIST_VAR3(L"Voice/VelocityKeyOn", m_voice_velocity, DR_IN, DT_FLOAT, L"", L"", IO_HOST_CONTROL | IO_HIDE_PIN | IO_PAR_POLYPHONIC, L"");
	LIST_VAR3(L"Voice/Pitch", m_voice_pitch, DR_IN, DT_FLOAT, L"", L"", IO_HOST_CONTROL | IO_HIDE_PIN | IO_PAR_POLYPHONIC, L"");
	LIST_VAR3(L"Voice/Aftertouch", m_voice_aftertouch, DR_IN, DT_FLOAT, L"", L"", IO_HOST_CONTROL | IO_HIDE_PIN | IO_PAR_POLYPHONIC, L"");
	LIST_VAR3(L"Voice/VirtualVoiceId", m_voice_id, DR_IN, DT_INT, L"", L"", IO_HOST_CONTROL | IO_HIDE_PIN | IO_PAR_POLYPHONIC, L"");
	LIST_VAR3(L"Bender", m_bender, DR_IN, DT_FLOAT, L"", L"", IO_HOST_CONTROL | IO_HIDE_PIN, L"");
	LIST_VAR3(L"Voice/PortamentoEnable", m_portamento_enable, DR_IN, DT_FLOAT, L"", L"", IO_HOST_CONTROL | IO_HIDE_PIN | IO_PAR_POLYPHONIC, L"");
	LIST_VAR3(L"HoldPedal", m_hold_pedal, DR_IN, DT_FLOAT, L"", L"", IO_HOST_CONTROL | IO_HIDE_PIN, L"");
	// can't do yet.	LIST_VAR3(L"RPN-Raw/0", m_bend_ammount, DR_IN, DT_FLOAT, L"", L"", IO_HOST_CONTROL | IO_HIDE_PIN, L""); // Bend Ammount
//	LIST_VAR3(L"GlideStartPitch", GlideStartPitch_, DR_IN, DT_FLOAT, L"", L"", IO_HOST_CONTROL | IO_HIDE_PIN, L"");
}

int ug_midi_to_cv_redirect::Open()
{
	GetPlug(1)->AssignVariable(&midi_channel);

	voiceState_.voiceControlContainer_ = parent_container;

#if 0
	{
		// On reset, restore tuning table.
		auto app = AudioMaster()->getShell();
		auto tuningTable = voiceState_.getTuningTable();
		auto tunningTableRaw = app->getPersisentHostControl(voiceState_.voiceControlContainer_->Handle(), HC_VOICE_TUNING, RawView((const void*)tuningTable, sizeof(int) * 128));
		memcpy(&tuningTable, tunningTableRaw.data(), sizeof(tuningTable));

		//		_RPT2(_CRT_WARN, "ug_midi_to_cv_redirect[%x] TUNE %x\n", voiceState_.voiceControlContainer_->Handle(), voiceState_.GetIntKeyTune(61)); // C#4
	}
#endif

	// To avoid double-ups from MIDI going into both patch-automator and MIDI-CV, 
	// notify Patch-Automator to avoid allocating voices etc.
	if (ParentContainer()->has_own_patch_manager() )
	{
		get_patch_manager()->setMidiCvVoiceControl();
	}

	return ug_midi_device::Open();
}

void ug_midi_to_cv_redirect::OnMidiData(int size, unsigned char* midi_bytes)
{
	get_patch_manager()->OnMidi(&voiceState_, SampleClock(), midi_bytes, size, true);
}

// unlikely value indicating 'not set'.
#define INITIAL_PORTAMENTO_VALUE -1000000.0f

ug_midi_to_cv::ug_midi_to_cv() :
	pitch_pl(0)
	,portamento_increment(0.f)
	,pitch_bend_increment(0.f)
	,pitch_bend(0.f)
	,pitch_bend_target(0.f)
	,portamento_pitch(0.5f) // default output middle-A (CK request)
	,portamento_target(0.5f)
	,velocity_so(SOT_S_CURVE, 8 ) // now variable, see NoteOn()
	,gate_so(SOT_NO_SMOOTHING)
	,last_note(-1)
	,voiceResetBetweenNotes(true)
	, m_bend_amt(0.0f)
	//, polyGlide(false)
	//, polyGlideRetrigger(false)
	, noteKeyDown_(false)
	, m_portamento_enable(false)
	,ignoreNoteOnPitch_(false)
	, m_held(false)
{
	SET_CUR_FUNC( &ug_midi_to_cv::sub_process );
	SetFlag(UGF_POLYPHONIC_GENERATOR_CLONED| UGF_HAS_HELPER_MODULE |UGF_DELAYED_GATE_NOTESOURCE);
}

int ug_midi_to_cv::Open()
{
	ug_notesource::Open();

	// Only the first MIDI-CV need MIDI. Save waking all on MIDI data by severing MIDI-in
	if( CloneOf() != this )
	{
		GetPlug(0)->DeleteConnectors();
	}

	// time saving
	pitch_pl = GetPlug(PLG_PITCH);// ->GetSampleBlock();
	OutputChange( SampleClock(), GetPlug(PLG_PITCH), ST_STATIC );
	velocity_so.SetPlug( GetPlug(PLG_VELOCITY) );
	aftertouch_so.SetPlug( GetPlug(PLG_AFTERTOUCH) );
	gate_so.SetPlug( GetPlug(PLG_GATE) );
	return 0;
}

void ug_midi_to_cv::sub_process(int start_pos, int sampleframes)
{
	bool can_sleep = true;
/*
	// update master portamento if nesc
	if( master_portamento_increment != 0.0f )
	{
		// calc amount of port time remaining
		float remain = ( master_portamento_target - master_portamento_pitch ) / master_portamento_increment;
		float sf = (float)sampleframes;

		if( remain > sf )
		{
			master_portamento _pitch += sf * master_portamento_increment;
			can_sleep = false;
		}
		else
		{
			master_ portamento_pitch = master_portamento_target;
			master_portamento_increment = 0.0f;
		}
	}
*/
	assert( portamento_increment != 0.f || portamento_pitch == portamento_target );

	// output pitch
	if( static_output_count > 0 )
	{
		if( pitch_bend_increment == 0.0f && portamento_increment == 0.0f )
		{
			assert( pitch_bend == pitch_bend_target );
//			pitch_pl->GetSampleBlock()->SetRange(start_pos, sampleframes, pitch_bend + portamento_pitch);
			pitch_pl->SetRange(start_pos, sampleframes, pitch_bend + portamento_pitch);
			static_output_count -= sampleframes;

			if( static_output_count > 0 )
			{
				can_sleep = false;
			}
		}
		else
		{
			can_sleep = false;
			assert(	static_output_count == AudioMaster()->BlockSize() );
			int count = sampleframes;
			float* p = pitch_pl->GetSamplePtr() + start_pos;

			while( count > 0 )
			{
				int todo = count;
				bool pb_done = false;
				bool pm_done = false;

				if( portamento_increment != 0.0f )
				{
					float remain = 0.5f + ( portamento_target - portamento_pitch ) / portamento_increment;

					if( remain < todo )
					{
						todo = (int)remain;

						// in rare circumstances, when portamento_increment VERY small,
						// numeric errors can cause todo < 0
						// eliminate < 0
						if( todo < 0 )
							todo = 0;

						pm_done = true;
					}
				}

				if( pitch_bend_increment != 0.0f )
				{
					float remain = 0.5f + ( pitch_bend_target - pitch_bend ) / pitch_bend_increment;

					if( remain < todo )
					{
						pb_done = true;
						todo = (int)remain;

						// in rare circumstances, when increment VERY small,
						// numeric errors can cause todo < 0
						// eliminate < 0
						if( todo < 0 )
							todo = 0;
					}
				}

				assert(todo >= 0 );
				int c = todo;

				while( c-- > 0 )
				{
					pitch_bend += pitch_bend_increment;
					portamento_pitch += portamento_increment;
					*p++ = pitch_bend + portamento_pitch;
				}

				count -= todo;
				assert( sampleframes >= count );

				if( pm_done )
				{
					portamento_pitch = portamento_target;
					portamento_increment = 0.0f;

					if( pitch_bend_increment == 0.0f ) // finished sweep
					{
						OutputChange( SampleClock() + sampleframes - count + 1, GetPlug(PLG_PITCH), ST_STOP );
					}
				}

				if( pb_done )
				{
					pitch_bend = pitch_bend_target;
					pitch_bend_increment = 0.0f;

					if( portamento_increment == 0.0f ) // finished sweep
					{
						// plus one to allow exact target to be processed
						OutputChange( SampleClock() + sampleframes - count + 1, GetPlug(PLG_PITCH), ST_STOP );
					}
				}
			}
		}
	}

	/*
		if( portamento_increment != 0.0f || pitch_bend_increment != 0.0f )
		{
			can_sleep = false;
			int start_pos2 = start_pos;

			// first portamento
			int remainder = sampleframes;
			if( portamento_increment != 0.0f )
			{
				// calc amount of port time remaining
				float remain = ( portamento_target - portamento_pitch ) / portamento_increment;
				int todo = min( remain, sampleframes );
				for( int i = 0 ; i < todo ; i++ )
				{
					portamento_pitch += portamento_increment;
					//RecalcPitch();
					pitch_bl->SetAt( i + start_pos, portamento_pitch );
				}

				remainder -= todo;
				start_pos2 += todo;

				if( remainder > 0 )
				{
					portamento _pitch = portamento_target;
					portamento_increment = 0.0f;
					sta tic _output_count = GetIoMgr()->Get BlockSize();
				}
				else // update 'pitch' variable
				{
					RecalcPitch();
				}
			}

			// do remaining samples
			if( remainder > 0 )
			{
				pitch_bl->SetRange( start_pos2, remainder, portamento_pitch );
			}

			// now ADD pitch bend, if any
			remainder = sampleframes;
			float *p = pitch_bl->GetBlock() + start_pos;
			if( pitch_bend_increment != 0.0f )
			{
				// calc amount of port time remaining
				float remain = ( pitch_bend_target - pitch_bend ) / pitch_bend_increment;
				int todo = min( remain, sampleframes );
				for( int i = 0 ; i < todo ; i++ )
				{
					pitch_bend += pitch_bend_increment;
					//pitch_bl->SetAt( i + start_pos, pitch_bend );
					*p++ += pitch_bend;
				}

				remainder -= todo;
				// do remaining samples
				if( remainder > 0 )
				{
					pitch_bend = pitch_bend_target;
					pitch_bend _increment = 0.0f;
					sta tic _output_count = GetIoMgr()->Get BlockSize();
				}
				else // update 'pitch' variable
				{
					RecalcPitch();
				}
			}

			// do remaining samples
			while( remainder -- > 0 )
			{
				*p++ += pitch_bend;
			}
		}
		else
		{
			pitch_bl->SetRange( start_pos, sampleframes, pitch );
			sta tic _output_count -= sampleframes;
			if( sta tic _output_count > 0 )
			{
				can_sleep = false;
			}
		}
	*/
	// output gate
	//gate_bl->SetRange( start_pos, sampleframes, gate );
	gate_so.Process( start_pos, sampleframes, can_sleep);
	// output velocity
	velocity_so.Process( start_pos, sampleframes, can_sleep);
	// output aftertouch
	aftertouch_so.Process( start_pos, sampleframes, can_sleep);

	if( can_sleep )
	{
		SET_CUR_FUNC( &ug_base::process_sleep );
	}
}

void ug_midi_to_cv::PitchBend( float bend_amt)
{
	Wake();
	m_bend_amt = bend_amt; // save for pitch-bend range changes.
	bool send_stat_change = pitch_bend_increment == 0.0f;
	pitch_bend_target = bender_to_voltage(bend_amt);
	// glide pitch bend toward new pitch
	// calculate time in samples for glide
	float smooth_time = getSampleRate() / MIDI_CONTROLLER_SMOOTHING;
	pitch_bend_increment = ( pitch_bend_target - pitch_bend ) / smooth_time;  //difference v

	if( send_stat_change && pitch_bend_increment != 0 ) // ensure bend amnt not zero
	{
		SET_CUR_FUNC( &ug_midi_to_cv::sub_process );
		ResetStaticOutput();
		OutputChange( SampleClock(), GetPlug(PLG_PITCH), ST_RUN );
	}

	//	_RPT1(_CRT_WARN, "ug_midi_to_cv::PitchBend %f\n", pitch_bend_target );
}

void ug_midi_to_cv::Aftertouch(float val)
{
	SET_CUR_FUNC( &ug_midi_to_cv::sub_process );
	aftertouch_so.Set( SampleClock(), val );
}

void ug_midi_to_cv::Retune( float pitch )
{
	portamento_target = pitch;
    // Default is no glide
    portamento_increment = 0.0f;

	const bool polyGlide = m_portamento_enable >= 0.5f || voiceResetBetweenNotes == false;
	if (polyGlide)
	{
        // glide starts at pitch of last played voice
        float portamento_time = GetPlug(PLG_PORTAMENTO)->getValue();
        if( portamento_time > 0.0f ) // force no glide when portamento set to zero
        {
            // glide pitch toward new pitch
            // calculate time in samples for glide
            // same scale as an envelope seqment - 2 Volts for quicker response near zero (0.0025 seconds)
            float time = TimecentToSecond(VoltageToTimecent( portamento_time * 10.f - 2.f ));
            
			const float minimumNonZeroGlide = 0.00001f; // Prevent floating divide-by-zero exception due to time being VERY small.
			if( time > minimumNonZeroGlide ) // force no glide when portam set to zero
            {
                time = time * getSampleRate();
                
                if( m_portamento_constant_rate == 0 )
                {
                    portamento_increment = ( portamento_target - portamento_pitch ) / time;  //difference v
                }
                else
                {
                    // Distance fixed at 1 octave.
                    if( portamento_target > portamento_pitch )
                    {
                        portamento_increment = 0.1f / time;
                    }
                    else
                    {
                        portamento_increment = -0.1f / time;
                    }
                }
            }
        }

		// Retrigger
		bool polyGlideRetrigger = m_portamento_enable > 7.0f;
		if (polyGlideRetrigger && noteKeyDown_ )
		{
#ifdef DEBUG_NOTES
			_RPT2(_CRT_WARN, "V%d ug_midi_to_cv *polyGlideRetrigger* clock=%d\n", pp_voice_num, SampleClock());
#endif
			gate_so.Set(SampleClock(), 0.0f);

			RUN_AT(SampleClock() + 3, &ug_midi_to_cv::delay_gate_on);
		}

		voiceResetBetweenNotes = false; // indicates velocity can jump to new value without glitches.
	}

	ResetStaticOutput();
	if (portamento_increment == 0.0f)
	{
		portamento_pitch = portamento_target;
	}

	if (portamento_increment != 0.0f || pitch_bend_increment != 0.0f)
	{
		OutputChange(SampleClock(), GetPlug(PLG_PITCH), ST_RUN);
	}
	else
	{
		OutputChange(SampleClock(), GetPlug(PLG_PITCH), ST_STATIC);
	}

	SET_CUR_FUNC( &ug_midi_to_cv::sub_process );
}

/*
> I was just wondering what the correct MIDI-velocity to sample volume mapping
> looks like.
But the specifications of the SoundFonts and the
Downloadable Sounds specify something like:

Attenuation in dB = 20/96 log10(127^2 / Input^2)

However, that doesn't seem to work well.

After checking Timidity's code and measuring the output
of the SBLive! card I came to the conclusion that velocity
can be mapped linearly to the [0 dB, -36 dB] range.

So I used LinearAmp = pow(-36*(127-Velocity)/(127*20))


  I have an authentic, offset-printed, not photocopied, copy of the
Version 1.0 MIDI spec from August 5 1983.

The volume section on page 8 says

   A logarithmic scale would be advisable.

0 = note off and "64 in case of no velocity sensors".

There is a horizontal scale of numbers 1 to 127.

The following musical intensity terms are spread across the scale at the
following approximate positions, but the scale is not linear, since the
three numbers on the scale - 1, 64 and 127 - are not evenly spaced:

   ppp      1 (Exactly.)

   pp      20

   p       38

   mp      55

   mf      72

   f       88

			  (There's a big gap.)

   ff     118

   fff    127 (Exactly.)

*/

void ug_midi_to_cv::NoteOn2( /*short chan,*/ short VoiceId, float velocity, float pitch)
{
//	short chan = 0;
#ifdef DEBUG_NOTES
	_RPT4(_CRT_WARN, "V%d ug_midi_to_cv::NoteOn() clock=%d pitch %f vel %f\n", pp_voice_num, (int)SampleClock(), pitch, velocity);
#endif

	SET_CUR_FUNC( &ug_midi_to_cv::sub_process );

	// set pitch bend to correct setting for this chan
	pitch_bend = pitch_bend_target = bender_to_voltage(m_bend_amt); //  chan_info[chan].bender);
	pitch_bend_increment = 0.0;

	auto voiceAllocationMode = getVoiceAllocationMode();
	bool monoMode = voice_allocation::isMonoMode(voiceAllocationMode); // ( voiceAllocationMode & 0xff ) >= VA_MONO;
	bool retrigger = voice_allocation::isMonoRetrigger(voiceAllocationMode); //( voiceAllocationMode & 0xff ) == VA_MONO_RETRIGGER;

	bool glide_pitch = ( monoMode != 0 ) && ( m_auto_glide == 0 || gate_so.output_level > 0.0f );// && master_portamento_pitch != INITIAL_PORTAMENTO_VALUE;
	auto constantRateGlide = m_portamento_constant_rate;
/* moved
	if( retrigger != 0 || monoMode == 0 )
	{
#ifdef DEBUG_NOTES
		_RPT2(_CRT_WARN, "V%d ug_midi_to_cv gate=0 clock=%d\n", pp_voice_num, SampleClock());
#endif
		gate_so.Set( SampleClock(), 0.0f );
	}
*/

//	_RPT2(_CRT_WARN, "V%d ug_midi_to_cv voiceResetBetweenNotes=%d\n", pp_voice_num, (int)voiceResetBetweenNotes);
	if(retrigger || voiceResetBetweenNotes ) // then velocity 'jumps' to new value
	{
		gate_so.Set(SampleClock(), 0.0f);
		velocity_so.Set(SampleClock(), velocity, 0);
	}
	else // velocity 'glides; to new value
	{
		float transition_time = fabsf(velocity - velocity_so.output_level);
		//		_RPT1(_CRT_WARN, "transition voltage %f\n", transition_time * 10.f );
		// transition time varies with how far it has to go
		transition_time = 60.f + transition_time * 60.f;
		velocity_so.Set(SampleClock(), velocity, (int)transition_time);
	}

	voiceResetBetweenNotes = false; // indicates velocity can jump to new value without glitches.
	noteKeyDown_ = true;

	RUN_AT(SampleClock() + 3, &ug_midi_to_cv::delay_gate_on);

	portamento_target = pitch;
    portamento_increment = 0.0f; // used to indicate no-protamento

	if( glide_pitch )
	{
		// glide starts at pitch of last played voice
		float portamento_time = GetPlug(PLG_PORTAMENTO)->getValue();
		if( portamento_time > 0.0f ) // force no glide when portam set to zero
		{
			// glide pitch toward new pitch
			// calculate time in samples for glide
			// same scale as an envelope seqment - 2 Volts for quicker response near zero (0.0025 seconds)
			float time = TimecentToSecond(VoltageToTimecent( portamento_time * 10.f - 2.f ));

            const float minimumNonZeroGlide = 0.00001f; // Prevent floating point overflow exception due to time being VERY small.
            if( time > minimumNonZeroGlide ) // force no glide when portam set to zero
            {
                time = time * getSampleRate();

			    if( constantRateGlide == 0 )
			    {
				    portamento_increment = ( portamento_target - portamento_pitch ) / time;  //difference v
			    }
			    else
			    {
				    // Distance fixed at 1 octave.
				    if( portamento_target > portamento_pitch )
				    {
					    portamento_increment = 0.1f / time;
				    }
				    else
                    {
                        portamento_increment = -0.1f / time;
                    }
                }

                // this ug needs to track portamento
                SET_CUR_FUNC(&ug_midi_to_cv::sub_process );
            }
		}
	}

	if (portamento_increment == 0.0f)
	{
		portamento_pitch = portamento_target;
	}

	//_RPT4(_CRT_WARN, "MCV %d, start %.4f, target %.4f, inc %f\n", this, portamento_pitch, portamento_target, portamento_increment );
	//_RPT1(_CRT_WARN, "%f seconds\n", (portamento_target-portamento_pitch) / (portamento_increment * SampleRate() ) );
	if( portamento_increment != 0.0f || pitch_bend_increment != 0.0f )
	{
		OutputChange( SampleClock(), GetPlug(PLG_PITCH), ST_RUN );
	}
	else // pitch jumps immediately to note
	{
		OutputChange( SampleClock(), GetPlug(PLG_PITCH), ST_ONE_OFF );
	}

	ResetStaticOutput();
}

void ug_midi_to_cv::NoteOff( /*short chan,*/ short NoteNum, short NoteVel)
{
	//	_RPT2(_CRT_WARN, "V%d ug_midi_to_cv::NoteOff() clock=%d\n", pp_voice_num, SampleClock() );
	SET_CUR_FUNC( &ug_midi_to_cv::sub_process );
	//	_RPT0(_CRT_WARN, "active\n" );
	//	_RPT2(_CRT_WARN, "V%d ug_midi_to_cv gate=0 clock=%d\n", pp_voice_num, SampleClock() );
	RUN_AT( SampleClock() + 3, &ug_midi_to_cv::delay_gate_off );

	noteKeyDown_ = false;
}

// This func raises the gate signal
// something that can't be done imediatly on note-on
// as gate must be lowered b4 raised
void ug_midi_to_cv::delay_gate_on()
{
#ifdef DEBUG_NOTES
	_RPT2(_CRT_WARN, "V%d ug_midi_to_cv gate=1 clock=%d\n", pp_voice_num, SampleClock());
#endif
	// When resetting gate in response to voice-stealing (voice-active -> 0 ). May be running sub-process-sleep and need to restart sub-process.
	SET_CUR_FUNC( &ug_midi_to_cv::sub_process );
	gate_so.Set( SampleClock(), 1.0f );
	lastGateHi = SampleClock();
}

void ug_midi_to_cv::delay_gate_off()
{
	if( lastGateHi == SampleClock() ) // hack arround zero-length notes (can't set gate 0 on same clock as it was set hi (else voice stealing screws up).
	{
		RUN_AT( SampleClock() + 1, &ug_midi_to_cv::delay_gate_off ); // retry later
		return;
	}

#ifdef DEBUG_NOTES
	_RPT2(_CRT_WARN, "V%d ug_midi_to_cv delay_gate=0 clock=%d\n", pp_voice_num, SampleClock());
#endif
	// When resetting gate in response to voice-stealing (voice-active -> 0 ). May be running sub-process-sleep and need to restart sub-process.
	SET_CUR_FUNC( &ug_midi_to_cv::sub_process );
	gate_so.Set( SampleClock(), 0.0f );
}

void ug_midi_to_cv::Resume()
{
	ug_notesource::Resume();
	gate_so.Resume();
	velocity_so.Resume();
	aftertouch_so.Resume();
	voiceResetBetweenNotes = true;
/*
	// Update portamento to account for lost time.
	if( !sleeping ) // can't get sampleclock if asleep( not up to date )
	{
		timestamp_t timeAsleep = SampleClock() - m_suspend_time;
		_RPT1(_CRT_WARN, "time asleep %fs\n",timeAsleep / SampleRate() );

		float sf = (float)timeAsleep;

		// update master portamento if nesc
		if( master_portamento_increment != 0.0f )
		{
			// calc amount of port time remaining
			float remain = ( master_portamento_target - master_portamento_pitch ) / master_portamento_increment;

			if( remain > sf )
			{
				master_portamento_pitch += sf * master_portamento_increment;
			}
			else
			{
				master_portamento_pitch = master_portamento_target;
				master_portamento_increment = 0.0f;
			}
		}

		if( portamento_increment != 0.0f )
		{
			float remain = 0.5f + ( portamento_target - portamento_pitch ) / portamento_increment;

			if( remain < sf )
			{
				portamento_pitch += portamento_increment * sf;
			}
			else
			{
				portamento_pitch = portamento_target;
	//			portamento_increment = 0.0f;
			}
		}
	}
	*/
}

void ug_midi_to_cv::BuildHelperModule()
{
	CreateMidiRedirector(this);
}

void ug_midi_to_cv::onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state )
{
	// Has voice reset? (pin value don't seem to update. but can ignore it).
	if( p_to_plug == GetPlug( PLG_VOICE_ACTIVE ) )
	{
#ifdef DEBUG_NOTES
		_RPT1(_CRT_WARN, "ug_midi_to_cv Voice RESET (%f). vel will jump.\n", m_voice_active);
#endif
		voiceResetBetweenNotes = true;
	}

	if( p_to_plug == GetPlug(PLG_BEND_RANGE) )
	{
		PitchBend(m_bend_amt);
	}

	if( CloneOf() == this && p_to_plug == GetPlug( PN_MONO_MODE ) ) //&& mono_mode_old )
	{
		// to avoid stuck notes when switching to or from mono-mode, turn off all notes.
		// turn off all note except one with priority
		// crashes when latency compensation involved (Oversampling)... parent_container->NoteOff( SampleClock(), -1 ); // All notes Off
	}

	if (p_to_plug == GetPlug(PLG_VOICE_TRIGGER))
	{
#ifdef DEBUG_NOTES
		_RPT4(_CRT_WARN, "V%d ug_midi_to_cv::TRIGGER  clock=%d pitch %f vel %f\n", pp_voice_num, (int)SampleClock(), m_voice_pitch, m_voice_velocity);
#endif
		if (m_voice_gate > 0.0f)
		{
			NoteOn2(m_voice_id, m_voice_velocity * 0.1f, m_voice_pitch * 0.1f);
			ignoreNoteOnPitch_ = true;
		}
	}
	if (p_to_plug == GetPlug(PLG_VOICE_PITCH))
	{
#ifdef DEBUG_NOTES
		_RPT4(_CRT_WARN, "V%d ug_midi_to_cv::PITCH  clock=%d pitch %f vel %f\n", pp_voice_num, (int)SampleClock(), m_voice_pitch, m_voice_velocity);
#endif
		if( !ignoreNoteOnPitch_ )
		{
			Retune(m_voice_pitch * 0.1f);
		}
		ignoreNoteOnPitch_ = false;
	}

	if (p_to_plug == GetPlug(PLG_VOICE_GATE))
	{
		if (m_voice_gate > 0.0f)
		{
#ifdef DEBUG_NOTES
			_RPT4(_CRT_WARN, "V%d ug_midi_to_cv::GateOn()  clock=%d pitch %f vel %f\n", pp_voice_num, (int)SampleClock(), m_voice_pitch, m_voice_velocity);
#endif
		}
		else
		{
#ifdef DEBUG_NOTES
			_RPT2(_CRT_WARN, "V%d ug_midi_to_cv::GateOff() clock=%d \n", pp_voice_num, (int)SampleClock());
#endif
			if (!m_held)
			{
				NoteOff(m_voice_id);
			}
		}
	}

	if (p_to_plug == GetPlug(PLG_HOLD_PEDAL) || p_to_plug == GetPlug(PLG_VOICE_ACTIVE))
	{
		bool held = m_hold_pedal >= 5.f && m_voice_active == 1.0f;

		if (held != m_held)
		{
			m_held = held;
			if (!held && m_voice_gate == 0.0f && gate_so.output_level != 0.0f )
			{
				NoteOff(m_voice_id);
			}
		}
	}

	if (p_to_plug == GetPlug(PLG_VOICE_AFTERTOUCH))
	{
		Aftertouch(m_voice_aftertouch * 0.1f);
	}

	if (p_to_plug == GetPlug(PLG_BENDER))
	{
		PitchBend(m_bender);
	}
	
	if (CloneOf() == this && (p_to_plug == GetPlug(PLG_POLY_MODE) || p_to_plug == GetPlug(PN_MONO_MODE) || p_to_plug == GetPlug(PLG_NOTE_PRIORITY)))
	{
		updateVoiceAllocationMode();
	}

	ug_notesource::onSetPin(p_clock, p_to_plug, p_state);
}


float ug_midi_to_cv::bender_to_voltage(float p_bend_amt)
{
	return p_bend_amt * (float) BendRange / 120.0f; // 12 semitones * 10V
}

