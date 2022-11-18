// UPlug.h
// Describes a UG's input or output variable. (What it's called and its address)
#pragma once

#include "InterfaceObject.h"
#include "se_types.h"

class ug_base;
class midi_output;

// An 'Active' input can't combine voices.
// eg osc freq, adding several note pitchs into an osc freq is not the same as
// feeding each pitch voltage into a seperate osc and summing the outputs.
// On the other hand feeding several voices into delay can be done either way
// you should set this on the input to any non-linear process eg clipping

enum UPlugFlags : int16_t
{
	PF_PP_NONE				= 0
	,PF_PP_ACTIVE			= (1 <<  1)
	,PF_PP_DOWNSTREAM		= (1 <<  2)
	,PF_PP_UPSTREAM			= (1 <<  3)
	// note that an output plug is on stat_change_list (save putting it on 2x)
	,PF_ADDER				= (1 <<  4)
	,PF_VALUE_CHANGED		= (1 <<  5)
	// PLUG is reponsible for deleting it's own buffers
	,PF_OWNS_BUFFER			= (1 <<  6)
	,PF_PATCH_STORE			= (1 <<  7)
	,PF_HOST_CONTROL		= (1 <<  8)
	,PF_BINARY_DATA			= (1 <<  9)
	, PF_PPW_POLYPHONIC		= (1 << 10) // Patch Param Watcher likes to flag polyphonic signals.
	, PF_PARAMETER_1_BLOCK_LATENCY = (1 << 11)
	, PF_POLYPHONIC_SOURCE	= (1 << 12)
	, PF_CV_HINT			= (1 << 13) // Container pin carrying CV signal (smoother filtering needed when oversampling)
};

// Attempt to get this class small.
#pragma pack(push,8)

class UPlug
{
public:
	void SetDefault(const char* utf8val);
	void SetDefault2(const char* utf8val);
	void SetDefaultDirect(const char* utf8val);

	void AssignBuffer(float* buffer);

	UPlug( ug_base* ug, UPlug* copy_plug);
	UPlug( ug_base* ug, EDirection dr, EPlugDataType dt);
	UPlug( ug_base* ug, InterfaceObject& pdesc );

	virtual ~UPlug();

	bool HasNonDefaultConnection();
	float getValue();
	state_type getState()
	{
		return state;
	}
	void setState(state_type p_state)
	{
		state = p_state;
	}
	inline bool isStreaming()
	{
		return state == ST_RUN;
	}
	inline void setStreamingA(bool isStreaming, timestamp_t p_clock) // SetStreaming. Absolute timestamp (not exactly equivalent SDK3's relative time).
	{
		if (isStreaming)
			TransmitState(p_clock, ST_RUN);
		else
			TransmitState(p_clock, ST_STATIC);
	}
	void TransmitState( timestamp_t p_clock, state_type p_stat );
	inline float* GetSamplePtr()
	{
		return m_buffer.buffer;
	}
	bool IsAdder();

	void debug_dump();
	void ReRoute();
	bool InUse() const
	{
		return !connections.empty();
	}
	void DisConnect(UPlug*);
	void DeleteConnectors();
	UPlug* GetTiedTo();
	inline int getPlugIndex() const
	{
		return plugIndex_;
	}
	void setIndex( int index )
	{
		plugIndex_ = static_cast<int16_t>(index);
	}
	// int UniqueId(){ return getPlugIndex(); }; // todo return actual ID (posible non-sequential)
	// NOTE: !! NOT UNIQUE ON AUTODUPLICATING PLUGS, THEY ALL GET SAME id!!!
	int UniqueId()
	{
		return uniqueId_;
	} // return actual ID (posible non-sequential)
	void setUniqueId(int uniqueId)
	{
		uniqueId_ = uniqueId;
	}// todo return actual ID (posible non-sequential)
	bool PPGetActiveFlag()
	{
		return (flags & PF_PP_ACTIVE) != 0;
	}
	bool GetPPDownstreamFlag()
	{
		return (flags & PF_PP_DOWNSTREAM) != 0;
	}
    struct FeedbackTrace* PPSetDownstream();
	void AssignVariable( void* p_io_variable, float** p_sample_ptr = 0 )
	{
		io_variable=p_io_variable;
		sample_ptr=p_sample_ptr;
	}
	void SetFlag( int flag )
	{
		flags = static_cast<UPlugFlags>( flags | flag );
	}
	void ClearFlag( int flag )
	{
		flags = static_cast<UPlugFlags>( flags & (0xffffffff ^ flag ));
	}
	inline bool GetFlag(int flag)
	{
		return (flags & flag) != 0;
	}
	void Transmit(timestamp_t timestamp, int32_t data_size, const void* data);
	void TransmitPolyphonic(timestamp_t timestamp, int physicalVoiceNumber, int32_t data_size, const void* data);

	ug_base* UG;
	void* io_variable;

	// TODO: BIG MESS. currently using 'io_variable' to store pointer to sample-buffer, and 'sample_ptr' to store pointer to module's float*
	// SHOULD BE: io_variable left as-is to point to float*, m_buffer.sample_buffer to float* when module dosnt provide, sample-buffer pointer stored elsewhere (global array or something)
	float** sample_ptr;
	std::vector<UPlug*> connections;
	void DeleteConnection(UPlug* other);

	// attach appropriate buffer (MIDI/Audio in/out whatever), anything larger than 4 bytes is allocated
	union
	{
//		USampBlock* sample_buffer;
		float* buffer;
		float* float_ptr;
		midi_output* midi_out;
		std::wstring* text_ptr; // in or out
		std::string* text_utf8_ptr; // in or out
		void* generic_pointer;
		short enum_ob; // in or out
		float float_ob; // in or out
		int int_ob; // in or out
		double* double_ptr; // in or out
		bool bool_ob; // in or out
	} m_buffer = {};

	UPlug* TiedTo;
	UPlug* Proxy;   // Shortcut to plug final destination

public:
	void CreateBuffer();
	void DestroyBuffer();
	void SetBufferValue( const char* p_val );
	void CloneBufferValue( const UPlug& clonePlug );

protected:
	timestamp_t m_last_one_off_time;

private:
	int uniqueId_; // not unique on autoduplicating plugs. hence not much use.

	// Smaller datatypes after here to pack them together.
	int16_t plugIndex_;

protected:
	state_type state;		// Is it active or dormant. depreciated with SDK3

public:
	EDirection Direction;
	EPlugDataType DataType;  // type of io_var {IOT_INPUT, IOT_OUTPUT, IOT_IN_ARRAY}
	UPlugFlags flags;

#if defined( _DEBUG )
	// need to ensure all modules send status on all pins at startup (since V1.1)
	bool debug_sent_status;
#endif

	bool CheckAllSame();

	void SetRange(int from_idx, int count, float value)
	{
//		assert(from_idx >= 0 && from_idx < debugBlocksize_);
//		assert(from_idx + count <= debugBlocksize_);
		assert(count > 0);

		for (int b = from_idx; b < from_idx + count; b++)
		{
			m_buffer.buffer[b] = value;
		}
	}
};

#pragma pack(pop)
