#pragma once
#include <span>
#include <vector>
#include "Controller.h"

struct IPresetsModel
{
    virtual int getPresetCount() = 0;
    virtual int getPresetIndex() = 0; // index according to controller (not nesc order displayed in combo)
    virtual void setPresetIndex(int) = 0;
    virtual MpController::presetInfo getPresetInfo(int index) = 0;
    virtual std::pair<bool, bool> CategorisePresetName(const std::string& name) = 0;
    virtual void SavePresetAs(const std::string& presetName) = 0;
    virtual void DeletePreset(int presetIndex) = 0;
    virtual bool isPresetModified() = 0;
};

class SubPresetManager : public IPresetsModel
{
    class MpController& controller;
    std::map<int32_t, paramInfo> parametersInfo;		// for parsing xml presets
    std::vector<int32_t> parameters_;

    std::vector<MpController::presetInfo> presets;
    int currentPresetIndex = 0;
    void ScanPresets();
    void syncPresetControls();
    DawPreset getPreset();

public:
    // callback
	std::function<void()> onPresetChanged = [](){};
    bool encryptPresets = false;

	SubPresetManager(MpController& pcontroller) : controller(pcontroller)
    {
    }

    void init(std::span<const int32_t> params);

    void exportPreset(int32_t presetTypeId, std::string fullPath);
    void importPreset(int32_t presetTypeId, std::string fullPath);

    // IPresetsModel
    int getPresetCount() override { return static_cast<int>(presets.size()); }
    int getPresetIndex() override { return currentPresetIndex; }
    void setPresetIndex(int) override;
    void setPreset(std::string xml);

    MpController::presetInfo getPresetInfo(int index) override
    {
        if (index < 0 || index >= presets.size())
            return { {}, {}, -1, {}, 0, false, true };

        return presets[index];
    }
    void SavePresetAs(const std::string& presetName) override;
    void DeletePreset(int presetIndex) override;
    bool isPresetModified() override { return false; }
    std::pair<bool, bool> CategorisePresetName(const std::string& name) override;
};