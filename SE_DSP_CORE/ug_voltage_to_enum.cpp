#include "pch.h"
#include <algorithm>
#include <math.h>
#include "ug_voltage_to_enum.h"
#include "modules/se_sdk3/it_enum_list.h"
#include "module_register.h"
#include "tinyxml/tinyxml.h"

SE_DECLARE_INIT_STATIC_FILE(ug_voltage_to_enum)

using namespace std;

namespace
{
REGISTER_MODULE_1(L"Voltage To List", IDS_MN_VOLTAGE_TO_LIST,IDS_MG_OLD,ug_voltage_to_enum ,CF_STRUCTURE_VIEW,L"Converts a voltage to a List, allows any voltage source to control a list selection input, e.g Oscillator Waveform selection");
REGISTER_MODULE_1(L"SE Int To List", IDS_MN_INT_TO_LIST, IDS_MG_CONVERSION, ug_int_to_enum, CF_STRUCTURE_VIEW, L"Converts a voltage to a List, allows any voltage source to control a list selection input, e.g Oscillator Waveform selection");
REGISTER_MODULE_1(L"Voltage To List2", IDS_MN_VOLTAGE_TO_LIST2, IDS_MG_CONVERSION, ug_voltage_to_enum2, CF_STRUCTURE_VIEW, L"Converts a voltage to a List, allows any voltage source to control a list selection input, e.g Oscillator Waveform selection");
}

// define plug numbers as constants
#define PN_INPUT 0
#define PN_OUTPUT 1

// Fill an array of InterfaceObjects with plugs and parameters
IMPLEMENT_UG_INFO_FUNC2(ug_voltage_to_enum)
{
	// IO Var, Direction, Datatype, ug_voltage_to_enum, ppactive, Default,Range/enum list
	//    LIST_VAR3( L"Enum in",input, DR_IN,DT_ENUM, L"0", L"cat,dog,moose", 0, L"Choice of stuff");
	LIST_PIN2( L"Signal in",in_ptr,   DR_IN, L"0", L"", IO_LINEAR_INPUT, L"Input signal");
	LIST_VAR3( L"List Out", enum_out, DR_OUT,  DT_ENUM, L"0", L"", 0, L"");
}

ug_voltage_to_enum::ug_voltage_to_enum() :
	m_enum_vals(0)
	,enum_out(0)
{
	SET_CUR_FUNC( &ug_voltage_to_enum::sub_process );
}

ug_voltage_to_enum::~ug_voltage_to_enum()
{
	delete [] m_enum_vals;
}

void ug_voltage_to_enum::Setup( ISeAudioMaster* am, TiXmlElement* xml )
{
	ug_base::Setup( am, xml );
	TiXmlElement* plugsElement = xml->FirstChildElement("Plugs");

	if( plugsElement )
	{
		for( TiXmlElement* plugElement = plugsElement->FirstChildElement(); plugElement; plugElement=plugElement->NextSiblingElement())
		{
			assert( strcmp(plugElement->Value(), "Plug" ) == 0 );
			const char* d = plugElement->Attribute("EnumList");

			if( d )
			{
				m_enum_list = Utf8ToWstring( d );
			}
		}
	}
}

ug_base* ug_voltage_to_enum::Clone( CUGLookupList& UGLookupList )
{
	ug_voltage_to_enum* clone = dynamic_cast<ug_voltage_to_enum*>( ug_base::Clone(UGLookupList) );

	clone->m_enum_list = m_enum_list;

	return clone;
}

// This is called when the input changes state
// IF IT RECEIVES ONE_OFF CHANGE MESSAGE, REMEMBER THAT the input concerned
// won't receive the new sample untill the next sample clock. So don't try
// to access any inputs values
void ug_voltage_to_enum::onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state)
{
	int in = p_to_plug->getPlugIndex();

	if( in == PN_INPUT ) // input
	{
		if( p_state == ST_RUN )
		{
			SET_CUR_FUNC( &ug_voltage_to_enum::sub_process );
		}
		else
		{
			OnChange( p_clock, p_to_plug->getValue() );
			SET_CUR_FUNC( &ug_base::process_sleep );
		}
	}
}

int ug_voltage_to_enum::Open()
{
	ug_base::Open(); // call base class
	m_low = m_hi = 99999999.0f; // unlikely value to trigger output stat update.
	enum_out = SHRT_MIN; // unlikely value to trigger output stat update.

	int idx = 0;
	it_enum_list itr(m_enum_list);

	for( itr.First() ; !itr.IsDone() ; itr.Next() )
	{
		idx++;
	}

	m_out_val_count = (float) idx;// - 1;
	m_enum_vals = new short[idx];
	idx = 0;

	for( itr.First() ; !itr.IsDone() ; itr.Next() )
	{
		enum_entry* e = itr.CurrentItem();
		m_enum_vals[idx] = static_cast<short>(e->value);
		idx++;
	}

	return 0;
}

void ug_voltage_to_enum::sub_process(int start_pos, int sampleframes)
{
	float* in1 = in_ptr + start_pos;

	for( int s = sampleframes ; s > 0 ; --s )
	{
		float i = *in1++;

		if( i < m_low || i > m_hi )
		{
			OnChange(SampleClock()+sampleframes-s,i);
		}
	}
}

void ug_voltage_to_enum::OnChange(timestamp_t p_clock, float p_input)
{
	float output_val_f = floorf( 0.5f + p_input * (m_out_val_count-1.f) );

	int output_val = (int) output_val_f;

	// constrain to legal range
	output_val = min(output_val, ((int)m_out_val_count) - 1 );
	output_val = max(output_val, 0);

	if(output_val >= 0 && m_enum_vals[output_val] != enum_out )
	{
		enum_out = m_enum_vals[output_val];

//		_RPT3(_CRT_WARN, "ug_voltage_to_enum choice %.3f V, %d (%d)\n", p_input * 10.f, output_val, m_enum_vals[output_val] );
//		_RPT1(_CRT_WARN, "ug_voltage_to_enum::OnChange %d\n", p_clock );

		SendPinsCurrentValue( p_clock, GetPlug(PN_OUTPUT) );
		CalcLimits(output_val);
	}
}

void ug_voltage_to_enum::CalcLimits(int p_idx)
{
	float zoneCount = max(m_out_val_count, 1.0f);
	float zone_size = 1.f / zoneCount; //m_out_val_count;
	float hysteresis = zone_size / 3.f;
	m_low = zone_size * p_idx - hysteresis;
	m_hi = zone_size * (p_idx + 1.f) + hysteresis;

	if (p_idx == 0.f)
	{
		m_low = -99999999.f; // no lower limit
	}

	if (p_idx >= m_out_val_count - 1)
	{
		m_hi = 99999999.f; // no upper limit
	}

	//	_RPT3(_CRT_WARN, "CalcLimits lo %.2f, hi %.2f  +/-  %.2f\n", m_low + hysteresis, m_hi - hysteresis, hysteresis );
}

void ug_voltage_to_enum2::CalcLimits(int p_idx)
{
	m_low = -99999999.f; // no lower limit
	m_hi = 99999999.f; // no upper limit

	if (m_out_val_count < 2)
	{
		return;
	}

	const float zone_size = 1.f / (m_out_val_count-1);
	const float hysteresis = zone_size / 3.f;

	if( p_idx > 0 )
	{
		m_low = zone_size * (p_idx - 1.f) + hysteresis;
	}

	if( p_idx < m_out_val_count - 1 )
	{
		m_hi = zone_size * (p_idx + 1.f) - hysteresis;
	}

//	_RPT4(_CRT_WARN, "CalcLimits %d lo %.2f, hi %.2f  +/-  %.2f\n", p_idx, m_low * 10.f, m_hi * 10.f, hysteresis );
}


/////////////////////////////////////////////////////////////////////////////////////////////////////

// define plug numbers as constants
#define PN_INPUT 0
#define PN_OUTPUT 1

// Fill an array of InterfaceObjects with plugs and parameters
IMPLEMENT_UG_INFO_FUNC2(ug_int_to_enum)
{
	// IO Var, Direction, Datatype, ug_int_to_enum, ppactive, Default,Range/enum list
	LIST_VAR3(L"Signal in", m_in, DR_IN, DT_INT, L"0", L"", 0, L"");
	LIST_VAR3(L"List Out", enum_out, DR_OUT, DT_ENUM, L"0", L"", 0, L"");
}

ug_int_to_enum::ug_int_to_enum() :
	m_enum_vals(0)
	, enum_out(0)
{
//	SET_CUR_FUNC(&ug_int_to_enum::sub_process);
}

ug_int_to_enum::~ug_int_to_enum()
{
	delete[] m_enum_vals;
}

void ug_int_to_enum::Setup(ISeAudioMaster* am, TiXmlElement* xml)
{
	ug_base::Setup(am, xml);
	TiXmlElement* plugsElement = xml->FirstChildElement("Plugs");

	if (plugsElement)
	{
		for (TiXmlElement* plugElement = plugsElement->FirstChildElement(); plugElement; plugElement = plugElement->NextSiblingElement())
		{
			assert(strcmp(plugElement->Value(), "Plug") == 0);
			const char* d = plugElement->Attribute("EnumList");

			if (d)
			{
				m_enum_list = Utf8ToWstring(d);
			}
		}
	}
	setupWasCalled = true;
}

ug_base* ug_int_to_enum::Clone(CUGLookupList& UGLookupList)
{
	ug_int_to_enum* clone = dynamic_cast<ug_int_to_enum*>(ug_base::Clone(UGLookupList));

	clone->m_enum_list = m_enum_list;

	return clone;
}

// This is called when the input changes state
// IF IT RECEIVES ONE_OFF CHANGE MESSAGE, REMEMBER THAT the input concerned
// won't receive the new sample untill the next sample clock. So don't try
// to access any inputs values
void ug_int_to_enum::onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state)
{
	int in = p_to_plug->getPlugIndex();

	if (in == PN_INPUT) // input
	{
		//if (p_state == ST_RUN)
		//{
		//	SET_CUR_FUNC(&ug_int_to_enum::sub_process);
		//}
		//else
		//{
			OnChange(p_clock, m_in);// p_to_plug->getValue());
			SET_CUR_FUNC(&ug_base::process_sleep);
		//}
	}
}

int ug_int_to_enum::Open()
{
	ug_base::Open(); // call base class
	enum_out = SHRT_MIN; // unlikely value to trigger output stat update.

	int idx = 0;
	it_enum_list itr(m_enum_list);

	for (itr.First(); !itr.IsDone(); itr.Next())
	{
		idx++;
	}

	m_out_val_count = idx;
	m_enum_vals = new short[(std::max)(1, m_out_val_count)];
	m_enum_vals[0] = 0;

	idx = 0;
	for (itr.First(); !itr.IsDone(); itr.Next())
	{
		enum_entry* e = itr.CurrentItem();
		m_enum_vals[idx] = static_cast<short>(e->value);
		idx++;
	}

	return 0;
}

void ug_int_to_enum::OnChange(timestamp_t p_clock, int32_t p_input)
{
	assert(setupWasCalled); // can't create without enum list

	int32_t output_val = p_input;// (int)output_val_f;

	// constrain to legal range
	output_val = min(output_val, ((int32_t)m_out_val_count) - 1);
	output_val = max(output_val, 0);

	if (output_val >= 0 && m_enum_vals[output_val] != enum_out)
	{
		enum_out = m_enum_vals[output_val];

		//		_RPT3(_CRT_WARN, "ug_int_to_enum choice %.3f V, %d (%d)\n", p_input * 10.f, output_val, m_enum_vals[output_val] );
		//		_RPT1(_CRT_WARN, "ug_int_to_enum::OnChange %d\n", p_clock );

		SendPinsCurrentValue(p_clock, GetPlug(PN_OUTPUT));
//		CalcLimits(output_val_f);
	}
}

