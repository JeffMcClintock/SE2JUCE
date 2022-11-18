#include "pch.h"
#include "ug_voice_splitter.h"

#include "SeAudioMaster.h"
#include "module_register.h"
#include "modules/shared/xp_simd.h"
#include "modules/se_sdk3/mp_sdk_audio.h"

SE_DECLARE_INIT_STATIC_FILE(ug_voice_splitter);

namespace
{
	REGISTER_MODULE_1(L"VoiceSplitter", IDS_MN_VOICE_SPLITTER, IDS_MG_SPECIAL, ug_voice_splitter, CF_STRUCTURE_VIEW, L"");
}

ug_voice_splitter::ug_voice_splitter() :
	voice_(-1)
	, current_output_plug(NULL)
	, current_output_plug_number(-2)
{
	SET_CUR_FUNC(&ug_voice_splitter::sub_process);

	// Prevent downstream modules being polyphonic.
	SetFlag(UGF_POLYPHONIC_DOWNSTREAM|UGF_VOICE_SPLITTER);
}

#define PLG_IN		0
#define PLG_OUT1	1

void ug_voice_splitter::ListInterface2(InterfaceObjectArray& PList)
{
	// IO Var, Direction, Datatype, Name, Default, defid (index into ug_base::PlugFormats)
	LIST_PIN2(L"Input", in_ptr, DR_IN, L"0", L"", 0, L"");
	LIST_PIN(L"Output", DR_OUT, L"", L"", IO_AUTODUPLICATE | IO_CUSTOMISABLE, L"");
}

void ug_voice_splitter::onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state)
{
	//	_RPT2(_CRT_WARN, "ug_voice_splitter::onSetPin v=%d clock=%d\n", voice_, p_clock );
	if (current_output_plug == 0) // then leave output alone, it's not mine.
		return;

	assert(p_to_plug->getPlugIndex() == PLG_IN);
	OutputChange(p_clock, current_output_plug, p_state);

	if (p_state == ST_RUN)
	{
		SET_CUR_FUNC(&ug_voice_splitter::sub_process);
	}
	else
	{
		SET_CUR_FUNC(&ug_voice_splitter::sub_process_static);
		ResetStaticOutput();
	}
}

void ug_voice_splitter::sub_process(int start_pos, int sampleframes)
{
	float* in1 = in_ptr + start_pos;
	float* out = out_ptr + start_pos;

	for (int s = sampleframes; s > 0; s--)
	{
		*out++ = *in1++;
	}
}

void ug_voice_splitter::sub_process_static(int start_pos, int sampleframes)
{
	//	sub_process2( start_pos, sampleframes );
	sub_process(start_pos, sampleframes);
	SleepIfOutputStatic(sampleframes);
}

int ug_voice_splitter::Open()
{
	ug_base::Open();
	numOutputs = GetPlugCount() - 1;

	if (pp_voice_num < 1) // monophonic. 0 or -1
	{
		voice_ = 0;
	}
	else
	{
		voice_ = pp_voice_num - 1; // skip voice 0 which is monophonic modules only.
	}

	assert(voice_ == (std::max)(0, pp_voice_num - 1)); // skip voice 0 which is monophonic modules only.

#if 0 //defined( _DEBUG )
	it_ug_clones itr(CloneOf());

	int v = 0;

	for (itr.First(); !itr.IsDone(); itr.Next())
	{
		if (itr.CurrentItem() == this)
		{
			assert(voice_ == v);
			break;
		}

		++v;
	}
#endif

	SET_CUR_FUNC(&ug_base::process_sleep);
	OnNewSetting();

	return 0;
}

void ug_voice_splitter::OnNewSetting()
{
	if (numOutputs < 1)
		return;

	// constrain output number to legal range
	int output_number = voice_;

	//output_number = min( output_number, numOutputs - 1 );
	if (output_number >= numOutputs)
	{
		output_number = -1; // hidden output.
	}
	else
	{
		current_output_plug_number = output_number + PLG_OUT1;
		current_output_plug = GetPlug(current_output_plug_number);
		out_ptr = current_output_plug->GetSamplePtr();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace gmpi;

namespace {

	// see also "SE Poly to Mono" (ug_adder2.cpp)
	int32_t r = RegisterPluginXml(
		R"XML(
<?xml version="1.0" ?>
<PluginList>
  <Plugin id="SE Poly to MonoA" name="Poly to Mono helper" category="Debug" >
    <Audio>
		<Pin name="Input" datatype="float" rate="audio" linearInput="false" />
		<Pin name="LastVoice" datatype="midi" direction="out" private="true" />
		<Pin name="VoiceActive" hostConnect="Voice/Active" datatype="float" isPolyphonic="true" />
    </Audio>
  </Plugin>
</PluginList>
)XML"
);
}

class PolyToMonoA : public MpBase2
{
	AudioInPin pinInput;
	FloatInPin pinVoiceActive;
	MidiOutPin pinLastVoice;

	int m_voice_number;
	bool activeState = false;

public:
	PolyToMonoA()
	{
		initializePin(pinInput);
		initializePin(pinLastVoice);
		initializePin(pinVoiceActive);
	}

	int32_t MP_STDCALL setHost(IMpUnknown* phost) override
	{
		// Manually set flag not supported by XML.
		auto ug = dynamic_cast<ug_base*>(phost);
		ug->SetFlag(UGF_HAS_HELPER_MODULE); // could be done in XML?

		return MpBase2::setHost(phost);
	}

	int32_t MP_STDCALL open() override
	{
		MpBase2::open();	// always call the base class

		// Determine voice number.
		m_voice_number = 0;
		gmpi_sdk::mp_shared_ptr<gmpi::IMpUnknown> com_object;
		int32_t res = getHost()->createCloneIterator(com_object.asIMpUnknownPtr());

		gmpi_sdk::mp_shared_ptr<gmpi::IMpCloneIterator> cloneIterator;
		res = com_object->queryInterface(gmpi::MP_IID_CLONE_ITERATOR, cloneIterator.asIMpUnknownPtr());

		gmpi::IMpUnknown* object = nullptr;
		cloneIterator->first();
		while (cloneIterator->next(&object) == gmpi::MP_OK)
		{
			auto clone = dynamic_cast<PolyToMonoA*>(object);
			if (clone == this)
			{
				break;
			}
			++m_voice_number;
		}

		setSubProcess(&PolyToMonoA::subProcessNothing);

		return gmpi::MP_OK;
	}

	void onSetPins() override
	{
		if (pinVoiceActive.isUpdated())
		{
			bool newActiveState = pinVoiceActive > 0.0f;

			if (newActiveState != activeState) // filter out voice refresh and other irrelevant updates.
			{
				activeState = newActiveState;
				//			_RPTW2(_CRT_WARN, L"V%d  ACTIVE %f)\n", m_voice_number, pinVoiceActive.getValue());

				// Notify helper of voice status
				unsigned char midiMessage2[] = { 0xF0, 0x7f, 0x7f };
				midiMessage2[1] = static_cast<unsigned char>(m_voice_number);
				midiMessage2[2] = (unsigned char) activeState;
				pinLastVoice.send(midiMessage2, sizeof(midiMessage2));
			}
		}
	}
};

namespace
{
	auto r2 = Register<PolyToMonoA>::withId(L"SE Poly to MonoA");
}
