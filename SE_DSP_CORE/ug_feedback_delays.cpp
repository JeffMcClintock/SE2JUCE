#include "pch.h"
#include "ug_feedback_delays.h"
#include "ug_container.h"
#include "module_register.h"
#include "./cancellation.h"
#include "SeAudioMaster.h"
#include "modules/shared/xp_simd.h"

SE_DECLARE_INIT_STATIC_FILE(ug_feedback_delays)

#define PN_IN1 0
#define PN_OUT1 1
#define PN_OUT2 2

namespace
{
REGISTER_MODULE_1(L"Feedback Delay", L"Feedback - Volts",
	L"Special/Feedback", ug_feedback_delay,
	CF_STRUCTURE_VIEW | CF_IS_FEEDBACK,
	L"");
}

// Fill an array of InterfaceObjects with plugs and parameters
void ug_feedback_delay::ListInterface2(InterfaceObjectArray &PList)
{
	// IO Var, Direction, Datatype, Name, Default, defid (index into
	// unit_gen::PlugFormats)
	LIST_PIN2(L"Audio In", input1_ptr, DR_IN, L"0", L"", IO_LINEAR_INPUT, L"");
	LIST_PIN2(L"Audio Out", output1_ptr, DR_OUT, L"5", L"", 0, L"");
	LIST_VAR3(L"Delay Time Out", feedbackLatencyMs, DR_OUT, DT_FLOAT, L"", L"", 0,
		L"The delay in milliseconds that this module adds to the signal "
		L"(OUTPUT ONLY).");

	// Dummy pins
	LIST_VAR3N(L"", DR_OUT, DT_MIDI, L"", L"", IO_HIDE_PIN, L"");
}

ug_feedback_delay::ug_feedback_delay()
	: feedback_out(0), m_last_output_stat(ST_STOP)
	, m_last_output_value(-123456.0f)   // unlikely value
{
}

void ug_feedback_delay::BuildHelperModule()
{
	ug_feedback_delay_out *generator2 = new ug_feedback_delay_out();
	generator2->setModuleType(moduleType);
	parent_container->AddUG(generator2);
	generator2->SetupWithoutCug();

	feedback_out = generator2;
	generator2->feedback_in = this;

	// Connect dummy connection to feedback-out so it is sorted downstream whenever possible (i.e. to remove unneeded delays).
	// this connection will be severed to create a delayed feedback path, but only if necessary.
	connect(GetPlug(3), generator2->GetPlug(0) ); // Dummy pins.

	// now proxy "Audio out" to new ug's "Audio out"
	GetPlug(1)->Proxy = generator2->GetPlug(1); // "Audio Out"
	GetPlug(2)->Proxy = generator2->GetPlug(2); // "Delay Time Out"
	generator2->cpuParent = cpuParent;

	AudioMaster2()->RegisterBypassableModule(this);
}

void ug_feedback_delay::BypassRedundentModules(int voice)
{
	BypassFeedbackModule(this, voice);
}

void BypassFeedbackModule(ug_base* u, int voice)
{
	auto dummyPin = u->plugs.back();
	if (dummyPin->connections.empty())
	{
		return;
	}

	// Feedback functionality not needed. Delete module.
	// Bypass signal in -> signal-out
	auto helperModule = dummyPin->connections[PN_IN1]->UG;
	auto moduleInputPin = u->plugs[PN_IN1];

	// remove connection to the feedback_in
	std::vector<UPlug*> fromPins(moduleInputPin->connections);
	for (auto fromPin : fromPins)
	{
	auto it = std::find(fromPin->connections.begin(), fromPin->connections.end(), moduleInputPin);
	fromPin->connections.erase(it);
	}

	{
		auto outgoingPin = helperModule->plugs[PN_OUT1];
		for (auto toPin : outgoingPin->connections)
		{
			auto it = std::find(toPin->connections.begin(), toPin->connections.end(), outgoingPin);

			// erase connection from feedback_out => some_module
			toPin->connections.erase(it);

			// connect the upstream module/s directly
			for (auto fromPin : fromPins)
			{
			u->connect(fromPin, toPin);
		}
		}
		outgoingPin->connections.clear();
	}

	{
		auto outgoingPin = helperModule->plugs[PN_OUT2];
		for (auto toPin : outgoingPin->connections)
		{
			auto it = std::find(toPin->connections.begin(), toPin->connections.end(), outgoingPin);
			toPin->connections.erase(it);

			if (toPin->connections.empty())
			{
				toPin->SetDefault("0"); // set destination pin to zero ms
			}
		}
		outgoingPin->connections.clear();
	}

	// remove and delete modules.
	u->parent_container->at(voice)->RemoveUG(u);
	helperModule->parent_container->at(voice)->RemoveUG(helperModule);

	u->AudioMaster()->UnRegisterDspMsgHandle(u->Handle());
	helperModule->AudioMaster()->UnRegisterDspMsgHandle(helperModule->Handle());

	u->AudioMaster2()->UnRegisterPin(u->plugs[1]);
	u->AudioMaster2()->UnRegisterPin(helperModule->plugs[1]);

	delete u;
	delete helperModule;
}

void ug_feedback_delay::sub_process(int start_pos, int sampleframes)
{
	const float *signal = input1_ptr + start_pos;
	float *output = feedback_out->output1_ptr + start_pos;

#ifdef CANCELLATION_TEST_ENABLE

	for( int s = sampleframes; s > 0; s-- )
	{
		*output++ = 0;
	}

#else

#if !GMPI_USE_SSE
    for (int s = sampleframes; s > 0; s--)
    {
        *output++ = *signal++;
    }
#else
    
#if 1

	// process fiddly non-sse-aligned prequel.
	while (sampleframes > 0 && reinterpret_cast<intptr_t>(output) & 0x0f)
	{
		*output++ = *signal++;
		--sampleframes;
	}

	// SSE intrinsics. 4 samples at a time.
	__m128* pIn1 = ( __m128* ) signal;
	__m128* pDest = ( __m128* ) output;

	while( sampleframes > 0 )
	{
		*pDest++ = *pIn1++;
		sampleframes -= 4;
	}

#else

// OK , but same CPU. (8 at a  time MORE CPU!)
	union pointertype
	{
		float* float_ptr;
		__m128* sse_ptr;
	};

	pointertype signal, output;
	signal.float_ptr = input1_ptr + start_pos;
	output.float_ptr = feedback_out->output1_ptr + start_pos;

	// process fiddly non-sse-aligned prequel.
	while( ( (int)output.float_ptr ) & 0x1f )
	{
		*output.float_ptr++ = *signal.float_ptr++;
		--sampleframes;
	}

	__m128* outputB = output.sse_ptr + 1;
	__m128* signalB = signal.sse_ptr + 1;

	// SSE intrinsics. 8 samples at a time.
	while( sampleframes > 0 )
	{
		*output.sse_ptr = *signal.sse_ptr;
		signal.sse_ptr += 2;
		output.sse_ptr += 2;
		*outputB = *signalB;
		signalB += 2;
		outputB += 2;
		sampleframes -= 8;
	}

#endif // SSE 8-samples at a time
#endif // GMPI_USE_SSE
#endif // CANCELLATION_TEST_ENABLE
}

void ug_feedback_delay::onSetPin(timestamp_t p_clock, UPlug* /* p_to_plug */, state_type p_state)
{
	// Feedback modules tends to circulate unnesc ST_STATIC indefinatly, wasting
	// CPU. Avoid that.
	bool sendStatusChange = true;

	if( p_state != ST_RUN )
	{
		float output_value = GetPlug(PN_IN1)->getValue();

		sendStatusChange = m_last_output_stat == ST_RUN || output_value != m_last_output_value;

		m_last_output_value = output_value;
	}

	if (sendStatusChange)
	{
		int relativeTime;
		if (feedbackLatency == 0)
		{
			// !!! receiver needs to ignore block pos in 0 latency case
			relativeTime = static_cast<int>(p_clock - AudioMaster()->BlockStartClock());
		}
		else
		{
			auto delayedTimestamp = p_clock + feedbackLatency;
			relativeTime = static_cast<int>(delayedTimestamp - end_time); // end_time <> feedback_out . next block start clock.
		}

		feedback_out->StatusChangeFromFeedbackIn(relativeTime, p_state);
		//			_RPT1(_CRT_WARN, "STOP[%d]>\n", static_cast<int>(p_clock));
	}

	m_last_output_stat = p_state;

	if( p_state == ST_RUN )
	{
		SET_CUR_FUNC(&ug_feedback_delay::sub_process);
	}
	else
	{
		ResetStaticOutput();
		SET_CUR_FUNC(&ug_feedback_delay::sub_process_static);
	}
}

void ug_feedback_delay::sub_process_static(int start_pos, int sampleframes)
{
#ifdef _DEBUG
	float *signal = input1_ptr + start_pos;
	for (int s = sampleframes; s > 0; s--)
	{
		assert( *signal++ == input1_ptr[start_pos]); // Module before this one has a staus problem!
	}
#endif

	sub_process(start_pos, sampleframes);
	SleepIfOutputStatic(sampleframes);
}

int ug_feedback_delay::Open()
{
	ug_midi_device::Open();
	SET_CUR_FUNC(&ug_feedback_delay::sub_process);
	// to satisfy debug code
	OutputChange(0, GetPlug(PN_OUT1), ST_STATIC);

	// Only add latency to events if help module is actually upstream.
	assert(feedback_out->SortOrder2 < SortOrder2); // check unneeded ones removed from patch.
	feedbackLatency = feedback_out->SortOrder2 < SortOrder2 ? AudioMaster()->BlockSize() : 0;

	feedbackLatencyMs = 1000.f * feedbackLatency / getSampleRate();
	RUN_AT(SampleClock(), &ug_feedback_delay::OnFirstSample);

	return 0;
}

void ug_feedback_delay::OnFirstSample()
{
	SendPinsCurrentValue(SampleClock(), GetPlug(PN_OUT2)); // not used, but prevent assert.
}

void ug_feedback_delay_out::OnFirstSample()
{
	if (GetPlug(PN_IN1)->connections.empty()) // dummy connection still intact?
	{
		feedbackLatencyMs = 1000.f * AudioMaster()->BlockSize() / getSampleRate();
	}

	SendPinsCurrentValue(SampleClock(), GetPlug(PN_OUT2));
}

struct FeedbackTrace *ug_feedback_delay::PPSetDownstream()
{
	ug_base::PPSetDownstream();
	auto ret = feedback_out->PPSetDownstream();

	if(feedback_out->GetFlag(UGF_PP_TERMINATED_PATH))
	{
		SetFlag(UGF_PP_TERMINATED_PATH);
	}

	return ret;
}

void ug_feedback_delay::PPPropogateVoiceNum(int n)
{
	ug_base::PPPropogateVoiceNum(n);
	feedback_out->PPPropogateVoiceNum(n);
}

ug_base* ug_feedback_delay::Clone(CUGLookupList& UGLookupList)
{
	auto clone = (ug_feedback_delay*) ug_base::Clone(UGLookupList);

	// attempt to find feedback in's clone.
	// depending on order things are cloned, may not be available yet.. (see also ug_feedback_delay_out::Clone() )
	auto clones_other_half =
		dynamic_cast<ug_feedback_delay_out*>(UGLookupList.LookupPolyphonic(feedback_out));

	if (clones_other_half)
	{
		clone->feedback_out = clones_other_half;
		clones_other_half->feedback_in = clone;
	}

	return clone;
}

ug_base* ug_feedback_delay_out::Clone(CUGLookupList& UGLookupList)
{
	auto clone = (ug_feedback_delay_out*) ug_base::Clone(UGLookupList);

	// attempt to find feedback in's clone.
	// depending on order things are cloned, may not be available yet.. (see also ug_feedback_delay_out::Clone() )
	auto clones_other_half =
		dynamic_cast<ug_feedback_delay*>(UGLookupList.LookupPolyphonic(feedback_in));

	if (clones_other_half)
	{
		clone->feedback_in = clones_other_half;
		clones_other_half->feedback_out = clone;
	}

	return clone;
}

///////////////////////////////////////////////////////////////
void ug_feedback_delay_out::ListInterface2(InterfaceObjectArray &PList)
{
	// first one non-funcional except for dummy connection to mate.
	LIST_VAR3N(L"", DR_IN, DT_MIDI, L"0", L"", 0, L"");
	LIST_PIN2(L"Audio Out", output1_ptr, DR_OUT, L"5", L"", 0, L"");
	/*
		LIST_PIN2(L"Delay Time Out", output2_ptr, DR_OUT, L"5", L"", 0,
				  L"The delay in milliseconds that this module adds to the signal "
				  L"(OUTPUT ONLY).");
	*/
	LIST_VAR3(L"Delay Time Out", feedbackLatencyMs, DR_OUT, DT_FLOAT, L"", L"", 0, L"");

	// Dummy pins
	LIST_VAR3N(L"", DR_OUT, DT_MIDI, L"", L"", IO_HIDE_PIN, L"");
}

ug_feedback_delay_out::ug_feedback_delay_out() : feedback_in(0)
{
#ifdef _DEBUG
	SetFlag(
	    UGF_DONT_CHECK_BUFFERS); // THIS MODULE SLEEPS REGARDLESS of buffer state
#endif
}

int ug_feedback_delay_out::Open()
{
#if 0
	// Setup pointers to mates.
	if (pp_voice_num == 0)
	{
		auto to = this; // only voice1 has pointer to helper (initially).
		auto from = to->feedback_in;

		while (to)
		{
			to->feedback_in = from;
			from->feedback_out = to;

			from = (ug_feedback_delay*) from->m_next_clone;
			to = (ug_feedback_delay_out*) to->m_next_clone;
		}
	}
#endif

	ug_base::Open();

	OutputChange(0, GetPlug(PN_OUT1), ST_STATIC);
	RUN_AT(SampleClock(), &ug_feedback_delay_out::OnFirstSample);

	SET_CUR_FUNC(&ug_feedback_delay_out::sub_process);
	return 0;
}

void ug_feedback_delay_out::StatusChangeFromFeedbackIn(int timeDelta, state_type p_state)
{
	Wake();

	// add status to queue
	statusQueue.push_back(std::pair<int, state_type>(timeDelta, p_state));
}

void ug_feedback_delay_out::sub_process(int start_pos, int sampleframes)
{
	int eraseCount = 0;
	for (auto& stat : statusQueue)
	{
		if (stat.first < sampleframes)
		{
			timestamp_t clock = AudioMaster()->BlockStartClock() + start_pos + stat.first;
			OutputChange(clock, GetPlug(PN_OUT1), stat.second);
//			_RPT2(_CRT_WARN, "STAT[%d](%d)>\n", static_cast<int>(clock - feedbackLatency), stat.second);
			++eraseCount;
		}
		else
		{
			stat.first -= sampleframes;
		}
	}

	if (eraseCount)
	{
		statusQueue.erase(statusQueue.begin(), statusQueue.begin() + eraseCount);
	}

	if (statusQueue.empty())
		SleepMode();
}

void ug_feedback_delay_out::PPSetUpstream()
{
	ug_base::PPSetUpstream();
	feedback_in->PPSetUpstream();
}

void ug_feedback_delay_out::PPPropogateVoiceNumUp(int n)
{
	ug_base::PPPropogateVoiceNumUp(n);
	feedback_in->PPPropogateVoiceNumUp(n);
}

///////////////////////////////////////////////////////////
typedef ug_feedback_delay_base<DT_FLOAT, DR_IN> ug_feedback_delay_float;
typedef ug_feedback_delay_base<DT_INT, DR_IN> ug_feedback_delay_int;
typedef ug_feedback_delay_base<DT_MIDI, DR_IN> ug_feedback_delay_midi;

typedef ug_feedback_delay_base<DT_TEXT, DR_IN> ug_feedback_delay_text;
typedef ug_feedback_delay_base<DT_DOUBLE, DR_IN> ug_feedback_delay_double;
typedef ug_feedback_delay_base<DT_BOOL, DR_IN> ug_feedback_delay_bool;
typedef ug_feedback_delay_base<DT_INT64, DR_IN> ug_feedback_delay_int64;
typedef ug_feedback_delay_base<DT_BLOB, DR_IN> ug_feedback_delay_blob;
typedef ug_feedback_delay_base<DT_ENUM, DR_IN> ug_feedback_delay_enum;

namespace
{
REGISTER_MODULE_1(L"Feedback Delay - MIDI", L"Feedback - MIDI",
	L"Special/Feedback", ug_feedback_delay_midi,
	CF_STRUCTURE_VIEW | CF_IS_FEEDBACK,
	L"");

REGISTER_MODULE_1(L"SE Feedback Delay - Float", L"Feedback - Float",
	L"Special/Feedback", ug_feedback_delay_float,
	CF_STRUCTURE_VIEW | CF_IS_FEEDBACK,
	L"");

REGISTER_MODULE_1(L"SE Feedback Delay - Int", L"Feedback - Int",
	L"Special/Feedback", ug_feedback_delay_int,
	CF_STRUCTURE_VIEW | CF_IS_FEEDBACK,
	L"");

REGISTER_MODULE_1(L"SE Feedback Delay - Text", L"Feedback - Text",
	L"Special/Feedback", ug_feedback_delay_text,
	CF_STRUCTURE_VIEW | CF_IS_FEEDBACK,
	L"");

REGISTER_MODULE_1(L"SE Feedback Delay - Double", L"Feedback - Double",
	L"Special/Feedback", ug_feedback_delay_double,
	CF_STRUCTURE_VIEW | CF_IS_FEEDBACK,
	L"");

REGISTER_MODULE_1(L"SE Feedback Delay - Bool", L"Feedback - Bool",
	L"Special/Feedback", ug_feedback_delay_bool,
	CF_STRUCTURE_VIEW | CF_IS_FEEDBACK,
	L"");

REGISTER_MODULE_1(L"SE Feedback Delay - Int64", L"Feedback - Int64",
	L"Special/Feedback", ug_feedback_delay_int64,
	CF_STRUCTURE_VIEW | CF_IS_FEEDBACK,
	L"");

REGISTER_MODULE_1(L"SE Feedback Delay - Blob", L"Feedback - Blob",
	L"Special/Feedback", ug_feedback_delay_blob,
	CF_STRUCTURE_VIEW | CF_IS_FEEDBACK,
	L"");
}
