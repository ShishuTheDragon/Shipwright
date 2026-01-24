#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/ShipInit.hpp"
#include "macros.h"
#include "soh/resource/type/Scene.h"
#include <soh/ResourceManagerHelpers.h>

extern "C" PlayState* gPlayState;

void SkipCommand(Gfx& gfx) {
    gfx.words.w0 = G_NOOP << 24;
    gfx.words.w1 = 0;
}

void KeepFirstTriangle(Gfx& gfx) {
    u32 triangleData = gfx.words.w0 & 0x00FFFFFF;
    gfx.words.w0 = (G_TRI1 << 24) | triangleData;
    gfx.words.w1 = 0;
}

void KeepSecondTriangle(Gfx& gfx) {
    u32 triangleData = gfx.words.w1 & 0x00FFFFFF;
    gfx.words.w0 = (G_TRI1 << 24) | triangleData;
    gfx.words.w1 = 0;
}

void AfterSceneCommands(int sceneNum) {
    if (sceneNum == SCENE_LOST_WOODS) {

        Gfx* gfx = ResourceMgr_LoadGfxByName("scenes/shared/spot10_scene/spot10_room_8DL_0017C8");
        if (gfx[0].words.w1 == 0xbeefbeef) {
            SkipCommand(gfx[81]);
            SkipCommand(gfx[82]);
            KeepSecondTriangle(gfx[83]);
            gfx[0].words.w1++;
        }

        auto XXX = 1;

    }
}


void RegisterSeamless() {
    COND_HOOK(AfterSceneCommands, true, AfterSceneCommands);
}

static RegisterShipInitFunc initFunc(RegisterSeamless);
