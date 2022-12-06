#include "pch.h"

#ifdef _WIN32
#include <direct.h>
#endif

#include "ug_system_command.h"
#include "Logic.h"
#include "module_register.h"

// Note: Windows 10S does not permit CreateProcessW(). Moved this to "Old" modules.
namespace
{
REGISTER_MODULE_1(L"OS Command", IDS_MN_OS_COMMAND,IDS_MG_OLD,ug_system_command,CF_STRUCTURE_VIEW,L"Executes a DOS command when triggered. Warning: Triggering this at a high frequency could overload your Operating System.");
}

// Fill an array of InterfaceObjects with plugs and parameters
void ug_system_command::ListInterface2(InterfaceObjectArray& PList)
{
	LIST_PIN2( L"Trigger in",in1_ptr,  DR_IN, L"0", L"", IO_POLYPHONIC_ACTIVE, L"Whenever this input exceeds 3.33 Volts, the DOS command is executed.");
	LIST_VAR3( L"Command",m_command, DR_IN,DT_TEXT, L"", L"", 0, L"The DOS command to execute.");
}

// invert input, unless indeterminate voltage
void ug_system_command::sub_process(int start_pos, int sampleframes)
{
	float* in = in1_ptr + start_pos;
	timestamp_t count = sampleframes;
	float* last = in + sampleframes;
	timestamp_t end_clock = SampleClock() + sampleframes;

	while( count > 0 )
	{
		// duration till next in change?
		if( cur_state )
		{
			while( *in >= LOGIC_LO && in < last )
			{
				in++;
			}
		}
		else
		{
			while( *in <= LOGIC_HI && in < last )
			{
				in++;
			}
		}

		count = last - in;

		if( count > 0 )
		{
			SetSampleClock(end_clock - count);
			input_changed( );
		}
	}

	if( in_state < ST_RUN )
	{
		SleepIfOutputStatic(sampleframes);
	}
}

void ug_system_command::onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state)
{
	Wake();
	// any input running?
	in_state = p_state;

	if( in_state == ST_RUN )
	{
		SET_CUR_FUNC( &ug_system_command::sub_process );
		ResetStaticOutput();
	}
	else
	{
		input_changed();
	}
}

ug_system_command::ug_system_command() :
	cur_state(false)
	,in_state(ST_STOP)
{
	// handled by Open()	SET_CUR_FUNC( sub_process );
}

void ug_system_command::input_changed()
{
	if( check_logic_input( GetPlug(0)->getValue(), cur_state ) )
	{
		if( cur_state )
		{
            
#ifdef _WIN32
			char buffer[_MAX_PATH];   // Get the current working directory
			_getcwd( buffer, _MAX_PATH );

// TODO: DSP-only method
			// waits for command to complete. _tsystem( m_command );
			// not available on 98. shellexecute()
			/*
						// split command up into command and parameters
						const int MAX_ARGS = 20;
						wchar_t *args[MAX_ARGS];
						args[0] = 0;
						std::wstring token;
						int curPos= 0;

						token= m_command.Tokenize(" ",curPos);
						int i = 0;
						while (token != "")
						{
							args[i] = new wchar_t[token.size() + 1];
							memcpy(args[i], token.GetBuffer(), token.size() * sizeof(wchar_t) );
							args[i][token.size()] = 0;

							// next
							token = m_command.Tokenize(" ",curPos);
							i++;
						};

						wchar_t *command = args[0];
						_tspawnv(_P_NOWAIT, command, args );
						std::wstring app_name;
			*/
			STARTUPINFOW sui;
			memset( &sui, 0, sizeof(sui) );
			PROCESS_INFORMATION pi;
			CreateProcessW(
			    NULL,
			    (LPWSTR) m_command.c_str(),
			    NULL,
			    NULL,
			    FALSE,
			    0,
			    0,0,&sui,&pi);
			//close handles opened by CreateProcess
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			// restore original folder
			_chdir( buffer );
#else
            // TODO!! Mac code here.
#endif

		}

		SET_CUR_FUNC( &ug_system_command::sub_process );
		ResetStaticOutput();
	}
}