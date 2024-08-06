#pragma once

class my_input_stream;

// basis of DSP module, receives messages from GUI and Document
class dsp_msg_target
{
public:
	dsp_msg_target() {}

	virtual ~dsp_msg_target() {}

	int Handle() const
	{
		return m_handle;
	}

	void SetHandle(int p_handle)
	{
		m_handle=p_handle;
	}

	virtual void OnUiMsg(int /* p_msg_id */, my_input_stream& /* p_stream */) {}

protected:
	int m_handle;
};

