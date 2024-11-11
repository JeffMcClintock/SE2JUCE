#include "pch.h"
#include "UgDebugInfo.h"
#include "EventProcessor.h"
#include "my_msg_que_output_stream.h"
#include "SeAudioMaster.h"
#include "ug_base.h"
#include "USampBlock.h"
#include "./iseshelldsp.h"


/*
rdtsc is much faster, but only avail on Pentiums and above. QusryPerformanceCounter is avail on all systems
but users slower, less accurate timers when rdtsc not available
#define USE_RDTSC

rdtsc is very complex to use on CPUs that throttle CPU speed to save power
*/

#define DEBUG_BUFFER_SIZE 100000

UgDebugInfo::UgDebugInfo(class EventProcessor* module) :
	m_module( dynamic_cast<ug_base*>( module ) )
		,cpuMeasuedCycles(0.0f)
{
	//m_module->cpuMeasuedCycles = 0.0f;
	//m_module->cpuPeakCycles = 0.0f;

	if( m_module->CloneOf() == m_module ) // voice zero only
	{
		m_module->AudioMaster()->RegisterModuleDebugger(this);
	}

	bufferEndValue.resize( m_module->plugs.size() );
}

void UgDebugInfo::Process( int buffer_offset, int blocksize )
{
	// buffer bounds check.
	VerfyBlocksStart( buffer_offset, blocksize );
	m_module->DoProcess( buffer_offset, blocksize );
	// buffer check.
	VerfyBlocksEnd( buffer_offset, blocksize );
}

void UgDebugInfo::VerfyBlocksStart( int buffer_offset, int blocksize )
{
#if 0 // !!! TODO !!!
	ug_base* ug = m_module;
	bool block_ok = true;
	std::wstring msg;
	int blocks = ug->plugs.size();

	for( int p = 0 ; p < blocks ; p++ )
	{
		UPlug* plg = ug->plugs[p];

		if( plg->DataType == DT_FSAMPLE)
		{
			USampBlock* block_ptr = plg->GetSampleBlock();

			if( block_ptr ) // IO Mod's Uparameters don't use sample block pointers
			{
				int* buf = (int*)block_ptr->GetBlock();
				bufferEndValue[p] = buf[buffer_offset+blocksize];
				/*
				dbg_copy_output_array[p]->Copy( block_ptr );

				if( false == plg->GetSampleBlock()->CheckOverwrite() )
				{
					block_ok = false;
					msg = _T("pre-check: buffer corrupted (off end)");
				}
				*/
			}
		}
	}

	if( block_ok == false && (ug->flags & UGF_OVERWRITING_BUFFERS) == 0)
	{
		ug->SetFlag(UGF_OVERWRITING_BUFFERS);
		ug->message(msg);
		/*
				_RPT2(_CRT_WARN, "\n******** ", start_pos, start_pos + sampleframes );
				ug->DebugPrintName();
				_RPT0(_CRT_WARN, " corrupting buffers ****** !!!\n" );
		*/
	}
#endif
}

void UgDebugInfo::VerfyBlocksEnd( int buffer_offset, int blocksize )
{
#if 0 // !!! TODO !!!
	ug_base* ug = m_module;
	std::wstring msg;
	bool block_ok = true;
	int end_pos = ug->AudioMaster()->BlockSize();
	int blocks = ug->plugs.size();

	for( int p = 0 ; p < blocks ; p++ )
	{
		UPlug* plg = ug->plugs[p];

		if( plg->DataType == DT_FSAMPLE)
		{
			if( plg->Direction == DR_OUT )
			{
				USampBlock* block_ptr = plg->GetSampleBlock();

				if( block_ptr ) // IO Mod's Uparameters don't use sample block pointers
				{
					/*
					if( false == dbg_copy_output_array[p]->Compare( plg->GetSampleBlock(), 0, start_pos ) )
					{
						block_ok = false;
						msg.Format(_T("Corrupting out buffers (pre-safe zone). Samples %d -> %d"),  0, start_pos );
					}
					if( false == dbg_copy_output_array[p]->Compare( plg->GetSampleBlock(), start_pos + sampleframes, end_pos ) )
					{
						block_ok = false;
						msg.Format(_T("Corrupting out buffers(after safe zone). Samples %d -> %d"),  start_pos + sampleframes, end_pos );
					}
					*/
					/*
					int* buf = (int*) block_ptr->GetBlock();

					if( bufferEndValue[p] != buf[buffer_offset+blocksize] )
					{
						block_ok = false;
						msg = _T("Writ ting past end of output buffers.\nThis problem can cause your project to crash.  Please contact the developer of this module to help improve it.\n\nLocation: ");
						msg = msg + ug->GetFullPath();
					}
					*/
					// Samples completely off end of buffer. Should never be written, with small leniency fo SSE modules.
					int overwrittenSampleCount = plg->GetSampleBlock()->CheckOverwrite();
					if( overwrittenSampleCount > 0 )
					{
						// Modules using SSE are allowed to overwrite up to 4 samples off end of buffer.
						// This simplifies the SSE logic. SE reserves extra samples for this purpose.
						if( overwrittenSampleCount > 3 || (ug->flags & UGF_SSE_OVERWRITES_BUFFER_END) == 0 )
						{
							block_ok = false;
							msg = _T("Writting past end of output buffers.\nThis problem can cause your project to crash.  Please contact the developer of this module to help improve it.\n\nLocation: ");
							msg = msg + ug->GetFullPath();
						}
					}
				}
			}
			else
			{
				/*
				//				if( ( plg->flags & PF_ADDER) == 0 )
				{
					USampBlock *sb = plg->GetSampleBlock();
					if( sb != 0 ) // IO mod inputs don't have block
					{
						if( false == dbg_copy_output_array[p]->Compare( sb, 0, end_pos ) )
						{
							block_ok = false;
							msg = _T("Corrupting in buffers. See debug Window..");
						}

						if( false == plg->GetSampleBlock()->CheckOverwrite() )
						{
							block_ok = false;
							msg = _T("Writ ting past end of INPUT buffers");
						}
					}
				}
					*/
			}
		}
	}

	if( block_ok == false && (ug->flags & UGF_OVERWRITING_BUFFERS) == 0)
	{
		ug->SetFlag(UGF_OVERWRITING_BUFFERS);
		ug->message(msg);
		ug->DebugIdentify();
		_RPT0(_CRT_WARN, " corrupting buffers ****** !!!\n" );
	}
#endif
}

void UgDebugInfo::CpuToGui()
{
	_RPTN(0, "     CpuToGui %f\n", cpuMeasuedCycles);

	const float cpu = cpuMeasuedCycles * ug_base::cpu_conversion_const;
	const float peakCpu = cpuPeak * ug_base::cpu_conversion_const2;

	int voiceCount = 0;
	ug_base* clone = m_module;
	while( clone )
	{
		++voiceCount;
		clone = clone->m_next_clone;
	}

	voiceCount = (std::min)(voiceCount, 128); // most reciever can handle.

	my_msg_que_output_stream strm( m_module->AudioMaster()->getShell()->MessageQueToGui(), m_module->Handle(), "cpu" );
	strm << (int32_t) (sizeof(float) + sizeof(float) + sizeof(int) + voiceCount ); // message length.
	strm << cpu;
	strm << peakCpu;

	strm << voiceCount;

	clone = m_module;
	while( voiceCount-- > 0 )
	{
		unsigned char state;
		ug_base* oversampler = dynamic_cast<ug_base*>( clone->AudioMaster() );

		if( clone->IsSuspended() || (oversampler && oversampler->IsSuspended() ) )
		{
			state = 1;
		}
		else
		{
			if( clone->sleeping )
			{
				state = 2;
			}
			else
			{
				state = 3;
			}
		}

		strm << state;
		clone = clone->m_next_clone;
	}

	strm.Send();

	cpuMeasuedCycles = cpuPeak = 0.0f; // reset.
}
