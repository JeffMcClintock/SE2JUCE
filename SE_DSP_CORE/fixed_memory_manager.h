// FIXED-POOL MEMORY MANAGEMENT.
#ifndef __FixedPoolMemoryManager_h__
#define __FixedPoolMemoryManager_h__

/*
#include "fixed_memory_manager.h"
*/

#include "./ug_event.h"
//#define DEBUG_FIXED_MEMORY_ALLOCATOR // TESTING!!!!

const uint32_t kMessagePoolSize = 0x40000;//Must be a power of 2
const uint32_t kMessagePoolMask = kMessagePoolSize-1;
const int32_t  kUnused          = -1;

const uint32_t kExtraDataAllocatorsPoolSize = 0x100000;//Must be a power of 2
const uint32_t kExtraDataAllocatorsPoolMask = kExtraDataAllocatorsPoolSize-1;
//const uint32_t kExtraDataPoolSize = 0x20000;
const uint32_t kExtraDataPoolSize = 0x800000;

struct sExtraDataAllocator
{

    void clear()
    {
        pBegin = pEnd = 0;
        prevUsedNeighbors = nextUsedNeighbors = 0;
    }

    char *pBegin; //points to the first byte allocated
    char *pEnd;   //points to ONE PAST the last byte allocated 
    sExtraDataAllocator *prevUsedNeighbors;
    sExtraDataAllocator *nextUsedNeighbors;
};


class FixedPoolMemoryManager
{
    // Events.
    SynthEditEvent m_MPMessagesPool[kMessagePoolSize];
    uint32_t m_NextMessage;

    // Extra-data (e.g. strings too large for event structure)
    sExtraDataAllocator m_ExtraDataAllocators[kExtraDataAllocatorsPoolSize];
    char m_ExtraDataPool[kExtraDataPoolSize];

public:
    FixedPoolMemoryManager()
    {
        m_NextMessage=0;

        //mark all messages in pool as unused
        for (uint32_t i=0; i<kMessagePoolSize; ++i)
            m_MPMessagesPool[i].eventType=kUnused;

        //Mark all allocators as unread
        for (uint32_t i=0; i<kExtraDataAllocatorsPoolSize; ++i)
        {
            m_ExtraDataAllocators[i].clear();
        }

        //The first and last allocators are the 'boundaries'.
        //Mark the first allocator as the beginning of 'ExtraDataPool'
        m_ExtraDataAllocators[0].pBegin = m_ExtraDataPool+0;
        m_ExtraDataAllocators[0].pEnd = m_ExtraDataAllocators[0].pBegin;
        m_ExtraDataAllocators[0].prevUsedNeighbors = 0;
        m_ExtraDataAllocators[0].nextUsedNeighbors = m_ExtraDataAllocators+kExtraDataAllocatorsPoolMask;


        //Mark the last allocator past the end of 'ExtraDataPool'
        m_ExtraDataAllocators[kExtraDataAllocatorsPoolMask].pBegin = m_ExtraDataPool+kExtraDataPoolSize;
        m_ExtraDataAllocators[kExtraDataAllocatorsPoolMask].pEnd = m_ExtraDataAllocators[kExtraDataAllocatorsPoolMask].pBegin;
        m_ExtraDataAllocators[kExtraDataAllocatorsPoolMask].prevUsedNeighbors = m_ExtraDataAllocators+0;
        m_ExtraDataAllocators[kExtraDataAllocatorsPoolMask].nextUsedNeighbors = 0;
    }

    /*
    DeAllocateMessage:
    To deallocate a message struct, we simply mark it as kUnused. 
    And we also deallocate the 'extraData' within the message
    */
    void DeAllocateMessage(SynthEditEvent *message)
    {
	    message->eventType = kUnused;

	    DeAllocateExtraData(message->extraData);

	    message->extraData = 0;
    }

    /*
    AllocateMessage:
    Messages are allocated from an array of messages 'm_MPMessagesPool',
    that has a pool if 'kMessagePoolSize' messages.
    Messages that are unused are marked by 'message.eventType == kUnused'.

    To allocate an unused message we simply need to search the m_MPMessagesPool for an unused message.

    To make the search process efficient, we use the following algorithm:
    We keep an index 'm_NextMessage' of the 'next message to try allocate'. 
    This index points one past the last message that was allocated.
    We first try to allocate m_MPMessagesPool[m_NextMessage], and if it is used, we look further on in the pool
    in a cyclic manner, untill we wrap back th ewhere we started.
    Once found an unused message, we return a pointer to it, 
    and update m_NextMessage to point to the next message in the pool (in a cyclic manner).
    */
    SynthEditEvent* AllocateMessage(timestamp_t p_timeStamp, int32_t p_eventType, int32_t p_parm1, int32_t p_parm2, int32_t p_parm3, int32_t p_parm4, char* extra)
    {
	    SynthEditEvent *retVal = nullptr;

	    //Search the pool for an unused message, starting from m_NextMessage to the3 end of the pool
	    uint32_t i=m_NextMessage;
	    for (; i<kMessagePoolSize; i++)
	    {
		    if (m_MPMessagesPool[i].eventType == kUnused)
		    {
			    retVal = m_MPMessagesPool + i;
			    break;
		    }

	    }

	    //If not found an unused message, wrap around and search the pool from beginning up to m_NextMessage
	    if (retVal == nullptr)
	    {

		    for (i=0; i<m_NextMessage; i++)
		    {
			    if (m_MPMessagesPool[i].eventType == kUnused)
			    {
				    retVal = m_MPMessagesPool + i;
				    break;
			    }

		    }
	    }

	    //At this point, if retVal!=0, it means we found an unused message
	    //And 'i' is the index of this message
	    if (retVal != nullptr)
	    {
		    //Mark message as used
		    retVal->eventType = p_eventType;	// See MpEventType enumeration.
		    retVal->timeStamp = p_timeStamp;	// Relative to block.
		    retVal->parm1 = p_parm1;			// Pin index if needed.
		    retVal->parm2 = p_parm2;			// Sizeof additional data. >4 implies extraData points to value.
		    retVal->parm3 = p_parm3;			// Pin value (if 4 bytes or less).
		    retVal->parm4 = p_parm4;			// Voice ID.
		    retVal->extraData = extra;				// Additional data.
		    retVal->next = 0;					// Next event in list.

		    //set m_NextMessage to point to next message in pool (with wrap around)
		    m_NextMessage = (i+1) & kMessagePoolMask;
	    }

		assert(retVal != nullptr); // You are out of event memory (ref kMessagePoolSize).
		
		return retVal;
    }

    /*
    AllocateExtraData:
    the first and last allocators in ExtraDataAllocators are 'reserved' for the allocation algorithm,
    and they mark the beginning and end of the 'm_ExtraDataPool'.

    Each used allocator always points to the neighboring used allocators from left and right.

    To allocate a range of ExtraData, we first search the pool 'ExtraDataAllocators'
    for a unused allocator.
    Note: this search is not including the first and last allocators. 

    Once we found a unused allocator, we know that the previous index allocator is used - so it's end
    marks the beginning the unused space. And it's nextUsedNeighbors.pBegin marks the end of the unused space. 

    We check if the unused space is enough for the required allocation.

    If yes, than we use it. And insert our allocator to the linked list.

    Otherwise we keep looking for the next unused allocator.

    This algorithm assures that allocators are always 'sorted' in terms of their allocated ranges.
    i.e.: for a used allocator ExtraDataAllocators[i]
    all allocators indexed j<i will use memory with smaller addresses
    and all allocators indexed j>i will use memory with bugger addresses 

    The fact that the allocators are 'sorted' makes the search much more efficient and simple. 

    */
    char *AllocateExtraData(uint32_t lSize)
    {

	    char *retVal=0;

	    sExtraDataAllocator *thisAllocator = m_ExtraDataAllocators+1;
	    sExtraDataAllocator *lastAllocator = m_ExtraDataAllocators+kExtraDataAllocatorsPoolSize-1;

        int TotalSizeRequired = lSize + sizeof(sExtraDataAllocator*);
	    while ( thisAllocator<lastAllocator )
	    {
		    //Look for an empty allocator
		    if (thisAllocator->pBegin == 0)
		    {

			    sExtraDataAllocator *prevAllocator = thisAllocator-1;
			    char *pBegin = prevAllocator->pEnd;
			    char *pEnd = prevAllocator->nextUsedNeighbors->pBegin;

			    //Find how much empty space we have between the next allocated neighbors,
			    //to the left and to the right of this allocator.
			    //If we found a big enough empty space, use it!
			    if ((pEnd - pBegin) >= TotalSizeRequired)
			    {
                    // Store pointer to the allocator at start of allocated region.
                    *((sExtraDataAllocator**)pBegin) = thisAllocator;

                    // Actual data is stored after the allocator's address.
                    retVal = pBegin + sizeof(sExtraDataAllocator*);

				    thisAllocator->pBegin = pBegin;
				    thisAllocator->pEnd = pBegin + TotalSizeRequired;
				    thisAllocator->prevUsedNeighbors = thisAllocator - 1;
				    thisAllocator->nextUsedNeighbors = thisAllocator->prevUsedNeighbors->nextUsedNeighbors;

				    thisAllocator->prevUsedNeighbors->nextUsedNeighbors = thisAllocator;
				    thisAllocator->nextUsedNeighbors->prevUsedNeighbors = thisAllocator;

				    break;
			    }
			    else
			    {
				    thisAllocator = prevAllocator->nextUsedNeighbors+1;
			    }
		    }
		    else
		    {
			    ++thisAllocator;
		    }

	    }

	    return retVal;
    }

    /*
    DeAllocateExtraData:
    We lookup the allocator from it's pointer stored at the start of the memory block,
    and remove and clear it.
    */
    void DeAllocateExtraData(char *pExtraData)
    {
	    if (pExtraData != 0)
	    {
            // Pointer to allocator was stored immediately before user-data.
            sExtraDataAllocator* thisAllocator = *(sExtraDataAllocator**) (pExtraData - sizeof(sExtraDataAllocator*) );

            thisAllocator->prevUsedNeighbors->nextUsedNeighbors = thisAllocator->nextUsedNeighbors;
		    thisAllocator->nextUsedNeighbors->prevUsedNeighbors = thisAllocator->prevUsedNeighbors;

		    thisAllocator->clear();
	    }
    }

};

#endif
