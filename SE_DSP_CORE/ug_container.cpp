#include "pch.h"

#include <list>
#include <assert.h>
#include "ug_container.h"

#include "SeAudioMaster.h"
#include "midi_defs.h"
#include "UgDatabase.h"
#include "./IDspPatchManager.h"
#include "./ug_patch_param_watcher.h"
#include "module_register.h"
#include "ug_event.h"
#include "ug_patch_automator.h"
#include "modules/shared/voice_allocation_modes.h"
#include "ug_patch_param_setter.h"
#include "dsp_patch_manager.h"
#include "tinyxml/tinyxml.h"
#include "dsp_patch_parameter_base.h"
#include "my_input_stream.h"
#include "ug_oversampler.h"
#include "ug_midi_automator.h"
#include "cancellation.h"
#include "iseshelldsp.h"
#include "modules/shared/PatchCables.h"
#include "cancellation.h"
#include "ug_oversampler_in.h"

SE_DECLARE_INIT_STATIC_FILE(ug_container);

using namespace std;

// Cancellation test wave recorder module.
#ifdef CANCELLATION_TEST_ENABLE

#include "ug_vst_out.h"
#include "ug_oversampler_out.h"
#include "modules/se_sdk3/mp_sdk_audio.h"

namespace{ int32_t r = RegisterPluginXml(
	"<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
	"<PluginList>"
		"<Plugin id=\"SE CancelationRecorder\" name=\"CR\" category=\"Modifiers\" >"
			"<Audio>"
				"<Pin id=\"0\" name=\"Filename\" direction=\"in\" datatype=\"string\"  />"
				"<Pin id=\"1\" name=\"Signal In\" direction=\"in\" datatype=\"float\" rate=\"audio\" />"
			"</Audio>"
		"</Plugin>"
	"</PluginList>"
	);
}

class Cancelationrecorder : public MpBase
{
public:
	FILE* fileHandle_;

	Cancelationrecorder(gmpi::IMpUnknown* host) : MpBase(host), fileHandle_(0)
	{
		initializePin(0,pinFilename);
		initializePin(1,pinSignalIn);

		SET_PROCESS(&Cancelationrecorder::subProcess);
	}

	~Cancelationrecorder()
	{
		if( fileHandle_ )
			fclose(fileHandle_);
	}

	void subProcess(int bufferOffset, int sampleFrames)
	{
		if( fileHandle_ == 0 )
		{
			fileHandle_ = _wfopen(pinFilename.getValue().c_str(), L"wb");
		}

		// get pointers to in/output buffers.
		float* signalIn = pinSignalIn.getBuffer() + bufferOffset;

		fwrite(signalIn, sizeof(*signalIn), sampleFrames, fileHandle_ );
	}

	StringInPin pinFilename;
	AudioInPin pinSignalIn;
};

REGISTER_PLUGIN(Cancelationrecorder, L"SE CancelationRecorder");

#endif

// provides Voice host controls in the same container as the MIDI CV,
// for when Patch Manager is in a higher container and can't know what container to send polyphony etc to.
class VoiceParameterWatcher final : public MpBase2
{
	IntInPin pinPolyphony;
	IntInPin pinReserveVoices;
	IntInPin pinVoiceAllocationMode;
	
	ug_container* container = {};
public:

	VoiceParameterWatcher()
	{
		initializePin(pinPolyphony);
		initializePin(pinReserveVoices);
		initializePin(pinVoiceAllocationMode);
	}
	
	int32_t open() override
	{
		container = dynamic_cast<ug_base*>(host.Get())->parent_container;
		return MpBase2::open();
	}

	void onSetPins() override
	{
		if (pinPolyphony.isUpdated())
		{
			container->setVoiceCount(pinPolyphony);
		}

		if (pinReserveVoices.isUpdated())
		{
			container->setVoiceReserveCount(pinReserveVoices);
		}

		if (pinVoiceAllocationMode.isUpdated())
		{
			if (voice_allocation::isMonoMode(pinVoiceAllocationMode))
			{
				const timestamp_t ts = container->AudioMaster()->NextGlobalStartClock();
				container->NoteOff(ts, -1); // All notes Off
			}
		}
	}
};

namespace
{
	auto r2 = gmpi::Register<VoiceParameterWatcher>::withXml(
R"XML(
<?xml version="1.0" encoding="utf-8" ?>
<PluginList>
	<Plugin id="SE VoiceParameterWatcher" name="VW" category="Debug" >
		<Audio>
			<Pin name="Polyphony" datatype="int" hostConnect="Polyphony" />
			<Pin name="ReserveVoices" datatype="int" hostConnect="ReserveVoices" />
			<Pin name="VoiceAllocationMode" datatype="int" hostConnect="VoiceAllocationMode" />
		</Audio>
	</Plugin>
</PluginList>
)XML");
}

namespace
{
// GUI pins are in control_group_auto_size.cpp
// plug indexes also defined in CContainer.cpp
static pin_description plugs[] =
{
	L"Spare"		, DR_IN		, DT_FSAMPLE,	L"0"	, L""				, IO_CONTAINER_PLUG|IO_RENAME|IO_AUTODUPLICATE|IO_CUSTOMISABLE, L"Connection to a module inside",
	// todo move to MIDI-CV
	L"Polyphony"	, DR_IN		, DT_ENUM ,		L"6"	, L"range 1,128"	, IO_DISABLE_IF_POS|IO_MINIMISED	, L"",
	// obsolete
	L"MIDI Automation", DR_IN	, DT_BOOL ,		L""		, L""				, IO_DISABLE_IF_POS|IO_MINIMISED	, L"",
	// obsolete
	L"Show Controls", DR_IN		, DT_ENUM ,		L"0"	, L"Off,On Panel,On Module",IO_DISABLE_IF_POS|IO_MINIMISED, L"",
};

static module_description_dsp mod =
{
	"Container", IDS_MN_CONTAINER, 0, &ug_container::CreateObject, CF_STRUCTURE_VIEW,plugs, static_cast<int>(std::size(plugs))
};

bool res = ModuleFactory()->RegisterModule( mod );

// Register GUI
static void GetModuleProperties(module_description& module)
{
	module.unique_id = "Container";
	module.name = L"Container";
	module.flags = CF_STRUCTURE_VIEW | CF_PANEL_VIEW;
}

static bool GetPinProperties(int index, pin_description& properties)
{
	switch (index)
	{
	case 0:
		properties.name = L"Controls on Parent"; // Backward Compatible only.
		properties.direction = DR_OUT;
		properties.datatype = DT_BOOL;
		properties.flags = IO_HIDE_WHEN_LOCKED | IO_MINIMISED | IO_REDRAW_ON_CHANGE;
		break;

	case 1:
		properties.name = L"Controls on Module";
		properties.direction = DR_IN;
		properties.datatype = DT_BOOL;
		properties.flags = IO_PARAMETER_SCREEN_ONLY | IO_REDRAW_ON_CHANGE;
		break;

	case 2:
		// very sneaky, "Ignore Program Change" changed to 'Visible' ("Controls on Parent")
		/*
				properties.name				= L"Ignore Program Change";
				properties.direction		= DR_IN;
				properties.datatype			= DT_BOOL;
				properties.flags			= IO_PARAMETER_SCREEN_ONLY;
				break;
			case 3:
		*/
		properties.name = L"Visible"; // was "Controls on Parent"
		properties.direction = DR_IN;
		properties.datatype = DT_BOOL;
		properties.flags = IO_HIDE_WHEN_LOCKED;
		break;

	case 3:
		properties.name = L"Ignore Program Change";
		properties.direction = DR_IN;
		properties.datatype = DT_BOOL;
		properties.flags = IO_PARAMETER_SCREEN_ONLY;
		break;

	default:
		return false;
	};

	return true;
};

bool res2 = ModuleFactory()->RegisterModule(&GetModuleProperties, &GetPinProperties);
}


short master_cont_polyphony = 8;
double* trash_double_ptr;
/*
// Fill an array of InterfaceObjects with plugs and parameters
IMPLEMENT_UG_INFO_FUNC2(ug_container)
{
	// Moved here V1.1
	LIST_VAR3( L"Ignore Program Change", trash_double_ptr, DR_IN, DT_BOOL , L"", L"",/ *IO_DISABLE_IF_POS|* /IO_MINIMISED, L"")

	// IO Var, Direction, Datatype, Name, Default, defid (index into ug_base::PlugFormats)
	// defid used to name a enum list or range of values
//	LIST_PIN( L"Spare", DR_CONTAINER_IO, L"0", L"", IO_RENAME|IO_AUTODUPLICATE|IO_CUSTOMISABLE, L"Connection to a module inside");
	LIST_PIN( L"Spare", DR_IN, L"0", L"", IO_CONTAINER_PLUG|IO_RENAME|IO_AUTODUPLICATE|IO_CUSTOMISABLE, L"Connection to a module inside");
//	LIST_VAR3( L"Polyphony", Polyphony, DR _PARAMETER, DT_ENUM , L"6", L"range 1,128", 0, L"Limits the number of notes you can play at once. (Changes take effect next time you re-start the sound)");
//	LIST_VAR3( L"MIDI Automation", trash_double_ptr, DR _PARAMETER, DT_BOOL , L"", L"",IO_DISABLE_IF_POS, L"")
//	LIST_VAR3( L"Show Controls", trash_double_ptr, DR _PARAMETER, DT_ENUM , L"0", L"Off,On Panel,On Module",IO_DISABLE_IF_POS, L"")
	LIST_VAR3( L"Polyphony", Polyphony, DR_IN, DT_ENUM , L"6", L"range 1,128", IO_MINIMISED, L"Limits the number of notes you can play at once. (Changes take effect next time you re-start the sound)");
	LIST_VAR3( L"MIDI Automation", trash_double_ptr, DR_IN, DT_BOOL , L"", L"",IO_DISABLE_IF_POS|IO_MINIMISED, L"")
	LIST_VAR3( L"Show Controls", trash_double_ptr, DR_IN, DT_ENUM , L"0", L"Off,On Panel,On Module",IO_DISABLE_IF_POS|IO_MINIMISED, L"")
}
*/
ug_container::ug_container() :
	automation_output_device(0)
	,automation_input_device(0)
	,m_patch_manager(0)
	, parameterSetter_(0)
	, parameterSetterSecondary_(0)
	, parameterWatcher_(0)
	,defaultSetter_(0)
	, nextRefreshVoice_(-1)
{
	push_back(new Voice(this, 0));		// create voice zero
	SET_CUR_FUNC(&ug_base::process_sleep);
}

ug_container::~ug_container()
{
	delete m_patch_manager;
}

// Add a unit gen to CURRENT voice
ug_base* ug_container::AddUG(ug_base* u)
{
	u->parent_container = this;
	VoiceList::AddUG(u);
	return u;
}

// add a ug to a specific voice, only needed for drum voice monitors
ug_base* ug_container::AddUG(Voice* p_voice, ug_base* p_ug)
{
	p_ug->parent_container = this;
	p_voice->AddUG(p_ug);
	return p_ug;
}

// call Open() function for all UGs
int ug_container::Open()
{
	// TODO !!! WHY !!!, remove and test
	AudioMaster()->InsertBlockDev(this);
	// Support passing CPU readings to parent container.
	AudioMaster()->AddCpuParent(this);

	// rather convoluted, but can't use patch-store pins in this module.
	if( automation_input_device )
	{
		automation_input_device->setMidiChannel( patch_control_container->get_patch_manager()->getMidiChannel() );
		automation_output_device->setMidiChannel( patch_control_container->get_patch_manager()->getMidiChannel() );
	}

	//no, crashes	ug_base::Open();
	SetFlag( UGF_OPEN ); // we aren't calling base class, so must do this here.

	// If this container is oversampled and polyphonic, leave voice-numbers intact, they have already been set by higher container.
	bool setVoiceNumbers = !GetPolyphonic();
	OpenAll( AudioMaster(), setVoiceNumbers );

	if( m_patch_manager )
	{
		m_patch_manager->InitializeAllParameters();
	}

	return 0;
}

// call Close() function for all UGs
int ug_container::Close()
{
	CloseAll();
	ug_base::Close();
	return 0;
}

// For oversampler.
ug_base* ug_container::Copy(ISeAudioMaster* audiomaster, CUGLookupList& UGLookupList )
{
	auto clone = dynamic_cast<ug_container*>(ug_base::Copy(audiomaster, UGLookupList));

	clone->front()->CopyFrom(audiomaster, clone, front(), UGLookupList);

	// Now tie IO plugs to container plugs.
	for( unsigned int x = 0; x < plugs.size() ; ++x )
	{
		UPlug* p = plugs[x];
		UPlug* tiedto = p->GetTiedTo();

		if( tiedto )
		{
			UPlug* Clonep = clone->plugs[x];
			ug_base* cloneIoMod = UGLookupList.LookupPolyphonic( tiedto->UG );
			UPlug* Clonetiedto = cloneIoMod->GetPlug( tiedto->getPlugIndex() );
			Clonep->TiedTo = Clonetiedto;
			Clonetiedto->TiedTo = Clonep;
		}
	}

	return clone;
}

void ug_container::CloneContainerVoices()
{
	// same for sub-containers.
	for( vector<Voice*>::iterator it = begin(); it != end(); ++it )
	{
		( *it )->CloneContainerVoices();
	}

	VoiceList::CloneVoices();
}

void ug_container::CloneConnectorsFrom( ug_base* FromUG, CUGLookupList& UGLookupList )
{
	ug_container* FromContainer = (ug_container*) FromUG;
	ug_base::CloneConnectorsFrom( FromUG, UGLookupList );
	front()->CloneConnectorsFrom( FromContainer->front(), UGLookupList );
}

bool ug_container::belongsTo(ug_container* container)
{
	auto c = this;
	while (c)
	{
		if (c == container)
			return true;

		c = c->parent_container;
	}
	return false;
}

ug_container* ug_container::getOutermostPolyContainer()
{
	auto fromVoiceContainer = this;

	while (true)
	{
		if (fromVoiceContainer->isContainerPolyphonic())
			break;

		auto parent = fromVoiceContainer->parent_container;
		if (parent == nullptr)
		{
			auto oversampler = dynamic_cast<ug_oversampler*>(fromVoiceContainer->AudioMaster());
			if (oversampler)
			{
				parent = oversampler->parent_container;
			}
		}

		if (parent)
			fromVoiceContainer = parent;
		else
			break;
	}

	return fromVoiceContainer;
}

void ug_container::ConnectPatchCables()
{
	auto param = get_patch_manager()->GetHostControl(HC_PATCH_CABLES);
	if (param)
	{
		// get current cable list.
		auto raw = param->GetValueRaw2(param->EffectivePatch());

		SynthEdit2::PatchCables cableList(raw);

		// make connections
		for (auto& c : cableList.cables)
		{
			auto from = dynamic_cast<ug_base*>(AudioMaster()->HandleToObject(c.fromUgHandle));
			auto to = dynamic_cast<ug_base*>(AudioMaster()->HandleToObject(c.toUgHandle));

			if (from && to) // are not muted.
			{
				// can fail on *terminated* patch points in oversampler,
				// because oversampler is set up before outer container (which is also connecting all patch cables). i.e we handled unterminated patch-cables BEFORE we even tried to connect any.
				// fix will require re-thinkin how oversamplers are constructed.
				assert(!from->GetFlag(UGF_VOICE_MON_IGNORE)); // 'from' is flagged as 'unterminated' (yet here we are connecting it). see ug_plugin3Base::PPGetActiveFlag()

				auto fromPlug = from->GetPlug(c.fromUgPin);
				auto toPlug = to->GetPlug(c.toUgPin);

				// Check not routing out of polyphonic container.
				{
					auto topVoiceContainerA = fromPlug->UG->parent_container->getOutermostPolyContainer();
					auto topVoiceContainerB = toPlug->UG->parent_container->getOutermostPolyContainer();

					if (topVoiceContainerA != topVoiceContainerB)
					{
						continue; // this patch cable is invalid.
					}
				}

				auto datatype = fromPlug->DataType;
				const wchar_t* feedbackModuleType = nullptr;

				const wchar_t* datatypes[] = { L"Enum", L"Text", L"MIDI", L"Double", L"Bool", L"Volts", L"Float", L"", L"Int", L"Int64", L"Blob" };

				wchar_t temp[30];

				switch (datatype)
				{
				case DT_FSAMPLE:
					feedbackModuleType = L"Feedback Delay";
					break;

				case DT_INT:
				case DT_MIDI:
				case DT_FLOAT:
				case DT_TEXT:
				case DT_STRING_UTF8:
				case DT_DOUBLE:
				case DT_BOOL:
				case DT_INT64:
				case DT_BLOB:
				case DT_ENUM:
					swprintf(temp, sizeof(temp) / sizeof(temp[0]), L"SE Feedback Delay - %ls", datatypes[datatype]);
					feedbackModuleType = temp;
					break;

				default:
					assert(false); // TODO
				}

				// Include feedback module in patch cable connection.
				auto destContainer = fromPlug->UG->ParentContainer();
				auto feedbackModule = ModuleFactory()->GetById(feedbackModuleType)->BuildSynthOb();
				destContainer->AddUG(feedbackModule);
				feedbackModule->SetupWithoutCug();
				feedbackModule->BuildHelperModule();

				// For Patch Cables, remove existing connection to default-setter.
				if (toPlug->connections.size() == 1)
				{
					// Not sure if latency adjustment might be connected. If so need to setect it.
					assert(toPlug->connections.front()->UG->getModuleType()->UniqueId() != L"SE LatencyAdjust");

					if (toPlug->connections.front()->UG->GetFlag(UGF_DEFAULT_SETTER))
					{
						toPlug->DeleteConnectors();
					}
				}

				connect(fromPlug, feedbackModule->GetPlug(0));
				connect_oversampler_safe(feedbackModule->GetPlug(1), toPlug);
			}
		}
	}
}

void ug_container::PostBuildStuff(bool xmlMethod)
{
	if (is_polyphonic)
	{
		auto m = ModuleFactory()->GetById(L"SE VoiceParameterWatcher")->BuildSynthOb();
		AddUG(m);
		m->SetupWithoutCug();
	}

	if (m_patch_manager)
	{
		if (dynamic_cast<DspPatchManager*>(m_patch_manager) != nullptr) // "real" patchmangers. !! TODO Patch Cables to be held GLOBALLY, so they work at all levels including "Main".
			ConnectPatchCables();

		// Silence optimization flag.
		if (auto param = get_patch_manager()->GetHostControl(HC_SILENCE_OPTIMISATION) ; param)
		{
			const auto silenceOptimization = (bool)param->GetValueRaw2(param->EffectivePatch(), 0);

			if (auto am = dynamic_cast<SeAudioMaster*>(AudioMaster()) ; am)
				am->setSilenceOptimisation(silenceOptimization);
		}

		// see also dsp_patch_parameter_base::OnValueChanged()
		if (auto param = get_patch_manager()->GetHostControl(HC_MPE_MODE) ; param)
		{
			const auto mpeMode = (int32_t) param->GetValueRaw2(param->EffectivePatch(), 0);
			
			if (auto am = AudioMaster2(); am)
				am->setMpeMode(mpeMode);
		}
	}

	if (xmlMethod)
	{
		SetUnusedPlugs2();
	}

	// Below here is only for top-level containers and oversamplers.
	if (parent_container != 0)
		return;

	// Cancellation test.
#ifdef CANCELLATION_TEST_ENABLE

	/* 
		Set CANCELLATION_BRANCH macro in cancellation.h to enable this
	*/

	FILE* logfile = fopen("C:\\temp\\cancellation\\log_" CANCELLATION_BRANCH ".txt", "w");
	string diagtext;

	if( parent_container == 0 && dynamic_cast<ug_oversampler*>(AudioMaster()) == 0)
	{
//? #ifndef SE_EDIT _SUPPORT
	assert(false);
//#endif
		// read module handle from file.
		int handle = -1;

		FILE* f1 = fopen("C:\\temp\\cancellation\\suspect.txt", "r");
		if( f1 )
		{
			fscanf(f1, "%d", &handle);
			fclose(f1);
		}

		ug_base* targetUg = dynamic_cast<ug_base*>( AudioMaster()->HandleToObject(handle) );

		if( targetUg == 0 )
		{
			// start trace from VST_OUT module.
			for( auto ug : front()->UGClones )
			{
				targetUg = dynamic_cast<ug_vst_out*>( ug );

				if( targetUg )
				{
					handle = targetUg->Handle();
					break;
				}
			}
			diagtext = "Can't find target handle, reverting to VST_OUT\n";
			fwrite(diagtext.c_str(),1, diagtext.size(), logfile);
		}
		else
		{
			diagtext = "Found target handle.\n";
			fwrite(diagtext.c_str(),1, diagtext.size(), logfile);
		}
		
		if( targetUg )
		{
			// go "inside" oversampler if nesc.
			ug_oversampler* overs = dynamic_cast<ug_oversampler*> ( targetUg );
			if( overs )
			{
				targetUg = overs->oversampler_out;
				diagtext = "Target is oversampler. Diverting to VST-IN module.\n";
				fwrite(diagtext.c_str(),1, diagtext.size(), logfile);
			}

			// put wave recorders on inputs to specified module.
			for( auto p : targetUg->plugs )
			{
				if (p->Direction == DR_IN && p->DataType == DT_FSAMPLE)
				{
					for (auto fromPlug : p->connections)
					{
						auto con_from_ug = fromPlug->UG;

						Module_Info* mtype = ModuleFactory()->GetById(L"SE CancelationRecorder");
						ug_base* cr = mtype->BuildSynthOb();

						// set filename based on target handle and plug.
						{
							std::ostringstream oss;
							oss << "C:\\temp\\cancellation\\from_"
								<< con_from_ug->Handle()
								<< "_" << fromPlug->getPlugIndex()
								<< "_to_" << targetUg->Handle()
								<< "." << CANCELLATION_BRANCH
								<< ".raw";

							targetUg->parent_container->AddUG(cr);
							cr->SetupWithoutCug();
							cr->GetPlug(0)->SetDefault(oss.str().c_str());
							cr->connect(fromPlug, cr->GetPlugById(1));
						}
						{
							std::ostringstream oss;
							oss << "Logging module: " << con_from_ug->Handle() << " (" << typeid(*(con_from_ug)).name() << ")\n";
							fwrite(oss.str().c_str(), 1, oss.str().size(), logfile);
						}
					}
				}
			}
		}
	}
	fclose(logfile);

#endif

	// Re-route container I/O plugs directly to target.
	ReRoutePlugs(); // (recursive). Also sets up polyphony.

	// Handle special-case: controls upstream of patch-automator.
	SetupSecondaryPatchParameterSetter();
}

void ug_container::IterateContainersDepthFirst(std::function<void(ug_container*)>& f)
{
	for (auto voice : *this)
	{
		voice->IterateContainersDepthFirst(f);
	}

	f(this);
}

// Called only on main container once everything built.
void ug_container::PostBuildStuff_pass2()
{
	// Calculate latency
	setupDelayCompensation();

	std::function<void(ug_container*)> f = [](ug_container* container)
		{
			container->CloneVoices();
		};

	IterateContainersDepthFirst(f);

	// RemoveDuplicateConnections() must be done after everything cloned.
	// before it was being done prematurely, bottom-up, before higher containers had a chance
	// to clone lower ones (oversampled).
	std::function<void(ug_container*)> f2 = [](ug_container* container)
	{
		auto paramWatcher = container->parameterWatcher_;
		while (paramWatcher)
		{
			paramWatcher->RemoveDuplicateConnections();
			paramWatcher = (ug_patch_param_watcher*)paramWatcher->m_next_clone;
		}
	};

	IterateContainersDepthFirst(f2);

	int maxSortOrderGlobal = -1; // for depth-first sort.

	std::function<void(ug_container*)> f3 = [&maxSortOrderGlobal](ug_container* container)
	{
		container->SortOrderSetup3(maxSortOrderGlobal); // determine execution order

		// Note: modules are not sorted untill ALL containers sorted, because sort can iterate into containers at a lower level.
		// perform sort. Rearranges modules in container
		// container->SortAll3();
	};

	IterateContainersDepthFirst(f3);

	std::function<void(ug_container*)> f4 = [&maxSortOrderGlobal](ug_container* container)
	{
		// perform sort. Rearranges modules in container
		container->SortAll3();
	};

	IterateContainersDepthFirst(f4);


	AudioMaster2()->CleanupBypassableModules();
}

void ug_container::setupDelayCompensation()
{
	// Calculate latency
	if (AudioMaster()->latencyCompensationMax() == 0)
	{
		return;
	}

	// restore dynamic latencies
	const auto& latencies = AudioMaster()->getShell()->GetModuleLatencies();
	for(auto& l : latencies)
	{
		if (l.second != 0)
		{
			auto module = dynamic_cast<ug_base*>(AudioMaster()->HandleToObject(l.first));
			module->latencySamples = l.second;
		}
	}

	for (auto& v : *this)
	{
		for (auto& m : v->UGClones)
		{
			m->calcDelayCompensation();
		}	
	}
}

int ug_container::calcDelayCompensation()
{
	const auto r = ug_base::calcDelayCompensation();

	// clean up 'islands' disconnected from other modules.
	for (auto& v : *this)
	{
		for (auto& m : v->UGClones)
		{
			m->calcDelayCompensation();
		}
	}

	return r;
}

void ug_container::SetupSecondaryPatchParameterSetter()
{
	assert(automation_input_device == nullptr || automation_input_device->GetFlag(UGF_PATCH_AUTO));

	for (auto v : *this)
	{
#ifdef VOICE_USE_VECTOR
			for (int i = 0; i < v->UGClones.size(); ++i) // iterate by index because inserting a module invalidates iterators.
			{
				auto m = v->UGClones[i];
#else
			for (auto m : v->UGClones)
			{
#endif
			auto c = dynamic_cast<ug_container*>(m);

			if (c)
			{
				c->SetupSecondaryPatchParameterSetter();
			}

			if (m->GetFlag(UGF_PATCH_AUTO))
			{
				m->FlagUpStream(UGF_UPSTREAM_PARAMETER);
			}
		}
	}
}

// Reconnect all container Ins and outs to their true destination
void ug_container::ReRoutePlugs()
{
	auto pps = GetParameterSetter();
	if (pps)
	{
		// don't work, can't route upward when outer containers still being built, esp with nested oversamplers.
		EnsurePpSetterDownstream();
	}

	// first do child containers.  This allows use of containers in poly voice chain
#ifdef _DEBUG
	if( SeAudioMaster::GetDebugFlag( DBF_TRACE_POLY ) )
	{
		DebugPrintName(true);
		_RPT0(_CRT_WARN, "- ReRoutePlugs\n" );
	}

#endif

	// This ensures all IO Mods are removed from audio path in child container, but not this container.
	// This means pp set downstream can flow through all child containers, but not escape this one.
	ReRouteContainers();
	// then run poly setup
	// this should be done while io mods 'guard' the exit to container, demarking voice chain
	// once 'this' is rerouted, ugs will connect directly to outside of container. or even to inside of annother
#ifdef _DEBUG
	if( SeAudioMaster::GetDebugFlag( DBF_TRACE_POLY ) )
	{
		DebugPrintName(true);
		_RPT0(_CRT_WARN, "- PolyphonicSetup\n" );
	}
#endif

	/*
	perhaps somehow just flag all outgoing connections as "poly-last" so polyphony dosn't "escape" later.
	// or do so during rerouting of plugs (would need to apply only to connections exiting main polyphonic container upward, not plain child containers)

	currently GetPolyAgregate() is used in PPSetDownstream() to indicate we've reached an I/O Mod, which forces end of polyphonic chain.
	// alternatly I/O modules could be rerouted later?
	or perhaps the test could be that polyphony ends when connection goes to any other container that is a non-child of poly container.

	could be fooled by container output connected back to it's own input?, in theory this should have a voice-adder on it.
	NO, "can't connect a module to itself", should disallow that.

	OR.. just slam voice-adders on every output of polyphonic containers ( isContainerPolyphonic() ), and optimise them away later (like feedback delays).
	*/

	if (isContainerPolyphonic()) // avoid trying to determin polyphony on nested oversamplers.
	{
		/* could do
		auto polyParent = parent_container->getOutermostPolyContainer();
		if(polyParent == nullptr)
		{
		*/
#ifdef _DEBUG
		if (SeAudioMaster::GetDebugFlag(DBF_TRACE_POLY))
		{
			_RPT0(_CRT_WARN, "\n----------------------------------\nPolyphonicSetup()\nSTAGE 1 - Identify Downstream modules\n");
		}
#endif
		auto e = get_patch_manager()->InitSetDownstream(this); // container_->getOutermostPolyContainer());

		if (e)
		{
#ifdef _DEBUG
			e->DebugDump();
#endif
			throw e;
		}

		front()->PolyphonicSetup();
	}

	// lastly, now that polyphony setup, rerout plugs
	// setup proxys for all plugs
	for( int x = GetPlugCount() - 1 ; x >= 0 ; x-- )
	{
		UPlug* p = GetPlug(x);
		UPlug* tiedto = p->GetTiedTo();

		// inner output connections must be deleted, even if no connection outside
		// else IO Mod become part of feedback loop (it's used to set all unconnected inputs)
		if( tiedto != NULL && tiedto->InUse() ) // ug_base::connect() should usually ensure so, but not always
		{
			if( p->Direction == DR_OUT )
			{
				p->ReRoute();
			}
			else
			{
				if( p->Direction == DR_IN )  // Container Inputs
				{
					tiedto->ReRoute();
				}
			}
		}
	}
}

void ug_container::SortOrderSetup3(int& maxSortOrderGlobal)
{
	std::list<ug_base*> ugs;
	ExportUgs(ugs); // get global list of ugs

#if 1
	for(auto& ug : ugs)
	{
		if (ug->SortOrder2 != -1) // already sorted?
			continue;

		// skip intermediate modules, since we'll be sorting them from downstream anyhow.
		// this may include being sorted by an outer container later (i.e. not here).
		bool hasOutputLines = false;
		for (auto p : ug->plugs)
		{
			if (p->Direction != DR_OUT || p->connections.empty())
				continue;

			for (auto to : p->connections)
			{
				assert(to->UG->SortOrder2 == -1);
			}

			hasOutputLines = true;
			break;
		}

		if (hasOutputLines)
			continue;
#else
	while (!ugs.empty())
	{
		// grab a new module at random.
		auto ug = ugs.front();
		ugs.pop_front();

		if (ug->SortOrder2 != -1) // already done?
			continue;

		// step forward downstream if possible
		bool stepped = true;
		while (stepped)
		{
			stepped = false;
			for (auto p : ug->plugs)
			{
				if (p->Direction != DR_OUT)
					continue;

				for (auto to : p->connections)
				{
					if (to->UG->SortOrder2 == -1)
					{
						ug = to->UG;
						stepped = true;
						break;
					}
				}

				if (stepped)
					break;
			}
		};
#endif
		// recurse upstream as far as possible, assigning sortorder
		auto e = ug->CalcSortOrder3(maxSortOrderGlobal);

		if (e)
		{
#if defined( _DEBUG )
			e->DebugDump();
#endif

			throw e;
		}
	}

	// Rare case. when patch consists only of modules feeding back into each other (no 'downstream' module at all)
	for (auto& ug : ugs)
	{
		if (ug->SortOrder2 != -1) // already sorted?
			continue;

		// recurse upstream as far as possible, assigning sortorder
		auto e = ug->CalcSortOrder3(maxSortOrderGlobal);

		if (e)
		{
#if defined( _DEBUG )
			e->DebugDump();
#endif

			throw e;
		}
	}
}

ug_base* ug_container::InsertAdder(EPlugDataType p_data_type)
{
	// Patch cables permit illegal connections, guard against crash.
	if (p_data_type != DT_FSAMPLE)
		return nullptr;

	std::wstring adder_type;
	switch( p_data_type )
	{
	case DT_FSAMPLE:
	{
		adder_type = (L"Float Adder2");
		break;
	}

	default:
		assert(false); // unsupported
	}

	Module_Info* adderType = ModuleFactory()->GetById( adder_type );
	ug_base* adder = adderType->BuildSynthOb();
	AudioMaster()->AssignTemporaryHandle(adder);
	adder->AddFixedPlugs();
	adder->SetAudioMaster( AudioMaster() );
	AddUG( adder );
	adder->SetupDynamicPlugs();
	return adder;
}

ug_patch_param_setter* ug_container::GetParameterSetter()
{
	if( parameterSetter_ == nullptr )
	{
		assert( (flags & UGF_OPEN) == 0 && "can't create after open, or it may miss open()");
		// perform functions normaly done by ug_base::Setup
		parameterSetter_ = dynamic_cast<ug_patch_param_setter*>(ModuleFactory()->GetById((L"SE Patch Parameter Setter"))->BuildSynthOb());
		AddUG( parameterSetter_ );
		parameterSetter_->SetupWithoutCug();
	}

	return parameterSetter_;
}

// The secondary setter is for modules UPSTREAM of the Patch-Automator (e.g. MIDI processors). These would normally create a feedback loop, but
// if we can accept a 1-block delay, these modules can be automated too.
ug_patch_param_setter* ug_container::GetParameterSetterSecondary()
{
	if (parameterSetterSecondary_)
	{
		return parameterSetterSecondary_;
	}

	assert((flags & UGF_OPEN) == 0 && "can't create after open, or it may miss open()");
	// perform functions normaly done by ug_base::Setup
	Module_Info* modType = ModuleFactory()->GetById((L"SE Patch Parameter Setter"));
	parameterSetterSecondary_ = dynamic_cast<ug_patch_param_setter*>(modType->BuildSynthOb());
	AddUG(parameterSetterSecondary_);
	parameterSetterSecondary_->SetupWithoutCug();
	return parameterSetterSecondary_;
}

ug_base* ug_container::GetDefaultSetter()
{
	if( defaultSetter_ )
	{
		return defaultSetter_;
	}

	assert( (flags & UGF_OPEN) == 0 && "can't create after open, or it may miss open()");
	// perform functions normaly done by ug_base::Setup
	auto ppWatcherType = ModuleFactory()->GetById( L"SE Default Setter" );
	defaultSetter_ = ppWatcherType->BuildSynthOb();
	AddUG( defaultSetter_ );
	defaultSetter_->SetupWithoutCug();
	return defaultSetter_;
}

// TODO: would probley be much faster to store pointer to it.
ug_patch_param_watcher* ug_container::GetParameterWatcher()
{
	if( parameterWatcher_ )
	{
		return parameterWatcher_;
	}

	assert( (flags & UGF_OPEN) == 0 && "can't create after open, or it may miss open()");

	// perform functions normaly done by ug_base::Setup
	Module_Info* ppWatcherType = ModuleFactory()->GetById( (L"SE Patch Parameter Watcher") );
	parameterWatcher_ = dynamic_cast<ug_patch_param_watcher*>( ppWatcherType->BuildSynthOb() );
	AddUG( parameterWatcher_ );
	parameterWatcher_->SetupWithoutCug();

	return parameterWatcher_;
}

void ug_container::ConnectHostControl(HostControls hostConnect, UPlug* plug)
{
	if (hostConnect == HC_SNAP_MODULATION__DEPRECATED)
	{
		return;
	}

	// Voice/Active and Voice-ID are always controled by the VoiceList.
	if (hostConnect == HC_VOICE_VIRTUAL_VOICE_ID || hostConnect == HC_VOICE_ACTIVE)
	{
		auto polyContainer = getOutermostPolyContainer();
		polyContainer->GetParameterSetter()->ConnectVoiceHostControl(polyContainer, hostConnect, plug);
		/*
		// Polyphonic containers control Voice-ID and Voice-Active.
		// Also when using MIDI-CV we control Gate.
		if (isContainerPolyphonic() || ParentContainer() == 0)
		{
			// _RPTW1(_CRT_WARN, L"%s controlled by local container.\n", GetHostControlName(hostConnect) );
			GetParameterSetter()->ConnectVoiceHostControl(this, hostConnect, plug);
		}
		else
			ParentContainer()->ConnectHostControl(hostConnect, plug);
			*/
	}
	else
	{
		get_patch_manager()->ConnectHostControl2(hostConnect, plug);
	}
}

void ug_container::Setup(class ISeAudioMaster* am, class TiXmlElement* xml)
{
	ug_base::Setup(am, xml);

	int temp = 0;
	xml->QueryIntAttribute("Polyphonic", &temp);
	if (temp == 1)
	{
		SetContainerPolyphonic();
	}
}

void ug_container::SetupPatchManager(class TiXmlElement* patchManager_xml, std::vector< std::pair<int32_t, std::string> >& pendingPresets)
{
	// ensure child modules know this container holds their patchdata.
	patch_control_container = this;

	// check for any preset that was saved during an async restart.
	const std::string* presetXml{};
	for (const auto& preset : pendingPresets)
	{
		if (preset.first != Handle())
			continue;

		presetXml = &preset.second;
	}

	BuildPatchManager(patchManager_xml, presetXml);
//	get_patch_manager()->setupContainerHandles(this);
}

// This must be done BEFORE any other module is built, so they can hook up their parameter pins to something.
void ug_container::BuildPatchManager(TiXmlElement* patchMgrXml, const std::string* presetXml)
{
	IDspPatchManager* dspPatchManager = new DspPatchManager( this );
	setPatchManager( dspPatchManager );

	int32_t midiChannel = -1;
	patchMgrXml->QueryIntAttribute("MidiChannel", &midiChannel);
	dspPatchManager->setMidiChannel( midiChannel );

	auto parameters_xml = patchMgrXml->FirstChildElement("Parameters");
	if (parameters_xml)
	{
		for (auto parameter_xml = parameters_xml->FirstChildElement(); parameter_xml; parameter_xml = parameter_xml->NextSiblingElement())
		{
			int32_t dataType = -1;
			parameter_xml->QueryIntAttribute("DataType", &dataType);
			auto parameter = dspPatchManager->createPatchParameter(dataType);
			parameter->Initialize(parameter_xml);
		}
	}
	if (presetXml)
	{
		constexpr bool overrideIgnoreProgramChange = true;
		dspPatchManager->setPresetState(*presetXml, overrideIgnoreProgramChange);
	}
}

// Must be done AFTER all other modules built, so it can detect if USER included a Patch-Automator,
// if not we need to supply one automatically.
void ug_container::BuildAutomationModules()
{
	// AUTOMATION SENDER.
	// Find Automation sender. Only pre-exists if user added it to patch.
	automation_input_device = front()->findPatchAutomator();

	if(	automation_input_device == 0 )
	{
		// Build Automation sender.
		Module_Info* automatioSenderType = ModuleFactory()->GetById( L"SE Patch Automator" );
		automation_input_device = (ug_patch_automator*) automatioSenderType->BuildSynthOb();
		AddUG(automation_input_device);
		automation_input_device->SetupWithoutCug();// automatioSenderType);
		automation_input_device->patch_control_container = this;
	}

	// AUTOMATION RECIEVER.
	// build 2nd ug to provide midi output ( has to be seperate ug, as must be last in sortorder)
	Module_Info* automatioReceiverType = ModuleFactory()->GetById( L"MIDI Automator Output" );
	automation_output_device = (ug_patch_automator_out*) automatioReceiverType->BuildSynthOb();
	automation_output_device->patch_control_container = this;
	AddUG(automation_output_device);

	automation_output_device->SetupWithoutCug();

	automation_output_device->automator_in = automation_input_device;
	// now proxy midi automator's "midi out" to automator_out's "midi out"
	assert( automation_output_device->GetPlug(L"MIDI Out") == automation_output_device->GetPlug(2) );
	automation_input_device->GetPlug(2)->Proxy = automation_output_device->GetPlug(2);
	assert( automation_output_device != 0 );

	// p_patch_control_container same as generator???
	assert(patch_control_container==this);
}

// Ensure ug_patch_param_setter always down-stream of ug_patch_automator by
void ug_container::EnsurePpSetterDownstream()
{
	assert(parameterSetter_);

	// connecting a dummy MIDI connection.
	const int ug_patch_automator_hidden_pin = 3; // "Hidden Output"
	assert(parameterSetter_->GetPlug(L"MIDI In") == parameterSetter_->GetPlug(0));

	auto currentContainer = this;
	auto innerPin = parameterSetter_->GetPlug(0);

	// handle situation of patch_automator in outer container by routing upward.
	while(!currentContainer->automation_input_device)
	{
		ug_base* iomod{};
		ug_base* outerContainer{};
		auto oversampler = dynamic_cast<ug_oversampler*>(currentContainer->AudioMaster());

		if (oversampler)
		{
			return; // FAILS due to outer containers still under construction, needs major rethink. !!!! (should work without oversamplers)
			//iomod = oversampler->oversampler_in;
			//outerContainer = oversampler->parent_container;
		}
		else
		{
			outerContainer = currentContainer;
			iomod = currentContainer->at(0)->findIoMod();
			if (!iomod)
			{
				iomod = ModuleFactory()->GetById(L"IO Mod")->BuildSynthOb();
				currentContainer->AddUG(iomod);
				iomod->SetupWithoutCug();
			}
		}

		auto ioPin = new UPlug(iomod, DR_OUT, DT_MIDI);
		iomod->AddPlug(ioPin);

		connect(ioPin, innerPin);

		auto outerPin = new UPlug(currentContainer, DR_IN, DT_MIDI);
		outerContainer->AddPlug(outerPin);

		outerPin->TiedTo = ioPin;
		ioPin->TiedTo = outerPin;

		// up one.
		if(currentContainer->parent_container)
		{
			currentContainer = currentContainer->parent_container;
		}
		else
		{
			// TODO: This will likely fail if oversampler don't have it's own patch-atomator,
			// because oversamplers get finalised while outer container is still being constructed
			// and at this point we may not have built the main-container's ug_patch_automator becuase we haven't fully parsed it's children.

			// oversampler?
			if(!oversampler)
			{
				// assert(false); // expected top-level container to have a patch_automator ('automation_input_device')
				return; // FAIL!!!!
			}
			currentContainer = oversampler->parent_container;
		}
		innerPin = outerPin;
	}

	// connect outer dummy to patch_automator
	connect(
		currentContainer->automation_input_device->GetPlug(ug_patch_automator_hidden_pin),
		innerPin
	);
}

// This is handy for snapshots, as appears to be called once per container, excluding in-lined containers.
void ug_container::SetUnusedPlugs2()
{
	front()->SetUnusedPlugs2();
}

void ug_container::setPatchManager(IDspPatchManager* p_patch_mgr)
{
	assert( m_patch_manager == 0 );
	m_patch_manager = p_patch_mgr;
}

bool ug_container::has_own_patch_manager()
{
	return m_patch_manager != 0;
}

IDspPatchManager* ug_container::get_patch_manager()
{
	if( m_patch_manager )
	{
		return m_patch_manager;
	}

	assert( ParentContainer() && "main container must have patch manager" );

	return ug_base::get_patch_manager();
}

/*
void ug_container::HandleEvent(SynthEditEvent* e)
{
	// send event to ug
	switch( e->eventType )
	{
	case UET_PROG_CHANGE:
	{
		// handle container's having patch slaved to parent (yet having own MIDI-CV)
		// not common, because usually when patch slaved to parent, container is expanded 'inline', exception is sub-container's with MIDI-CV
		SendProgChange( patch_control_container, e->parm1 );
	}
	break;

	default:
		ug_base::HandleEvent( e );
	};
}
*/

void ug_container::OnUiNotify2( int p_msg_id, my_input_stream& p_stream )
{
	ug_base::OnUiNotify2( p_msg_id, p_stream );

#if 0
	if( p_msg_id == id_to_long("setp") /*|| p_msg_id == id_to_long("mono") */) // Patch Change
	{
		m_patch_manager->OnUiMsg( p_msg_id, p_stream);
	}
#endif
	if( p_msg_id == id_to_long("setc") ) // MIDI Channel Change
	{
		int channel;
		p_stream >> channel;
		automation_input_device->setMidiChannel( channel );
		automation_output_device->setMidiChannel( channel );
	}
}

timestamp_t ug_container::CalculateOversampledTimestamp( ug_container* top_container, timestamp_t timestamp )
{
	if( top_container == this )
	{
		return timestamp;
	}

	if( ParentContainer() )
	{
		return ParentContainer()->CalculateOversampledTimestamp( top_container, timestamp );
	}
	else
	{
		return AudioMaster()->CalculateOversampledTimestamp( top_container, timestamp );
	}
}

void ug_container::SetPPVoiceNumber(int n)
{
	ug_base::SetPPVoiceNumber(n);

	for( auto it = begin() ; it != end() ; ++it )
	{
		(*it)->SetPPVoiceNumber(n);
	}
}

void ug_container::DoVoiceRefresh()
{
	if (++nextRefreshVoice_ >= size())
	{
		if (nextRefreshVoice_ > 251) // don't kick of next refresh right away. pause a little.
		{
			nextRefreshVoice_ = -1; // So next time called, will increment to zero.
		}
	}
	else
	{
#if 1 //defined( DEBUG_VOICE_ALLOCATION )
//		_RPT1(_CRT_WARN, "Voice-Refresh V %d.\n", nextRefreshVoice_);
#endif

		auto refresh = ((voiceAllocationMode_ >> 19) & 0x01) == NR_VOICE_REFRESH_ON;

		if(refresh)
			at(nextRefreshVoice_)->refresh();
	}
}

void ug_container::SetContainerPolyphonic()
{
	is_polyphonic = true;
}
