// Copyright 2006 Jeff McClintock

#ifndef waveshaper_dsp_H_INCLUDED
#define waveshaper_dsp_H_INCLUDED

#include "../se_sdk3/mp_sdk_audio.h"

#define WS_NODE_COUNT 11

class Waveshaper: public MpBase
{
public:
	Waveshaper(IMpUnknown* host);

	// overrides
	virtual int32_t MP_STDCALL open();

	// methods
	void subProcess(int bufferOffset, int sampleFrames);
	void onSetPins();

protected:
	// pins_
	AudioInPin pinInput;
	AudioOutPin pinOutput;
	StringInPin pinShape;

	float* m_lookup_table;
	static const int TABLE_SIZE = 512;
	virtual void FillLookupTable() = 0;

private:
	bool CreateSharedLookup(wchar_t* p_name, void** p_returnPointer, float p_sampleRate, int p_size);
	void SetupLookupTable();
	void onLookupTableChanged();
};

class Waveshaper2b : public Waveshaper
{
public:
    Waveshaper2b(IMpUnknown* host) : Waveshaper(host) {}

protected:
    virtual void FillLookupTable();
};
#endif