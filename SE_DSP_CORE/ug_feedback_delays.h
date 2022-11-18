#pragma once
#include "ug_midi_device.h"
#include "ug_container.h"
#include "UMidiBuffer2.h"

void BypassFeedbackModule(ug_base* u, int voice);

class ug_feedback_delay : public ug_midi_device
{
	timestamp_t end_time;

  public:
    ug_feedback_delay();
    DECLARE_UG_BUILD_FUNC(ug_feedback_delay);
    DECLARE_UG_INFO_FUNC2;

	void DoProcess(int buffer_offset, int sampleframes) override
	{
		const timestamp_t current_sample_clock = SampleClock();

		// end_time will be feedback-out modules current sample clock.
		end_time = current_sample_clock + sampleframes;

		ug_base::DoProcess(buffer_offset, sampleframes);
	}

    virtual void BuildHelperModule() override;
	void BypassRedundentModules(int voice) override;
    void onSetPin(timestamp_t p_clock, UPlug *p_to_plug, state_type p_state) override;
    int Open() override;
	void OnFirstSample();
    void sub_process(int start_pos, int sampleframes);
    void sub_process_static(int start_pos, int sampleframes);
    struct FeedbackTrace* PPSetDownstream() override;
    virtual void PPPropogateVoiceNum(int n) override;
    ug_base *Clone(CUGLookupList &UGLookupList) override;
	class ug_feedback_delay_out* feedback_out = {};

  private:
    float *input1_ptr;
    float *output1_ptr;
    float m_last_output_value;
    state_type m_last_output_stat;
	int feedbackLatency = 0;
	float feedbackLatencyMs = 0.0f;
};

class ug_feedback_delay_out : public ug_base
{
  public:
    DECLARE_UG_BUILD_FUNC(ug_feedback_delay_out);
    DECLARE_UG_INFO_FUNC2;
    ug_feedback_delay_out();

    int Open() override;
	void OnFirstSample();
    virtual void PPSetUpstream() override;
    virtual void PPPropogateVoiceNumUp(int n) override;
	void sub_process(int start_pos, int sampleframes);
	void StatusChangeFromFeedbackIn(int p_clock, state_type p_state);
	ug_base* Clone(CUGLookupList& UGLookupList) override;

    float *output1_ptr;
	float feedbackLatencyMs = 0.0f;
	ug_feedback_delay* feedback_in = {};

	std::vector< std::pair<int, state_type> > statusQueue;
};

template< int DATATYPE, int DIRECTION >
class ug_feedback_delay_base : public ug_base
{
public:
	ug_feedback_delay_base() :
		mate(0)
	{
		SET_CUR_FUNC(&ug_base::process_sleep);
	}

	static ug_base* CreateObject(){return new ug_feedback_delay_base<DATATYPE, DIRECTION>;}
	ug_base* Create() override
	{
		return CreateObject();
	}

	void ListInterface2(InterfaceObjectArray &PList) override
	{
		if (DIRECTION == DR_IN)
		{
			// LIST_VAR3(L"In", trash_sample_ptr, DR_IN, (EPlugDataType) DATATYPE, L"", L"", 0, L"");
			PList.push_back(new InterfaceObject(nullptr, (L"In"), DR_IN, (EPlugDataType)DATATYPE, L"", L"", 0, L""));
		}
		else
		{
			// non-functional except for dummy connection to mate.
			LIST_VAR3N(L"", DR_IN, DT_MIDI, L"0", L"", 0, L"");
		}

		//LIST_VAR3(L"Out", trash_sample_ptr, DR_OUT, (EPlugDataType) DATATYPE, L"", L"", 0, L"");
		PList.push_back( new InterfaceObject( nullptr, (L"Out"), DR_OUT, (EPlugDataType)DATATYPE, L"", L"",0, L"") );

		// Special-purpose dummy pin
		LIST_VAR3N(L"", DR_OUT, DT_MIDI, L"", L"", IO_HIDE_PIN, L"");
	}

	void BuildHelperModule() override
	{
		if (DIRECTION == DR_IN)
		{
			auto generator2 = new ug_feedback_delay_base<DATATYPE, DR_OUT>();

			generator2->setModuleType(moduleType);
			parent_container->AddUG(generator2);
			generator2->SetupWithoutCug();
			generator2->patch_control_container = patch_control_container;
			mate = generator2;
			generator2->mate = this;

			// Connect dummy connection to feedback-out so it is sorted downstream whenever possible (i.e. to remove unneeded delays).
			// this connection will be severed to create a delayed feedback path, but only if necessary.
			connect(plugs.back(), generator2->GetPlug(0)); // Dummy pins.

			// now proxy my "out" to new ug's "out"
			GetPlug(1)->Proxy = generator2->GetPlug(1);
			generator2->cpuParent = cpuParent;
			AudioMaster2()->RegisterBypassableModule(this);
		}
	}

	void BypassRedundentModules(int voice) override
	{
		BypassFeedbackModule(this, voice);
	}

	void PPSetUpstream() override
	{
		ug_base::PPSetUpstream();
		mate->PPSetUpstream();
	}

	void PPPropogateVoiceNumUp(int n) override
	{
		ug_base::PPPropogateVoiceNumUp(n);
		mate->PPPropogateVoiceNumUp(n);
	}

	// just relays events from feedback input
	void HandleEvent(SynthEditEvent *e) override
	{
		switch (e->eventType)
		{
		case UET_EVENT_MIDI: // relay change from delay input module
		case UET_EVENT_SETPIN:
		{
			if (DIRECTION == DR_IN) // Feedback-in module
			{
				auto delayed_event = new_SynthEditEventB(
					e->timeStamp, e->eventType, e->parm1, e->parm2, e->parm3, e->parm4, e->extraData);

				e->extraData = 0; // transfer extra data without re-allocating.
				delayed_event->timeStamp += feedbackLatency;
				mate->AddEvent(delayed_event);
			}
			else // Feedback-out (helper) module
			{
				unsigned char* data;

				if (e->parm2 <= sizeof(int))
				{
					// filter out non-relevant events. Less events = less subdivision of audio
					// block
					// 0xF0 indicates a system msg (clocks etc)
					data = (unsigned char *)&(e->parm3);
				}
				else // Must be SYSEX.
				{
					data = (unsigned char *)e->extraData;
				}

				GetPlug(1)->Transmit(e->timeStamp, e->parm2, data);
			}
		}
		break;

		default:
			ug_base::HandleEvent(e);
		};
	}

	int Open() override
	{
		ug_base::Open();

		if (GetPlug(1)->DataType != DT_MIDI)
		{
			const int zero = 0;
			GetPlug(1)->Transmit(SampleClock(), sizeof(zero), &zero); // should work for float and int.
		}

		// Only add latency to events if help module is actually upstream.
		if (DIRECTION == DR_IN) // Feedback-in module
		{
			feedbackLatency = mate->SortOrder2 > SortOrder2 ? AudioMaster()->BlockSize() : 0;
		}

		return 0;
	}

	ug_base* Clone(CUGLookupList &UGLookupList) override
	{
		ug_feedback_delay_base* clone = (ug_feedback_delay_base*)ug_base::Clone(UGLookupList);

		// attempt to find feedback in's clone.
		// depending on order things are clones, may not be available yet..
		ug_base* clones_other_half = UGLookupList.LookupPolyphonic(mate);

		if (clones_other_half)
		{
			clone->mate = clones_other_half;
			((ug_feedback_delay_base*)clones_other_half)->mate = clone;
		}

		return clone;
	}

	class ug_base* mate;
	int feedbackLatency = 0;
};






