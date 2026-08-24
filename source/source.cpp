#include <cstdint>
#include <exception>
#include <iostream>
#include <string>
#include <windows.h>
#include "gearLoader/gearLoader.hpp"
#include "baseMod/baseMod.hpp"
#include "frameMeter/frameMeter.h"
#include "frameMeter/gameStateStore.h"
#include "hooks/hooks.h"
#include "render/render.h"
#include "settings/modMenu.h"
#include "settings/settings.h"

constexpr const char* settingsPath = "./mods/FrameMeter/settings.bin";

struct CustomContext {
    BaseMod::Api* bmApi;
    GearLoader::Api* glApi;
};
static CustomContext uCtx;

void BASEMOD_CALL OnSave(
    void* _unused,
    const BaseMod::HookContext* ctx,
    const BaseMod::SaveGameInfo* info
) {
    SettingsManager::GetInstance().Serialize(settingsPath);
}

inline bool IsNetplayMatch(BaseMod::Api* api) {
    return api->GameData.GetMainMenuSelection() == ggxxacpr::MenuItem::ONLINE &&
        (api->GameData.GetGameModeFeatureFlags() & ggxxacpr::GameModeFeatureFlags::VERSUS) !=
            ggxxacpr::GameModeFeatureFlags::NONE;
}

void BASEMOD_CALL BeforeUpdate(
    CustomContext* uCtx,
    const BaseMod::HookContext* ctx,
    const BaseMod::GameUpdateInfo* info
) {
    if (uCtx->bmApi->GameData.IsInGame() &&
        !uCtx->bmApi->GameData.GetPauseState() &&
        !IsNetplayMatch(uCtx->bmApi)
    ) {
        auto& store = GameStateStore::GetInstance();
        auto P1 = uCtx->bmApi->GameData.GetPlayer(0);
        auto P2 = uCtx->bmApi->GameData.GetPlayer(1);
        if (P1.isValid()) store.P1Hitstop = P1.hitstopTime();
        if (P2.isValid()) store.P2Hitstop = P2.hitstopTime();
    }
}

void BASEMOD_CALL AfterUpdate(
    CustomContext* uCtx,
    const BaseMod::HookContext* ctx,
    const BaseMod::GameUpdateInfo* info
) {
    try {
        static FrameMeter frameMeter;
        auto api = uCtx->bmApi;
        
        if (api->GameData.IsInGame()) {
            // Disabled during netplay
            if (IsNetplayMatch(api)) return;

            if (!api->GameData.GetPauseState()) {
                frameMeter.Update(api, GameStateStore::GetInstance());
            }
            
            Draw(frameMeter, api);
            DrawUntechGauge(api);
            DrawHitStunDecayGauge(api);
            DrawStunGauge(api);
        } else {
            frameMeter = FrameMeter();
        }
    } catch (std::exception e) {
        std::cout << "[FrameMeter][ERROR] Exception caught: " << e.what() << std::endl;
    }
}


GEARLOADER_EXPORT void GEARLOADER_CALL Init(GearLoaderContext* ctx, GearLoaderApi* api) {
    auto glApi = new GearLoader::Api(api, ctx);

    const BaseMod_Api* retApi;
    SemanticVersion retVer;
    int result = glApi->RetrieveModApi<BaseMod_Api>(
        BASEMOD_NAME,
        BASEMOD_API_VERSION,
        &retApi,
        &retVer
    );

    if (result || !retApi) {
        glApi->Log(GearLoader::LogLevel::ERR, "BaseMod API wasn't found");
        return;
    }

    BaseMod::Api* bmApi = new BaseMod::Api(retApi);
    uCtx.bmApi = bmApi;
    uCtx.glApi = glApi;

    LoadGlyphAtlasResources();
    SettingsManager::GetInstance().Deserialize(settingsPath);
    bmApi->Hooks.AfterSaveGame(OnSave, nullptr);
    bmApi->Hooks.BeforeGameUpdate(BeforeUpdate, &uCtx);
    bmApi->Hooks.AfterGameUpdate(AfterUpdate, &uCtx);
    RegisterModMenu(bmApi);

    InstallCustomHooks();

    std::cout << "[FrameMeter] Initialized" << std::endl;
}
