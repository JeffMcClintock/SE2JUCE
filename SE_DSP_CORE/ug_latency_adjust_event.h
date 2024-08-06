#pragma once
#include <assert.h>
#include "ug_base.h"

class LatencyAdjustEventBased2 : public ug_base
{
public:
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(LatencyAdjustEventBased2);

	int datatype;

	virtual void DoProcess(int /*buffer_offset*/, int sampleframes) override
	{
		if (!events.empty())
		{
			assert(latencySamples > 0);

			while (!events.empty())
			{
				auto e = events.front();
				events.pop_front();

				if (e->eventType == gmpi::EVENT_PIN_SET || e->eventType == gmpi::EVENT_MIDI)
				{
					// transfer event to dest module with adjusted timestamp.
					bool first = true;
					// Rather than send junk zero on first sample, "stretch" initial even back to start of time.
					if (e->timeStamp != 0)
					{
						e->timeStamp += latencySamples;
					}

					for (auto destPin : GetPlug(1)->connections)
					{
						if (first)
						{
							// reuse event without need for copying it.
							e->parm1 = destPin->getPlugIndex();
							destPin->UG->AddEvent(e);
							first = false;
						}
						else
						{
							// this might never happen. Seems only ever has one output? No,
							// Happens with polyphonic oversampling. assert(false);
							// copy event.
							auto e2 = new_SynthEditEventB(e->timeStamp, e->eventType, destPin->getPlugIndex(), e->parm2, e->parm3, e->parm4, nullptr);

							auto extraDataSize = e->parm2;
							if (extraDataSize > sizeof(int32_t))
							{
								// copy extra data, if any.
								e2->extraData = AudioMaster()->AllocateExtraData(extraDataSize);
								memcpy(e2->extraData, e->extraData, extraDataSize);
							}

							destPin->UG->AddEvent(e2);
						}
					}

#if defined( _DEBUG )
					GetPlug(1)->debug_sent_status = true;
#endif
				}
				else
				{
					ug_base::HandleEvent(e);
					delete_SynthEditEvent(e);
				}
			}
		}
		SetSampleClock(SampleClock() + sampleframes);

		SleepMode();
	}

	virtual ug_base* Clone(CUGLookupList &UGLookupList) override
	{
		auto clone = (LatencyAdjustEventBased2 *)ug_base::Clone(UGLookupList);
		clone->latencySamples = latencySamples;
		clone->datatype = datatype;
		return clone;
	}
};