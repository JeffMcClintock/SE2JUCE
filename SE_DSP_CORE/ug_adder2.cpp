#include "pch.h"
#include <assert.h>
#include "ug_adder2.h"

#include "module_register.h"
#include "ug_container.h"

SE_DECLARE_INIT_STATIC_FILE(ug_adder2)

using namespace std;
using namespace gmpi;

namespace
{
REGISTER_MODULE_1( L"Float Adder2", IDS_MN_FLOAT_ADDER2,IDS_MG_DEBUG,ug_adder2 ,CF_HIDDEN,L"");
}
namespace
{
REGISTER_MODULE_1_BC(118,L"Voice Combiner", IDS_MN_VOICE_COMBINER,IDS_MG_SPECIAL,ug_voice_combiner,CF_STRUCTURE_VIEW,L"Forces signal to be monophonic. Usefull for triggering a monophonic LFO from a MIDI CV gate signal.");
}

namespace
{
	// see also "SE Poly to MonoA" (ug_voice_splitter.cpp)
	auto r2 = internalSdk::Register<ug_poly_to_monoB>::withXml(
		(R"XML(
<?xml version="1.0" ?>
<PluginList>
  <Plugin id="SE Poly to Mono" name="Poly to Mono" category="Special" >
    <Audio>
		<Pin name="Output" datatype="float" rate="audio" direction="out" />
		<Pin name="LastVoice" datatype="midi" direction="in" private="true" />
		<Pin name="Input" datatype="float" rate="audio" isAdderInputPin="true" />
    </Audio>
  </Plugin>
</PluginList>
)XML")
		);
}

/////////////////////////////////////////////////////////////////
// Voice combiner is simply an adder, with a additional flag set.
ug_voice_combiner::ug_voice_combiner()
{
	SetFlag(UGF_POLYPHONIC_AGREGATOR);
}
/////////////////////////////////////////////////////////////////

// Fill an array of InterfaceObjects with plugs and parameters
void ug_adder2::ListInterface2(InterfaceObjectArray& PList)
{
	// IO Var, Direction, Datatype, Name, Default, defid (index into unit_gen::PlugFormats)
	// defid used to name a enum list or range of values
	// connection code relys on output listed first
	LIST_PIN2( L"Out", output_ptr, DR_OUT, L"0", L"", 0, L"");
	LIST_PIN( L"In", DR_IN, L"0", L"", IO_ADDER|IO_LINEAR_INPUT, L"");
}

ug_adder2::ug_adder2() :
	static_input_sum(123456.789f) // unlikely value to trigger initial status update.
	,running_inputs(NULL)
    ,running_input_max_size(0)

{
	SetFlag(UGF_SSE_OVERWRITES_BUFFER_END);
}

ug_adder2::~ug_adder2()
{
	// because this destructor is executed b4 base class,
	// when the base class goes to delete the input plug( which has pointer to input array)
	// input array is already deleted, invalidating plug's pointer. So need to delete plugs here
	// before input pointers auto deleted by dstr
	delete [] running_inputs;
}

void ug_adder2::NewConnection(UPlug* p_from)
{
	auto p = plugs.back();

	if (p->InUse())
	{
		p = new UPlug(this, p);
		p->CreateBuffer();
		AddPlug(p);
	}

	connect(p_from, p);
}

void ug_adder2::sub_process_static(int start_pos, int sampleframes)
{
	// process fiddly non-sse-aligned prequel.
	float* o = output_ptr + start_pos;
	const float ss = static_input_sum;

#ifndef GMPI_SSE_AVAILABLE

	// No SSE. Use C++ instead.
	for (int s = sampleframes; s > 0; --s)
	{
		*o++ = ss;
	}

#else
	auto s = sampleframes;

	// Use SSE instructions.
	while (reinterpret_cast<intptr_t>(o) & 0x0f)
	{
		*o++ = ss;
		--s;
	}

	__m128* pDest = (__m128*) o;
	const __m128 staticSum = _mm_set_ps1( ss );

	// move first runing input plus static sum.
	while (s > 0)
	{
		*pDest++ = staticSum;
		s -= 4;
	}
#endif

	// mechanism to switch to idle after one block filled staticaly
	SleepIfOutputStatic(sampleframes);
}

void ug_adder2::sub_process_mixed(int start_pos, int sampleframes)
{
	// process fiddly non-sse-aligned prequel.
	float* i = running_inputs[0] + start_pos;
	float* o = output_ptr + start_pos;

	const float ss = static_input_sum;

#ifdef GMPI_SSE_AVAILABLE
	// do some SSE.

	auto lSampleFrames = sampleframes;
	while (reinterpret_cast<intptr_t>(o) & 0x0f)
	{
		*o++ = *i++ + ss;
		--lSampleFrames;
	}

	// SSE intrinsics
	__m128* pSource = (__m128*) i;
	__m128* pDest = (__m128*) o;
	const __m128 staticSum = _mm_set_ps1( ss );

	// move first running input plus static sum.
	while (lSampleFrames > 0)
	{
		*pDest++ = _mm_add_ps( *pSource++, staticSum );
		lSampleFrames -= 4;
	}

	// add further inputs to output block
	for( int c = running_input_size; c > 0 ; --c )
	{
		i = running_inputs[c] + start_pos;
		o = output_ptr + start_pos;

		auto lSampleFrames = sampleframes;
		while (reinterpret_cast<intptr_t>(o) & 0x0f)
		{
			*o++ += *i++;
			--lSampleFrames;
		}

		pSource = (__m128*) i;
		pDest = (__m128*) o;

		while (lSampleFrames > 0)
		{
			*pDest = _mm_add_ps( *pSource++, *pDest );
            pDest++;
			lSampleFrames -= 4;
		}
	}

#else

	// Do some C++ instead.
	for (int s = sampleframes; s > 0; --s)
	{
		*o++ = *i++ + ss;
	}

	// add further inputs to output block
	for (int c = running_input_size; c > 0; --c)
	{
		i = running_inputs[c] + start_pos;
		o = output_ptr + start_pos;

		for (int s = sampleframes; s > 0; --s)
		{
			*o++ += *i++;
		}
	}

#endif

	/*
		float *out1 = output_ptr + start_pos;
		// direct copy one input to output
		// NOT as fast as ASM
		float *i = running_inputs[0] + start_pos;
		float *o = out1;
		const float static_ins = static_input_sum;
		for(int s = sampleframes ; s > 0 ; s-- )
		{
			*o++ = *i++ + static_ins;
		}
	* /

		float *i = running_inputs[0];
		__as m
		{
			mov     eax, dword ptr i
			mov     edx, dword ptr start_pos
			lea     ecx,[eax+edx*4]				// ecx in source ptr
			mov     edi, dword ptr out1			// edi is dest   ptr
			mov     eax, dword ptr sampleframes
			dec		eax

			mov     esi, dword ptr [this]				; get 'this' pointer
			fld     dword ptr [esi].static_input_sum // push to st(1) l_static_input_sum

			ContinueLoop:
			fld     dword ptr [ecx+eax*4]		// load a float from source buffer
			fadd    st(0), st(1)				// add sta-tic inputs
			fstp	DWORD PTR [edi+eax*4]		// save to dest buffer
			dec		eax
			jge     ContinueLoop				// loop for next conversion

			fstp    st(0)						// Pop first pushed FP value
		}

		// add further inputs to output block
		for( int c = running_input_size; c > 0 ; --c )
		{
	/ *
			i = running_inputs[c] + start_pos;
			o = out1;
			while( o != last_output_ptr )
			{
				*o++ += *i++;
			}
	* /
			i = running_inputs[c];

			__as m
			{
				mov     eax, dword ptr i
				mov     edx, dword ptr start_pos
				lea     ecx,[eax+edx*4]
				mov     edi, dword ptr out1   //edi is pointer index to dest buf
				mov     eax, dword ptr sampleframes
				dec		eax

			ContinueLoop3:
				fld     dword ptr [ecx+eax*4]   // load a float from source buffer
				fadd    DWORD PTR [edi+eax*4]   // add dest buffer
				fstp	DWORD PTR [edi+eax*4]	// save to dest buffer
				dec		eax
				jge     ContinueLoop3			// loop for next conversion
			}
		}*/
}

// compiler pushing ebx, esi, edi. avoid use if pos (still does, why?)
void ug_adder2::sub_process(int start_pos, int sampleframes)
{
#ifndef GMPI_SSE_AVAILABLE
	// No SSE. Use C++ instead.
	float *out1 = output_ptr + start_pos;
	// This just as fast as ASM
	float *o = out1;
	float *i = running_inputs[0] + start_pos;

	for(int s = sampleframes ; s > 0 ; --s )
	{
		*o++ = *i++;
	}

	// add further inputs to output block
	for (int c = running_input_size; c > 0; --c)
	{
		i = running_inputs[c] + start_pos;
		o = output_ptr + start_pos;

		for (int s = sampleframes; s > 0; --s)
		{
			*o++ += *i++;
		}
	}

#else
	// process fiddly non-sse-aligned prequel.
	float* i = running_inputs[0] + start_pos;
	float* o = output_ptr + start_pos;

	auto lSampleFrames = sampleframes;
	while (reinterpret_cast<intptr_t>(o) & 0x0f)
	{
		*o++ = *i++;
		--lSampleFrames;
	}

	// SSE intrinsics
	__m128* pSource = (__m128*) i;
	__m128* pDest = (__m128*) o;

	// move first runing input plus static sum.
	while (lSampleFrames > 0)
	{
		*pDest++ = *pSource++;
		lSampleFrames -= 4;
	}

	// add further inputs to output block
	for( int c = running_input_size; c > 0 ; --c )
	{
		i = running_inputs[c] + start_pos;
		o = output_ptr + start_pos;

		auto lSampleFrames = sampleframes;
		while (reinterpret_cast<intptr_t>(o) & 0x0f)
		{
			*o++ += *i++;
			--lSampleFrames;
		}

		pSource = (__m128*) i;
		pDest = (__m128*) o;

		while (lSampleFrames > 0)
		{
			*pDest = _mm_add_ps( *pSource++, *pDest );
            pDest++;
			lSampleFrames -= 4;
		}
	}
	// Use SSE instructions.
#endif

	/*
			o = out1;
			i = running_inputs[c] + start_pos;
			for(int s = sampleframes ; s > 0 ; s-- )
			{
				*o++ += *i++;
			}
	*/
	/*
		// ASM faster than C++ above
			float* i = running_inputs[c];

			__as m
			{
				mov     eax, dword ptr i
				mov     edx, dword ptr start_pos
				lea     ecx,[eax+edx*4]
				mov     edi, dword ptr out1		// edi is pointer index to dest buf
				mov     eax, dword ptr sampleframes
				dec		eax

			ContinueLoop4:
				fld     dword ptr [ecx+eax*4]	//load a float from source buffer
				fadd    DWORD PTR [edi+eax*4]	// add dest buffer
				fstp	DWORD PTR [edi+eax*4]	// save to dest buffer
				dec		eax
				jge     ContinueLoop4			//loop for next conversion
			}
		}
		*/
}

void ug_adder2::onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state)
{
	bool previousOutputStatic = running_input_size < 0;
	float previousStaticOutput = static_input_sum;

	CacheRunningInputs();

	if( running_input_size < 0 )
	{
		if (!previousOutputStatic || previousStaticOutput != static_input_sum)
		{
			ResetStaticOutput();
			SET_CUR_FUNC(&ug_adder2::sub_process_static);
			GetPlug(0)->TransmitState(p_clock, ST_STATIC);
		}
	}
	else
	{
		if( static_input_sum == 0.f )
		{
			SET_CUR_FUNC( &ug_adder2::sub_process );
		}
		else
		{
			SET_CUR_FUNC( &ug_adder2::sub_process_mixed );
		}
		GetPlug(0)->TransmitState(p_clock, ST_RUN);
	}

#ifdef DEBUG_STAT_CHANGES

	// if at least 1 running then output runs, else stops
	if( running_input_size == -1 )
	{
		_RPT3(_CRT_WARN, "\n ug_adder2(%x) output_change ST_ONE_OFF.  %d/%d inputs active\n", (int) this, running_input_size + 1, running_input_size + 1);
	}
	else
	{
		_RPT3(_CRT_WARN, "\n ug_adder2(%x) output_change ST_RUN.  %d/%d inputs active\n", (int) this, running_input_size+1, running_input_size+1);
	}

#endif
}

void ug_adder2::CacheRunningInputs()
{
	// Determin which input are running/stopped
	running_input_size = -1;
	static_input_sum = 0.f;

	auto it = plugs.begin();
	++it; // skip first plug (it's an output).

	for( ; it != plugs.end(); ++it )
	{
		auto p = *it;

		if( p->getState() == ST_RUN )
		{
			running_inputs[++running_input_size] = p->GetSamplePtr();
		}
		else
		{
//			static_input_sum += p->GetSampleBlock()->GetAt(BlockPos());
			static_input_sum += p->GetSamplePtr()[getBlockPosition()];
		}
	}
}

bool ug_adder2::BypassPin(UPlug* fromPin, UPlug* toPin)
{
	ug_base::BypassPin(fromPin, toPin);
	CacheRunningInputs();
	return true;
}

int ug_adder2::Open()
{
	ug_base::Open();
	running_input_max_size = (int) plugs.size() - 1;
	running_inputs = new float * [running_input_max_size];
#ifdef _DEBUG

	// clear re-allocated array
	for( int i = 0 ; i < running_input_max_size ; i++ )
	{
		running_inputs[i] = 0;
	}

#endif
	return 0;
}

void ug_poly_to_monoB::BuildHelperModule()
{
	auto helper = ModuleFactory()->GetById(L"SE Poly to MonoA")->BuildSynthOb();
	parent_container->AddUG(helper);
	helper->patch_control_container = patch_control_container;
	helper->SetupWithoutCug();

	connect(helper->GetPlug(1), GetPlug(1)); // Helper.LastVoice -> this.LastVoice (MIDI)

	// defensive default connection to helper input, in case it's never connected to anything.
	helper->GetPlug(0)->SetDefault("0");

	helper->cpuParent = cpuParent;
}

// Don't work at all if patch isn't polyphonic.
struct FeedbackTrace* ug_poly_to_monoB::PPSetDownstream()
{
	auto helper = GetPlug(1)->connections.front()->UG;

	// delete unnesc default connection to helper audio input.
	auto oldfrom = helper->GetPlug(0)->connections.front();
	oldfrom->DeleteConnection(helper->GetPlug(0));

	// Duplicate audio connection to helper, so it is sorted in correct order.
	connect(GetPlug(2)->connections.front(), helper->GetPlug(0));

	return ug_base::PPSetDownstream();
}
/*
// Don't work at all if patch isn't polyphonic.
bool ug_poly_to_monoB::PPGetActiveFlag()
{
	// in the monophonic case, helper will not be connected during PPSetDownstream(), so connect it now.
	auto helper = GetPlug(1)->connections.front()->UG;
	
	if (helper->GetPlug(0)->connections.empty())
	{
		connect(GetPlug(2)->connections.front(), helper->GetPlug(0));
		currentActiveVoiceNumber = 0;
	}

	return ug_base::PPGetActiveFlag();
}
*/