#include "pch.h"
#define _USE_MATH_DEFINES
#include <cmath>
#include <math.h>
#include "modules/shared/xp_simd.h"
#include <algorithm>
#include "ug_vca.h"

#include "ULookup.h"
#include "module_register.h"
#include "./modules/shared/xplatform.h"

SE_DECLARE_INIT_STATIC_FILE(ug_vca)

using namespace std;



/*
	// Scale dB to Volts fairly precisely.
	if( parameterValue > -35.0f )
	{
		parameterValue = 10.0f + parameterValue * 9.0f / 35.0f;
	}
	else
	{
		// Slope steepens below -35dB.
		parameterValue = 49.393f * expf( parameterValue * 0.114f );
	}
*/


// !!!known problem. Most modes treat negative voltages as silence, except linear which inverts the audio. Would be better if
// liniear mode was consistant (only positive gain values accepted as valid).
// fix when ported/updated to new version.

// 2. CPU use is huge when ouside range of lookup table, perhaps a aproximated spline curve or similar would work better.

namespace
{
REGISTER_MODULE_1(L"VCA", IDS_MN_VCA,IDS_MG_MODIFIERS,ug_vca ,CF_STRUCTURE_VIEW,L"Controls the volume of a signal. 10V is full volume.  Connect an ADSR to apply a volume envelope to a sound. Has a choice of 3 response curves. See <a href=signals.htm>Signal Levels and Conversions</a> for more");
}


#define PN_SIGNAL	0
#define PN_GAIN		1
#define PN_OUT		2
#define PN_RESPONSE 3

// Fill an array of InterfaceObjects with plugs and parameters
//void ug_vca::ListInterface(InterfaceObjectArray &PList)
IMPLEMENT_UG_INFO_FUNC2(ug_vca)
{
	// IO Var, Direction, Datatype, Name, Default, defid (index into unit_gen::PlugFormats)
	LIST_PIN2( L"Signal", in1_ptr, DR_IN, L"0", L"", IO_LINEAR_INPUT, L"Audio Input Signal");
	LIST_PIN2( L"Volume", in2_ptr, DR_IN, L"8", L"", IO_POLYPHONIC_ACTIVE, L"");
	LIST_PIN2( L"Output", out_ptr, DR_OUT, L"0", L"", 0, L"");
	LIST_VAR3( L"Response Curve",scale_type,DR_IN,DT_ENUM, L"1", L"Exponential=1,Linear,Decibel,Decibel (old)=0",0, L"In db mode: 10V to 2V gives 0 to -40db, 2V to 0v gives -40db to -70db (silence)" );
}

float calcDecibelGainCurve( float volume )
{
	// db curve, corrected [same code also used in ug_pan]
	// 10 -> 1V maps 0db -> -35db
	constexpr float slope = 35.f / 9.f; // drops 35 db over 9 volts
	float db = slope * ( volume - 1.f ) ;
	/* creates a 'step'
		if( db < -3.5f ) // last 1 volt maps -35db to -70db (inaudible)
		{
	//				db = (db + 3.5f) * 2.f - 3.5f;
			db = (db + 3.5f) * 11.57f - 3.5f;
		}
	*/
	float res = powf( 10.f, db * 0.5f );
	constexpr float tailOffThreashold = 0.1f; // 1 Volts
	constexpr float inversetailOffThreashold = 1.0f / tailOffThreashold;

	// Smooth tail-off to zero.
	if( volume < tailOffThreashold )
	{
		res *= sinf( (float) M_PI_2 * volume * inversetailOffThreashold );
	}

	return res;
}

ug_vca::ug_vca() :
	fixed_gain(0.f)
	, staticLevel_(99999.f)
{
	SetFlag(UGF_SSE_OVERWRITES_BUFFER_END);
	return;
}

int ug_vca::Open()
{
#if defined( _DEBUG )
	// check for bug where CalcGain(0) returning very small value which held voice open (should be zero)
	short temp = scale_type;
	float res;
	scale_type = 0;
	res = CalcGain(0.f);
	assert( res == 0.f );
	scale_type = 1;
	res = CalcGain(0.f);
	assert( res == 0.f );
	scale_type = 2;
	res = CalcGain(0.f);
	assert( res == 0.f );
	scale_type = 3;
	res = CalcGain(0.f);
	assert( res == 0.f );
	scale_type = temp;
#endif
	// Create volume curve lookup tables
	CreateSharedLookup2( L"UVCA Trunc Exp Curve", shared_lookup_table, -1, VCA_TABLE_SIZE_30V+2, true, SLS_ALL_MODULES );

	// Fill lookup table if not done already
	//const double c1  = 1 / ( 1 - EXP( -3 ));      //  scale factor truncated curve
	//y = 1 - c1 * (1 - EXP( -3 * ( 1 - x )))
	if( !shared_lookup_table->GetInitialised() )
	{
		const float c1  = 1.f / ( 1.f - expf( -3.f ));      //  scale factor truncated curve

		for( int j = 1 ; j < VCA_TABLE_SIZE_30V + 2 ; ++j )
		{
			float temp_float = 1.f - c1 * (1.f - expf( 3.f * ( j/(float)VCA_TABLE_SCALE_30V - 1.0f )));
			shared_lookup_table->SetValue( j, temp_float );
			//			_RPT2(_CRT_WARN, "%d  %f\n", j, temp_float );
		}

		// Fix small rounding error on 'zero' value.
		shared_lookup_table->SetValue( 0, 0.0f );
		shared_lookup_table->SetInitialised();
	}

	InitVolTable( this, shared_lookup_table_db2 );
	InitVolTable_old( this, shared_lookup_table_db );

	return ug_base::Open();
}

void ug_vca::process_both_run(int start_pos, int sampleframes)
{
	float* in1 = in1_ptr + start_pos;
	float* in2 = in2_ptr + start_pos;
	float* out1 = out_ptr + start_pos;

	for( int s = sampleframes; s > 0 ; s-- )
	{
/*
		float idx = *in2++;

		if( idx > 1.0f ) // BAD! Better to increase table size to something reasonable and ignore anything over 20V.
		{
			*out1++ = *in1++ * CalcGain(idx);
		}
		else
		{
			*out1++ = *in1++ * fastInterpolation( idx, TABLE_SIZE, m_lookup_table );
		}
*/
		// New. Table extends to 30V, anything over ignored/clipped.
		*out1++ = *in1++ * fastInterpolationClipped2( *in2++ * (float) VCA_TABLE_SCALE_30V, VCA_TABLE_SIZE_30V, m_lookup_table );
	}

/* slower (needs control word set.)
			// REFERENCE CODE, TABLE LOOKUP (LINEAR INTERPOLATION)
			int int_idx;
			__as m
			{
				// multiply index by table size
				fld		dword ptr [idx]
				fmul	dword ptr l_table_size
				mov   ecx, dword ptr [this]

				// !comparing float idx to integer zero to skip negative indexes.
				// relys on sign-bit of float same position as int.
				cmp		dword ptr [idx],0
				jge		in_range
				fstp	st(0)
				fldz
				in_range:
				// split index into integer and fractional part
				fist   dword ptr [int_idx]	// int_idx = (int) idx
				fisub   dword ptr [int_idx]	// frac = idx - int_idx
				mov	  ecx, dword ptr[ecx]this.m_lookup_table
				mov	  edx, dword ptr [int_idx]
#ifdef _DEBUG
				fst   dword ptr [idx_frac]
#endif
				// perform table lookup, interpolate
				lea		esi,[ecx+edx*4]		// esi = & (table[int_idx] )
				fld		dword ptr [esi]     // table[0]
				fld		dword ptr [esi+4]   // table[1]
				fsub	st(0),st(1)			// table[1] - table[0]
				mov         eax,dword ptr [in1]
				mov         ecx,dword ptr [out1]
				fmulp	st(2),st(0)			// * idx_frac
				fadd						// + table[0]
#ifdef _DEBUG
				fst		dword ptr [temp_gain]
#endif
				// VCA specific
				fmul        dword ptr [eax]	// multiply by *in

				// inc pointers, store result
				add         eax,4
				mov         dword ptr [in1],eax
				fstp        dword ptr [ecx]

				add         ecx,4
				mov         dword ptr [out1],ecx
			}
#ifdef _DEBUG
			idx = max(idx,0.f);
			assert( int_idx == (int) floor(idx*l_table_size) );
			assert( idx_frac == idx*l_table_size - floor(idx*l_table_size));
			float* p = m_lookup_table + int_idx;
			float compare_gain = p[0] + idx_frac * (p[1] - p[0]);
			float error = fabs( (temp_gain - compare_gain ) / (1.0f + temp_gain) );
			assert( error < 0.000001f ); //temp_gain == compare_gain );

			if( s < sampleframes )
			{
				//				assert( fabs(*(out1-1) - *(in1-1) * compare_gain) < 0.001 );
			}

			//			float s 1 = *s2++;
			//		_RPT2(_CRT_WARN, "CalcGain %f, table%f\n", CalcGain(idx/l_table_size), ( s1 * (1.0f - idx_frac) + s2 * idx_frac ) );
			//			*out++ = *in1++ * temp_gain; //(s[0] + idx_frac * (s[1] - s[0]));
#endif
*/

//#if defined(SE_ USE_ASM)
//	__as m
//	{
//		fldcw	m_fp_mode_ normal		// restore original control word
//	}
//#endif
}

void ug_vca::process_both_run_linear(int start_pos, int sampleframes)
{
	float* in1 = in1_ptr + start_pos;
	float* in2 = in2_ptr + start_pos;
	float* out = out_ptr + start_pos;

#ifndef GMPI_SSE_AVAILABLE
	// No SSE. Use C++ instead.
	for( int s = sampleframes; s > 0 ; s-- )
	{
		float gain = *in2++;
		*out++ = *in1++ * max( gain, 0.f ); //limit to >= 0
	}
#else
	// Use SSE instructions.

	// process fiddly non-sse-aligned prequel.
	while (sampleframes > 0 && reinterpret_cast<intptr_t>(out) & 0x0f)
	{
		float gain = *in2++;
		*out++ = *in1++ * max( gain, 0.f ); //limit to >= 0
		--sampleframes;
	}

	// SSE intrinsics
	__m128* pIn1 = (__m128*) in1;
	__m128* pIn2 = (__m128*) in2;
	__m128* pDest = (__m128*) out;
	const __m128 staticZero = _mm_setzero_ps();

	// move first running input plus static sum.
	while (sampleframes > 0)
	{
		*pDest++ = _mm_mul_ps( *pIn1++, _mm_max_ps(*pIn2++, staticZero) );
		sampleframes -= 4;
	}
#endif
}

void ug_vca::process_vol_fixed(int start_pos, int sampleframes)
{
	float* in1 = in1_ptr + start_pos;
	float* out = out_ptr + start_pos;

	const float gain = fixed_gain;

#ifndef GMPI_SSE_AVAILABLE
	// No SSE. Use C++ instead.
	for (int s = sampleframes; s > 0; s--)
	{
		*out++ = *in1++ * gain;
	}
#else

	/*
	__as m
	{
	  mov	ecx, dword ptr [this]				; get 'this' pointer

	//      float *p_left = input1 + start_pos;
	  mov	eax, dword ptr [ecx].in1_ptr
	  mov	edx, dword ptr start_pos
	  lea   esi,[eax+edx*4]  // + start_pos

	  mov	ebx,sampleframes		// count

	  mov	edi, dword ptr [ecx].out_ptr   //edi is pointer index to dest buf
	  lea   edi,[edi+edx*4]  // + start_pos

	  lea	ebx,[esi+ebx*4]			//ebx holds ptr after last input sample

	  fld	dword ptr [ecx].fixed_gain     //push to st(1) FloatToShortMul == 32768.0
	//  jmp     StartLoop			// not nesc provided sampleframes never zero
	ContinueLoop:

		fld	dword ptr [esi]			//load a float from source buffer
		// !! faster: store 4 in a register, add that to esi
		add	esi,4					//increment source buffer
		fmul	st(0), st(1)
		fstp	dword ptr [edi]     //convert to smallint and save to dest buffer
		add	edi,4					//increment dest buffer
		cmp	esi,ebx					//check if done
		jb	ContinueLoop			//loop for next conversion

	  // Cleanup:
	  fstp    st(0)					//Pop first pushed FP value
	}
	*/
	// process fiddly non-sse-aligned prequel.
	while (sampleframes > 0 && reinterpret_cast<intptr_t>(out) & 0x0f)
	{
		*out++ = *in1++ * gain;
		--sampleframes;
	}

	// SSE intrinsics
	__m128* pIn1 = (__m128*) in1;
	__m128* pDest = (__m128*) out;
	const __m128 staticGain = _mm_set_ps1( gain );

	// move first running input plus static sum.
	while (sampleframes > 0)
	{
		*pDest++ = _mm_mul_ps( *pIn1++, staticGain );
		sampleframes -= 4;
	}
#endif
}

void ug_vca::process_bypass(int start_pos, int sampleframes)
{
	const float* in = in1_ptr + start_pos;
	float* out = out_ptr + start_pos;

#ifndef GMPI_SSE_AVAILABLE

	// No SSE. Use C++ instead.
	for (int s = sampleframes; s > 0; s--)
	{
		*out++ = *in++;
	}
#else
	// Use SSE instructions.

	// process fiddly non-sse-aligned prequel.
	while (sampleframes > 0 && reinterpret_cast<intptr_t>(out) & 0x0f)
	{
		*out++ = *in++;
		--sampleframes;
	}

	// SSE intrinsics
	const __m128* pIn1 = (__m128*) in;
	__m128* pDest = (__m128*) out;

	while (sampleframes > 0)
	{
		*pDest++ = *pIn1++;
		sampleframes -= 4;
	}
#endif
}

void ug_vca::process_both_stop(int start_pos, int sampleframes)
{
	float* in1 = in1_ptr + start_pos;
	float* out = out_ptr + start_pos;
	const float out_val = *in1 * fixed_gain;

	assert(out_val == staticLevel_);
#ifndef GMPI_SSE_AVAILABLE

	// No SSE. Use C++ instead.
	for( int s = sampleframes; s > 0 ; s-- )
	{
		*out++ = out_val;
	}

#else
	auto s = sampleframes;

	// Use SSE instructions.
	// process fiddly non-sse-aligned prequel.
	while (s > 0 && reinterpret_cast<intptr_t>(out) & 0x0f)
	{
		*out++ = out_val;
		--s;
	}

	// SSE intrinsics
	__m128* pDest = (__m128*) out;
	const __m128 staticLevel = _mm_set_ps1( out_val );

	// move first runing input plus static sum.
	while (s > 0)
	{
		*pDest++ = staticLevel;
		s -= 4;
	}
#endif

	SleepIfOutputStatic(sampleframes);
}

bool ug_vca::PPGetActiveFlag()
{
	// Multiplier/VCA is special.  Only active if BOTH inputs are active
	return GetPlug(PN_SIGNAL)->GetPPDownstreamFlag() && GetPlug(PN_GAIN)->GetPPDownstreamFlag();  // Multiplier is only polyphonic if both inputs are downstream
}

void ug_vca::onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state )
{
	float newStaticLevel = 888888888.8f;

	if (p_to_plug == GetPlug(PN_RESPONSE))
	{
		OnCurveChange();
		fixed_gain = CalcGain( GetPlug( PN_GAIN )->getValue() );
	}

	if( p_to_plug == GetPlug( PN_GAIN ) && p_state < ST_RUN )
	{
		fixed_gain = CalcGain( GetPlug( PN_GAIN )->getValue() );
		//		_RPT1(_CRT_WARN, "fixed_gain %f\n", fixed_gain );
	}

	state_type out_stat = max( GetPlug(PN_SIGNAL)->getState(), GetPlug(PN_GAIN)->getState() );

	// if one input is zero (and not changing) no need to recalc output
	if (GetPlug(PN_SIGNAL)->getState() < ST_RUN && GetPlug(PN_SIGNAL)->getValue() == 0.0f)
	{
		out_stat = ST_ONE_OFF;
		newStaticLevel = 0.0f;
	}

	if (GetPlug(PN_GAIN)->getState() < ST_RUN && GetPlug(PN_GAIN)->getValue() <= 0.0f )
	{
		out_stat = ST_ONE_OFF;
		newStaticLevel = 0.0f;
	}

	// Check that output stat is actually changing
	if( out_stat < ST_RUN )
	{
		SET_CUR_FUNC( &ug_vca::process_both_stop );

		if (GetPlug(PN_OUT)->isStreaming() || staticLevel_ != newStaticLevel)
		{
			newStaticLevel = GetPlug(PN_SIGNAL)->getValue() * fixed_gain;
			ResetStaticOutput();
			//GetPlug(PN_OUT)->TransmitState(p_clock, out_stat);
			GetPlug(PN_OUT)->setStreamingA(false, p_clock);
		}
	}
	else
	{
		if( GetPlug(PN_GAIN)->getState() < ST_RUN )
		{
			if (fixed_gain == 1.0f)
			{
				SET_CUR_FUNC(&ug_vca::process_bypass);
			}
			else
			{
				SET_CUR_FUNC(&ug_vca::process_vol_fixed);
			}
		}
		else
		{
			if( scale_type == 2 )
			{
				SET_CUR_FUNC( &ug_vca::process_both_run_linear );
			}
			else
			{
				SET_CUR_FUNC(&ug_vca::process_both_run );
			}
		}
		GetPlug(PN_OUT)->TransmitState( p_clock, out_stat );
	}
	staticLevel_ = newStaticLevel;
}

/*
Actually pro mixer faders are not linear in dB, but piecewise linear.
For example in one popular 100mm fader layout, the top 48mm is -5dB per 12
mm, the next 36mm are -10dB per 12mm, and the next 12mm are -20 dB per 12mm
and the bottom 4 mm go from -60dB to -infinity.
*/
float ug_vca::CalcGain( float vol )
{
	//	vol = max( vol, 0.f );  // volumes < zero don't make sense
	if( !( vol > 0.f) ) // faster. Condition arranged to treat INF as zero.
		return 0.f;

	if( scale_type != 2 && vol > 3.0f ) // table clips at 30V, keep this routine consistent (else weird when moving sliders).
		vol = 3.f;

	float res;

	switch( scale_type )
	{
	case 0:// Old db curve (faulty)
		{
		    // 10 -> 2V maps 0db -> -30db
		    const float slope = 5.f; // drops 40 db over 8 volts
		    float db = slope * (vol - 1.f);

		    if( db < -4.f ) // last 2 volt maps -40db to -70db (inaudible)
			{
				db = (db + 4.f) * 3.f - 4.f;
			}

			res = powf( 10.f, db ); //  this has mistake (left for backward compat)
		}
	break;

	case 1: // exp curve
		{
		    assert( vol >= 0.0f && "needs special case for zero due to numerical errors" );
		    const float c1 = 1.f / ( 1.f - expf( -3.f ));      //  scale factor truncated curve
		    res = 1.f - c1 * (1.f - expf( 3.f * (vol - 1.0f)));
		}
		break;

	case 2: // linear response curve
			res = vol;
		break;

	case 3:
		{
		    res = calcDecibelGainCurve( vol );
		}
		break;

	default:
			res = 132.f; // flush out bugs
		assert(false);
		break;
	};

	// VERY important, most of these formulas have round-off error near zero.
	// VCA must output 0.0 when input gain is zero, else voice is held open wasting much CPU
	// taper low volume to absolute zero
	if( vol < 0.001 )
	{
		res = res * vol * 1000.f;
	}

	//kill_denormal(res);
	return res;
}

void ug_vca::InitVolTable_old(ug_base* p_ug, ULookup * &table)
{
	// this had an error in my db formula, goes quiet way too soon
	p_ug->CreateSharedLookup2(L"db Curve - old", table, -1, VCA_TABLE_SIZE_30V+2, true, SLS_ALL_MODULES );

	if( !table->GetInitialised() )
	{
		for( int j = 1 ; j < VCA_TABLE_SIZE_30V + 2; j++ )
		{
			// 10 -> 8V maps 0db -> -40db
			float db = 50.f * (float)j / (float)VCA_TABLE_SCALE_30V - 50.f;

			if( db < -40.f ) // last 2 volt maps -40db to -70db (inaudible)
			{
				db = (db + 40.f) * 3.f - 40.f;
			}

			// wrong !!!!!, should be powf( 10.f, db * 0.05f ); fixed below in InitVolTable()
			float gain = powf( 10.f, db * 0.1f );
			table->SetValue( j, gain );
		}

		// truncate quietest value to zero
		table->SetValue( 0, 0.f );
		table->SetInitialised();
	}
}

void ug_vca::InitVolTable(ug_base* p_ug, ULookup * &table)
{
	// corrected formula
	p_ug->CreateSharedLookup2( L"db Curve", table, -1, VCA_TABLE_SIZE_30V+2, true, SLS_ALL_MODULES );

	if( !table->GetInitialised() )
	{
		for( int j = 0 ; j < VCA_TABLE_SIZE_30V + 2; ++j )
		{
			table->SetValue( j, calcDecibelGainCurve(((float)j / (float)VCA_TABLE_SCALE_30V)) );
		}

		// truncate quietest value to zero. fixes small rounding errors and prevents signal leakage holding voice open
		table->SetValue( 0, 0.f );
		table->SetInitialised();
	}
}

void ug_vca::OnCurveChange()
{
	switch( scale_type )
	{
	case 0:
		m_lookup_table = shared_lookup_table_db->GetBlock( ); // db curve
		break;

	case 1:
		m_lookup_table = shared_lookup_table->GetBlock( ); // exp curve
		break;

	case 2:
			//		lookup_table = NULL; // linear
			break;

	case 3:
		m_lookup_table = shared_lookup_table_db2->GetBlock( ); // db curve 2
		break;

	default:
			assert(false);
		break;
	};
}
