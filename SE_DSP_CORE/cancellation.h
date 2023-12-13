#pragma once
#include "./modules/shared/xplatform.h"

// Take a snapshot of EVERY audio connection at a specified timestamp (in sampleframes).
// !!! DO NOT SHIP PLUGINS WITH THIS ENABLED !!!
#if 0 //def _DEBUG
#define CANCELLATION_SNAPSHOT_TIMESTAMP 37371
#endif

/*
1. Build plugins with CANCELLATION_SNAPSHOT_TIMESTAMP defined
2. Run plugins playing a note
3. For each one, rename the output 'C:\temp\cancellation\V13\snapshotA.raw' and 'snapshotB.raw'
4. In special Jeff build of SynthEdit - run menu 'File/Analyse Cancellation'.
   ref SE_DSP_CORE\CancellationAnalyse.cpp for filepaths
	
=== to match blocksize on Windows to mac ===

int SeAudioMaster::getOptimumBlockSize()
{
#ifdef PROFILE_VARIOUS_BLOCK_SIZES
	return profileBlockSize;
#endif
#ifdef _DEBUG // trying to emulate macOS. blocksize 68, driver 1156 samples
	return 68;
#endif
	return OPTIMUM_BLOCK_SIZE;
}
*/


#if defined( CANCELLATION_SNAPSHOT_TIMESTAMP )
#define CANCELLATION_TEST_ENABLE2
#define CANCELLATION_BRANCH "SE15B"
#endif