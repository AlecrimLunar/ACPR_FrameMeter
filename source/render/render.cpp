#include "render.h"
#include <cstdint>
#include <unordered_map>
#include "gearLoader/ggxxacpr.hpp"
#include "baseMod/baseMod.hpp"
#include "frameMeter/frameMeter.h"
#include "settings/settings.h"

using Color = uint32_t;

constexpr Color meterBgColor = 0xFF000000;

static std::unordered_map<FrameType, Color> frameTypeColorMap = {
    {FrameType::NONE,               0xFF000000},
    {FrameType::NEUTRAL,            0xFF1B1B1B},
    {FrameType::MOVEMENT,           0xFF41F8FC},
    {FrameType::CH_STATE,           0xFF01B597},
    {FrameType::STARTUP,            0xFF01B597},
    {FrameType::ACTIVE,             0xFFCB2B67},
    {FrameType::ACTIVE_THROW,       0xFFCB2B67},
    {FrameType::RECOVERY,           0xFF006FBC},
    {FrameType::BLOCKSTUN,          0xFFC8C800},
    {FrameType::HITSTUN,            0xFFC8C800},
    {FrameType::TECHABLE_HITSTUN,   0xFF969600},
    {FrameType::KNOCK_DOWN_HITSTUN, 0xFF969600},
};
static std::unordered_map<FrameProperty, Color> framePropertyColorMap = {
    {FrameProperty::DEFAULT,           0x00000000},
    {FrameProperty::INVULN_FULL,       0xFFFFFFFF},
    {FrameProperty::INVULN_STRIKE,     0xFF0080FF},
    {FrameProperty::INVULN_THROW,      0xFFFF8000},
    {FrameProperty::PARRY,             0xFF785000},
    {FrameProperty::GUARD_POINT_FULL,  0xFF785000},
    {FrameProperty::GUARD_POINT_HIGH,  0xFF785000},
    {FrameProperty::GUARD_POINT_LOW,   0xFF785000},
    {FrameProperty::ARMOR,             0xFF785000},
    {FrameProperty::FRC,               0xFFFFFF00},
    {FrameProperty::SLASH_BACK,        0xFFFF0000},
};

// Drawing dimension constants
namespace Dim {
    constexpr int pipHeight = 8;
    constexpr int pipHeightEntity = 5;
    constexpr int pipWidth = 6;
    constexpr int pipSpacing = 1;
    constexpr int borderSize = 1;

    constexpr int propertyHighlightHeight = 2;

    constexpr int textMargin = 4;
    constexpr float textScale = 1.0f;
    constexpr int textHeight = static_cast<int>(15 * textScale);
    constexpr int textAdvantageXOffset = 148;
    constexpr int textTotalXOffset = 260;
};

inline uint32_t Alpha(uint32_t val) {
    return (val & 0xFFFFFF) | (SettingsManager::GetInstance().Opacity << 24);
}
void DrawFramePip(BaseMod::Api* api, Frame& frame, int x, int y, bool isEntityPip) {
    constexpr int maxProperties = 2;
    int pipHeight = isEntityPip ? Dim::pipHeightEntity : Dim::pipHeight;
    int zPos = SettingsManager::GetInstance().zPos;
    uint32_t alpha = (SettingsManager::GetInstance().Opacity & 0xFF) << 24;

    // main pip
    api->NativeFunctions.DrawQuad(
        x, y,
        x + Dim::pipWidth,
        y + pipHeight,
        zPos, Alpha(frameTypeColorMap[frame.Type])
    );

    int topHighlightCount = 0;
    for (int i = 0; i < frame.Properties.size(); i++) {
        if (frame.Properties[i] == FrameProperty::DEFAULT) continue;
        // bot highlight for FRC
        if (frame.Properties[i] == FrameProperty::FRC) {
            api->NativeFunctions.DrawQuad(
                x, y + pipHeight - Dim::propertyHighlightHeight,
                x + Dim::pipWidth,
                y + pipHeight,
                zPos, Alpha(framePropertyColorMap[frame.Properties[i]])
            );
        }
        // top highlights
        else if (topHighlightCount < maxProperties) {
            int highlightWidth = Dim::pipWidth / maxProperties;
            api->NativeFunctions.DrawQuad(
                x + (Dim::pipWidth / maxProperties) * topHighlightCount,
                y,
                x + Dim::pipWidth,
                y + Dim::propertyHighlightHeight,
                zPos, Alpha(framePropertyColorMap[frame.Properties[i]])
            );
            topHighlightCount++;
        }
    }
}

void DrawFrameRunSum(BaseMod::Api* api, int runsum, float x, float y, float z) {
    float alpha = SettingsManager::GetInstance().Opacity / 255.0f;
    if (runsum > 0) {
        int onesDigit = runsum % 10;
        int tensDigit = runsum / 10;
        DrawRunSumText(api, std::to_string(onesDigit), x, y + 1.0f, z, alpha, 1.0f, false, Font::BLACK);
        if (tensDigit > 0) {
            DrawRunSumText(api, std::to_string(tensDigit),
                x - (Dim::pipWidth + Dim::pipSpacing) + 1.0f,
                y + 1.0f, z, alpha, 1.0f, false, Font::BLACK);
        }
    }
}

void DrawMeterGroup(PlayerMeterGroup& group, BaseMod::Api* api, int x, int y, bool flip) {
    auto& settings = SettingsManager::GetInstance();
    int32_t zPos = settings.zPos;
    uint8_t alpha8 = static_cast<uint8_t>(settings.Opacity);
    uint32_t alphaComponent = (settings.Opacity & 0xFF) << 24;

    // background
    api->NativeFunctions.DrawQuad(
        x - Dim::borderSize,
        y - Dim::borderSize,
        x + (Dim::pipWidth + Dim::pipSpacing) * group.PlayerMeter.Frames.size() + (Dim::borderSize - Dim::pipSpacing),
        y + Dim::pipHeight + Dim::borderSize,
        zPos, meterBgColor & alphaComponent
    );

    int pipX = x;
    for (int i = 0; i < group.PlayerMeter.Frames.size(); i++) {
        DrawFramePip(api, group.PlayerMeter.FrameAt(i), pipX, y, false);
        if (settings.DisplayRunSums)
            DrawFrameRunSum(api, group.PlayerMeter.RunSumAt(i), pipX, y, zPos);
        // Draw frame divider | TODO: Model frame dividers in the FrameMeter class
        if (
            group.PlayerMeter.MoveStartAt(i) &&
            group.PlayerMeter.FrameAt(i).Type != FrameType::NEUTRAL &&
            group.PlayerMeter.FrameAt(i-1).Type != FrameType::NONE &&
            group.PlayerMeter.FrameAt(i-1).Type != FrameType::NEUTRAL &&
            i != 0
        ) {
            api->NativeFunctions.DrawQuad(pipX - 1, y - 1, pipX, y + Dim::pipHeight + 1, zPos - 1, 0xFFFFFFFF);
            api->NativeFunctions.DrawQuad(pipX - 1, y - 1, pipX + 1, y, zPos - 1, 0xFFFFFFFF);
            api->NativeFunctions.DrawQuad(pipX - 1, y + Dim::pipHeight, pipX + 1, y + Dim::pipHeight + 1, zPos - 1, 0xFFFFFFFF);
        }
        pipX += Dim::pipWidth + Dim::pipSpacing;
    }

    // Entity meter
    if (!group.HideEntityMeter) {
        int pipX = x;
        int entityY = flip ?
            y + Dim::pipHeight + Dim::pipSpacing :
            y - Dim::pipHeightEntity - Dim::pipSpacing;

        // background
        api->NativeFunctions.DrawQuad(
            x - Dim::borderSize,
            entityY - Dim::borderSize,
            x + (Dim::pipWidth + Dim::pipSpacing) * group.EntityMeter.Frames.size() + (Dim::borderSize - Dim::pipSpacing),
            entityY + Dim::pipHeightEntity + Dim::borderSize,
            zPos + 1, meterBgColor & alphaComponent
        );

        for (auto& frame : group.EntityMeter.Frames) {
            DrawFramePip(api, frame, pipX, entityY, true);
            pipX += Dim::pipWidth + Dim::pipSpacing;
        }
    }

    // Draw text
    int textY = flip ?
        y + Dim::pipHeight + (Dim::pipSpacing + Dim::pipHeightEntity + Dim::borderSize + Dim::textMargin) :
        y - (Dim::pipSpacing + Dim::pipHeightEntity + Dim::borderSize + Dim::textMargin) - Dim::textHeight;
    std::string startup = "STARTUP: " + (group.Startup == -1 ? "-" : std::to_string(group.Startup));
    std::string advantage = "ADV: " + (group.DisplayAdvantage ? std::to_string(group.Advantage) : "-");
    std::string total = "TOTAL: " + (group.Total == -1 ? "-" : std::to_string(group.Total));

    api->NativeFunctions.RenderCockpitFontText(
        startup.c_str(),
        x, textY, zPos, alpha8, Dim::textScale
    );
    api->NativeFunctions.RenderCockpitFontText(
        advantage.c_str(),
        x + Dim::textAdvantageXOffset, textY, zPos, alpha8, Dim::textScale
    );
    api->NativeFunctions.RenderCockpitFontText(
        total.c_str(),
        x + Dim::textTotalXOffset, textY, zPos, alpha8, Dim::textScale
    );
}

void DrawFrameMeter(FrameMeter& frameMeter, BaseMod::Api* api) {
    int length = (Dim::pipWidth + Dim::pipSpacing) * frameMeter.size() - Dim::pipSpacing + Dim::borderSize * 2;
    int x = (640 - length) / 2;
    int y = SettingsManager::GetInstance().yPos;
    DrawMeterGroup(frameMeter.P1Meter, api, x, y, false);
    DrawMeterGroup(frameMeter.P2Meter, api, x, y + Dim::pipHeight + Dim::pipSpacing, true);
}
inline void DrawGaugeNumbersForPlayer(BaseMod::Api* api, ggxxacpr::Player player) {
    enum {
        HEALTH, GUARD, BURST, TENSION,
        EDDIE,
        ZAPPA,
        ROBO_HEAT,
        ROBO_GAIN,
        ABA,
        HOS_ACTUAL,
        HOS_TARGET,
    };
    struct GaugeNumberPositions { int X, Y, Z; };
    static const GaugeNumberPositions dim[11] = {
        {104, 47,  269},
        {128, 68,  274},
        {254, 113, 268},
        {135, 440, 55 },
        {155, 421, 57 }, // EDDIE
        {255, 427, 58 }, // ZAPPA
        {255, 428, 57 }, // ROBO Heat
        {259, 420, 57 }, // ROBO Gain
        {220, 421, 64 }, // ABA
        {147, 427, 65 }, // HOS Actual
        {147, 415, 65 }, // HOS Target
    };

    constexpr int midPointX = 320.0f;
    constexpr uint32_t backdropWhite = 0xC0FFFFFF;
    constexpr uint32_t backdropBlack = 0xC0000000;
    
    if (!player.isValid()) return;

    bool isP1 = !player.playerIndex();
    bool textAlignment = isP1;
    int offsetFactor = isP1 ? -1 : 1;

    DrawRunSumText(api, std::to_string(player.health()), midPointX + dim[HEALTH].X * offsetFactor, dim[HEALTH].Y, dim[HEALTH].Z, 1.0f, 1.0f, textAlignment, Font::BLACK_BORDERED);
    DrawRunSumText(api, std::to_string(player.guardBar()), midPointX + dim[GUARD].X * offsetFactor, dim[GUARD].Y, dim[GUARD].Z, 1.0f, 1.0f, textAlignment, Font::BLACK_BORDERED);
    DrawRunSumText(api, std::to_string(player.burstMeter()), midPointX + dim[BURST].X * offsetFactor, dim[BURST].Y, dim[BURST].Z, 1.0f, 1.0f, textAlignment, Font::BLACK_BORDERED);
    DrawRunSumText(api, std::to_string(player.tension()), midPointX + dim[TENSION].X * offsetFactor, dim[TENSION].Y, dim[TENSION].Z, 1.0f, 1.0f, textAlignment, Font::BLACK_BORDERED);

    if (!player.getRaw()->playerEntityDataPtr) return;

    auto& charSpec = player.getRaw()->playerEntityDataPtr->characterSpecific;
    switch(player.id()) {
        case ggxxacpr::EntityId::EDDIE:
            DrawRunSumText(api, std::to_string(charSpec.Eddie.EddieGauge), midPointX + dim[EDDIE].X * offsetFactor, dim[EDDIE].Y, dim[EDDIE].Z, 1.0f, 1.0f, !textAlignment, Font::BLACK_BORDERED);
            break;
        case ggxxacpr::EntityId::ZAPPA:
            if (charSpec.Zappa.Summon == 4)
                DrawRunSumText(api, std::to_string(charSpec.Zappa.RaouTimer), midPointX + dim[ZAPPA].X * offsetFactor, dim[ZAPPA].Y, dim[ZAPPA].Z, 1.0f, 1.0f, textAlignment, Font::BLACK_BORDERED);
            break;
        case ggxxacpr::EntityId::ROBO_KY:
            DrawRunSumText(api, std::to_string(charSpec.RoboKy.HeatGain / 111 + 30), midPointX + dim[ROBO_GAIN].X * offsetFactor, dim[ROBO_GAIN].Y, dim[ROBO_GAIN].Z, 1.0f, 1.0f, textAlignment, Font::BLACK_BORDERED);
            DrawRunSumText(api, std::to_string(charSpec.RoboKy.Heat), midPointX + dim[ROBO_HEAT].X * offsetFactor, dim[ROBO_HEAT].Y, dim[ROBO_HEAT].Z, 1.0f, 1.0f, textAlignment, Font::BLACK_BORDERED);
            break;
        case ggxxacpr::EntityId::ABA:
            if (charSpec.ABA.InstallMode > 0)
                DrawRunSumText(api, std::to_string(charSpec.ABA.BloodGauge), midPointX + dim[ABA].X * offsetFactor, dim[ABA].Y, dim[ABA].Z, 1.0f, 1.0f, textAlignment, Font::BLACK_BORDERED);
            break;
        case ggxxacpr::EntityId::ORDER_SOL:
            DrawRunSumText(api, std::to_string(charSpec.OrderSol.ChargeLevelTarget), midPointX + dim[HOS_TARGET].X * offsetFactor, dim[HOS_TARGET].Y, dim[HOS_TARGET].Z, 1.0f, 1.0f, textAlignment, Font::BLACK_BORDERED);
            DrawRunSumText(api, std::to_string(charSpec.OrderSol.ChargeLevelActual), midPointX + dim[HOS_ACTUAL].X * offsetFactor, dim[HOS_ACTUAL].Y, dim[HOS_ACTUAL].Z, 1.0f, 1.0f, textAlignment, Font::BLACK_BORDERED);
            break;
    }
}
void DrawGaugeNumbers(BaseMod::Api* api) {
    DrawGaugeNumbersForPlayer(api, api->GameData.GetPlayer(0));
    DrawGaugeNumbersForPlayer(api, api->GameData.GetPlayer(1));
}

void Draw(FrameMeter& frameMeter, BaseMod::Api* bmApi) {
    if (SettingsManager::GetInstance().Display)
        DrawFrameMeter(frameMeter, bmApi);

    if (SettingsManager::GetInstance().DisplayGaugeNumbers)
        DrawGaugeNumbers(bmApi);
}

void DrawHitStunDecayGaugePerPlayer(ggxxacpr::Player player, BaseMod::Api* bmApi) {
    constexpr int screenWidth = 640;
    constexpr int baseX = 20;
    constexpr int y = 170;
    // constexpr int baseX = 107;
    // constexpr int y = 95;
    constexpr int height = 9;
    constexpr int width = 60;
    constexpr int borderSize = 1;
    constexpr int meterLengthFrames = 60;

    constexpr int maxDecayLvl = 5;
    uint32_t lvlColors[6] = {0xFF0000FF, 0xFF00FF00, 0xFFFFFF00, 0xFFFF8000, 0xFFFF0000, 0xFF800000};
    uint32_t lvlThresholds[6] {0, 180, 300, 420, 600, 860};

    auto native = bmApi->NativeFunctions;

    int pIndex = player.playerIndex();
    int x = pIndex ? baseX : screenWidth - baseX - width;
    // int z = SettingsManager::GetInstance().zPos;
    int z = 280;

    int comboTime = player.comboTime();
    int decayLvl = 0;
    for (int i = 0; i <= maxDecayLvl; i++) {
        if (comboTime < lvlThresholds[i]) {
            break;
        }
        decayLvl = i;
    }
    
    // Draw bg
    uint32_t bgColor = comboTime > 0 ?
        lvlColors[std::min(decayLvl + 1, maxDecayLvl)] :
        lvlColors[0];
    native.DrawQuad(x, y, x + width, y + height, z, bgColor);
    // Draw edges
    uint32_t color = lvlColors[decayLvl];
    native.DrawQuad(x,                      y, x + borderSize, y + height, z, color);
    native.DrawQuad(x + width - borderSize, y, x + width,      y + height, z, color);
    native.DrawQuad(x, y,                       x + width, y + borderSize, z, color);
    native.DrawQuad(x, y + height - borderSize, x + width, y + height,     z, color);

    // Draw fill
    if (player.comboTime() > 0) {
        int lvlStart = lvlThresholds[decayLvl];
        int lvlEnd = lvlThresholds[std::min(decayLvl + 1, maxDecayLvl)];
        int lvlSize = lvlEnd - lvlStart;
        if (lvlSize) {
            int fillLength = (lvlSize - player.comboTime() + lvlStart) * width / lvlSize;
            if (pIndex) {
                native.DrawQuad(x, y, x + fillLength, y + height, z, color);
            } else {
                native.DrawQuad(x + width - fillLength, y, x + width, y + height, z, color);
            }
        }
        // Gauge Text
        if (SettingsManager::GetInstance().DisplayGaugeNumbers) {
            constexpr int gaugeTextXOffset = 1;
            constexpr int gaugeTextYOffset = 1;
            int timeLeft = std::max(0, (lvlSize - player.comboTime() + lvlStart));
            if (pIndex) {
                DrawRunSumText(bmApi, std::to_string(timeLeft), x + gaugeTextXOffset, y + gaugeTextYOffset, z, 1.0f, 1.0f, false, Font::BLACK_BORDERED);
            } else {
                DrawRunSumText(bmApi, std::to_string(timeLeft), x + width - gaugeTextXOffset, y + gaugeTextYOffset, z, 1.0f, 1.0f, true, Font::BLACK_BORDERED);
            }
        }
    }

    // Draw label text
    constexpr int textYOffset = height + 1;
    if (pIndex) {
        DrawRunSumText(bmApi, "DECAY LVL: " + std::to_string(decayLvl), x, y + textYOffset, z, 1.0f, 1.0f, false, Font::BLACK_BORDERED);
    } else {
        DrawRunSumText(bmApi, "DECAY LVL: " + std::to_string(decayLvl), x + width, y + textYOffset, z, 1.0f, 1.0f, true, Font::BLACK_BORDERED);
    }
}
void DrawHitStunDecayGauge(BaseMod::Api* bmApi) {
    if (!SettingsManager::GetInstance().DisplayUntechGauges) return;

    auto p1 = bmApi->GameData.GetPlayer(0);
    auto p2 = bmApi->GameData.GetPlayer(1);

    if (p1.isValid()) DrawHitStunDecayGaugePerPlayer(p1, bmApi);
    if (p2.isValid()) DrawHitStunDecayGaugePerPlayer(p2, bmApi);
}
void DrawUntechGaugePerPlayer(ggxxacpr::Player player, BaseMod::Api* bmApi) {
    constexpr int screenWidth = 640;
    constexpr int baseX = 20;
    constexpr int y = 160;
    // constexpr int baseX = 107;
    // constexpr int y = 85;
    constexpr int height = 9;
    constexpr int width = 60;
    constexpr int borderSize = 1;
    constexpr int meterLengthFrames = 60;
    constexpr uint32_t color = 0xFF00FFFF;

    auto native = bmApi->NativeFunctions;

    int pIndex = player.playerIndex();
    int x = pIndex ? baseX : screenWidth - baseX - width;
    // int z = SettingsManager::GetInstance().zPos;
    int z = 280;

    // Draw bg
    native.DrawQuad(x, y, x + width, y + height, z, color & 0x33FFFFFF);
    
    // Draw fill
    if (player.untechTimer() >= 0) {
        int fillLength = player.untechTimer() * width / meterLengthFrames;
        if (pIndex) {
            native.DrawQuad(x, y, x + fillLength, y + height, z, color);
        } else {
            native.DrawQuad(x + width - fillLength, y, x + width, y + height, z, color);
        }

        // Gauge Text
        if (SettingsManager::GetInstance().DisplayGaugeNumbers) {
            constexpr int gaugeTextXOffset = 1;
            constexpr int gaugeTextYOffset = 1;
            if (pIndex) {
                DrawRunSumText(bmApi, std::to_string(player.untechTimer()), x + gaugeTextXOffset, y + gaugeTextYOffset, z, 1.0f, 1.0f, false, Font::BLACK_BORDERED);
            } else {
                DrawRunSumText(bmApi, std::to_string(player.untechTimer()), x + width - gaugeTextXOffset, y + gaugeTextYOffset, z, 1.0f, 1.0f, true, Font::BLACK_BORDERED);
            }
        }
    }

    // Draw edges
    native.DrawQuad(x,                      y, x + borderSize, y + height, z, color);
    native.DrawQuad(x + width - borderSize, y, x + width,      y + height, z, color);
    native.DrawQuad(x, y,                       x + width, y + borderSize, z, color);
    native.DrawQuad(x, y + height - borderSize, x + width, y + height,     z, color);

    // Draw label text
    constexpr int textYOffset = -8;
    if (pIndex) {
        DrawRunSumText(bmApi, "UNTECH", x, y + textYOffset, z, 1.0f, 1.0f, false, Font::BLACK_BORDERED);
    } else {
        DrawRunSumText(bmApi, "UNTECH", x + width, y + textYOffset, z, 1.0f, 1.0f, true, Font::BLACK_BORDERED);
    }
}
void DrawUntechGauge(BaseMod::Api* bmApi) {
    if (!SettingsManager::GetInstance().DisplayUntechGauges) return;

    auto p1 = bmApi->GameData.GetPlayer(0);
    auto p2 = bmApi->GameData.GetPlayer(1);

    if (p1.isValid()) DrawUntechGaugePerPlayer(p1, bmApi);
    if (p2.isValid()) DrawUntechGaugePerPlayer(p2, bmApi);
}
void DrawStunGaugePerPlayer(ggxxacpr::Player player, BaseMod::Api* bmApi) {
    constexpr int screenWidth = 640;
    constexpr int baseX = 215;
    constexpr int y = 84;
    constexpr int height = 6;
    constexpr int width = 38;
    constexpr int borderSize = 1;
    constexpr uint32_t color = 0xFF8D918D;
    constexpr uint32_t borderColor = 0xFF000000;

    auto native = bmApi->NativeFunctions;

    int pIndex = player.playerIndex();
    int x = pIndex ? screenWidth - baseX - width : baseX;
    // int z = SettingsManager::GetInstance().zPos;
    int z = 280;

    // Draw bg
    native.DrawQuad(x, y, x + width, y + height, z, borderColor & 0x33FFFFFF);
    // Draw fill
    if (player.dizzyThreshold()) {
        int fillLength = player.dizzyBuildup() / 100 * width / player.dizzyThreshold();
        if (pIndex) {
            native.DrawQuad(x, y, x + fillLength, y + height, z, color);
        } else {
            native.DrawQuad(x + width - fillLength, y, x + width, y + height, z, color);
        }
    }
    // Draw edges
    native.DrawQuad(x,                      y, x + borderSize, y + height, z, borderColor);
    native.DrawQuad(x + width - borderSize, y, x + width,      y + height, z, borderColor);
    native.DrawQuad(x, y,                       x + width, y + borderSize, z, borderColor);
    native.DrawQuad(x, y + height - borderSize, x + width, y + height,     z, borderColor);

    // Gauge Text
    if (SettingsManager::GetInstance().DisplayGaugeNumbers) {
        constexpr int gaugeTextXOffset = 1;
        constexpr int gaugeTextYOffset = height + 1;
        std::string gaugeString = std::to_string(player.dizzyBuildup() / 100) + "/" + std::to_string(player.dizzyThreshold());
        if (pIndex) {
            DrawRunSumText(bmApi, gaugeString, x + gaugeTextXOffset, y + gaugeTextYOffset, z, 1.0f, 1.0f, false, Font::BLACK_BORDERED);
        } else {
            DrawRunSumText(bmApi, gaugeString, x + width - gaugeTextXOffset, y + gaugeTextYOffset, z, 1.0f, 1.0f, true, Font::BLACK_BORDERED);
        }
    }

    // Draw label text
    // constexpr int textYOffset = -8;
    // DrawRunSumText(bmApi, "STUN", x, y + textYOffset, z, 1.0f, 1.0f, false, Font::BLACK_BORDERED);
}
void DrawStunGauge(BaseMod::Api* bmApi) {
    if (!SettingsManager::GetInstance().DisplayDizzyGauge) return;

    auto p1 = bmApi->GameData.GetPlayer(0);
    auto p2 = bmApi->GameData.GetPlayer(1);

    if (p1.isValid()) DrawStunGaugePerPlayer(p1, bmApi);
    if (p2.isValid()) DrawStunGaugePerPlayer(p2, bmApi);
}
