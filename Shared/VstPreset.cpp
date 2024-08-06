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
	// Steinbergs implementation don't handle UTF8
	class FileStreamUtf8 : public IBStream
	{
	public:
		//------------------------------------------------------------------------
#ifdef _WIN32
		static IBStream* open(const wchar_t* filename, const wchar_t* mode)
		{
			FILE* file = _wfopen(filename, mode);
#else
		static IBStream* open(const char* filename, const char* mode)
		{
			FILE* file = fopen(filename, mode);
#endif
			return file ? new FileStreamUtf8(file) : nullptr;
		}

		//---from FUnknown------------------
		DECLARE_FUNKNOWN_METHODS

		//---from IBStream------------------
		tresult PLUGIN_API read(void* buffer, int32 numBytes, int32* numBytesRead = nullptr) SMTG_OVERRIDE
		{
			size_t result = fread(buffer, 1, static_cast<size_t> (numBytes), file);
			if (numBytesRead)
				*numBytesRead = (int32)result;
			return static_cast<int32> (result) == numBytes ? kResultOk : kResultFalse;
		}
		tresult PLUGIN_API write(void* buffer, int32 numBytes, int32* numBytesWritten = nullptr) SMTG_OVERRIDE
		{
			size_t result = fwrite(buffer, 1, static_cast<size_t> (numBytes), file);
			if (numBytesWritten)
				*numBytesWritten = (int32)result;
			return static_cast<int32> (result) == numBytes ? kResultOk : kResultFalse;
		}
		tresult PLUGIN_API seek(int64 pos, int32 mode, int64* result = nullptr) SMTG_OVERRIDE
		{
			if (fseek(file, (int32)pos, mode) == 0)
			{
				if (result)
					*result = ftell(file);
				return kResultOk;
			}
			return kResultFalse;
		}
		tresult PLUGIN_API tell(int64* pos) SMTG_OVERRIDE
		{
			if (pos)
				*pos = ftell(file);
			return kResultOk;
		}

		//------------------------------------------------------------------------
	protected:
		FileStreamUtf8(FILE* file) : file(file) { FUNKNOWN_CTOR }
		virtual ~FileStreamUtf8()
		{
			fclose(file);
			FUNKNOWN_DTOR
		}

		FILE* file;
	};

	IMPLEMENT_FUNKNOWN_METHODS(FileStreamUtf8, IBStream, IBStream::iid)

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
		Steinberg::tresult PLUGIN_API initialize(FUnknown* /*context*/) override
		{
			assert(false);
			return 0;
		}

		/** This function is called before the Plug-in is unloaded and can be used for
			cleanups. You have to release all references to any host application interfaces. */
		Steinberg::tresult PLUGIN_API terminate() override
		{
			assert(false);
			return 0;
		}

		/** Called before initializing the component to get information about the controller class. */
		Steinberg::tresult PLUGIN_API getControllerClassId(Steinberg::TUID /*classId*/) override
		{
			assert(false);
			return 0;
		}

		/** Called before 'initialize' to set the component usage (optional). See \ref IoModes */
		Steinberg::tresult PLUGIN_API setIoMode(Steinberg::Vst::IoMode /*mode*/) override
		{
			assert(false);
			return 0;
		}

		/** Called after the Plug-in is initialized. See \ref MediaTypes, BusDirections */
		Steinberg::int32 PLUGIN_API getBusCount(Steinberg::Vst::MediaType /*type*/, Steinberg::Vst::BusDirection /*dir*/) override
		{
			assert(false);
			return 0;
		}

		/** Called after the Plug-in is initialized. See \ref MediaTypes, BusDirections */
		Steinberg::tresult PLUGIN_API getBusInfo(Steinberg::Vst::MediaType /*type*/, Steinberg::Vst::BusDirection /*dir*/, Steinberg::int32 /*index*/, Steinberg::Vst::BusInfo& /*bus*/ /*out*/) override
		{
			assert(false);
			return 0;
		}

		/** Retrieves routing information (to be implemented when more than one regular input or output bus exists).
			The inInfo always refers to an input bus while the returned outInfo must refer to an output bus! */
		Steinberg::tresult PLUGIN_API getRoutingInfo(Steinberg::Vst::RoutingInfo& /*inInfo*/, Steinberg::Vst::RoutingInfo& /*outInfo*/ /*out*/) override
		{
			assert(false);
			return 0;
		}

		/** Called upon (de-)activating a bus in the host application. The Plug-in should only processed an activated bus,
			the host could provide less see \ref AudioBusBuffers in the process call (see \ref IAudioProcessor::process) if last buses are not activated */
		Steinberg::tresult PLUGIN_API activateBus(Steinberg::Vst::MediaType /*type*/, Steinberg::Vst::BusDirection /*dir*/, Steinberg::int32 /*index*/, Steinberg::TBool /*state*/) override
		{
			assert(false);
			return 0;
		}

		/** Activates / deactivates the component. */
		Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool /*state*/) override
		{
			assert(false);
			return 0;
		}

		/** Sets complete state of component. */
		Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* /*state*/) override
		{
			assert(false);
			return 0;
		}

		/** Retrieves complete state of component. */
		Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) override
		{
			return patchManager_->getState(state, preset_);
		}

	};

	IMPLEMENT_REFCOUNT(Vst3PresetProxy);

	tresult PLUGIN_API Vst3PresetProxy::queryInterface(FIDString piid, void** obj)
	{
		QUERY_INTERFACE(piid, obj, IComponent::iid, IComponent)
		QUERY_INTERFACE(piid, obj, IPluginBase::iid, IPluginBase)
		QUERY_INTERFACE(piid, obj, FUnknown::iid, FUnknown)
			*obj = 0;
		return kNoInterface;
	}

	struct PresetWriterHelper : public IVstPresetWriter
	{
		std::string presetXml;

		PresetWriterHelper(std::string ppresetXml) : presetXml(ppresetXml) {}

		int getState(Steinberg::IBStream* state, int /*patch*/) override
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
		size_t bytes_required = 1 + wcstombs(0, p_cstring.c_str(), 0);
#endif

		std::string res;
		res.resize(bytes_required);

#if defined(_WIN32)
		WideCharToMultiByte(CP_ACP, 0, p_cstring.c_str(), -1, (LPSTR) res.data(), (int)bytes_required, NULL, NULL);
#else
		wcstombs((char*)res.data(), p_cstring.c_str(), bytes_required);
#endif

		return res;
	}

	void WritePreset(std::wstring wfilename, std::string categoryName, std::string vendorName, std::string productName, Steinberg::FUID *processorId, std::string xmlPreset)
	{
		// Steinberg::Vst::FileStream does not handle UTF8.
//		std::string filename_asci = WStringToMultibyte(wfilename);

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

#ifdef _WIN32
		auto stream = FileStreamUtf8::open(wfilename.c_str(), L"wb");
#else
		std::string filename_utf8 = WStringToUtf8(wfilename);
		auto stream = FileStreamUtf8::open(filename_utf8.c_str(), "wb");
#endif
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

#ifdef _WIN32
		auto stream = FileStreamUtf8::open(wfilename.c_str(), L"rb");
#else
		std::string filename_utf8 = WStringToUtf8(wfilename);
		auto stream = FileStreamUtf8::open(filename_utf8.c_str(), "rb");
#endif

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
