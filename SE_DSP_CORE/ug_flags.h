#pragma once

// WARNING: also defined in the SDK, don't rearrange values
// unit_gen flags, indicate special abilities
enum UgFlags
{
	UGF_NONE					= 0
	,UGF_IO_MOD					= 1
	// DON'T WANT VOICE HELD OPEN BECAUSE A SCOPE IS CONNECTED. This says that *inputs* of flagged module are ignored.
	,UGF_VOICE_MON_IGNORE		= (1 <<  1)		// Flag should not be needed since I introduced the more sophisticated UGF_PP_TERMINATED_PATH. Remove in 1.5 !!!
	,UGF_POLYPHONIC				= (1 <<  2)
	,UGF_VOICE_SPLITTER         = (1 <<  3)
	,UGF_POLYPHONIC_DOWNSTREAM	= (1 <<  4)
	,UGF_POLYPHONIC_GENERATOR	= (1 <<  5)
	,UGF_POLYPHONIC_AGREGATOR	= (1 <<  6)		// A ug that always combines voices. 
	,UGF_SUSPENDED				= (1 <<  7)
	,UGF_OPEN					= (1 <<  8)
	,UGF_NEVER_EXECUTES			= (1 <<  9)
	,UGF_DELAYED_GATE_NOTESOURCE= (1 << 10)
	// Module takes advantage of spare samples at end of buffer to make better use of SSE.
	,UGF_SSE_OVERWRITES_BUFFER_END = (1 << 11)
	//,UGF_DEBUG_WATCH			= (1 << 12)
	// Module initiates parameter changes or triggers voices, so needs to be upstream of all other modules using parameters or host-controls.
	// Any exceptions to this, e.g. controls connected before MIDI-CV need special UGF_UPSTREAM_PARAMETER flag to signify latency on parameter changes. 
	,UGF_PATCH_AUTO				= (1 << 13)
	,UGF_DEBUG_POLY_LAST		= (1 << 14)
	,UGF_OVERWRITING_BUFFERS	= (1 << 15)
	,UGF_DONT_CHECK_BUFFERS		= (1 << 16)
	,UGF_SEND_TIMEINFO_TO_HOST	= (1 << 17)
	// UG THAT HANDLES PARAMETER UPDATES FROM GUI VIA PATCH MANAGER
	,UGF_PARAMETER_SETTER		= (1 << 18)
	// UG THAT PASSES OUTPUT PARAMETERS UPDATES TO THE GUI VIA PATCH MANAGER
	,UGF_PP_TERMINATED_PATH		= (1 << 19) // Module is (or leads to) a proper end-of-voice summation point.
	// MIDI-CV is cloned (drumtrigger is not)
	,UGF_POLYPHONIC_GENERATOR_CLONED= (1 << 20)
	,UGF_VOICE_MUTE				= (1 << 21)	// ALWAYS INSERTED BEFORE VOICE-ADDER (but should be ignored by InsertVoiceAdders() )
	,UGF_DEFAULT_SETTER			= (1 << 22)
	,UGF_UPSTREAM_PARAMETER		= (1 << 23)	// Module processing MIDI input to Patch-Automator.
//	,UGF_ALWAYS_EXPORT			= (1 << 25)	// e.g. Voice-Mute. moved to CF_ flags
	,UGF_HAS_HELPER_MODULE		= (1 << 26)	// e.g. Poly-to-Mono.
	,UGF_NEVER_SUSPEND			= (1 << 27)
};


