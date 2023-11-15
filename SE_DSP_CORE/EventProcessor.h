#if !defined(AFX_EVENTPROCESSOR_H__555A8AA1_0CE1_11D5_B73D_00104B15CCF0__INCLUDED_)
#define AFX_EVENTPROCESSOR_H__555A8AA1_0CE1_11D5_B73D_00104B15CCF0__INCLUDED_

#pragma once

#include <memory>
#include <cmath>
#include "./dsp_msg_target.h"
#include "./ug_event.h"
#include "./ug_flags.h"

class SeAudioMaster;
class EventProcessor;

// #define LOG_PIN_EVENTS
//#define LOG_ALL_MODULES_CPU

typedef void (EventProcessor::*ug_func)(); // Pointer to member function
#define RUN_AT(delay, func) RunDelayed( delay, static_cast <ug_func> (func) )

class EventIterator
{
public:
	EventIterator( SynthEditEvent* node )
	{
		current_ = node;
	}
	SynthEditEvent* operator*() const
	{
		return current_;
	}
	//const T* operator->() const;
	EventIterator& operator++()
	{
		current_ = current_->next_;
		return *this;
	}
	EventIterator& operator--()
	{
		current_ = current_->prev_;
		return *this;
	}

	bool equal(EventIterator const& rhs) const
	{
		return current_ == rhs.current_;
	}

private:
	SynthEditEvent* current_;
};

class EventList : protected SynthEditEvent
{
public:
	EventList()
	{
		prev_ = next_ = this;
	}
	~EventList() {}
	// reverse searches start at last item.
	SynthEditEvent* front()
	{
		return next_;
	}
	SynthEditEvent* begin()
	{
		return next_;
	}
	SynthEditEvent* rbegin()
	{
		return prev_;
	}
	SynthEditEvent* end()
	{
		return this;
	}
	// reverse searches end at pointer to first item.
	SynthEditEvent* rend()
	{
		return this;
	}
	void push_front( SynthEditEvent* e )
	{
		e->next_ = next_;
		e->prev_ = this;
		next_ = e;
		e->next_->prev_ = e;
	}

	void pop_front()
	{
		//		#if defined( _DEBUG )
		SynthEditEvent* popped = next_;
		assert( popped != this );
		//		#endif
		next_ = next_->next_;
		next_->prev_ = this;
		popped->prev_ = 0; // flags 'not on list'.
	}

	void insertAfter( const EventIterator& it, SynthEditEvent* e )
	{
		// setup new event.
		e->next_ = (*it)->next_;
		e->prev_ = *it;
		// correct next/previous event.
		e->next_->prev_ = e;
		(*it)->next_ = e;
	}
	EventIterator erase( const EventIterator& it )
	{
		SynthEditEvent* e = *it;
		e->prev_->next_ = e->next_;
		e->next_->prev_ = e->prev_;
		e->prev_ = 0; // flags 'not on list'.
		return EventIterator( e->next_ );
	}

	bool empty() const
	{
		return next_ == this;
	}

	void TransferFrom(EventList& other)
	{
		if( other.empty() )
		{
			prev_ = next_ = this;
		}
		else
		{
			next_ = other.next_;
			prev_ = other.prev_;
			next_->prev_ = this;
			prev_->next_ = this;
		}

		other.next_ = other.prev_ = &other;
	}
};

inline bool operator==( EventIterator const& lhs, EventIterator const& rhs )
{
	return lhs.equal(rhs);
}
inline bool operator!=( EventIterator const& lhs, EventIterator const& rhs )
{
	return !lhs.equal(rhs);
}

#define SKIP_LIST_DOUBLE_LINKED
#define SKIP_LIST_V2

class SkipListNode
{
public:
	// optimized for 1000 objects.
	static const int SKIPLIST_HEIGHT = 5;

	SkipListNode()
	{
		Prev(0) = nullptr;
	}

	bool isInList()
	{
		return Prev(0) != nullptr;
	}

#ifdef SKIP_LIST_V2
	inline EventProcessor*& Next(int level)
	{
		return next_[level];
	}
	inline EventProcessor*& Prev(int level)
	{
		return prev_[level];
	}

protected:
	EventProcessor* next_[SKIPLIST_HEIGHT];
	EventProcessor* prev_[SKIPLIST_HEIGHT];

#else

	inline EventProcessor*& Next(int level)
	{
		return Neighbors[level].next_;
	}
	inline EventProcessor*& Prev(int level)
	{
		return Neighbors[level].prev_;
	}
protected:
	struct NeighborPair
	{
		class EventProcessor* next_;
		class EventProcessor* prev_;
	};
	NeighborPair Neighbors[SKIPLIST_HEIGHT]; 
#endif

	friend class ModuleSkipList;
};

class EventProcessor : public dsp_msg_target
{
public:
	virtual void DoProcess( int /*buffer_offset*/, int /*sampleframes*/ ) {} // early in vtable to eke out any performance gain.
	virtual void HandleEvent(SynthEditEvent* /*e*/) {}

	EventProcessor();
	virtual ~EventProcessor();
	virtual void Wake();

	void UpdateCpu(int64_t nanosecondsElapsed)
#if defined( LOG_ALL_MODULES_CPU )
		;
	static std::ofstream logFileCsv;
#else
	{
		// New way.
		float fcpu = (float)nanosecondsElapsed;

		cpuRunningAverage += (fcpu - cpuRunningAverage) * 0.1f; // rough running average.
		cpuRunningMedian += copysignf(cpuRunningAverage * 0.005f, fcpu - cpuRunningMedian);

		cpuMeasuedCycles += cpuRunningMedian;
		cpuPeakCycles = (std::max)(cpuPeakCycles, fcpu);
	}
#endif

	void DiscardOldPinEvents( int pin_index, int datatype);
	void AddEvent(SynthEditEvent* p_event, int datatype = DT_NONE);
	void DeleteEvents();
	void SetClockRescheduleEvents(timestamp_t p_sampleclock);
	void RunDelayed( timestamp_t p_timestamp, ug_func p_function );
#ifdef _DEBUG
	void SetSampleClock(timestamp_t p_sample_clock);
	timestamp_t SampleClock() const;
#else // inline for speed
	void SetSampleClock(timestamp_t p_sample_clock)
	{
		private_sample_clock = p_sample_clock;
	}
	inline timestamp_t SampleClock() const
	{
		return private_sample_clock;
	}
#endif
	float getSampleRate();
	int getBlockPosition(); // sub position within current block
	inline class ISeAudioMaster* AudioMaster()
	{
		return m_audio_master;
	}
	void SetAudioMaster( class ISeAudioMaster* p_am)
	{
		m_audio_master = p_am;
	}

	EventList events;
	UgFlags flags;
	bool sleeping;

#ifdef _DEBUG
	// hacky attempt to cope with move of some functions from ug_base
	virtual void DebugIdentify() {}
	void CheckForOutputOverwrite() {}

	void DebugTimestamp();
	virtual void DebugPrintName(bool /*withHandle*/ = false) {}
	virtual void DebugPrintContainer(bool /*withHandle*/ = false) {}
#endif

#if defined(LOG_PIN_EVENTS )
	void LogEvents(std::ofstream& eventLogFile);
#endif

private:
	timestamp_t private_sample_clock;
	ISeAudioMaster* m_audio_master; // really just same as vsthost class??? combine?? !!

public:
	int SortOrder2 = -1;

	void SetFlag( int flag, bool setit = true )
	{
		if( setit )
		{
			flags = static_cast<UgFlags>( flags | flag );
		}
		else
		{
			flags = static_cast<UgFlags>( flags & (0xffffffff ^ flag ));
		}
#ifdef _DEBUG
		DebugOnSetFlag(flag, setit);
#endif
	}
	bool GetFlag( int flag ) const
	{
		return (flags & flag) != 0;
	}

#ifdef _DEBUG
	void DebugOnSetFlag(int flag, bool setit);
#endif
	int moduleContainerIndex = -1;

	// editor only
	float cpuRunningAverage;
	float cpuRunningMedian;
	float cpuMeasuedCycles;
	float cpuPeakCycles = 0.0f;
	std::unique_ptr<class UgDebugInfo> m_debugger;
};

#endif // !defined(AFX_EVENTPROCESSOR_H__555A8AA1_0CE1_11D5_B73D_00104B15CCF0__INCLUDED_)
