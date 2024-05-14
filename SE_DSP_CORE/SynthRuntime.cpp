#include <string>
#include <fstream>
#include <math.h>
#include "SynthRuntime.h"
#include "my_msg_que_input_stream.h"
#include "QueClient.h"
#include "./SeAudioMaster.h"
#include "./midi_defs.h"
#include "ug_patch_automator.h"
#include "./UniqueSnowflake.h"
#include "./my_msg_que_output_stream.h"
#include "module_info.h"
#include "BundleInfo.h"
#include "./modules/se_sdk3_hosting/Controller.h"
#include "UgDatabase.h"

using namespace std;

SynthRuntime::SynthRuntime() :
	usingTempo_(false),
    generator(nullptr),
    reinitializeFlag(false)
{
}

void SynthRuntime::prepareToPlay(
	IShellServices* shell,
	int32_t sampleRate,
	int32_t maxBlockSize,
	bool runsRealtime)
{
	shell_ = shell;

	// this can be called multiple times, e.g. when performing an offline bounce.
	// But we need to rebuild the DSP graph from scratch only when something fundamental changes.
	// e.g. sample-rate, block-size, polyphony, latency compensation, patch cables.

	const bool mustReinitilize =
		generator == nullptr ||
		generator->SampleRate() != sampleRate ||
		generator->BlockSize() != generator->CalcBlockSize(maxBlockSize) ||
		(!runsRealtime && runsRealtimeCurrent) ||	// i.e. we switched *to* an offline render. need to ensure cancellation is consistant.
		reinitializeFlag;

	reinitializeFlag = false;

	if (mustReinitilize)
	{
		ModuleFactory()->RegisterExternalPluginsXmlOnce(nullptr);

		// cache xml document to save time re-parsing on every restart.
		if (!currentDspXml.RootElement())
		{
			auto bundleinfo = BundleInfo::instance();
			auto dspXml = bundleinfo->getResource("dsp.se.xml");
			if (!dspXml.empty() && '<' != dspXml[0])
			{
				Scramble(dspXml);
			}

			currentDspXml.Parse(dspXml.c_str());
			if (currentDspXml.Error())
			{
#if defined( SE_EDIT_SUPPORT )
				std::wostringstream oss;
				oss << L"Module XML Error: [SynthEdit.exe]" << currentDspXml.ErrorDesc() << L"." << currentDspXml.Value();
				getShell()->SeMessageBox(oss.str().c_str(), L"", MB_OK | MB_ICONSTOP);
#else
#if defined (_DEBUG)
				// Get error information and break.
				TiXmlNode* e = currentDspXml.FirstChildElement();
				while (e)
				{
					_RPT1(_CRT_WARN, "%s\n", e->Value());
					TiXmlElement* pElem = e->FirstChildElement("From");
					if (pElem)
					{
						const char* from = pElem->Value();
					}
					e = e->LastChild();
				}
				assert(false);
#endif
#endif
			}
		}

		std::lock_guard<std::mutex> x(generatorLock);

//		_RPT3(_CRT_WARN, "AudioMaster rebuilt %x, SR=%d thread = %x\n", this, sampleRate, (int)GetCurrentThreadId());
		std::vector< std::pair<int32_t, std::string> > pendingPresets;
		if(generator)
		{
			//            _RPT0(CRT_WARN,"\n=========REINIT DSP=============\n");

			// Retain current preset (including non-stateful) when changing sample-rate etc.
			// JUCE will load a preset (causing updates in DSP queue) then immediatly reset the processor.
			// In this case, we need to ensure Processor has latest preset from DAW.
			// Action any waiting parameter updates on GUI->DSP queue.
			ServiceDspRingBuffers();

			// ensure preset from DAW is up-to-date.
			generator->HandleInterrupt();

			//const bool saveExtraState = true;
			//generator->getPresetState(presetChunk, saveExtraState);
			const bool saveExtraState = true;
			generator->getPresetsState(pendingPresets, saveExtraState);

			generator->Close();

			// can't leave pointers to deleted objects around waiting for Que.
			ResetMessageQues();

			delete generator;
		}

		// BUILD SYNTHESIZER GRAPH.
		generator = new SeAudioMaster( (float) sampleRate, this, BundleInfo::instance()->getPluginInfo().latencyConstraint);
		generator->setBlockSize(generator->CalcBlockSize(maxBlockSize));

		std::vector<int32_t> mutedContainers; // unused at preset. (Waves thing).
		generator->BuildDspGraph(&currentDspXml, pendingPresets, mutedContainers);

#if 0
		// Apply preset before Open(), else gets delayed by 1 block causing havok with BPM Clock (receives BPM=0 for 1st block).
		if (!presetChunk.empty())
		{
			generator->setPresetState_UI_THREAD(presetChunk, false);
			assert(reinitializeFlag == false); // loading prior preset should not have changed any persistant host-controls.
		}
#endif
		generator->Open();

		generator->synth_thread_running = true;

		const auto newLatencySamples = generator->getLatencySamples();
		if(currentPluginLatency != newLatencySamples)
		{
			currentPluginLatency = newLatencySamples;

			{
				my_msg_que_output_stream strm( MessageQueToGui(), (int)UniqueSnowflake::APPLICATION, "ltnc");
				strm << (int)0; // message length.
				strm.Send();
			}
		}
	}

	// this can change regardless of if we reinit or not.
	generator->SetHostControl(HC_PROCESS_RENDERMODE, runsRealtime ? 0 : 2);
	runsRealtimeCurrent = runsRealtime;
}

SynthRuntime::~SynthRuntime()
{
//    _RPT2(_CRT_WARN, "~SynthRuntime() %x thread = %x\n", this, (int)GetCurrentThreadId());
    if( generator)
    {
        generator->Close();
        delete generator;
    }
}

void SynthRuntime::ServiceDspWaiters2(int sampleframes, int guiFrameRateSamples)
{
	pendingControllerQueueClients.ServiceWaitersIncremental(MessageQueToGui(), sampleframes, guiFrameRateSamples);
	peer->Service();
}

// Periodic poll of parameter update messages from GUI.
// GUI >> DSP.
void SynthRuntime::ServiceDspRingBuffers()
{
	peer->ControllerToProcessorQue()->pollMessage(generator);
}

// Send VU meter value to GUI.
void SynthRuntime::RequestQue( QueClient* client, bool noWait )
{
	pendingControllerQueueClients.AddWaiter(client);
}

std::wstring SynthRuntime::ResolveFilename(const std::wstring& name, const std::wstring& extension)
{
	std::wstring l_filename = name;
	std::wstring file_ext;
	std::wstring fileType = extension;
	file_ext = GetExtension(l_filename);

	// Attempt to determin file type. First by supplied extension, then by examining filename.
	if( fileType.empty() )
	{
		if( !file_ext.empty() )
		{
			fileType = file_ext;
		}
	}

	// Add stock file extension if filename has none.
	if( file_ext.empty() )
	{
		if( extension.empty() )
		{
			return l_filename;
		}

		//		file_ext = extension;
		l_filename += (L".") + extension;
	}

	// Is this a relative or absolute path?
#ifdef _WIN32
	if( l_filename.find(':') == string::npos )
#else
    if( l_filename.size() > 1 && l_filename[0] != '/' )
#endif
	{
		std::wstring default_path = getDefaultPath( fileType );
		l_filename = combine_path_and_file( default_path, l_filename );
	}

	// !!may need to search local directory too
	return l_filename;
}
//extern HINSTANCE ghInst;

std::wstring SynthRuntime::getDefaultPath(const std::wstring& p_file_extension )
{
    return BundleInfo::instance()->getResourceFolder();
}

void SynthRuntime::GetRegistrationInfo(std::wstring& p_user_email, std::wstring& p_serial)
{
	assert(false);
}

void SynthRuntime::DoAsyncRestart()
{
	reinitializeFlag = true;
}

void SynthRuntime::ClearDelaysUnsafe()
{
	std::lock_guard<std::mutex> x(generatorLock);

	if (generator)
	{
		generator->ClearDelaysUnsafe();
	}
}

void SynthRuntime::OnSaveStateDspStalled()
{
//#if defined(SE_TAR GET_AU) && !TARGET_OS_IPHONE
    // Action any waiting parameter updates from AudioUnit.
    shell_->flushPendingParameterUpdates();
    
    // Action any waiting parameter updates on GUI->DSP queue.
    ServiceDspRingBuffers();
//#endif
}

int SynthRuntime::getLatencySamples()
{
	// ! FL Studio calls this from foreground thread. lock in case generator restarting.
	std::lock_guard<std::mutex> x(generatorLock);

	// _RPT1(0, "SynthRuntime::getLatencySamples() -> %d\n", currentPluginLatency);
	return currentPluginLatency;
}

int32_t SynthRuntime::SeMessageBox(const wchar_t* msg, const wchar_t* title, int flags)
{
	return 0;
}

void SynthRuntime::onSetParameter(int32_t handle, RawView rawValue, int voiceId)
{
	shell_->onSetParameter(handle, rawValue, voiceId);
}

void SynthRuntime::setPresetUnsafe(DawPreset const* preset)
{
	std::lock_guard<std::mutex> x(generatorLock); // protect against setting preset during a restart of the processor (else preset gets lost).

#if 0 //def _DEBUG
	{
		auto xml = preset->toString(0);
		xml = xml.substr(0, 500);

		_RPTN(0, "\nSynthRuntime::setPresetUnsafe()\n %s\n\n", xml.c_str());
	}
#endif

	if (!generator)
	{
		missedPreset = preset;
		return;
	}

	missedPreset = {};

	// TODO check behaviour during DSP restart
	generator->interrupt_preset_.store(preset, std::memory_order_release);
	generator->TriggerInterrupt();
}
