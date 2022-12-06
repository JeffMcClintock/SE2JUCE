#include "pch.h"
#include <assert.h>
#include <algorithm>
#include "ug_envelope_base.h"
#include "math.h"

#include "CVoiceList.h"
#include <float.h>
#include "ug_event.h"
#include "ug_envelope.h"
#include "SeAudioMaster.h"
#include "./modules/shared/xplatform.h"

using namespace std;

// repeated in other env
//#define FAST_DECAY _SAMPLES 15
/*
A linear ENV to a expo VCA will give a 'unnatural' attack response.
This results in a sound with lack off 'punch' as the attack starts to slow
and ends to fast.
In general a log envelope works better with VCAs of both the expo
and linear type.
Odd as it may seem, it is quite common in analogue synths to use
log envelopes in combination with expo VCAs and VCFs.
*/

ug_envelope_base::ug_envelope_base() :
	num_segments(8)
	,cur_segment(7)
	,decay_mode(true)
	,output_val(0.f)
	,fixed_level(0.f)
	,fixed_segment_level(0.f)
	,output_ptr(NULL)
	,gate_state(false)
{
	SET_CUR_FUNC( &ug_envelope_base::sub_process );
	current_segment_func = static_cast <process_func_ptr> ( &ug_envelope_base::sub_process7 );
}

int ug_envelope_base::Open()
{
	ug_control::Open();

	fixed_increment = 0.f;
	cur_segment	= end_segment;
	assert( cur_segment < num_segments );
	// store plug's datablock pointer for faster access
	pn_output	= GetPlug(L"Signal Out")->getPlugIndex();
	pn_gate		= GetPlug(L"Gate")->getPlugIndex();
	pn_level	= GetPlug(L"Overall Level")->getPlugIndex();
	output_ptr	= GetPlug(pn_output)->GetSamplePtr();
	ResetStaticOutput(); // didn't help orion bug
	ChooseSubProcess();
	return 0;
}

void ug_envelope_base::Resume()
{
	assert(gate_ptr == GetPlug(pn_gate)->GetSamplePtr()); // never cache buffer ptr.
	assert(level_ptr == GetPlug(pn_level)->GetSamplePtr());

	//	_RPT0(_CRT_WARN, "ug_envelope_base::Resume()\n" );
	ug_control::Resume();
	// voice resume after indefinte period
	// must reset envelope level to zero because if voice was stolen, will be in mid-air.
	output_val = 0.f;
	fixed_increment = 0.f;
	cur_segment	= end_segment;
	gate_state = false;
	/* incorrect??
		// resume is called before module awake, and before sampleclock valid (it's in 'next' block),
		// so need to use audiomaster's sampleclock.
		RUN_AT( AudioMaster()->SampleClock(), &ug_envelope_base::ChooseSubProcess );
	*/
	Wake(); // base class won't wake sleeping module on resume unless some events are pending.
	// input pins not valid yet. schedule proper restart.
	//	RUN_AT( SampleClock(), &ug_envelope_base::ChooseSubProcess );
	RUN_AT( SampleClock(), &ug_envelope_base::VoiceReset );
}

void ug_envelope_base::VoiceReset()
{
	OnGateChange( GetPlug(pn_gate)->getValue() );
}

void ug_envelope_base::process_with_gate(int start_pos, int sampleframes)
{
	float* gate	= gate_ptr + start_pos;
	int last = start_pos + sampleframes;
	int to_pos = start_pos;
	int cur_pos = start_pos;

	while( cur_pos < last )
	{
		// duration till next gate change?
		while( (*gate > 0.f ) == gate_state && to_pos < last )
		{
			to_pos++;
			gate++;
		}

		//		if( to_pos > cur_pos ) // no harm (sub_process seems to handle sampleframes = 0 ok)
		{
			sub_process( cur_pos, to_pos - cur_pos );
		}

		if( to_pos == last )
		{
			return;
		}

		cur_pos = to_pos;
		OnGateChange(*gate);
	}
}

// master function, calls helpers for each segment
void ug_envelope_base::sub_process(int start_pos, int sampleframes)
{
	//	assert( SampleClock() != 122850 );
	timestamp_t target_sample_clock = SampleClock() + sampleframes;
#ifdef _DEBUG
	// sub_process9 needs a level plug.
	ug_envelope* e = dynamic_cast<ug_envelope*>( this );
	assert( e->level_plugs[cur_segment] != 0 || current_segment_func != static_cast < process_func_ptr> (&ug_envelope::sub_process9));
#endif

	do
	{
		(this->*(current_segment_func))( start_pos, sampleframes );
		int samples_remain = (int) (target_sample_clock - SampleClock());
		start_pos += sampleframes - samples_remain;
		sampleframes = samples_remain;
	}
	while( sampleframes > 0 );

	assert( sampleframes == 0 );
}

// rate zero, level STOP
void ug_envelope_base::sub_process3(int start_pos, int sampleframes)
{
	int count = sampleframes;
	float* out = start_pos + output_ptr;
	float level = output_val * fixed_level;

	while( count-- > 0 )
	{
		*out++ = level;
	}

	SetSampleClock( SampleClock() + sampleframes );
	// Sleep IfOutputStatic(sampleframes);
	static_output_count -= sampleframes;

	if( static_output_count <= 0 )
	{
		// if gate stopped, can sleep , else must keep watching it
		if( GetPlug(pn_gate)->getState() != ST_RUN )
		{
			SET_CUR_FUNC( &ug_base::process_sleep );
		}

		current_segment_func = static_cast <process_func_ptr> ( &ug_envelope_base::sub_process7 ); // do nothing sub-process
	}
}

// rate zero (sustain), level RUN, sl fixed
void ug_envelope_base::sub_process4(int start_pos, int sampleframes)
{
	int count = sampleframes;
	//	timestamp_t start_clock = SampleClock();
	float* out	= start_pos + output_ptr;
	float* level	= start_pos + level_ptr;

	while( count-- > 0 )
	{
		*out++ = output_val * *level++;
	}

	if( sampleframes > 0 )
	{
		SetSampleClock( SampleClock() + sampleframes ); //- count - 1);
	}
}

// slope fixed, level RUN, SL stop
void ug_envelope_base::sub_process5b(int start_pos, int sampleframes)
{
	int count = min( sampleframes, fixed_segment_time_remain );

	if( count == 0 ) // segment ended on last call
	{
		if( sampleframes != 0 ) // can happen, must do nothing if zero.
		{
			next_segment();
		}

		return;
	}

	fixed_segment_time_remain -= count;
	SetSampleClock( SampleClock() + count );
	float* out	= start_pos + output_ptr;
	float* level	= start_pos + level_ptr;
	float increment = fixed_increment;

	while( count-- > 0 )
	{
		output_val += increment; // mayby output_val should be moved to temp var for speed?
		*out++ = output_val * *level++;
	}

	// when incmenting output val has resulted in end of segment
	// set output value to exact end level (undoes any overshoot due to quantisation of segment length)
	// NEXT time this routine is called, it triggers next_segment().
	// because at the end of a segment, the sampleclock could 'point' to
	// start of next block, so we delay calling next_segment untill then
	// calc last sample written and set to exact end value
	assert( fixed_segment_time_remain >= 0 );

	if( fixed_segment_time_remain == 0 )
	{
		assert( sampleframes != 0 );
		output_val = fixed_segment_level;
		out--;
		level--;
		*out = output_val * *level;
	}
}

// rate non-zero, STOP. level STOP , seg lev stop
void ug_envelope_base::sub_process6(int start_pos, int sampleframes)
{
	int count = min( sampleframes, fixed_segment_time_remain );
	fixed_segment_time_remain -= count;
	SetSampleClock( SampleClock() + count );

	float* out = start_pos + output_ptr;

	for( int i = count ; i > 0 ; --i )
	{
		m_scaled_output_val += m_scaled_increment;
		*out++ = m_scaled_output_val;
	}

	output_val += fixed_increment * (float) count; // so next routine knows where to start

	assert( fixed_segment_time_remain >= 0 );

	if( fixed_segment_time_remain == 0 )
	{
		if( sampleframes == 0 ) // can happen, must do nothing if zero.
		{
			return;
		}

		// when incmenting output val has resulted in end of segment
		// set output value to exact end level (undoes any overshoot due to quantisation of segment length)
		// NEXT time this routine is called, it triggers next_segment().
		// because at the end of a segment, the sampleclock could 'point' to
		// start of next block, so we delay calling next_segment untill then
		if( count != 0 )
		{
			// calc last sample written and set to exact end value
			output_val = fixed_segment_level;
			count--;
			float* out_ptr = output_ptr + start_pos + count;
			*out_ptr = output_val * fixed_level;
		}
		else
		{
			next_segment();
		}
	}
}

// Sus/end. level STOP, output sta-tic
void ug_envelope_base::sub_process7(int /*start_pos*/, int sampleframes)
{
	SetSampleClock( SampleClock() + sampleframes);
}

// switch to next segment, perhaps prematurely
void ug_envelope_base::next_segment() // Called when envelope section ends
{
	//	DEBUG_TIMESTAMP;
	//	_RPT1(_CRT_WARN, "next_segment() cur_segment = %d\n", cur_segment );
	// reached sustain or end?
	fixed_increment = 0.f; // indicated sustain, or end

	// as user can switch end segment, need >= test, not ==
	if( cur_segment < end_segment  )
	{
		if( cur_segment != sustain_segment || GetPlug(pn_gate)->getValue() <= 0.f )
		{
			//			 _RPT1(_CRT_WARN, "segment %d\n",cur_segment  );
			fixed_increment = 1.f; // just to flag non-hold level, will be calculated properly if needed in ChooseSubProcess()
			cur_segment++;
			assert( cur_segment < num_segments );
		}
	}

	// ug may be asleep (if gate was not running)
	if( GetPlug(pn_gate)->getState() != ST_RUN )
	{
		SET_CUR_FUNC( &ug_envelope_base::sub_process );
	}

	//	float debug_gate = GetPlug(pn_gate)->getValue();
	//	assert( cur_segment != end_segment || debug_gate == 0.f );
	ChooseSubProcess();
}

void ug_envelope_base::OnGateChange(float p_gate)
{
	bool new_gate_state = ( p_gate > 0.f );

	if( new_gate_state == gate_state ) // prevent spurious gate changes from onplugstatechange()
		return;

	gate_state = new_gate_state;

	//	DEBUG_TIMESTAMP;
	//	_RPT2(_CRT_WARN, "%d ug_envelope_base::OnGateChange. %.3f\n",SampleClock(), p_gate );
	if( gate_state )
	{
		if( decay_mode ) // trigger new attack
		{
			decay_mode = false;
			cur_segment = -1;
			next_segment();
		}
	}
	else
	{
		if( !decay_mode )
		{
			decay_mode = true;

			//			_RPT1(_CRT_WARN, "Uenvelope::Release (%d)\n",(int) SampleClock() );
			//				_RPT0(_CRT_WARN, "Release\n" );
			// if env is at sustain, move to next segment
			// else move to end segment
			if( cur_segment != sustain_segment || sustain_segment >= end_segment )
			{
				cur_segment = end_segment - 1;
				assert( cur_segment < num_segments );
			}

			next_segment();
		}
	}
}

