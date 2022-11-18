#pragma once
#include "ug_midi_device.h"
#include "iseshelldsp.h"

class ug_vst_out : public ug_midi_device, public ISpecialIoModuleAudio
{
    enum { ProcessNotFading, ProcessFading };

public:
	ug_vst_out();
	~ug_vst_out();
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_vst_out);

	template<int fadePolicy>
    void subProcessTemplate(int start_pos, int sampleframes)
    {
	    int out_num = 0;
        int inc = bufferIncrement_;
	    float level;

	    for(int j = 1 ; j < plugs.size() ; j++ )
	    {
		    float* from = plugs[j]->GetSamplePtr() + start_pos;
		    float* to = m_outputs[out_num];
            float output;

		    level = fadeLevel_;
		    for( int s = sampleframes ; s > 0 ; s-- )
		    {
                if constexpr( fadePolicy )
                {
		            output = *from++ * level;

			        level += fadeIncrement_;
                    if( level < 0.0f )
                        level = 0.0f;
                    if( level > 1.0f )
                        level = 1.0f;
                }
                else
                {
		            output = *from++;
                }

	            *to = output;

                to += inc;
		    }

            m_outputs[out_num++] += sampleframes * inc;
			fadeLevel_ = level;
	    }

        if constexpr( fadePolicy == ProcessFading )
        {
	        if( fadeIncrement_ < 0.0f )
            {
	            if( fadeLevel_ <= 0.0 )
	            {
		            fadeLevel_ = 0.0f; // indicates fade-out complete.
		            AudioMaster()->onFadeOutComplete();
	            }
            }
            else
            {
	            if( fadeLevel_ >= 1.0 )
	            {
		            fadeLevel_ = 1.0f; // indicates fade-up complete.
                    ChooseProcess();
	            }
            }
        }
    }

	void sub_process_check_mode(int start_pos, int sampleframes);

	int Open() override;
	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state) override;
	void setIoBuffers(float** p_outputs, int numChannels, int stride = 1) override;
	void HandleEvent(SynthEditEvent* e) override;
	void StartFadeOut(bool isDucked = true);
    void ChooseProcess();

	int getOverallPluginLatencySamples()
	{
		// If latency compensation disabled, latency will be -1;
		return (std::max)(0, cumulativeLatencySamples);
	}

	uint64_t updateSilenceFlags(int output, int count);

	MidiBuffer3 MidiBuffer;

private:
	float** m_outputs;
	float fadeLevel_;
    float targetLevel;
	float fadeIncrement_;
    int bufferIncrement_;

	std::vector<bool> pinIsSilent;
	std::vector<bool> pinChanged;
};
