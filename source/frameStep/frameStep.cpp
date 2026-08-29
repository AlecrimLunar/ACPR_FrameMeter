#include "frameStep.h"
#include "baseMod/baseMod.hpp"

static bool repause = false;

inline bool FrameStepInput(BaseMod::GameDataApi& gData) {
    auto input = gData.GetPlayerInputStructArr();
    
    return (
        (input[0].InputPress2 & INPUT_RIGHT) ||
        (input[1].InputPress2 & INPUT_RIGHT)
    );
}

void HandleFrameStep_PreUpdate(BaseMod::Api* api) {
    auto& data = api->GameData;
    if (data.GetPauseState() == 2 &&
        data.GetPauseDisplayState() == 0 &&
        FrameStepInput(data)
    ) {
        *data.GetCApi()->GetPauseState() = 0;
        repause = true;
    }
}
void HandleFrameStep_PostUpdate(BaseMod::Api* api) {
    if (api->GameData.GetPauseState() == 0 && repause) {
        *api->GameData.GetCApi()->GetPauseState() = 2;
        repause = false;
    }
}
