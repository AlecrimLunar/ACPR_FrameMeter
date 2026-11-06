#include "modMenu.h"
#include <cstdint>
#include "baseMod/baseMod.hpp"
#include "settings/settings.h"

static BaseMod::Api* _bmApi = nullptr;
constexpr int numEntries = 12;
static const char* onOffLabels[2] = {"OFF", "ON"};

BASEMOD_CALL void OnDisplayChanged(int32_t value) {
    auto& settings = SettingsManager::GetInstance();
    if (_bmApi) {
        ActiveGameMode mode = GetActiveGameMode(_bmApi);
        settings.SetDisplayForMode(mode, value);
    }
    settings.Serialize(settingsPath);
}

BASEMOD_CALL void Defaults() {
    auto& settings = SettingsManager::GetInstance();
    settings.SetToDefaults();
    if (_bmApi) {
        settings.SyncDisplayWithMode(GetActiveGameMode(_bmApi));
    }
    settings.Serialize(settingsPath);
}

static BaseMod::ModMenuEntry entries[numEntries] = {
    {"DISPLAY", &SettingsManager::GetInstance().Display, 0, 1, onOffLabels, nullptr, OnDisplayChanged},
    {"OPACITY", &SettingsManager::GetInstance().Opacity, 0, 255},
    {"Y POS", &SettingsManager::GetInstance().yPos, 0, 480},
    {"Z POS", &SettingsManager::GetInstance().zPos, 0, 800},
    {"RECORD HITSTOP", &SettingsManager::GetInstance().RecordHitstop, 0, 1, onOffLabels},
    {"RECORD SUPERFLASH", &SettingsManager::GetInstance().RecordSuperFlash, 0, 1, onOffLabels},
    {"IGNORE DISTANT PROJECTILES", &SettingsManager::GetInstance().IgnoreDistantProjectiles, 0, 1, onOffLabels},
    {"DISPLAY UNTECH GAUGES", &SettingsManager::GetInstance().DisplayUntechGauges, 0, 1, onOffLabels},
    {"DISPLAY DIZZY GAUGE", &SettingsManager::GetInstance().DisplayDizzyGauge, 0, 1, onOffLabels},
    {"DISPLAY GAUGE VALUES", &SettingsManager::GetInstance().DisplayGaugeNumbers, 0, 1, onOffLabels},
    {"DISPLAY FRAME RUN SUMS", &SettingsManager::GetInstance().DisplayRunSums, 0, 1, onOffLabels},
    {"DEFAULT", nullptr, 0, 0, nullptr, Defaults},
};

void UpdateModMenuForMode(ActiveGameMode mode) {
    if (mode == ActiveGameMode::ONLINE_MATCH) {
        entries[0].MaxValue = 0;
    } else {
        entries[0].MaxValue = 1;
    }
}

void RegisterModMenu(BaseMod::Api* bmApi) {
    _bmApi = bmApi;
    bmApi->ModMenu.RegisterMenuTab("FRAME METER", entries, numEntries);
}
