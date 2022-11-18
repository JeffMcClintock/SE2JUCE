#include "pch.h"
#include <assert.h>
#include <algorithm>
#include "ug_envelope.h"
#include "math.h"

#include "sample.h"
#include <float.h>
#include "./modules/shared/xplatform.h"

using namespace std;
/*
A linear ENV to a expo VCA will give a 'unnatural' attack response.
This results in a sound with lack off 'punch' as the attack starts to slow
and ends to fast.
In general a log envelope works better with VCAs of both the expo
and linear type.
Odd as it may seem, it is quite common in analogue synths to use
log envelopes in combination with expo VCAs and VCFs.
*/

ug_envelope::ug_envelope()
{
	//	SET_CUR_FUNC( sub_process );
	current_segment_func = static_cast <process_func_ptr> ( &ug_envelope_base::sub_process7 );
	// this is derived from ug_control (for multistage env). so need to ignore program changes
	ignore_prog_change = true;
/*
	for (float v = 0.0f ; v < 12.0f; v += 1.0f)
	{
		float i = (float)TimecentToSecond(VoltageToTimecent(v - 2.0f));
		_RPTW2(_CRT_WARN, L"%f,%f\n", v,i);
	}
*/
}

void ug_envelope::ChooseSubProcess()
{
	//	DEBUG_TIMESTAMP;
	state_type new_output_state = ST_RUN;
	bool segment_end_level_run;// = level_plugs[cur _segment] && level_plugs[cur _segment]->getState() == ST_RUN;

	// special case, end segment clamped to zero
	if( cur_segment == end_segment )
	{
		fixed_segment_level = 0.f;
		segment_end_level_run = false;
	}
	else
	{
		if( level_plugs[cur_segment] != NULL )
		{
			fixed_segment_level = level_plugs[cur_segment]->getValue();
			segment_end_level_run = level_plugs[cur_segment]->getState() == ST_RUN;
		}
		else
		{
			fixed_segment_level = fixed_levels[cur_segment];
			segment_end_level_run = false;
		}
	}

	// what main process to use
	if( GetPlug(pn_gate)->getState() == ST_RUN )
	{
		SET_CUR_FUNC(&ug_envelope_base::process_with_gate);
	}
	else
	{
		SET_CUR_FUNC(&ug_envelope_base::sub_process);
	}

	if( fixed_increment == 0.f ) // sustain or end
	{
		if( segment_end_level_run )
		{
			assert(level_plugs[cur_segment] != 0 );
			current_segment_func = static_cast <process_func_ptr> (&ug_envelope::sub_process9);
			//			_RPT0(_CRT_WARN, "sub_process9\n" );
		}
		else
		{
			if( GetPlug(pn_level)->getState() == ST_RUN && cur_segment == sustain_segment)
			{
				current_segment_func = static_cast <process_func_ptr> (&ug_envelope_base::sub_process4); // level changing, but rate fixed, seg level fixed
				//				_RPT0(_CRT_WARN, "sub_process4\n" );
			}
			else // special case optimised.  Flat Sustain or end (no change in level)
			{
				/* already done up above
					if( level_plugs[cur_segment] != NULL )
					{
						fixed_segment_level = level_plugs[cur_segment]->getValue();
					}
					else
					{
						fixed_segment_level = fixed_levels[cur_segment];
					}
					*/
				/*
								// hack to avoid accessing plug during resume (fixed_level ain't relevant if segment level = 0)
								if( fixed_segment_level == 0.f )
								{
									fixed_level = 0.f;
								}
								else
								{*/
				fixed_level = GetPlug(pn_level)->getValue();
				//				}
				new_output_state = ST_STATIC; // NO!, need to handle one-off suring sustain ST_STOP;
				output_val = fixed_segment_level;
				current_segment_func = static_cast <process_func_ptr> (&ug_envelope_base::sub_process3); // level changing, but rate fixed, seg level fixed
				//				_RPT0(_CRT_WARN, "sub_process3 OPT\n" );
				ResetStaticOutput(); // need to reset nesc?
			}
		}
	}
	else // attack/decay
	{
		if( segment_end_level_run )
		{
			current_segment_func = static_cast <process_func_ptr> (&ug_envelope::sub_process8); // level changing, but rate fixed, seg level fixed
			//			_RPT0(_CRT_WARN, "sub_process8\n" );
		}
		else
		{
			bool segment_time_fixed = true; // we can pre-calc the segment length

			/*
			current_segment_func = static_cast <process_func_ptr> (sub_process5); // seg level fixed. rate, master_level RUN
			segment_time_fixed = false;
			_RPT0(_CRT_WARN, "sub_process5\n" );
			*/
			if( GetPlug(pn_level)->getState() == ST_RUN || rate_plugs[cur_segment]->getState() == ST_RUN)
			{
				if( rate_plugs[cur_segment]->getState() == ST_RUN )
				{
					current_segment_func = static_cast <process_func_ptr> (&ug_envelope::sub_process5); // seg level fixed. rate, master_level RUN
					segment_time_fixed = false;
					//					_RPT0(_CRT_WARN, "sub_process5\n" );
				}
				else // special case optimised. fixed rate to fixed end level, master level changing
				{
					current_segment_func = static_cast <process_func_ptr> (&ug_envelope_base::sub_process5b); // seg level fixed. rate, master_level RUN
					//					_RPT0(_CRT_WARN, "sub_process5b OPT\n" );
				}
			}
			else // special case optimised. fixed rate to fixed end level
			{
				current_segment_func = static_cast <process_func_ptr> (&ug_envelope_base::sub_process6); // level changing, but rate fixed, seg level fixed
				//				_RPT0(_CRT_WARN, "sub_process6 OPT\n" );
			}

			// pre-calculate some constants to accelerate main loops
			if(segment_time_fixed)
			{
				fixed_increment = CalcIncrement( rate_plugs[cur_segment]->getValue() );
			}

			/*
						if( m_fast_decay_mode ) // note mute? (very fast note off)
						{
			//				_RPT1(_CRT_WARN, "V%d ug_envelope FAST NOTE OFF sub_process6\n", pp_voice_num );
							segment_time_fixed = true;
							current_segment_func = static_cast <process_func_ptr> (&ug_envelope::sub_process5); // level changing, but rate fixed, seg level fixed
							// aim for all envelopes to end in x samples at same time
							fixed_increment = output_val / FAST_DECAY_SAMPLES; // very fast decay
							//assert( fixed_segment_level == 0.f);
							// reset end level to overide any change above
							fixed_segment_level = 0.f;
						}
			*/
			/// THIS DON"T WORK, inc re-calculated in next code segment !!!!!!!!!!!!
			//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			// pre-calculate some constants to accelerate main loops
			if(segment_time_fixed)
			{
				//fixed_increment = CalcIncrement( rate_plugs[cur_segment]->getValue() );
				float delta_level = fixed_segment_level - output_val;
				fixed_increment = (float) copysign(fixed_increment, delta_level);
				float remaining = ( fixed_segment_level - output_val ) / fixed_increment;

				//				if( fabs(fixed_increment) < INSIGNIFIGANT_SAMPLE ) // fast note off can cause increment to be zero. fix divide by zero probs
				//				{
				//					remaining = 0;
				//				}
				if( ! isfinite( remaining ) ) // divide by zero?
				{
					remaining = 0;
				}

				fixed_segment_time_remain = (int) remaining; // implied floor() important
				assert( remaining >= 0.f);
				fixed_segment_time_remain = max(fixed_segment_time_remain,1); // minimum segment length 1 sample (else overshoot fix don't kick in)
				// store pre-calculated values (sub_process6 only)
				fixed_level = GetPlug(pn_level)->getValue();
				m_scaled_increment = fixed_increment * fixed_level;
				m_scaled_output_val = output_val * fixed_level;
			}
		}
	}

	OutputChange( SampleClock(), GetPlug(pn_output), new_output_state );
	//	_RPT1(_CRT_WARN, "fixed_increment %f\n", fixed_increment );
	//	_RPT1(_CRT_WARN, "fixed_segment_level %f\n", fixed_segment_level );
	//	float debug_gate = GetPlug(pn_gate)->getValue();
	//	assert( fixed_segment_level > 0.f || debug_gate == 0.f );
#ifdef _DEBUG
	// sub_process9 needs a level plug.
	ug_envelope* e = dynamic_cast<ug_envelope*>( this );
	assert( e->level_plugs[cur_segment] != 0 || current_segment_func != static_cast < process_func_ptr> (&ug_envelope::sub_process9));
#endif
}

void ug_envelope::onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state)
{
	assert( p_clock == SampleClock() );
	UPlug* to = p_to_plug;

	if( to == GetPlug(pn_gate) )
	{
		//		_RPT2(_CRT_WARN, "%d ug_envelope::PlugStateChange. %.3f\n",SampleClock(), GetPlug(pn_gate)->getValue() );
		if(	p_state == ST_RUN )
		{
			SET_CUR_FUNC( &ug_envelope_base::process_with_gate );
		}
		else
		{
			OnGateChange( GetPlug(pn_gate)->getValue() );

			// in some cases need to re-select subprocess
			if( process_function == static_cast<process_func_ptr>(&ug_envelope_base::process_with_gate) )
			{
				ChooseSubProcess();
			}
		}
	}
	else
	{
		if( fixed_increment != 0.f || cur_segment < end_segment ) // env not ended?
		{
			if( to == GetPlug(pn_level) || to == rate_plugs[cur_segment] || to == level_plugs[cur_segment])
			{
				ChooseSubProcess();
			}
		}
	}
}

float ug_envelope::CalcIncrement(float p_rate)
{
	//	float test = VoltageToTimecent(p_rate * 10.f);
	//	test = TimecentToSecond(test);
	float i = (float) TimecentToSecond(VoltageToTimecent(p_rate * 10.f));
	return 1.f / ( i * getSampleRate() );
}

// slope, run/fixed, level RUN, SL stop
void ug_envelope::sub_process5(int start_pos, int sampleframes)
{
	int count = sampleframes;
	float* out	= start_pos + output_ptr;
	float* level	= start_pos + level_ptr;
	float* rate	= start_pos + rate_plugs[cur_segment]->GetSamplePtr();
	float delta_level = fixed_segment_level - output_val;

	if( delta_level > 0.0 )
	{
		while( count-- > 0 )
		{
			output_val += CalcIncrement( *rate++ );

			if( output_val >= fixed_segment_level )
			{
				break;
			}

			*out++ = output_val * *level++; // mayby output_val should be moved to temp var for speed?
		}
	}
	else
	{
		while( count-- > 0 )
		{
			output_val -= CalcIncrement( *rate++ );

			if( output_val <= fixed_segment_level )
			{
				break;
			}

			*out++ = output_val * *level++;
		}
	}

	if( sampleframes > 0 )
	{
		SetSampleClock( SampleClock() + sampleframes - count - 1);

		if( count >= 0 ) // implies seg just ended
		{
			output_val = fixed_segment_level;
			*out = fixed_segment_level * *level;
			next_segment();
		}
	}
}
// DO ALL. rate RUN, sustain RUN, level RUN, seg L run
void ug_envelope::sub_process8(int start_pos, int sampleframes)
{
	int count = sampleframes;
	float* out	= start_pos + output_ptr;
	float* level	= start_pos + level_ptr;
	float* rate	= start_pos + rate_plugs[cur_segment]->GetSamplePtr();
	float* seg_level = start_pos + level_plugs[cur_segment]->GetSamplePtr();
	float delta_level = *seg_level - output_val;

	if( delta_level > 0.0 )
	{
		while( count-- > 0 )
		{
			output_val += CalcIncrement( *rate++ ); // mayby output_val should be moved to temp var for speed?

			if( output_val >= *seg_level )
			{
				break;
			}

			*out++ = output_val * *level++;
			seg_level++;
		}
	}
	else
	{
		while( count-- > 0 )
		{
			output_val -= CalcIncrement( *rate++ );

			if( output_val <= *seg_level )
			{
				break;
			}

			*out++ = output_val * *level++;
			seg_level++;
		}
	}

	if( sampleframes > 0 )
	{
		SetSampleClock( SampleClock() + sampleframes - count - 1);

		if( count >= 0 )
		{
			output_val = *seg_level;
			*out = *seg_level * *level;
			next_segment();
		}
	}
}

// rate 0, sustain RUN, level RUN, seg L run
void ug_envelope::sub_process9(int start_pos, int sampleframes)
{
	if( sampleframes > 0 )
	{
		float* out	= start_pos + output_ptr;
		float* level	= start_pos + level_ptr;
		float* seg_level	= start_pos + level_plugs[cur_segment]->GetSamplePtr();
		int count = sampleframes;

		while( count-- > 0 )
		{
			*out++ = *seg_level++ * *level++;
		}

		output_val = *(seg_level-1); // hack so next segment starts right
		SetSampleClock( SampleClock() + sampleframes );
	}
}
/*
void ug_envelope::HandleEvent(SynthEditEvent *e)
{
	switch( e->eventType )
	{
	case UET _NOTE_MUTE: // rapid release of note, because other voice want's to play same note
		{
			NoteMute();
		}
		break;
	default:
		ug_base::HandleEvent( e );
	};
}
*/
/*
void ug_envelope::On GateChange(float p_gate)
{
	gate _state = ( p_gate > 0.f );

//	DEBUG_TIMESTAMP;
//	_RPT2(_CRT_WARN, "%d ug_envelope::On GateChange. %.3f\n",SampleClock(), p_gate );

	if( gate _state )
	{
		if( decay_mode )
		{
			m_fast_decay_mode = false;
			decay_mode = false;
			cur _segment = -1;
			next_segment();
		}
	}
	else
	{
		if( !decay_mode )
		{
			decay_mode = true;
			//	_RPT1(_CRT_WARN, "Uenvelope::Release (%d)\n",(int) SampleClock() );
			//	_RPT0(_CRT_WARN, "Release\n" );
			// if env is at sustain, move to next segment
			// else move to end segment
			if( cur _segment != sustain_segment || sustain_segment >= end_ segment )
			{
				cur _segment = end_segment - 1;
			}
			next_segment();
		}
	}
}

void ug_envelope::NoteMute()
{
	decay_mode = true;
	if( cur _segment != end_ segment || output_val != 0.f )
	{
		cur _segment = end_segment - 1;
		m_fast _decay_mode = true;
		next_segment();
	}
	else
	{
		_RPT1(_CRT_WARN, "V%d ug_envelope FAST NOTE OFF - ignored\n", pp_voice_num );
	}
}

int ug_envelope::Open()
{
	ug_base::Open();

//	DEBUG_TIMESTAMP;
//	_RPT0(_CRT_WARN, "ug_envelope::Open()\n" );

	ug_notesource *ns = getVoice()->NoteSource;

	if( ns )
	{
		ns->RegisterEnvelopeGenerator(this);
		_RPT1(_CRT_WARN, "V%d ug_envelope registers\n", pp_voice_num );
	}
	else
	{
		_RPT1(_CRT_WARN, "V%d ug_envelope no notesource\n", pp_voice_num );
	}
	fixed_increment = 0.f;
	cur _segment	= end_segment;

	// store plug's datablock pointer for faster access
	pn_output	= GetPlug(L"Signal Out")->getPlugIndex();
	pn_gate	= GetPlug(L"Gate")->getPlugIndex();
	pn_level	= GetPlug(L"Overall Level")->getPlugIndex();

	output_ptr	= GetPlug(pn_output)->Get SamplePtr();
	gate_ptr	= GetPlug(pn_gate)->Get SamplePtr();
	level_ptr	= GetPlug(pn_level)->Get SamplePtr();

	ResetStaticOutput(); // didn't help orion bug

	ChooseSubProcess();
	return 0;
}
void ug_envelope::Resume()
{
//	_RPT0(_CRT_WARN, "ug_envelope::Resume()\n" );
	ug_base::Resume();

	// voice resume after indefinte period
	// must reset envelope level to zero (filter env may have slower release for example)
	output_val = 0.f;
	fixed_increment = 0.f;
	cur _segment	= end_segment;
}
*/
/*
void ug_envelope::process_with_gate(int start_pos, int sampleframes)
{
	float *gate	= gate_ptr + start_pos;

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

		On GateChange(*gate);
	}
}

// master function, calls helpers for each segment
void ug_envelope::sub_process(int start_pos, int sampleframes)
{
//	assert( SampleClock() != 122850 );

	timestamp_t target_sample_clock = SampleClock() + sampleframes;

	do
	{
		(this->*(current_segment_func))( start_pos, sampleframes );
		int samples_remain = target_sample_clock - SampleClock();
		start_pos += sampleframes - samples_remain;
		sampleframes = samples_remain;
	}
	while( sampleframes > 0 );

	assert( sampleframes == 0 );
}

// rate zero, level STOP
void ug_envelope::sub_process3(int start_pos, int sampleframes)
{
	int count = sampleframes;
	float *out = start_pos + output_ptr;
	float level = output_val * fixed_level;

	while( count-- > 0 )
	{
		*out++ = level;
	}

	SetSampleClock( SampleClock() + sampleframes );

	// SleepIf OutputStatic(sampleframes);
	static_output_count -= sampleframes;
	if( static_output_count <= 0 )
	{
		// if gate stopped, can sleep , else must keep watching it
		if( GetPlug(pn_gate)->getState() != ST_RUN )
		{
			SET_CUR_FUNC( &ug_base::process_sleep );
		}

		current_segment_func = static_cast <process_func_ptr> ( sub_process7 ); // do nothing sub-process
	}
}

// rate zero (sustain), level RUN, sl fixed
void ug_envelope::sub_process4(int start_pos, int sampleframes)
{
	int count = sampleframes;
	timestamp_t start_clock = SampleClock();

	float *out	= start_pos + output_ptr;
	float *level	= start_pos + level_ptr;

	while( count-- > 0 )
	{
		*out++ = output_val * *level++;
	}

	if( sampleframes > 0 )
	{
		SetSampleClock( SampleClock() + sampleframes - count - 1);
	}
}
*/

/*
// slope fixed, level RUN, SL stop
void ug_envelope::sub_process5b(int start_pos, int sampleframes)
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

	float *out	= start_pos + output_ptr;
	float *level	= start_pos + level_ptr;

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
*/

/*
// rate non-zero, STOP. level STOP , seg lev stop
void ug_envelope::sub_process6(int start_pos, int sampleframes)
{
	int count = min( sampleframes, fixed_segment_time_remain );
	fixed_segment_time_remain -= count;

	SetSampleClock( SampleClock() + count );

	__as m
	{
	mov	eax,dword ptr [this] // eax holds 'this' pointer

	//      float *out_ptr = start_pos + output_ptr;
	mov     ecx,dword ptr [eax].output_ptr
	mov     edx,dword ptr start_pos
	lea     esi,[ecx+edx*4]			// edx holds output_ptr

	add		edx, count
	lea     edx,[ecx+edx*4]			// calc final output ptr

	//	while( count-- > 0 )
	cmp		edx, esi
	je	int skip_loop // avoid loop if count is zero

	//	output_val += fixed_increment * (float) count;
	fld		dword ptr [eax].fixed_increment
	fimul	dword ptr count
	fadd	dword ptr [eax].output_val
	fstp	dword ptr [eax].output_val

	// load scaled inc and out_val
	fld		dword ptr [eax].m_scaled_increment
	fld		dword ptr [eax].m_scaled_output_val

loop_top:
	fadd	st(0), st(1)	// scaled_output_val += scaled_increment;

	fst		dword ptr [esi]	// *out++ = scaled_output_val;
	add		esi, 4

	cmp		edx, esi
	jne		int loop_top

	// cleanup
	fstp	dword ptr [eax].m_scaled_output_val
	fstp    st(0)		// pop 'scaled_increment'
skip_loop:
	}

// done in asm	output_val += count * fixed_increment; // so next routine knows where to start

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
			float *out_ptr = output_ptr + start_pos + count;
			*out_ptr = output_val * fixed_level;
		}
		else
		{
			next_segment();
		}
	}
}

// Sus/end. level STOP, output sta-tic
void ug_envelope::sub_process7(int start_pos, int sampleframes)
{
	SetSampleClock( SampleClock() + sampleframes);
}

/ * ASM handy?
		__as m // !!doesn't seem much faster
		{
			mov     eax, dword ptr remain
			fld	dword ptr scaled_inc  // push to st(1) _scaled_inc
			fld	dword ptr scaled_cnt  // push to st(0) _scaled_cnt
			mov     edi, dword ptr out_ptr   //edi is pointer index to dest buf
			// while remain-- > 0
			test	eax, eax
			jle	end_loop1

loop1:
			// scaled_cnt += scaled_inc
			fadd    st(0), st(1)
			sub	eax, 1
			fst		dword ptr [edi]		// save to dest buffer

			add     edi,4               //increment dest buffer
			test	eax, eax
			jg	loop1

end_loop1:
			fstp	dword ptr scaled_cnt  // push to st(0) _scaled_cnt
			fstp    st(0)               //Pop 2nd pushed FP value
			mov		dword ptr out_ptr, edi
		}
*/


/*
// switch to next segment, perhaps prematurely
void ug_envelope::next_segment() // Called when envelope section ends
{
	//	DEBUG_TIMESTAMP;
	//	_RPT1(_CRT_WARN, "next_segment() cur_segment = %d\n", cur_segment );
	// reached sustain or end?
	if(  cur_segment == end_ segment  )
	{
		m_fast_decay_mode = false; // done
		fixed_increment = 0.f; // indicated sustain, or end
		// TRACE0("hold_level = true;\n");
	}
	else
	{
		if(  cur_segment == sustain_segment && GetPlug(pn_gate)->getValue() > 0.f && ! m_fast_decay_mode )
		{
			fixed_increment = 0.f; // indicated sustain, or end
		}
		else
		{
			fixed_increment = 1.f; // just to flag non-hold level, will be calculated properly if needed in ChooseSubProcess()
			cur _segment++;
			// _RPT1(_CRT_WARN, "segment %d\n",cur _segment  );
		}
	}

	// ug may be asleep (if gate was not running)
	if( GetPlug(pn_gate)->getState() != ST_RUN )
	{
		SET_CUR_FUNC( sub_process );
	}

	ChooseSubProcess();
}
*/
