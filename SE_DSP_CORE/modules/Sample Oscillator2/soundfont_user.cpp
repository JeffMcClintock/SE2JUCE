#include <algorithm>
#include "soundfont_user.h"
#include "csoundfont.h"
#include "SampleManager.h"

#define RIGHT 1
#define LEFT 0

short sampleChannel::silence[8] = {0,0,0,0,0,0,0,0};

void soundfontUser::GetZone(short p_chan, short p_note, short p_vel)
{
//#define DEBUG_ZONES

	partials.clear();

	SampleManager::Instance()->GetPlayingZones( sampleHandle, p_chan, p_note, p_vel, playingZonesTemp);

	for( auto zone : playingZonesTemp)
	{
		auto& z = *zone;

		auto sampleHeader = SampleManager::Instance()->GetSampleHeader(sampleHandle, z.Get(53).wAmount);
		
		if (sampleHeader->sfSampleType & 0x8000) // rom sample?
		{
			#if defined( DEBUG_ZONES )
				// _RPT0(_CRT_WARN,"Sample Play:ROM SAMPLE!!!!\n");
			#endif

			continue;
		}

		partials.push_back({});
		auto& partial = partials.back();

		partial.zone = zone;
		partial.right.cur_sample = sampleHeader;

		#if defined( DEBUG_ZONES )
//			_RPT2(_CRT_WARN, "SampleHeader R %d = %x\n", z.Get(53).wAmount, partial.right.cur_sample );
		#endif

		/*
			If sfSampleType indicates a mono sample, then wSampleLink is undefined and its value should be
			conventionally zero, but will be ignored regardless of value. If sfSampleType indicates a left or right
			sample, then wSampleLink is the sample header index of the associated right or left stereo sample
			respectively. Both samples should be played entirely syncrhonously, with their pitch controlled by the
			right sample’s generators.
		*/
		if( (partial.right.cur_sample->sfSampleType & monoSample) == 0 ) // stereo sample?
		{
			partial.left.cur_sample = SampleManager::Instance()->GetSampleHeader( sampleHandle, partial.right.cur_sample->wSampleLink );
			#if defined( DEBUG_ZONES )
//					_RPT2(_CRT_WARN, "SampleHeader L %d = %x\n", partial.right.cur_sample->wSampleLink, partial.left.cur_sample );
			#endif

			if( partial.left.cur_sample != 0 && (partial.right.cur_sample->sfSampleType & leftSample) != 0 ) // left sample, need to swap
			{
				// Swap Left/Right Samples.
				std::swap(partial.right.cur_sample, partial.left.cur_sample);
			}
		}
		else
		{
			partial.left.cur_sample = {};
		}

		#if defined( DEBUG_ZONES )
//				_RPT0(_CRT_WARN,"Sample Play:Good SAMPLE\n");
		#endif

		int zone_startAddrsOffset		= z.Get(0).shAmount + ( z.Get(4).shAmount << 15 );
		int zone_endAddrsOffset			= z.Get(1).shAmount + ( z.Get(12).shAmount << 15 );
		int zone_startloopAddrsOffset	= z.Get(2).shAmount + ( z.Get(45).shAmount << 15 );
		int zone_endloopAddrsOffset		= z.Get(3).shAmount + ( z.Get(50).shAmount << 15 );

		#if defined( DEBUG_ZONES )
//				_RPT0(_CRT_WARN,"ZONE\n");
//				_RPT1(_CRT_WARN,"   %d   zone_startAddrsOffset\n", (int)zone_startAddrsOffset);
//				_RPT1(_CRT_WARN,"   %d   zone_endAddrsOffset\n", (int)zone_endAddrsOffset);
//				_RPT1(_CRT_WARN,"   %d   zone_startloopAddrsOffset\n", (int)zone_startloopAddrsOffset);
//				_RPT1(_CRT_WARN,"   %d   zone_endloopAddrsOffset\n", (int)zone_endloopAddrsOffset);
		#endif

		const auto sampleChunk = (short*)SampleManager::Instance()->GetSampleChunk(sampleHandle);

		partial.right.s_end = partial.right.s_loop_st = partial.right.s_loop_end = partial.right.s_ptr = sampleChunk;
		partial.right.s_ptr		+= partial.right.cur_sample->dwStart	+ zone_startAddrsOffset;
		partial.right.s_loop_st	+= partial.right.cur_sample->dwStartloop+ zone_startloopAddrsOffset;
		partial.right.s_loop_end+= partial.right.cur_sample->dwEndloop	+ zone_endloopAddrsOffset;
		partial.right.s_end		+= partial.right.cur_sample->dwEnd		+ zone_endAddrsOffset;

		if( partial.left.cur_sample )
		{
			partial.left.s_end       = partial.left.s_loop_st = partial.left.s_loop_end = partial.left.s_ptr = sampleChunk;
			partial.left.s_ptr      += partial.left.cur_sample->dwStart     + zone_startAddrsOffset;
			partial.left.s_loop_st  += partial.left.cur_sample->dwStartloop + zone_startloopAddrsOffset;
			partial.left.s_loop_end += partial.left.cur_sample->dwEndloop   + zone_endloopAddrsOffset;
			partial.left.s_end      += partial.left.cur_sample->dwEnd       + zone_endAddrsOffset;
		}

		// Note sample startpoint for debugging purposes.
		#if defined( _DEBUG )
		partial.left.sampleStartAddress = partial.left.s_ptr;
		partial.right.sampleStartAddress = partial.right.s_ptr;

		assert(partial.right.s_ptr >= partial.right.sampleStartAddress && partial.right.s_ptr <= partial.right.s_end );
		#endif

		partial.left.loop_mode = partial.right.loop_mode = z.Get( ZoneGenerator::SAMPLE_MODES /*54*/).wAmount & 3; // low 2 bits
		// loop mode 0 - None, 1 - continuous loop, 3 loop while key on
//			_RPT1(_CRT_WARN,"Sample Play:Loop Mode %d\n",partial.loop_mode);
		if ((partial.right.loop_mode & 1) == 0)
		{
			partial.left.s_loop_end  = partial.left.s_end; // just in case
			partial.right.s_loop_end = partial.right.s_end;
		}

		partial.left.s_end_section = partial.left.s_end; // when to consider next move
		partial.right.s_end_section = partial.right.s_end;
		if (partial.left.s_end_section > partial.left.s_loop_end)
			partial.left.s_end_section = partial.left.s_loop_end;

		if (partial.right.s_end_section > partial.right.s_loop_end)
			partial.right.s_end_section = partial.right.s_loop_end;

		// Panning.
		partial.SetPan( 0.001f * (float) z.Get( ZoneGenerator::PAN ).shAmount );  // 500 = hard right. -500=hard left.

		//swprintf(msg, 200, L"%d, %f,%f", z.Get( ZoneGenerator::PAN ).shAmount, partial.pan_left_level, partial.pan_right_level );
		#if defined( DEBUG_ZONES )
//			_RPT3(_CRT_WARN,"PAN:%d   L:%f,R:%f   \n", (int)z.Get( ZoneGenerator::PAN ).shAmount, partial.pan_left_level, partial.pan_right_level );
		#endif
	}

	playingZonesTemp.clear();
}

void partial::SetPan( float pan ) // 0.5 = hard right. -0.5 = hard left.
{
	//wchar_t msg[200];
	//swprintf(msg, 200, L"%f, %f,%f", pan, sqrtf(pan + 0.5f), sqrtf(1.0f - pan - 0.5f) );
	//GetApp()->SeMessageBox( msg, L"",MB_OK|MB_ICONSTOP );

    pan_right_level = sqrtf( (std::max)(0.5f + pan, 0.0f) ); // prone to small -ve errors returning #inf
	pan_left_level  = sqrtf( (std::max)(0.5f - pan, 0.0f) );
};
