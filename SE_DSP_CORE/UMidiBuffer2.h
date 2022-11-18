#if !defined(AFX_UMidiBuffer3_H__11A04A61_C642_11D4_B6F6_00104B15CCF0__INCLUDED_)
#define AFX_UMidiBuffer3_H__11A04A61_C642_11D4_B6F6_00104B15CCF0__INCLUDED_

#pragma once

#include "MidiEvent.h"
#include "se_types.h"
#include "ug_base.h"

struct MidiEvent3
{
	timestamp_t timestamp;
	int size;
	unsigned char data[10]; // 10 is just token value.
};

class MidiBuffer3
{
	static const int nullentry = -9999;
    enum { BUFFER_SIZE = 1024}; // * 4 bytes. power-or-two please.
	static const int bufferwrapper = BUFFER_SIZE-1;

	std::vector<int32_t> buffer;
	size_t m_read_pos;
	size_t m_write_pos;

	inline void UpdateReadPosImp()
	{
		auto e = (MidiEvent3*)&buffer[m_read_pos];
		auto totalSize = (sizeof(e->timestamp) + sizeof(e->size) + e->size + sizeof(int32_t) - 1) / sizeof(int32_t);

		m_read_pos = (m_read_pos + totalSize) & bufferwrapper;
	}

public:
	MidiBuffer3() :
		m_read_pos(0)
		,m_write_pos(0)
	{
		buffer.assign(BUFFER_SIZE, 0);
	}

	void Clear()
	{
		m_read_pos = m_write_pos = 0;
	}

	inline void Add(timestamp_t timestamp, const unsigned char* data, int size)
	{
		assert(m_write_pos >= 0 && m_write_pos < buffer.size());
		const size_t sizeof_header = sizeof(timestamp) + sizeof(int32_t);

		if (size + sizeof_header > FreeSpace() * sizeof(int32_t))
		{
			_RPT0(_CRT_WARN, "OVERLOAD on MIDI input!!!!\n");
			return;
		}

		// check if enough space before end of buffer.
		auto e = (MidiEvent3*)&buffer[m_write_pos];
		auto totalSize = (sizeof(e->timestamp) + sizeof(e->size) + size + sizeof(int32_t) - 1) / sizeof(int32_t);
		if (m_write_pos + totalSize > buffer.size())
		{
			// skip remainder of buffer and wrap.
			Add(nullentry, reinterpret_cast<const unsigned char*>( buffer.data() ), (int) (sizeof(int32_t) * (buffer.size() - m_write_pos) - sizeof_header));

			Add(timestamp, data, size);
			return;
		}

		e->timestamp = timestamp;
		e->size = size;

		auto dest = e->data;
		for (int i = 0; i < size; i++)
		{
			*dest++ = data[i];
		}

		// commit.
        m_write_pos = (m_write_pos + totalSize) & bufferwrapper;
	}

	inline bool IsEmpty()
	{
		auto isempty = (m_read_pos == m_write_pos);
		if (!isempty)
		{
			auto e = (MidiEvent3*)&buffer[m_read_pos];

			if (e->timestamp == nullentry)
			{
				UpdateReadPosImp();
				return IsEmpty();
			}
		}

		return isempty;
	}

	inline timestamp_t PeekNextTimestamp()
	{
		if (IsEmpty())
		{
			return 0;
		}

		return *(timestamp_t*)&buffer[m_read_pos];
	}

	inline MidiEvent3* Current()
	{
		assert(!IsEmpty());
		auto e = (MidiEvent3*)&buffer[m_read_pos];

		if (e->timestamp == nullentry)
		{
			UpdateReadPosImp();
			assert(!IsEmpty());
			return Current();
		}
		return e;
	}

	inline void UpdateReadPos()
	{
		assert(!IsEmpty());
		UpdateReadPosImp();
	}

	inline size_t FreeSpace()
	{
		if (m_read_pos > m_write_pos)
		{
			return m_read_pos - m_write_pos - 1;
		}

		return m_read_pos + buffer.size() - m_write_pos - 1;
	}
};

class midi_output
{
public:
	midi_output() : m_plug(0) {}

	void Send(timestamp_t timestamp, const void* data1, int size1)
	{
		if (m_plug) // NULL if not connected
		{
			for (auto to : m_plug->connections)
			{
				to->UG->SetPinValue(timestamp, to->getPlugIndex(), DT_MIDI, data1, size1);
			}
		}
	}
	void Send(timestamp_t timestamp, const void* data1, size_t size1)
	{
		if (m_plug) // NULL if not connected
		{
			for (auto to : m_plug->connections)
			{
				to->UG->SetPinValue(timestamp, to->getPlugIndex(), DT_MIDI, data1, static_cast<int>(size1));
			}
		}
	}
	void SetUp( class UPlug* p_plug )
	{
		m_plug = p_plug;
	}

protected:
	class UPlug* m_plug;
};

#endif // !defined(AFX_UMidiBuffer3_H__11A04A61_C642_11D4_B6F6_00104B15CCF0__INCLUDED_)
