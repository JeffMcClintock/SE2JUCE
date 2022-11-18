#pragma once

#include "ug_adder2.h"

class Voice;

class ug_voice_monitor : public ug_adder2
{
public:
	void HandleEvent(SynthEditEvent* e) override;
	void DoProcess( int buffer_offset, int sampleframes ) override;

	int Open() override;

	DECLARE_UG_BUILD_FUNC(ug_voice_monitor);

	ug_voice_monitor();
	virtual void Resume() override;

#if defined( _DEBUG )
	std::wstring DebugPrintMonitoredModules(UPlug* fromPlug);
#endif

protected:
	void InputSetup();

private:
	int static_input_count;
	Voice* m_voice = nullptr;
	enum MyStateType{ VM_VOICE_OPEN, VM_VOICE_MUTING, VM_VOICE_SUSPENDING} state_;
	ug_base* m_voice_parameter_setter;
	timestamp_t suspendMessageNoteOffTime;

	state_type current_out_state;
	state_type current_out_state_sent;
	int updateVoicePeakCount;
	float voicePeakLevel;
	int peakCheckBlockPeriod;

#if defined( _DEBUG )
	int reportTimer = 1000;
#endif
};

