#pragma once
#include <map>
#include "ug_midi_device.h"
#include "IDspPatchManager.h"
#include "SeAudioMaster.h"

#define MCV_NOTE_MEM_SIZE 16

// replaces both ug_patch_sel and ug_midi_automator_in
class ug_patch_automator : public ug_midi_device
{
	friend class ug_patch_automator_out;
public:
	ug_patch_automator();
	~ug_patch_automator();
	DECLARE_UG_BUILD_FUNC(ug_patch_automator);

	int Open() override;
	void assignPlugVariable(int p_plug_desc_id, UPlug* p_plug) override;
	void process_timing(int start_pos, int sampleframes);
	virtual void OnMidiData( int size, unsigned char* midi_bytes ) override;
	void setMidiChannel( int channel );
	void setTempoTransmit();
	void UpdateSongPosition(double qnEventTime, double ppqPos);
	bool wantsTempo()
	{
		return transmitTempo_;
	}
	void UpdateTempo( struct my_VstTimeInfo* timeInfo );
	void debugTimingPrint();
	void HandleEvent(SynthEditEvent* e) override;

	bool m_send_automation_on_patch_change;

private:
	midi_output midi_out;
	midi_output midi_out_hidden;

	short polyMode_;
	short monoRetrigger_;
	short monoNotePriority_;
	float portamentoTime_;

	int guiQueServicePeriod_;

	bool transmitTempo_;
	double incrementPerSample;
	int lastWholeQuarterNoteSent = -99;

	my_VstTimeInfo timeInfo;
	VoiceControlState voiceState_;
};
