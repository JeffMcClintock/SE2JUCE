#include "VstPreset.h"
#include "conversion.h"
#include "../tinyXml2/tinyxml2.h"

#include "public.sdk/source/vst/vstpresetfile.h"
#include "public.sdk/source/common/memorystream.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/base/ibstream.h"

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace VstPresetUtil
{
	struct IVstPresetWriter
	{
		virtual int getState(Steinberg::IBStream* state, int patch) = 0;
	};

	// Fake Processor for obtaining preset
	class Vst3PresetProxy :
		public Steinberg::Vst::IComponent
	{
		IVstPresetWriter* patchManager_;
		int preset_;

	public:
		Vst3PresetProxy(IVstPresetWriter* pm, int preset = 0) : patchManager_(pm), preset_(preset) {}

		DECLARE_FUNKNOWN_METHODS

		/** The host passes a number of interfaces as context to initialize the Plug-in class.
			@note Extensive memory allocations etc. should be performed in this method rather than in the class' constructor!
			If the method does NOT return kResultOk, the object is released immediately. In this case terminate is not called! */
		virtual Steinberg::tresult PLUGIN_API initialize(FUnknown* context)
		{
			assert(false);
			return 0;
		}

		/** This function is called before the Plug-in is unloaded and can be used for
			cleanups. You have to release all references to any host application interfaces. */
		virtual Steinberg::tresult PLUGIN_API terminate()
		{
			assert(false);
			return 0;
		}

		/** Called before initializing the component to get information about the controller class. */
		virtual Steinberg::tresult PLUGIN_API getControllerClassId(Steinberg::TUID classId)
		{
			assert(false);
			return 0;
		}

		/** Called before 'initialize' to set the component usage (optional). See \ref IoModes */
		virtual Steinberg::tresult PLUGIN_API setIoMode(Steinberg::Vst::IoMode mode)
		{
			assert(false);
			return 0;
		}

		/** Called after the Plug-in is initialized. See \ref MediaTypes, BusDirections */
		virtual Steinberg::int32 PLUGIN_API getBusCount(Steinberg::Vst::MediaType type, Steinberg::Vst::BusDirection dir)
		{
			assert(false);
			return 0;
		}

		/** Called after the Plug-in is initialized. See \ref MediaTypes, BusDirections */
		virtual Steinberg::tresult PLUGIN_API getBusInfo(Steinberg::Vst::MediaType type, Steinberg::Vst::BusDirection dir, Steinberg::int32 index, Steinberg::Vst::BusInfo& bus /*out*/)
		{
			assert(false);
			return 0;
		}

		/** Retrieves routing information (to be implemented when more than one regular input or output bus exists).
			The inInfo always refers to an input bus while the returned outInfo must refer to an output bus! */
		virtual Steinberg::tresult PLUGIN_API getRoutingInfo(Steinberg::Vst::RoutingInfo& inInfo, Steinberg::Vst::RoutingInfo& outInfo /*out*/)
		{
			assert(false);
			return 0;
		}

		/** Called upon (de-)activating a bus in the host application. The Plug-in should only processed an activated bus,
			the host could provide less see \ref AudioBusBuffers in the process call (see \ref IAudioProcessor::process) if last buses are not activated */
		virtual Steinberg::tresult PLUGIN_API activateBus(Steinberg::Vst::MediaType type, Steinberg::Vst::BusDirection dir, Steinberg::int32 index, Steinberg::TBool state)
		{
			assert(false);
			return 0;
		}

		/** Activates / deactivates the component. */
		virtual Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool state)
		{
			assert(false);
			return 0;
		}

		/** Sets complete state of component. */
		virtual Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state)
		{
			assert(false);
			return 0;
		}

		/** Retrieves complete state of component. */
		virtual Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state)
		{
			return patchManager_->getState(state, preset_);
		}

	};

	IMPLEMENT_REFCOUNT(Vst3PresetProxy);

	tresult PLUGIN_API Vst3PresetProxy::queryInterface(FIDString iid, void** obj)
	{
		QUERY_INTERFACE(iid, obj, IComponent::iid, IComponent)
			QUERY_INTERFACE(iid, obj, IPluginBase::iid, IPluginBase)
			QUERY_INTERFACE(iid, obj, FUnknown::iid, FUnknown)
			*obj = 0;
		return kNoInterface;
	}

	struct PresetWriterHelper : public IVstPresetWriter
	{
		std::string presetXml;

		PresetWriterHelper(std::string ppresetXml) : presetXml(ppresetXml) {}

		int getState(Steinberg::IBStream* state, int patch) override
		{
			Steinberg::int32 chunkSize = (Steinberg::int32) presetXml.size();
			Steinberg::int32 bytesWritten;

			state->write(&chunkSize, sizeof(chunkSize), &bytesWritten);
			state->write((void*)presetXml.data(), chunkSize, &bytesWritten);

			return 0;
		}
	};

	void WriteAttribute(tinyxml2::XMLDocument &doc, tinyxml2::XMLElement* parentNode, const char* id, std::string value, const char* flags = "" )
	{
		auto xml = doc.NewElement("Attribute");
		parentNode->LinkEndChild(xml);
		xml->SetAttribute("id", id);
		xml->SetAttribute("value", value.c_str());
		xml->SetAttribute("type", "string");
		xml->SetAttribute("flags", flags);
	}

	inline std::string WStringToMultibyte(const std::wstring& p_cstring)
	{
#if defined(_WIN32)
		size_t bytes_required = 1 + WideCharToMultiByte(CP_ACP, 0, p_cstring.c_str(), -1, 0, 0, NULL, NULL);
#else
		// size_t bytes_required = 1 + p_cstring.size();
        mbstate_t state{};
        const wchar_t* src = p_cstring.c_str();
        const size_t bytes_required = /* 1 + */ wcsrtombs(nullptr, &src, 0, &state);
#endif

		std::string res;
		res.resize(bytes_required);

#if defined(_WIN32)
		WideCharToMultiByte(CP_ACP, 0, p_cstring.c_str(), -1, (LPSTR) res.data(), (int)bytes_required, NULL, NULL);
#else
		wcstombs(res.data(), p_cstring.c_str(), bytes_required);
#endif

		return res;
	}

	void WritePreset(std::wstring wfilename, std::string categoryName, std::string vendorName, std::string productName, Steinberg::FUID *processorId, std::string xmlPreset)
	{
		// Steinberg::Vst::FileStream does not handle UTF8.
		std::string filename_asci = WStringToMultibyte(wfilename);

		tinyxml2::XMLDocument doc;
		doc.LinkEndChild(doc.NewDeclaration());

		auto metaInfo = doc.NewElement("MetaInfo");
		doc.LinkEndChild(metaInfo);

		WriteAttribute(doc, metaInfo, "MediaType",		"VstPreset",		"writeProtected");
		WriteAttribute(doc, metaInfo, "PlugInCategory",	"Instrument|Synth", "writeProtected"); // !!!! could be effect !!!! (don't seem to matter)
		WriteAttribute(doc, metaInfo, "PlugInName",		productName,		"writeProtected");
		WriteAttribute(doc, metaInfo, "PlugInVendor",	vendorName,			"writeProtected");

		if (!categoryName.empty())
		{
			WriteAttribute(doc, metaInfo, "MusicalCategory", categoryName); // e.g. "Bass"
		}

		// populate document here ...
		tinyxml2::XMLPrinter vst3MetaInfoPrinter;
		doc.Accept(&vst3MetaInfoPrinter);

		auto stream = Steinberg::Vst::FileStream::open(filename_asci.c_str(), "wb");
		if (stream)
		{
			PresetWriterHelper component(xmlPreset);
			Vst3PresetProxy proxy(&component);

			Steinberg::Vst::PresetFile::savePreset(stream, *processorId, &proxy, 0, vst3MetaInfoPrinter.CStr(), (Steinberg::int32) vst3MetaInfoPrinter.CStrSize() - 1); // CStrSize(): Note the size returned includes the terminating null.
			stream->release();
		}
	}

	// Returns XML payload string from SE vstpreset.
	std::string ReadPreset(std::wstring wfilename, std::string* returnCategory)
	{
		// Steinberg::Vst::FileStream does not handle UTF8.
		std::string filename_asci = WStringToMultibyte(wfilename);
		std::string returnPresetString;

		auto stream = Steinberg::Vst::FileStream::open(filename_asci.c_str(), "rb");
		if (stream)
		{
			PresetFile pf(stream);
			if (pf.readChunkList())
			{
				auto e = pf.getEntry(kComponentState);
				if (e)
				{
					ReadOnlyBStream* readOnlyBStream = new ReadOnlyBStream(stream, e->offset, e->size);
					FReleaser readOnlyBStreamReleaser(readOnlyBStream);

					int32 bytesRead;
					int32 chunkSize = 0;
					readOnlyBStream->read(&chunkSize, sizeof(chunkSize), &bytesRead);

					returnPresetString.resize(chunkSize);
					readOnlyBStream->read((void*)returnPresetString.data(), chunkSize);
				}

				e = pf.getEntry(kMetaInfo);
				if (e)
				{
					ReadOnlyBStream* readOnlyBStream = new ReadOnlyBStream(stream, e->offset, e->size);
					FReleaser readOnlyBStreamReleaser(readOnlyBStream);

					//int32 bytesRead;
					//int32 chunkSize = 0;
					//readOnlyBStream->read(&chunkSize, sizeof(chunkSize), &bytesRead);

					std::string metaInfoXml;
					metaInfoXml.resize(e->size);
					readOnlyBStream->read((void*)metaInfoXml.data(), e->size);

					tinyxml2::XMLDocument xmldoc;
					xmldoc.Parse(metaInfoXml.c_str());

					if (!xmldoc.Error())
					{
						auto metainfo = xmldoc.FirstChildElement("MetaInfo");
						if (metainfo)
						{
							for (auto AttributeE = metainfo->FirstChildElement("Attribute"); AttributeE; AttributeE = AttributeE->NextSiblingElement())
							{
								auto id = AttributeE->Attribute("id");
								auto val = AttributeE->Attribute("value");
								if (strcmp(id, "MusicalCategory") == 0)
								{
									if (returnCategory)
									{
										*returnCategory = val;
									}
								}
							}
						}
					}
				}
			}
			stream->release();
		}
		return returnPresetString;
	}
}
