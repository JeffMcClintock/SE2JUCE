#include "pch.h"
#include <math.h>
#include "modules/shared/xp_simd.h"
#include "ug_delay.h"

#include "SeAudioMaster.h"
#include "conversion.h"
#include "module_register.h"
#include "./modules/shared/xplatform.h"

SE_DECLARE_INIT_STATIC_FILE(ug_delay);

namespace
{
REGISTER_MODULE_1_BC(17,L"Delay", IDS_MN_DELAY,IDS_MG_DEBUG,ug_delay ,CF_STRUCTURE_VIEW,L"Creates an echo effect");
REGISTER_MODULE_1_BC(117,L"Delay2", IDS_MN_DELAY2,IDS_MG_EFFECTS,ug_delay2,CF_STRUCTURE_VIEW,L"Creates an echo effect");
}

#define PN_SIGNAL		0
#define PN_MODULATION	1
#define PN_OUTPUT		2
#define PN_DELAY_TIME	3
#define PN_FEEDBACK	5

// Fill an array of InterfaceObjects with plugs and parameters
void ug_delay::ListInterface2(InterfaceObjectArray& PList)
{
	// IO Var, Direction, Datatype, Name, Default, range/enum list
	// defid used to name a enum list or range of values
	LIST_PIN2( L"Signal In", input_ptr, DR_IN, L"", L"", IO_LINEAR_INPUT, L"");
	LIST_PIN2( L"Modulation", modulation_ptr, DR_IN, L"5", L"5,-5,5,-5",IO_POLYPHONIC_ACTIVE, L"Varies the delay time dynamically ( -5V to +5V )");
	LIST_PIN2( L"Signal Out", output_ptr, DR_OUT, L"", L"", 0, L"");
	//	LIST_VAR3( L"Delay Time (secs)",delay_time, DR _PARAMETER, DT_FLOAT , L"0.25", L"",0, L"Max delay time in Seconds. Limited to maximum 10s.");
	//	LIST_VAR3( L"Interpolate Output", interpolate, DR _PARAMETER, DT_BOOL , L"0", L"", 0, L"Provides smoother modulation of delay time, but increases CPU load");
	LIST_VAR3( L"Delay Time (secs)",delay_time, DR_IN, DT_FLOAT , L"0.25", L"",IO_MINIMISED, L"Max delay time in Seconds. Limited to maximum 10s.");
	LIST_VAR3( L"Interpolate Output", interpolate, DR_IN, DT_BOOL , L"0", L"", IO_MINIMISED, L"Provides smoother modulation of delay time, but increases CPU load");
	LIST_PIN2( L"Feedback", feedback_ptr, DR_IN, L"0", L"10,0,10,0",IO_POLYPHONIC_ACTIVE, L"");
}

void ug_delay2::ListInterface2(InterfaceObjectArray& PList)

{
	// IO Var, Direction, Datatype, Name, Default, range/enum list
	// defid used to name a enum list or range of values
	LIST_PIN2( L"Signal In", input_ptr, DR_IN, L"", L"", IO_LINEAR_INPUT, L"");
	LIST_PIN2( L"Modulation", modulation_ptr, DR_IN, L"10", L"10,0,10,0",IO_POLYPHONIC_ACTIVE, L"Varies the delay time dynamically ( 0 to 10V )");
	LIST_PIN2( L"Signal Out", output_ptr, DR_OUT, L"", L"", 0, L"");
	//	LIST_VAR3( L"Delay Time (secs)",delay_time, DR _PARAMETER, DT_FLOAT , L"1.0", L"",0, L"Max delay time in Seconds. Limited to maximum 10s.");
	//	LIST_VAR3( L"Interpolate Output", interpolate, DR _PARAMETER, DT_BOOL , L"0", L"", 0, L"Provides smoother modulation of delay time, but increases CPU load");
	LIST_VAR3( L"Delay Time (secs)",delay_time, DR_IN, DT_FLOAT , L"1.0", L"",IO_MINIMISED, L"Max delay time in Seconds. Limited to maximum 10s.");
	LIST_VAR3( L"Interpolate Output", interpolate, DR_IN, DT_BOOL , L"0", L"", IO_MINIMISED, L"Provides smoother modulation of delay time, but increases CPU load");
	LIST_PIN2( L"Feedback", feedback_ptr, DR_IN, L"0", L"10,0,10,0",IO_POLYPHONIC_ACTIVE, L"");
}

ug_delay::ug_delay() :
	buffer(nullptr)
	,interpolate(false)
	,count(0)
//	,fixed_feedback(0.0)
	,m_modulation_input_offset(0.5f)
	,m_prev_out(0.f)
{
}

/* Chris K..
!!!I noticed in your Delay2 - that the feedback is of the "Feedback into the next input sample" system.
With this system the Feedback signal is delayed One additional sample ( Feedback time = DelayTime + 1sm )
I think this is the most generally used - but causes the signal to drift/lag by 1 sample each feedback cycle - so after 44 feedback cycles the signal is 1ms late @44.1Khz.
(Also complicates tuned delay lines)
!!!
*/


void ug_delay::onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state)
{
	if( p_to_plug == GetPlug(PN_DELAY_TIME) )
	{
		CreateBuffer();
		return;
	}

	state_type mod_state = GetPlug(PN_MODULATION)->getState();
	state_type fb_state = GetPlug(PN_FEEDBACK)->getState();

    typedef void(ug_delay2::*myProcessPtr)(int,int);

	if( interpolate )
	{
		if( mod_state == ST_RUN )
		{
			if( fb_state == ST_RUN )
			{
                myProcessPtr s = &ug_delay::subProcess<PolicyModulationDigitalChanging, PolicyInterpolationLinear, PolicyFeedbackModulated >;
				SET_CUR_FUNC( s );

			}
			else
			{
				SET_CUR_FUNC( (myProcessPtr) (&ug_delay::subProcess<PolicyModulationDigitalChanging, PolicyInterpolationLinear, PolicyFeedbackFixed >) );
			}
		}
		else // Not modulated.
		{
			if( fb_state == ST_RUN )
			{
				SET_CUR_FUNC( (myProcessPtr) (&ug_delay::subProcess<PolicyModulationFixed, PolicyInterpolationLinear, PolicyFeedbackModulated >) );
			}
			else
			{
				SET_CUR_FUNC( (myProcessPtr) (&ug_delay::subProcess<PolicyModulationFixed, PolicyInterpolationLinear, PolicyFeedbackFixed >) );
			}
		}
	}
	else // no interpolate
	{
		if( mod_state == ST_RUN )
		{
			if( fb_state == ST_RUN )
			{
				//SET_CUR_FUNC( &ug_delay::sub_process_modulated_feedback );
				SET_CUR_FUNC( (myProcessPtr) (&ug_delay::subProcess<PolicyModulationDigitalChanging, PolicyInterpolationNone, PolicyFeedbackModulated >) );
			}
			else
			{
				//SET_CUR_FUNC( &ug_delay::sub_process_modulated );
				SET_CUR_FUNC( (myProcessPtr) (&ug_delay::subProcess<PolicyModulationDigitalChanging, PolicyInterpolationNone, PolicyFeedbackFixed >) );
			}
		}
		else // not modulate.
		{
			if( fb_state == ST_RUN )
			{
				//SET_CUR_FUNC( &ug_delay::sub_process_feedback );
				SET_CUR_FUNC( (myProcessPtr) (&ug_delay::subProcess<PolicyModulationFixed, PolicyInterpolationNone, PolicyFeedbackModulated >) );
			}
			else
			{
				//SET_CUR_FUNC( &ug_delay::sub_process );
				SET_CUR_FUNC( (myProcessPtr) (&ug_delay::subProcess<PolicyModulationFixed, PolicyInterpolationNone, PolicyFeedbackFixed >) );
			}
		}
	}

	// if input state changes, reset sta-tic output count
	if( p_to_plug == GetPlug(PN_SIGNAL) )
	{
		resetStaticCounter();
		OutputChange( p_clock, GetPlug(PN_OUTPUT), ST_RUN );
	}

	if( GetPlug(PN_SIGNAL)->getState() < ST_RUN )
	{
		current_process_func = process_function;
		SET_CUR_FUNC( &ug_delay::sub_process_static );
	}
}

// Shut down delay after signal died down
void ug_delay::sub_process_static(int start_pos, int sampleframes)
{
	(this->*(current_process_func))( start_pos, sampleframes );
	// now check output for signal
	float* output = output_ptr + start_pos;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		if( *output++ != 0.f )
		{
			resetStaticCounter();
			return;
		}
	}

	SleepIfOutputStatic(sampleframes);

	if( static_output_count <= 0 )
	{
		OutputChange( SampleClock() + sampleframes, GetPlug(PN_OUTPUT), ST_ONE_OFF );
	}
}

int ug_delay::Open()
{
	ug_base::Open();
	OutputChange( SampleClock(), GetPlug(PN_OUTPUT), ST_RUN );

	if( !buffer )
		CreateBuffer();

	return 0;
}

ug_delay::~ug_delay()
{
	if( buffer )
		delete [] buffer;
}

void ug_delay::CreateBuffer()
{
	if( buffer )
	{
		delete [] buffer;
		buffer = NULL;
	}

	buffer_size = (int) ( getSampleRate() * delay_time );

	if( buffer_size < 1 )
		buffer_size = 1;

	if( buffer_size > getSampleRate() * 10 )	// limit to 10 s sample
		buffer_size = (int) getSampleRate() * 10;

	/*	buffer = new float[buffer_size];
	memset(buffer, 0, buffer_size * sizeof(float) ); // clear buffer
*/
	// allow extra to avoid overwritting 'tail'
	padded_size = buffer_size + interpolationExtraSamples;

	// Add some samples off-end to simplify interpolation.
	int allocatedSize = padded_size + interpolationExtraSamples;

	buffer = new float[allocatedSize];
	memset(buffer, 0, allocatedSize * sizeof(float) ); // clear buffer
	buffer[buffer_size + interpolationExtraSamples] = 10000000; // testing for glitches

	// ensure we arn't accessing data outside buffer
	count = 0;
	resetStaticCounter();
}

/*
IIRC Metaflanger has true "thru the null" flanging which means that the
"delayed" signal whose delay time is modulated can coincide and even
precede the "dry" signal. This is achieved most probably by putting a
small sta-tic delay into the dry signal path. Not too many
run-on-the-mill digital flangers have this capability.. there the delay
is always behind the dry signal. I could misremember though so be warned.

Another cool strangeness is that you can use a phase linear filter to
limit flanging to a specific frequency range only and still mix it back
into the original with confidence (due to phase linearity).
*/

/*
// process assuming modulation not changing every sample
// with interpolation
// BOTH INPUTS RUNNING
void ug_delay::tick_interpolated2()
{
	//multiply buffer size by mod_factor to get offset into buffer
__as m
{
	mov         ecx,dword ptr [this]
	mov         edx,dword ptr [ecx].mod_factor		; EDX = in1
	mov			eax,0x400000
	sub         eax,dword ptr [edx]			; EAX = *EDX
	imul        dword ptr [ecx].buffer_size				; EDX is buffer_size
//	shl			edx,17			// adjust output size >> 15
//	shr			eax,15
//	add			eax,edx						; EAX >> 15
	shld		edx,eax, 17	// (64 bit shift right)


	// now split result into offset + fine (lowest 8 bit)
	mov			eax,edx
	and			edx,0xff
	mov         dword ptr [ecx]this.read_offset_fine,edx
	sar			eax,8 // shift arith right (signed)
	// ensure factor > 0
	jge			label1
	mov			eax,0x0
label1:
	// ensure factor < buffer size
	cmp			eax,dword ptr [ecx].buffer_size
	jl			label2
	mov			eax,dword ptr [ecx].buffer_size
	sub			eax,2	// allow space for interpolation
label2:
	mov         dword ptr [ecx]this.read_offset,eax		; read_offset = EAX
/ * not needed as read_point 1 does it anyway
	// read_offset %= buffer_size (limit read offset to less than buffer size)
	cdq			// 32 bit sign extend eax into edx
	idiv        dword ptr [ecx].buffer_size				; EDX is buffer_size
	// remainder stored in edx
	mov         dword ptr [ecx]this.read_offset,edx		; read_offset = EAX
* /
}
	int read_pointer1 = (count + read_offset) % buffer_size;
	int read_pointer2 = ( read_pointer1 + 1) % buffer_size;
	output = ( buffer[ read_pointer2 ] * read_offset_fine + buffer[ read_pointer1 ] * ( 0x100 - read_offset_fine ) ) / 0x100;
	buffer[count++] = *input;
	count %= buffer_size;
/ *
	if( (SeAudioMaster::sample_clock & 0xfff) ==0)
	{
	_RPT2(_CRT_WARN, "\n ug_delay  offset(%d) into %d size buffer", (int) read_offset,  (int) buffer_size);
	}
* /
}
*/

/*
void ug_delay::sub_process(int start_pos, int sampleframes)
{


float* input = input_ptr + start_pos;
float* output = output_ptr + start_pos;
float prev_out = m_prev_out;

for( int s = sampleframes ; s > 0 ; s-- )
{
buffer[count] = *input++ + prev_out * fixed_feedback;
// *output++ = prev_out = kill _denormal2( buffer[ ( count + read_offset) % buffer_size ] );
*output++ = prev_out = buffer[ ( count + read_offset) % buffer_size ];
++count %= buffer_size;
}

m_prev_out = prev_out;
}

void ug_delay::sub_process_modulated(int start_pos, int sampleframes)
{


float* input = input_ptr + start_pos;
float* modulation = modulation_ptr + start_pos;
float* output = output_ptr + start_pos;
float prev_out = m_prev_out;

for( int s = sampleframes ; s > 0 ; s-- )
{
CalcModulation(*modulation++);
buffer[count] = *input++ + prev_out * fixed_feedback;
*output++ = prev_out = buffer[ ( count + read_offset) % buffer_size ];
++count %= buffer_size;
}

m_prev_out = prev_out;
}

void ug_delay::sub_process_interpolated(int start_pos, int sampleframes)
{
float* input = input_ptr + start_pos;
float* output = output_ptr + start_pos;
float prev_out = m_prev_out;

for( int s = sampleframes ; s > 0 ; s-- )
{
buffer[count] = *input++ + prev_out * fixed_feedback;
int read_pointer1 = (count + read_offset) % buffer_size;
int read_pointer2 = ( read_pointer1 + 1) % buffer_size;
*output++ = prev_out = buffer[ read_pointer1 ] + read_offset_fine * ( buffer[ read_pointer2 ] - buffer[ read_pointer1 ] );
++count %= buffer_size;
}

m_prev_out = prev_out;
}

void ug_delay::sub_process_modulated_interpolated(int start_pos, int sampleframes)
{
float* input = input_ptr + start_pos;
float* modulation = modulation_ptr + start_pos;
float* output = output_ptr + start_pos;
float prev_out = m_prev_out;

for( int s = sampleframes ; s > 0 ; s-- )
{
CalcModulation(*modulation++);
buffer[count] = *input++ + prev_out * fixed_feedback;

/ *
int read_pointer1 = (count + read_offset) % buffer_size;
int read_pointer2 = ( read_pointer1 + 1) % buffer_size;
* /

int read_pointer1 = count + read_offset;
if( read_pointer1 >= buffer_size )
{
read_pointer1 -= buffer_size;
}

// *output++ = prev_out = kill _denormal2( buffer[ read_pointer1 ] + read_offset_fine * ( buffer[ read_pointer2 ] - buffer[ read_pointer1 ] ));
*output++ = prev_out = buffer[ read_pointer1 ] + read_offset_fine * ( buffer[ read_pointer1 + 1 ] - buffer[ read_pointer1 ] );
/ *
++count; // %= buffer_size;
if( count >= buffer_size )
{
count = 0;
}
* /
if( count >= buffer_size )
{
// make a copy of same sample at buffer start and end to ease interpolation.
buffer[count - buffer_size] = buffer[count];
if( count >= buffer_size + interpolationExtraSamples - 1 )
{
count -= buffer_size;
}
}
++count; // %= buffer_size;
}

m_prev_out = prev_out;
}

//////////////// with feedback
/ *
> I wonder if that makes my delay calculations slightly wrong?
No........The delay time is perfectly correct with either feedback system........the 1st delayed signal/Echo will occur properly.
However, using your method - the Feedback time is wrong by 1 sample - since it is carried forward (*not fed back directly)
Each time the signal regenerates through the Feedback, it slips a further 1 sample.

affects tuning?
* /
void ug_delay::sub_process_feedback(int start_pos, int sampleframes)
{
float* input = input_ptr + start_pos;
float* output = output_ptr + start_pos;
float* feedback = feedback_ptr + start_pos;
float prev_out = m_prev_out;

for( int s = sampleframes ; s > 0 ; s-- )
{
buffer[count] = *input++ + prev_out * *feedback++;
*output++ = prev_out = buffer[ ( count + read_offset) % buffer_size ];
++count %= buffer_size;
}

m_prev_out = prev_out;
}

void ug_delay::sub_process_interpolated_feedback(int start_pos, int sampleframes)
{


float* input = input_ptr + start_pos;
float* output = output_ptr + start_pos;
float* feedback = feedback_ptr + start_pos;
float prev_out = m_prev_out;

for( int s = sampleframes ; s > 0 ; s-- )
{
buffer[count] = *input++ + prev_out * *feedback++;
int read_pointer1 = (count + read_offset) % buffer_size;
int read_pointer2 = ( read_pointer1 + 1) % buffer_size;
*output++ = prev_out = buffer[ read_pointer1 ] + read_offset_fine * ( buffer[ read_pointer2 ] - buffer[ read_pointer1 ] );
++count %= buffer_size;
}

m_prev_out = prev_out;
}

void ug_delay::sub_process_modulated_feedback(int start_pos, int sampleframes)
{


float* input = input_ptr + start_pos;
float* modulation = modulation_ptr + start_pos;
float* output = output_ptr + start_pos;
float* feedback = feedback_ptr + start_pos;
float prev_out = m_prev_out;

for( int s = sampleframes ; s > 0 ; s-- )
{
CalcModulation(*modulation++);
buffer[count] = *input++ + prev_out * *feedback++;
*output++ = prev_out = buffer[ ( count + read_offset) % buffer_size ];
++count %= buffer_size;
}

m_prev_out = prev_out;
}

void ug_delay::sub_process_modulated_interpolated_feedback(int start_pos, int sampleframes)
{


float* input = input_ptr + start_pos;
float* modulation = modulation_ptr + start_pos;
float* output = output_ptr + start_pos;
float* feedback = feedback_ptr + start_pos;
float prev_out = m_prev_out;

for( int s = sampleframes ; s > 0 ; s-- )
{
CalcModulation(*modulation++);
buffer[count] = *input++ + prev_out * *feedback++;
int read_pointer1 = (count + read_offset) % buffer_size;
int read_pointer2 = ( read_pointer1 + 1) % buffer_size;
*output++ = prev_out = buffer[ read_pointer1 ] + read_offset_fine * ( buffer[ read_pointer2 ] - buffer[ read_pointer1 ] );
++count %= buffer_size;
}

m_prev_out = prev_out;
}
*/
/*
void ug_delay::CalcModulation( float p_modulation )
{
float d = (m_modulation_input_offset - p_modulation ) * buffer_size;

read_offset = FastRealToIntTruncateTowardZero(d)); // fast float-to-int using SSE. truncation toward zero.

read_offset_fine = d - (float) read_offset;

if( read_offset < 1 )
{
read_offset = 1;
read_offset_fine = 0.0f;
}
else
{
if( read_offset > buffer_size )
{
read_offset = buffer_size;
read_offset_fine = 0.0f;
}
}
*/
/*
int debug_read_offset = read_offset;
float debug_read_offset_fine = read_offset_fine;

//	_RPT2(_CRT_WARN, "ug_delay::CalcModulation(%f) read_offset=%f\n", p_modulation, d );
// * slower than SSE
__as m
{
fld cw	m_fp_mode_round_down		// load modified control word

mov		ecx, dword ptr [this]
fld		dword ptr [ecx].m_modulation_input_offset
fsub	dword ptr [p_modulation]
fimul	dword ptr [ecx].buffer_size

fist	dword ptr [ecx].read_offset
fisub   dword ptr [ecx].read_offset
mov		eax, dword ptr [ecx].read_offset
fstp	dword ptr [ecx].read_offset_fine

cmp		eax,1
jge		above_one
mov		dword ptr [ecx].read_offset, 1 // minimum val is one (max delay)
above_one:
cmp		eax, dword ptr [ecx].buffer_size // min delay
jl		done_it
mov		eax, dword ptr [ecx].buffer_size
mov		dword ptr [ecx].read_offset, eax

done_it:
fld cw	m_fp_mode _normal	// restore original control word
}

assert( debug_read_offset == read_offset );
assert( debug_read_offset_fine == read_offset_fine );
}
*/
