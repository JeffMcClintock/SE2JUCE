#include "pch.h"
#include "ug_wave_player.h"
#include "module_register.h"
#include "iseshelldsp.h"
#include "BundleInfo.h"
#include "conversion.h"

// how many samples used during interpolation (4 each side)
#define GUARD_ZONE 8

#define PN_GATE 0
#define PN_PITCH 1
#define PN_OUT1 2
#define PN_OUT2 3
#define PN_FILE 4
#define PN_MODE 5

namespace
{
REGISTER_MODULE_1(L"Wave Player", IDS_MN_WAVE_PLAYER,IDS_MG_INPUT_OUTPUT,ug_wave_player,CF_STRUCTURE_VIEW,L"Inputs sound from a WAVE file.  You can use several at once.  Wave files are 'streamed' there is no practical limit to file size.  Can be triggered from a 'MIDI to CV' or 'Drum Trigger' module to play wave files under MIDI control.  Wave file looping information is supported.  For more sophisticated sample playback, use the 'Soundfont Player' module.");
}
SE_DECLARE_INIT_STATIC_FILE(ug_wave_player)

bool ug_wave_player::m_user_inhibit_error_message = false;

ug_wave_player::ug_wave_player()
{
	buffer_pointer[2] = 0; // end marker
}

void ug_wave_player::ListInterface2(InterfaceObjectArray& PList)
{
	// IO Var, Direction, Datatype, Wave_in, ppactive, Default,Range/enum list
	LIST_PIN2( L"Gate", gate_ptr, DR_IN, L"10", L"", IO_POLYPHONIC_ACTIVE, L"Triggers wave file playback");
	LIST_PIN2( L"Pitch Shift", pitch_ptr, DR_IN, L"5", L"100, 0, 10, 0", IO_POLYPHONIC_ACTIVE, L"1 Volt per Octave, 5V = Original Pitch");
	LIST_PIN2( L"Left Out", output[0], DR_OUT , L"", L"", 0, L"");
	LIST_PIN2( L"Right Out",output[1],  DR_OUT, L"", L"", 0, L"");
	LIST_VAR3( L"File Name",FileName, DR_IN,DT_TEXT, L"Wavefile", L"wav", IO_FILENAME, L"Enter the .WAV filename.  Check the menu 'Audio - Preferences' for the default directory.");
}

int ug_wave_player::Open()
{
	interpolation_table2 = GetInterpolationtable();
	OutputChange(SampleClock(), GetPlug(PN_OUT1), ST_STATIC);
	OutputChange(SampleClock(), GetPlug(PN_OUT2), ST_STATIC);

	return ug_base::Open();
}

void ug_wave_player::onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state)
{
	const int plug_number = p_to_plug->getPlugIndex();

	if (plug_number == PN_FILE /*pointless 4 clones. SampleClock() > 5 &&*/)
	{
		if (/*p_file_mode &&*/ m_currently_loaded_file != FileName) // 2nd bit fix for qurik in Patch Mem Text resending value on patch change, even when ignore PC=true
		{
			m_inhibit_error_msg = false; // reset error message (only main clone actually counts)
			ResetStaticOutput();

			if (open_file()) // fails
			{
				SET_CUR_FUNC(&ug_wave_player::sub_process_file_silence); // prevent crash
				OutputChange(p_clock, GetPlug(PN_OUT1), ST_STOP);
				OutputChange(p_clock, GetPlug(PN_OUT2), ST_STOP);
				return;
			}

			play_flag = false;
			SET_CUR_FUNC(&ug_wave_player::sub_process_file);
			current_process_func = static_cast <process_func_ptr> (&ug_wave_player::sub_process_file_silence);
		}
	}
	if (plug_number == PN_GATE)
	{
		// note: if gate signal stops, we still need to check it at least once,
		// hence following...
		plug_status_gate = (std::max)(p_state, ST_ONE_OFF);

//		if (p_file_mode)
		{
			// check file loaded ok
			if (cursor)
			{
				SET_CUR_FUNC(&ug_wave_player::sub_process_file); // wake up
			}
			else
			{
				SET_CUR_FUNC(&ug_wave_player::sub_process_file_silence); // prevent crash
				OutputChange(p_clock, GetPlug(PN_OUT1), ST_STOP);
				OutputChange(p_clock, GetPlug(PN_OUT2), ST_STOP);
			}
		}
	}

	if (plug_number == PN_PITCH)
	{
		plug_status_pitch = p_state;

		if (cursor)
		{
			increment = calc_increment(GetPlug(PN_PITCH)->getValue());
		}
	}
}

int ug_wave_player::calc_increment(float pitch_shift) // Re calc increment for a new freq
{
	float octaves = (pitch_shift - 0.5f) * 10.f;
	//	float transposition = semitones / 12.f;
	float ratio = powf(2.f, octaves) / getSampleRate();
	//	float temp_inc = 0x1000000 * ratio;
	//	return (int) temp_inc * waveheader->nSamplesPerSec;
	float temp_inc = 0x1000000 * ratio * (float)cursor->SampleRate();
	int inc = (int)temp_inc;

	if (inc >= 0) // check for overflow in extreme cases
		return inc;

	return INT_MAX; // 0x100000000; // 256 times normal speed
}

void ug_wave_player::sub_process_file(int start_pos, int sampleframes)
{
	// need to monitor 'gate' signal?
	if (plug_status_gate == ST_STOP)
	{
		(this->*(current_process_func))(start_pos, sampleframes);
		return;
	}

	float* gate = gate_ptr + start_pos;
	int sample_count = sampleframes;
	int l_start_pos = start_pos;
	timestamp_t l_sample_clock = SampleClock();

	while (sample_count > 0)
	{
		// monitor incoming gate
		if (gate_state_hi)
		{
			while (*gate > 0.f && sample_count > 0)
			{
				gate++;
				sample_count--;
			}
		}
		else
		{
			while (*gate <= 0.f && sample_count > 0)
			{
				gate++;
				sample_count--;
			}
		}

		// call appropriate function
		int l_sampleframes = sampleframes - sample_count - (l_start_pos - start_pos);

		if (l_sampleframes > 0)
		{
			(this->*(current_process_func))(l_start_pos, l_sampleframes);
			l_start_pos += l_sampleframes;
			l_sample_clock += l_sampleframes;
			SetSampleClock(l_sample_clock); // allow sub_process to set out stat at correct timestamp
		}

		if (sample_count > 0) // indicates gate flipped
		{
			gate_state_hi = !gate_state_hi;

			if (gate_state_hi)
			{
				retrigger_sample(AudioMaster()->BlockStartClock() + l_start_pos);
			}
		}
	}

	// need to monitor 'gate' signal anymore?
	if (plug_status_gate == ST_ONE_OFF)
	{
		plug_status_gate = ST_STOP;
	}
}

// file played, now output silence
void ug_wave_player::sub_process_file_silence(int start_pos, int sampleframes)
{
	if (static_output_count >= 0)
	{
		float* out1 = output[0] + start_pos;
		float* out2 = output[1] + start_pos;

		for (int s = sampleframes; s > 0; s--)
		{
			*out1++ = 0.f;
			*out2++ = 0.f;
		}
	}

	if (plug_status_gate == ST_STOP)
	{
		SleepIfOutputStatic(sampleframes);
	}
	else // just track consecutive zeros, can relax once output buffer all zero
	{
		static_output_count -= sampleframes;
	}
}

void ug_wave_player::sub_process_file_mono(int start_pos, int sampleframes)
{
	float* out1 = output[0] + start_pos;
	float* out2 = output[1] + start_pos;
	float* pitch_shift = pitch_ptr + start_pos;
	const float* gate = gate_ptr + start_pos;

	int loop_counter = sampleframes;
	int l_increment = increment;
	bool pitch_changing = plug_status_pitch == ST_RUN;

	while (loop_counter-- > 0)
	{
		// increment pointer
		if (pitch_changing)
		{
			l_increment = calc_increment(*pitch_shift++);
		}

		if (s_ptr_fine > 0xffffff)
		{
			s_ptr += s_ptr_fine >> 24;
			s_ptr_fine &= 0xffffff;
		}

		if (s_ptr >= s_end)
		{
			if (refill_buffer(*gate > 0.0f)) // no samples remain
			{
				loop_counter++; // compensate for initial decrement
				timestamp_t l_sample_clock = SampleClock() + sampleframes - loop_counter;
				OutputChange(l_sample_clock, GetPlug(PN_OUT1), ST_STOP);
				OutputChange(l_sample_clock, GetPlug(PN_OUT2), ST_STOP);
				static_output_count -= loop_counter;

				while (loop_counter-- > 0)
				{
					*out1++ = *out2++ = 0.f;
				}

				return;
			}
		}

		// perform interpolation
		//float test_frac = s_ptr_fine / (float) 0x1000000;
		//float test_frac2 = fract_part / 32.f;
		//_RPT1(_CRT_WARN, "Interpolating %.2f between two samples\n", test_frac );
		// set up pointers to sample and table
		const float* ptr = s_ptr - 3;
		// reduce fraction part to range 0-31
		// fract_part = s_ptr_fine >> 19
		const float* interpolation_table_ptr = interpolation_table2 + (s_ptr_fine >> 19) * 8;
		// now look up interpolation filter table
		// perform 2 filters at once, one 1 sub sample ahead
		float a = 0.f;

		for (int x = 4; x > 0; x--)
		{
			a += *ptr++ * *interpolation_table_ptr++;
			a += *ptr++ * *interpolation_table_ptr++; //unrolled
		}

		float a2 = 0.f;
		ptr = s_ptr - 3;

		for (int x = 4; x > 0; x--)
		{
			a2 += *ptr++ * *interpolation_table_ptr++;
			a2 += *ptr++ * *interpolation_table_ptr++;
		}

		// now linear interpolation between 2 subsamples
		constexpr float scale = (1.f / 0x100000);
		// save lowest 19 bits for interpolation
		int fract_part_fine = s_ptr_fine & 0x7ffff;
		*out1++ = *out2++ = scale * (a * (0x80000 - fract_part_fine) + a2 * fract_part_fine);

		s_ptr_fine += l_increment;
		++gate;
	}
}

void ug_wave_player::sub_process_file_stereo(int start_pos, int sampleframes)
{
	float* out1 = output[0] + start_pos;
	float* out2 = output[1] + start_pos;
	float* pitch_shift = pitch_ptr + start_pos;
	const float* gate = gate_ptr + start_pos;
	int loop_counter = sampleframes;
	int l_increment = increment;
	bool pitch_changing = plug_status_pitch == ST_RUN;

	while (loop_counter-- > 0)
	{
		// increment pointer
		if (pitch_changing)
		{
			l_increment = calc_increment(*pitch_shift++);
		}

		if (s_ptr_fine > 0xffffff)
		{
			int i = s_ptr_fine >> 24;
			s_ptr += i + i; // * 2 for stereo
			s_ptr_fine &= 0xffffff;
		}

		if (s_ptr >= s_end)
		{
			if (refill_buffer(*gate > 0.0f))
			{
				//  end
				loop_counter++; // compensate for initial decrement
				timestamp_t l_sample_clock = SampleClock() + sampleframes - loop_counter;
				OutputChange(l_sample_clock, GetPlug(PN_OUT1), ST_STOP);
				OutputChange(l_sample_clock, GetPlug(PN_OUT2), ST_STOP);
				static_output_count -= loop_counter;

				while (loop_counter-- > 0)
				{
					*out1++ = *out2++ = 0.f;
				}

				return;
			}
		}


		//-------------------------------------------------new-----------------------------------
		// set up pointers to sample and table
		// !!mono code float *ptr = s_ptr - 3;
		const int nchan = 2;
		const float* ptr = s_ptr - 3 * nchan;
		// reduce fraction part to range 0-31
		const float* interpolation_table_ptr = interpolation_table2 + (s_ptr_fine >> 19) * 8;
		// now look up interpolation filter table
		// perform 2 filters at once, one 1 sub sample ahead
		float a_l = 0.f;
		float a_r = 0.f;

		for (int x = 8; x > 0; x--)
		{
			a_l += *ptr++ * *interpolation_table_ptr;
			a_r += *ptr++ * *interpolation_table_ptr++;
		}

		float a2_l = 0.f;
		float a2_r = 0.f;
		//!!ptr = s_ptr - 3;
		ptr = s_ptr - 3 * nchan;

		for (int x = 8; x > 0; x--)
		{
			a2_l += *ptr++ * *interpolation_table_ptr;
			a2_r += *ptr++ * *interpolation_table_ptr++;
		}

		// now linear interpolation between 2 subsamples
		constexpr float scale = (1.f / 0x100000); // 0x80000 for unity gain. (samples half the level they should be)
		// save lowest 19 bits for interpolation
		int fract_part_fine = s_ptr_fine & 0x7ffff;
		*out1++ = scale * (a_l * (0x80000 - fract_part_fine) + a2_l * fract_part_fine);
		*out2++ = scale * (a_r * (0x80000 - fract_part_fine) + a2_r * fract_part_fine);

		s_ptr_fine += l_increment;
		++gate;
	}
}

int ug_wave_player::open_file()
{
	// clones get sent stat change in filename plug right after Open()
	// no need to reload file
	if (m_currently_loaded_file == FileName)
		return false;

	// delete any prev loaded headers
	m_currently_loaded_file.clear();
	cursor = nullptr;
	loop_start = loop_end = -1;

	if (FileName.empty())
		return false;

	auto l_filename = AudioMaster()->getShell()->ResolveFilename(FileName, L"wav");

	std::wstring errorMessage = L"Can't open: " + FileName + L". Check: filename is correct, check it's a WAVE file";

	if (BundleInfo::instance()->isEditor)
	{
		errorMessage += L", check your directory settings (Audio/Preferences).";
	}

	try
	{
		cursor = wavReader.open(WStringToUtf8(l_filename));
	}
	catch (char* errormes)
	{
		errorMessage = Utf8ToWstring(errormes);
		cursor = nullptr;
	}

	if (!cursor)
	{
		// don't need message repeated on every voice.
		if (pp_voice_num <= 1 && !m_inhibit_error_msg && !m_user_inhibit_error_message)
		{
			m_inhibit_error_msg = true;
			message(errorMessage.c_str(), 0);
		}

		return 1;
	}

	if (cursor && cursor->ChannelsCount() > 2)
	{
		message(L"Multichannel WAVE file format is not supported");
		cursor = nullptr;
		return 1;
	}

	m_currently_loaded_file = FileName;
	return 0;
}

void ug_wave_player::retrigger_sample(timestamp_t p_sampleclock)
{
	// sample load may have failed
	if (!cursor)
	{
		return;
	}

	play_flag = true;

	cursor->Reset();

	SET_CUR_FUNC(&ug_wave_player::sub_process_file);

	if (cursor->ChannelsCount() == 1)
	{
		current_process_func = static_cast <process_func_ptr> (&ug_wave_player::sub_process_file_mono);
	}

	if (cursor->ChannelsCount() == 2)
	{
		current_process_func = static_cast <process_func_ptr> (&ug_wave_player::sub_process_file_stereo);
	}

	s_ptr_fine = 0;

	// load rest of buffer from file
	refill_buffer(true);

	OutputChange(p_sampleclock, GetPlug(PN_OUT1), ST_RUN);
	OutputChange(p_sampleclock, GetPlug(PN_OUT2), ST_RUN);

	increment = calc_increment(GetPlug(PN_PITCH)->getValue());
}
