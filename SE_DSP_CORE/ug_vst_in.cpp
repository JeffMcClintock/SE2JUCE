#include "pch.h"
#include "ug_vst_in.h"
#include "module_register.h"

#ifdef CANCELLATION_TEST_ENABLE2
#define _USE_MATH_DEFINES
#include <math.h>
#endif


SE_DECLARE_INIT_STATIC_FILE(ug_vst_in)

// macros suck.
#undef min


namespace
{
REGISTER_MODULE_1(L"VST Input", IDS_MN_VST_INPUT,IDS_MG_DEBUG,ug_vst_in ,CF_HIDDEN,L"");
}

// Fill an array of InterfaceObjects with plugs and parameters
void ug_vst_in::ListInterface2(InterfaceObjectArray& PList)
{
	// IO Var, Direction, Datatype, Name, Default, defid (index into unit_gen::PlugFormats)
	// defid used to name a enum list or range of values
	LIST_VAR3(L"MIDI Out",midi_out, DR_OUT, DT_MIDI2 ,L"0",L"", 0,L"");
	LIST_PIN(L"Audio",DR_OUT, L"",L"", IO_AUTODUPLICATE,L"");
}

ug_vst_in::ug_vst_in() :
	m_inputs(0)
    ,bufferIncrement_(1)
    {
}

ug_vst_in::~ug_vst_in()
{
	delete [] m_inputs;
}

int ug_vst_in::Open()
{
	ug_base::Open();
	int count = (int) plugs.size() - 1;
	m_inputs = new const float *[count];

	for(int j = 1 ; j < plugs.size() ; j++ )
	{
		OutputChange( SampleClock(), GetPlug(j), ST_RUN );
	}

	if( count == 0 )
	{
		SET_CUR_FUNC( &ug_base::process_sleep );
	}
	else
	{
		SET_CUR_FUNC( &ug_vst_in::sub_process );
	}

	return 0;
}

int ug_vst_in::Close()
{
	return ug_base::Close();
}

void ug_vst_in::sub_process(int start_pos, int sampleframes)
{
	int out_num = 0;
    int inc = bufferIncrement_;

#ifdef CANCELLATION_TEST_ENABLE2
	const auto m_increment = M_PI * 2.0 * 1000. / SampleRate();

	float phase;
	for(int j = 1 ; j < plugs.size() ; j++ )
	{
		phase = m_phase;
		FSAMPLE* to = plugs[j]->GetSamplePtr() + start_pos;
        for( int s = sampleframes ; s > 0 ; --s )
        {
			phase += m_increment;
			*to++ = 0.5f * sinf( phase );
        }
	}
	m_phase = phase;
	if( m_phase > (M_PI * 2.0))
	{
		m_phase -= (M_PI * 2.0);
	}
#else
	for(int j = 1 ; j < plugs.size() ; j++ )
	{
		float* to = plugs[j]->GetSamplePtr() + start_pos;
		auto from = m_inputs[out_num];
		// memcpy( to, from, sizeof(float) * sampleframes );
        for( int s = sampleframes ; s > 0 ; --s )
        {
            *to++ = *from;
            from += inc;
        }
		m_inputs[out_num++] += sampleframes * inc;
	}
#endif
}

void ug_vst_in::setInputs( const float* const* p_inputs, int numChannels, int numSidechains )
{
    numSidechains_ = numSidechains;

	int lastIndex = (int) plugs.size() - 2;

	// for stereo effect, fed by mono, just copy 1st channel to second.
	for( int i = lastIndex ; i >= numChannels ; --i )
	{
		m_inputs[i] = p_inputs[0];
	}

	lastIndex = std::min( numChannels - 1, lastIndex);
	for( int i = lastIndex ; i >= 0 ; --i )
	{
		m_inputs[i] = p_inputs[i];
	}
}

void ug_vst_in::setInputSilent(int input, bool isSilent)
{
	auto p = GetPlug(input + 1);
	const auto newState = isSilent ? ST_STATIC : ST_RUN;

	if (p->getState() != newState)
	{
		OutputChange(SampleClock(), p, newState);
	}
}
