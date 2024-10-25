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

		presets.push_back(
			{
				preset.name,
				preset.category,
				-1,
				dir_entry.path().wstring(),
				0,
				false,
				false
			});
	}
}

void SubPresetManager::SavePresetAs(const std::string& presetName)
{
	std::filesystem::path rootPresetFolder = BundleInfo::instance()->getPresetFolder();
	std::filesystem::path subPresetFolder = rootPresetFolder / "SubPresets";

	assert(!rootPresetFolder.empty()); // you need to call BundleInfo::initPresetFolder(manufacturer, product) when initializing this plugin.

	std::filesystem::create_directory(subPresetFolder);

	const auto fullPath = subPresetFolder / (presetName + ".xmlpreset");

	DawPreset preset = *controller.getPreset(presetName);

	// filter the parameters we want to save.
	std::map<int32_t, paramValue> params;
	for (auto p : parameters_)
	{
		params[p] = preset.params[p];
	}

	std::swap(params, preset.params);

	std::ofstream myfile;
	myfile.open(fullPath);

	myfile << preset.toString(BundleInfo::instance()->getPluginId());

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

	if (xml.empty())
		return;

	DawPreset subpreset(parametersInfo, xml);

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
