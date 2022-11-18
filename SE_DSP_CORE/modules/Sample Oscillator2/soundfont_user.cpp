#include <algorithm>
#include "soundfont_user.h"
#include "csoundfont.h"
#include "SampleManager.h"


#define RIGHT 1
#define LEFT 0

short partial::silence[8] = {0,0,0,0,0,0,0,0};

void partial::DoLoop( bool gateState )
{
	// has loop ended, or entire sample ended?
	// loop mode 0 - None, 1 - continuous loop, 3 loop while key on
	if( loop_mode == 0 || ( loop_mode == 3 && gateState == false ) )
	{
		s_end_section = s_end;

		// has entire sample ended?
		if( s_ptr_r >= s_end )
		{
			// switch output to silent sample.
			s_ptr_r = s_ptr_l = s_loop_st = &( silence[3] );
			s_loop_end = & ( silence[7] );
			s_increment = 0;
			s_ptr_l_offset = 0;

			#if defined( _DEBUG )
			sampleStartAddressRight = sampleStartAddressLeft = silence;
			#endif
		}
	}
	else
	{
		// very safe way to loop, even with extreme increments
		s_ptr_r = s_loop_st + ((s_ptr_r - s_loop_end) % (s_loop_end - s_loop_st));
		assert( s_ptr_r >= sampleStartAddressRight && s_ptr_r <= s_end );
	}
}


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
		partial.cur_sample_r = sampleHeader;

		#if defined( DEBUG_ZONES )
//			_RPT2(_CRT_WARN, "SampleHeader R %d = %x\n", z.Get(53).wAmount, partial.cur_sample_r );
		#endif

		/*
			If sfSampleType indicates a mono sample, then wSampleLink is undefined and its value should be
			conventionally zero, but will be ignored regardless of value. If sfSampleType indicates a left or right
			sample, then wSampleLink is the sample header index of the associated right or left stereo sample
			respectively. Both samples should be played entirely syncrhonously, with their pitch controlled by the
			right sample’s generators.
		*/
		if( (partial.cur_sample_r->sfSampleType & monoSample) == 0 ) // stereo sample?
		{
			partial.cur_sample_l = SampleManager::Instance()->GetSampleHeader( sampleHandle, partial.cur_sample_r->wSampleLink );
			#if defined( DEBUG_ZONES )
//					_RPT2(_CRT_WARN, "SampleHeader L %d = %x\n", partial.cur_sample_r->wSampleLink, partial.cur_sample_l );
			#endif

			if( partial.cur_sample_l != 0 && (partial.cur_sample_r->sfSampleType & leftSample) != 0 ) // left sample, need to swap
			{
				// Swap Left/Right Samples.
				sfSample *t = partial.cur_sample_r;
				partial.cur_sample_r = partial.cur_sample_l;
				partial.cur_sample_l = t;
			}
		}
		else
		{
			partial.cur_sample_l = 0;
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

		partial.s_end		 = partial.s_loop_st = partial.s_loop_end = partial.s_ptr_r = (short *) SampleManager::Instance()->GetSampleChunk( sampleHandle );
		partial.s_ptr_r		+= partial.cur_sample_r->dwStart	+ zone_startAddrsOffset;
		partial.s_loop_st	+= partial.cur_sample_r->dwStartloop+ zone_startloopAddrsOffset;
		partial.s_loop_end	+= partial.cur_sample_r->dwEndloop	+ zone_endloopAddrsOffset;
		partial.s_end		+= partial.cur_sample_r->dwEnd		+ zone_endAddrsOffset;


		/*
		// Fix for bad loop points. (need 4 samples on either side for interpolation).
		if( partial.s_loop_st < partial.s_ptr_r + 4 )
		{
			partial.s_loop_st = partial.s_ptr_r + 4;
			_RPT0(_CRT_WARN, "Loop start out of bounds, moved.\n" );
		}
		//if( partial.s_loop_end > partial.s_end - 4 )
		//{
		//	partial.s_loop_end = partial.s_end - 4;
		//	_RPT0(_CRT_WARN, "Loop end out of bounds, moved.\n" );
		//}
*/
		// left and right pointer are locked together. just work out offset between them
		if( partial.cur_sample_l )
		{
			partial.s_ptr_l_offset = partial.cur_sample_l->dwStart - partial.cur_sample_r->dwStart;
			partial.s_ptr_l = partial.s_ptr_r + partial.s_ptr_l_offset;
		}

		// Note sample startpoint for debugging purposes.
		#if defined( _DEBUG )
		partial.sampleStartAddressRight = partial.s_ptr_r;
		partial.sampleStartAddressLeft = partial.s_ptr_l;
		assert( partial.s_ptr_r >= partial.sampleStartAddressRight && partial.s_ptr_r <= partial.s_end );
		#endif

		partial.loop_mode = z.Get( ZoneGenerator::SAMPLE_MODES /*54*/).wAmount & 3; // low 2 bits
		// loop mode 0 - None, 1 - continuous loop, 3 loop while key on
//			_RPT1(_CRT_WARN,"Sample Play:Loop Mode %d\n",partial.loop_mode);
		if( (partial.loop_mode & 1) == 0 )
			partial.s_loop_end = partial.s_end; // just in case

		//partial.s_end_section = min( partial.s_loop_end, partial.s_end ); // when to consider next move
		partial.s_end_section = partial.s_end; // when to consider next move
		if( partial.s_end_section > partial.s_loop_end )
			partial.s_end_section = partial.s_loop_end;

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
