#include <filesystem>
#include <fstream>
#include "SubPresetManager.h"
#include "BundleInfo.h"
#include "Controller.h"

void SubPresetManager::init(std::span<const int32_t> params)
{
	currentPresetIndex = 0;
	parameters_.clear();
	parameters_.insert(parameters_.end(), params.begin(), params.end());

	parametersInfo.clear();
	for (auto p : parameters_)
	{
		assert(controller.parametersInfo.find(p) != controller.parametersInfo.end()); // is this a persistant parameter?

		parametersInfo[p] = controller.parametersInfo[p];
	}

	ScanPresets();
}

void SubPresetManager::ScanPresets()
{
	const char* xmlPresetExt = ".xmlpreset";

	presets.clear();

	std::string presetCategory{ "AI Presets" };

	// blank preset to represet current unsaved preset.
	presets.push_back(
		{
			{},		// name
			{},		// category
			-1,		// factory preset
			{},		// file path
			0,		// hash
			false,	// is factory
			true	// is session
		});

	std::filesystem::path rootPresetFolder = BundleInfo::instance()->getPresetFolder();
	std::filesystem::path subPresetFolder = rootPresetFolder / "SubPresets";

    if(!std::filesystem::exists(subPresetFolder))
        return;
    
	for (auto const& dir_entry : std::filesystem::directory_iterator{ subPresetFolder })
	{
		if (dir_entry.path().extension() != xmlPresetExt)
			continue;

		std::string xml;
		controller.FileToString(dir_entry.path(), xml);

		if (xml.empty())
			continue;

		DawPreset preset(parametersInfo, xml);
		
		// calc the hash omitting the name
		const auto name = preset.name;
		preset.name.clear();
		preset.calcHash();

		presets.push_back(
			{
				name,
				preset.category,
				-1,
				dir_entry.path().wstring(),
				preset.hash,
				false,
				false
			});
	}

	syncPresetControls();
}

DawPreset SubPresetManager::getPreset()
{
	DawPreset currentPreset = *controller.getPreset();

	// filter the parameters.
	{
		std::map<int32_t, paramValue> params;
		for (auto p : parameters_)
		{
			params[p] = currentPreset.params[p];
		}

		std::swap(params, currentPreset.params);

		currentPreset.name.clear(); // ignore the name from the main plugin preset.
		currentPreset.calcHash();
	}

	return currentPreset;
}

void SubPresetManager::importPreset(int32_t presetTypeId, std::string fullPath)
{
	std::string xml;
	controller.FileToString(toPlatformString(fullPath), xml);

	if (!xml.empty() && '<' != xml[0])
	{
		Scramble(xml); // decrypt
	}

	setPreset(xml);
}

void SubPresetManager::exportPreset(int32_t presetTypeId, std::string fullPath)
{
	std::filesystem::path path(fullPath);
	path.replace_extension("optimus");

	DawPreset currentPreset = getPreset();
	auto presetXml = currentPreset.toString(presetTypeId, path.stem().string());

	if (encryptPresets)
	{
		Scramble(presetXml); // encrypt
	}

	std::ofstream myfile;
	myfile.open(path.c_str());

	myfile << presetXml;
}

void SubPresetManager::syncPresetControls()
{
	DawPreset currentPreset = getPreset();

	for (int i = 0; i < presets.size(); ++i)
	{
		if (presets[i].hash == currentPreset.hash)
		{
			currentPresetIndex = i;
			break;
		}
	}
}

void SubPresetManager::SavePresetAs(const std::string& presetName)
{
	std::filesystem::path rootPresetFolder = BundleInfo::instance()->getPresetFolder();
	std::filesystem::path subPresetFolder = rootPresetFolder / "SubPresets";

	assert(!rootPresetFolder.empty()); // you need to call BundleInfo::initPresetFolder(manufacturer, product) when initializing this plugin.

	std::filesystem::create_directory(rootPresetFolder);
	std::filesystem::create_directory(subPresetFolder);

	const auto fullPath = subPresetFolder / (presetName + ".xmlpreset");

	DawPreset currentPreset = getPreset();

	std::ofstream myfile;
	myfile.open(fullPath);

	myfile << currentPreset.toString(BundleInfo::instance()->getPluginId(), presetName);

	myfile.close();

//	setModified(false);
//	undoManager.initial(this, getPreset());

	ScanPresets();

	currentPresetIndex = 0;

	// find the new preset and select it.
	for (int32_t presetIndex = 0; presetIndex < presets.size(); ++presetIndex)
	{
		if (presets[presetIndex].name == presetName)
		{
			currentPresetIndex = presetIndex;
			break;
		}
	}

	onPresetChanged();
}

void SubPresetManager::DeletePreset(int presetIndex)
{
	if (presetIndex < 0 || presetIndex >= presets.size())
		return;

	std::filesystem::remove(presets[presetIndex].filename);

	presets.erase(presets.begin() + presetIndex);

	// select previous preset
	currentPresetIndex = (std::max)(0, presetIndex - 1);

	onPresetChanged();
}

void SubPresetManager::setPresetIndex(int presetIndex)
{
	if (presetIndex < 0 || presetIndex >= presets.size())
		return;

	currentPresetIndex = presetIndex;

	// load sub-preset from file.
	std::string xml;
	controller.FileToString(toPlatformString(presets[presetIndex].filename), xml);

	setPreset(xml);
}

void SubPresetManager::setPreset(std::string xml)
{
	if (xml.empty())
		return;

	DawPreset subpreset(parametersInfo, xml);

	for (auto& p : subpreset.params)
		controller.setParameterValue(true, p.first, gmpi::MP_FT_GRAB);

	for (auto& p : subpreset.params)
	{
		const float val = *(float*)p.second.rawValues_[0].data();
		controller.setParameterValue(val, p.first);

		_RPTN(0, "%f\n", val);
	}

	// ensure this is recorded as a single undo transaction. (undo don't work since these are IPC)
	controller.undoTransanctionStart();
	for (auto& p : subpreset.params)
		controller.setParameterValue(false, p.first, gmpi::MP_FT_GRAB);
	controller.undoTransanctionEnd();

#if 0 // didn't work due to ignore-PC on relevant params
	// apply sub-preset to full preset
	DawPreset fullpreset = *controller.getPreset();

	for (auto& p : subpreset.params)
	{
		if (fullpreset.params.find(p.first) == fullpreset.params.end())
			continue;

		_RPTN(0, "%f, %f\n", *(float*)fullpreset.params[p.first].rawValues_[0].data(), *(float*)p.second.rawValues_[0].data());
		fullpreset.params[p.first].rawValues_ = p.second.rawValues_;
	}

	controller.setPresetFromSelf(&fullpreset);
#endif
}

std::pair<bool, bool> SubPresetManager::CategorisePresetName(const std::string& newName)
{
	bool isReadOnly = false;
	bool isExistingName = false;

	for (int i = 0; i < getPresetCount(); ++i)
	{
		auto preset = getPresetInfo(i);
		if (preset.isSession)
			continue;

		if (preset.name == newName)
		{
			isReadOnly = preset.isFactory;
			isExistingName = true;
			break;
		}
	}

	return { isReadOnly, isExistingName };
}
