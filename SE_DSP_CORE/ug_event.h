#pragma once

#include "modules/se_sdk3/mp_api.h"
#include "modules/se_sdk2/se_datatypes.h"
#include "se_types.h"

// Allocate from fixed pool.
#if 0

#define SE_FIXED_POOL_MEMORY_ALLOCATION

#include "ISeAudioMaster.h"
#include "iseshelldsp.h"

#endif

class intrusiveListItem
{
	friend class EventList;
	friend class EventIterator;

protected:
	struct SynthEditEvent* prev_;
	struct SynthEditEvent* next_;
};

struct AbsoluteTimeStamp
{
	timestamp_t timeStamp;
};

struct SynthEditEvent : public intrusiveListItem, AbsoluteTimeStamp, gmpi::MpEvent
{
	void* Data()
	{
		assert( eventType == UET_EVENT_SETPIN || eventType == UET_EVENT_MIDI );

		if (extraData) // e->parm2 > sizeof(int))
		{
			return extraData;
		}
		else // less than 4 bytes.
		{
			return &parm3;
		}
	}

#if defined( SE_FIXED_POOL_MEMORY_ALLOCATION )

	// prevent use of operator new and delete, except by allocation routines in cProcessor.

	friend class  FixedPoolMemoryManager;

protected:
	SynthEditEvent()
	{
	};

	~SynthEditEvent()
	{
	};

#else

	SynthEditEvent(timestamp_t p_timeStamp = 0, int32_t p_eventType = 0, int32_t p_parm1 = 0, int32_t p_parm2 = 0, int32_t p_parm3 = 0, int32_t p_parm4 = 0, char* extra = 0)
	{
		timeStamp = p_timeStamp;
		eventType = p_eventType;
		parm1 = p_parm1;
		parm2 = p_parm2;
		parm3 = p_parm3;
		parm4 = p_parm4;
		next = 0;
		extraData = extra;
	};

	~SynthEditEvent()
	{
		delete [] extraData;
	};

#endif

};

// Waves platform - Allocate from fixed pool.
#if defined( SE_FIXED_POOL_MEMORY_ALLOCATION )

#define new_SynthEditEvent(p_timeStamp, p_eventType, p_parm1, p_parm2, p_parm3, p_parm4) (AudioMaster()->AllocateMessage(p_timeStamp, p_eventType, p_parm1, p_parm2, p_parm3, p_parm4 ))
#define new_SynthEditEventB(p_timeStamp, p_eventType, p_parm1, p_parm2, p_parm3, p_parm4, extra) (AudioMaster()->AllocateMessage(p_timeStamp, p_eventType, p_parm1, p_parm2, p_parm3, p_parm4, extra ))
//#define delete_SynthEditEvent(e) AudioMaster()->getShell()->DeAllocateMessage(e)
#define delete_SynthEditEvent(e) { if(e){ if( e->extraData ) {	AudioMaster()->DeAllocateMessage(e);} else	{   e->eventType = -1;}}}

#else

#define new_SynthEditEvent new SynthEditEvent
#define new_SynthEditEventB new SynthEditEvent
#define delete_SynthEditEvent(e) delete(e)

#endif
