#pragma once

#include "ug_base.h"
#include "UMidiBuffer2.h"
#include "cancellation.h"

class ug_vst_in : public ug_base
{
#ifdef CANCELLATION_TEST_ENABLE2
    float m_phase = 0.f;
#endif

public:
	ug_vst_in();
	~ug_vst_in();
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_vst_in);

	void sub_process(int start_pos, int sampleframes);
	int Open() override;
	int Close() override;
    void setInputs( const float* const* p_inputs, int numChannels, int numSidechains = 0 );
    void setIncrement( int i )
    {
        bufferIncrement_ = i;
    }
    int getIncrement()
    {
        return bufferIncrement_;
    }
    int getNumSidechains()
    {
        return numSidechains_;
    }
    void setInputSilent(int input, bool isSilent);

private:
	midi_output midi_out;
	const float** m_inputs;
    int bufferIncrement_;
    int numSidechains_;
};
