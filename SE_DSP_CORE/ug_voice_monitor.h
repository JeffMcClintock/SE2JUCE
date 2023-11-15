#pragma once
#include "ug_adder2.h"

class ug_voice_monitor : public ug_adder2
{
public:
	void HandleEvent(SynthEditEvent* e) override;
	void DoProcess( int buffer_offset, int sampleframes ) override;

	int Open() override;

	DECLARE_UG_BUILD_FUNC(ug_voice_monitor);

	ug_voice_monitor();
	void Resume() override;

#if defined( _DEBUG )
	std::wstring DebugPrintMonitoredModules(UPlug* fromPlug);
#endif

protected:
	void InputSetup();

private:
	ug_base* m_voice_parameter_setter = {};
	state_type current_out_state = ST_STOP;
	state_type current_out_state_sent = ST_ONE_OFF; // unused value to trigger update
	int silentBlocks = 0;
	int updateVoicePeakCount = {};
	float voicePeakLevel = {};
	int peakCheckBlockPeriod = {};

#if defined( _DEBUG )
	int reportTimer = 1000;
#endif
};

