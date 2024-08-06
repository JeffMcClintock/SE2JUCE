#include "pch.h"
#include "WAVE.h"
#include "module_register.h"
#include "ug_wave_recorder.h"
#include "modules/shared/wav_file.h"
#include "SeAudioMaster.h"

// copied from header mmreg.h
#define WAVE_FORMAT_PCM     1

#define PN_RIGHT_IN	1
#define PN_FILE		2
namespace
{
REGISTER_MODULE_1(L"Wave Recorder", IDS_MN_WAVE_RECORDER,IDS_MG_INPUT_OUTPUT,ug_wave_recorder,CF_STRUCTURE_VIEW,L"Sends it's input to a file.  You can record several 'tracks' of audio to your hard disk at once.");
}
SE_DECLARE_INIT_STATIC_FILE(ug_wave_recorder)

void ug_wave_recorder::ListInterface2(InterfaceObjectArray& PList)
{
	// IO Var, Direction, Datatype, Name, Default, defid (index into unit_gen::PlugFormats)
	// defid used to name a enum list or range of values
	LIST_PIN2(L"Left (Mono)",input[0], DR_IN,L"",L"", IO_LINEAR_INPUT,L"");
	LIST_PIN2( L"Right",input[1],  DR_IN, L"", L"", IO_LINEAR_INPUT,L"");
	LIST_VAR3( L"File Name",FileName, DR_IN,DT_TEXT, L"Wavefile", L"wav", IO_FILENAME, L"Enter the .WAV filename.  Check the menu 'Audio - Preferences' for the default directory.");
	LIST_VAR3( L"Format", m_format, DR_IN, DT_ENUM   , L"0", L"16 bit, 32 bit float", IO_MINIMISED, L"Sets the file format");
	LIST_VAR3( L"Time Limit", total_time, DR_IN, DT_DOUBLE, L"4", L"", IO_MINIMISED, L"Sets length of WAV file, in seconds");
	LIST_VAR3( L"Play Wavefile", play_file, DR_IN, DT_BOOL, L"0", L"",IO_MINIMISED|IO_HIDE_PIN, L"Plays sound once it is written to file");
	LIST_VAR3( L"Report Stats", debug, DR_IN, DT_BOOL, L"0", L"", IO_MINIMISED, L"After writing to File, shows efficiency statistics");
}

ug_wave_recorder::ug_wave_recorder()
{
	SetFlag(UGF_POLYPHONIC_AGREGATOR);
}

ug_wave_recorder::~ug_wave_recorder()
{
	if (fileHandle)
	{
		//f1.Close();
		fclose(fileHandle);
	}
}
int ug_wave_recorder::Open()
{
	//	_RPT0(_CRT_WARN, "ug_wave_out::Open()\n" );
	ug_base::Open();

	// mono or stereo output?
	if (plugs[PN_RIGHT_IN]->HasNonDefaultConnection())
		n_channels = 2;
	else
		n_channels = 1;

	return 0;
}

int ug_wave_recorder::Close()
{
	//	_RPT0(_CRT_WARN, "ug_wave_recorder::Close()\n" );
	CloseFile();
#if 0
	// Calculate program's efficiency
	if ((SeAudioMaster::profileBlockSize || debug))
	{
		__int64 lpFrequency;
		QueryPerformanceFrequency((LARGE_INTEGER*)&lpFrequency);
		__int64 end_time;
		QueryPerformanceCounter((LARGE_INTEGER*)&end_time);
		__int64 elapsed = end_time - start_time;
		double sample_generate_time = (double)elapsed / lpFrequency;
		double sample_play_time = SampleClock() / getSampleRate();
		double efficiency = 100 * sample_play_time / sample_generate_time;

		if (SeAudioMaster::profileBlockSize)
		{
			std::ofstream outfile;
			const char* filename = "C:\\temp\\profile.txt";

			static bool isFirst = true;
			if (isFirst)
			{
				outfile.open(filename);
				outfile << "Block Size, Efficiency %\n";
				isFirst = false;
			}
			else
			{
				outfile.open(filename, std::ios::app); // append instead of overwrite
			}
			outfile << AudioMaster()->BlockSize() << ", " << efficiency << "\n";
		}
		else
		{
			if (BundleInfo::instance()->isEditor)
			{
				std::wostringstream oss;
				oss << sample_play_time << L" seconds of sound rendered in " << sample_generate_time << "s.  Efficiency = " << efficiency << "\n";
				message(oss.str());
				Sleep(250); // else terminates before que serviced.
			}
		}
	}
#endif

	return ug_base::Close();
}

void ug_wave_recorder::flush_buffer()
{
	//	_RPT0(_CRT_WARN, "ug_wave_recorder::flush_buffer()\n" );
	//	_RPT2(_CRT_WARN, "FB h= %x t=%x\n", f1.m_hFile, AfxGetThread() );
	auto buf_count = (intptr_t)wo_pointer - (intptr_t)buffer;
	assert(buf_count <= BUF_SIZE);
	assert(buf_count >= 0);

	if (buf_count > 0)
	{
		try
		{
			//f1.Write(buffer, static_cast<UINT>(buf_count) );
			fwrite(buffer, 1, buf_count, fileHandle);
		}
		catch (...)
		{
			message(L"Error writing file");
			AudioMaster2()->end_run();
		}
	}

	sample_count += (8 * buf_count) / (bits_per_sample * n_channels);
	wo_pointer = buffer;
	assert(wo_pointer == &buffer[0]);
	RUN_AT(SampleClock() + (8 * BUF_SIZE) / (bits_per_sample * n_channels), &ug_wave_recorder::flush_buffer);
}

void ug_wave_recorder::onSetPin(timestamp_t /*p_clock*/, UPlug* p_to_plug, state_type /*p_state*/)
{
	int plug_number = p_to_plug->getPlugIndex();

	// WARN IF USER ATTEMPTS TO CHANGE FILENAME/MODE
	if (SampleClock() > 5)
	{
		if (plug_number == PN_FILE)
		{
			message(L"For change to take effect, Restart sound");
		}
	}
	else
	{
		if (plug_number == PN_FILE)
		{
			wo_pointer = buffer;

			// allow render to 'null device' for benchmarking purposes
			if (FileName.empty() || open_file() != 0)
			{
				SET_CUR_FUNC(&ug_base::process_nothing);
			}
			else
			{
				RUN_AT(SampleClock() + (8 * BUF_SIZE) / (bits_per_sample * n_channels), &ug_wave_recorder::flush_buffer);
			}
		}
	}
}

int ug_wave_recorder::open_file()
{
	std::wstring l_filename = AudioMaster()->getShell()->ResolveFilename(FileName, L"wav");
	fileHandle = fopen(WStringToUtf8(l_filename).c_str(), "wb");

	if (!fileHandle)
	{
		message(L"File open error : " + l_filename);

		if (AudioMaster2())
		{
			AudioMaster2()->end_run();
		}

		return 1;
	}

	// Write empty wave file header
	wave_file_header wav_head;

	try
	{
		//f1.Write( &wav_head, 44 );
		fwrite(&wav_head, 1, 44, fileHandle);
	}
	catch (...)
	{
		message(L"Error writing file");
		AudioMaster2()->end_run();
		return 1;
	}

#ifdef _WIN32
	QueryPerformanceCounter((LARGE_INTEGER*)&start_time);  // added here to improving CPU accuracy (first module was always way out).
#endif

	current_format = m_format; // note it's initial value (user could change it)

	if (current_format == 0) // 16 bit int
	{
		bits_per_sample = 16;

		if (n_channels == 1)
		{
			SET_CUR_FUNC(&ug_wave_recorder::sub_process_to_file_mono);
		}
		else
		{
			SET_CUR_FUNC(&ug_wave_recorder::sub_process_to_file_stereo);
		}
	}
	else				// 32 bit float
	{
		bits_per_sample = 32;

		if (n_channels == 1)
		{
			SET_CUR_FUNC(&ug_wave_recorder::sub_process_to_file_float_mono);
		}
		else
		{
			SET_CUR_FUNC(&ug_wave_recorder::sub_process_to_file_float_stereo);
		}
	}

	float seconds = (float)total_time;
	timestamp_t total_samples;

	total_samples = (timestamp_t)(total_time * getSampleRate());

	// Time of zero indicates 'unlimited'.
	if (total_samples <= 0)
	{
		total_samples = SE_TIMESTAMP_MAX;
		seconds = (float)total_samples / getSampleRate();
	}

	RUN_AT(SampleClock() + total_samples - 1, &ug_wave_recorder::TimeUp);
	//	sample_count = total_samples;
	sample_count = 0;

	return 0;
}

void ug_wave_recorder::CloseFile()
{
	if (!fileHandle)	// check file is open
		return;

	flush_buffer();	// Write any remaining data to file
	//f1.SeekToBegin();	// Rewind buffer to file header
	fseek(fileHandle, 0, SEEK_SET);

	// Write file header
	wave_file_header wav_head;
	memcpy(wav_head.chnk1_name, "RIFF", 4);
	memcpy(wav_head.chnk2_name, "WAVE", 4);
	memcpy(wav_head.chnk3_name, "fmt ", 4);
	memcpy(wav_head.chnk4_name, "data", 4);

	if (current_format == 0)
	{
		wav_head.wFormatTag = WAVE_FORMAT_PCM;
	}
	else
	{
		wav_head.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
	}

	wav_head.wBitsPerSample = static_cast<SHORT>(bits_per_sample);
	wav_head.chnk3_size = 16;
	wav_head.nChannels = static_cast<SHORT>(n_channels);
	wav_head.chnk4_size = (LONG)(sample_count * wav_head.wBitsPerSample / 8 * wav_head.nChannels);
	wav_head.chnk1_size = wav_head.chnk4_size + 36;
	wav_head.nSamplesPerSec = (int)getSampleRate();
	wav_head.nAvgBytesPerSec = wav_head.nSamplesPerSec * wav_head.nChannels * wav_head.wBitsPerSample / 8;
	wav_head.nBlockAlign = wav_head.wBitsPerSample / 8 * wav_head.nChannels;

	try
	{
		//f1.Write(&wav_head, 44);
		fwrite(&wav_head, 1, 44, fileHandle);
	}
	catch (...)
	{
		message(L"Error writing file header");
		AudioMaster2()->end_run();
	}

	//	std::wstring filename = f1.GetFilePath();
	//	_RPT2(_CRT_WARN, "ug_wave_recorder::Closing %x thread %x\n", f1.m_hFile, AfxGetThread() );
//	f1.Close();	// Close the file
	fclose(fileHandle);

#if 0 // avoid linking winmm
	//	_RPT0(_CRT_WARN, "ug_wave_recorder::Close File***\n" );
	if (play_file)	// play wavefile
	{
		const std::wstring l_filename = AudioMaster()->getShell()->ResolveFilename(FileName, L"wav");
		sndPlaySound(l_filename.c_str(), SND_ASYNC);
	}
#endif
}

void ug_wave_recorder::TimeUp() // Preset time limit is up
{
	if (AudioMaster2()) // avoid crash in oversampler.
	{
		AudioMaster2()->end_run(); // only works in Editor
	}

	CloseFile();
	SET_CUR_FUNC(&ug_base::process_nothing);
}

void ug_wave_recorder::sub_process_to_file_float_stereo(int start_pos, int sampleframes)
{
	float* p_left = input[0] + start_pos;
	float* p_right = input[1] + start_pos;
	float* write_pos = (float*)wo_pointer;

	for (int s = sampleframes; s > 0; s--)
	{
		*write_pos++ = *p_left++;
		*write_pos++ = *p_right++;
	}

	wo_pointer = write_pos;
}

void ug_wave_recorder::sub_process_to_file_float_mono(int start_pos, int sampleframes)
{
	float* p_left = input[0] + start_pos;
	float* write_pos = (float*)wo_pointer;

	for (int s = sampleframes; s > 0; s--)
	{
		*write_pos++ = *p_left++;
	}

	wo_pointer = write_pos;
}

// 16 bit samples.
void ug_wave_recorder::sub_process_to_file_mono(int start_pos, int sampleframes)
{
	assert(buffer + BUF_SIZE > wo_pointer);
	assert(buffer <= wo_pointer);
	assert(sampleframes > 0);

	float* p_left = input[0] + start_pos;
	short* write_pos = (short*)wo_pointer;
	const float scale_factor = 0x7fff;

	for (int s = sampleframes; s > 0; s--)
	{
		//		*write_pos++ = (short) (*p_left++ * scale_factor); // convert each sample from float to short (16 bit)

		float sample = scale_factor * (std::min)(1.0f, (std::max)(-1.0f, *p_left++));
		*write_pos++ = (short)FastRealToIntTruncateTowardZero(sample); // fast float-to-int using SSE. truncation toward zero.
	}
	wo_pointer = write_pos;
}

void ug_wave_recorder::sub_process_to_file_stereo(int start_pos, int sampleframes)
{
	float* p_left = input[0] + start_pos;
	float* p_right = input[1] + start_pos;

	short* write_pos = (short*)wo_pointer;
	const float scale_factor = 0x7fff;
	for (int s = sampleframes; s > 0; s--)
	{
		//short samp = (short) (*p_left++ * scale_factor); // convert each sample from float to short (16 bit)
		//*write_pos++ = samp;
		float sample = scale_factor * (std::min)(1.0f, (std::max)(-1.0f, *p_left++));
		*write_pos++ = (short)FastRealToIntTruncateTowardZero(sample); // fast float-to-int using SSE. truncation toward zero.

		//samp = (short)(*p_right++ * scale_factor);
		//*write_pos++ = samp;
		sample = scale_factor * (std::min)(1.0f, (std::max)(-1.0f, *p_right++));
		*write_pos++ = (short)FastRealToIntTruncateTowardZero(sample); // fast float-to-int using SSE. truncation toward zero.
	}
	wo_pointer = write_pos;
}
