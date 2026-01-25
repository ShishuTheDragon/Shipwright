#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/ShipInit.hpp"
#include "macros.h"
#include "soh/resource/type/Scene.h"
#include <soh/ResourceManagerHelpers.h>
#include "align_asset_macro.h"
#include <functions.h>
#include <scenes/overworld/spot05/spot05_room_0.h>
#include "global.h"

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
    // if (sceneNum == SCENE_LOST_WOODS)
    {
        Gfx* gfx = ResourceMgr_LoadGfxByName("scenes/shared/spot10_scene/spot10_room_8DL_0017C8");
        if (gfx[0].words.w1 == 0xbeefbeef) {
            SkipCommand(gfx[81]);
            SkipCommand(gfx[82]);
            KeepSecondTriangle(gfx[83]);
            gfx[0].words.w1++;
        }
    }

    // if (sceneNum == SCENE_SACRED_FOREST_MEADOW)
    {
        Gfx* gfx = ResourceMgr_LoadGfxByName("scenes/shared/spot05_scene/spot05_room_0DL_0084C8");
        if (gfx[0].words.w1 == 0xbeefbeef) {
            SkipCommand(gfx[38]);
            SkipCommand(gfx[39]);
            KeepSecondTriangle(gfx[40]);
            gfx[0].words.w1++;
        }
    }
}

namespace {
    f32 x = 1000;
    f32 y = 0;
    f32 z = -5100;
}

extern "C" void SeamlessHook_DrawNextScene() {
    auto play = gPlayState;
    auto gfxCtx = play->state.gfxCtx;

    MtxF mfTrans;
    SkinMatrix_SetTranslate(&mfTrans, x, y, z);

    OPEN_DISPS(gfxCtx);

    gSPMatrix(POLY_OPA_DISP++, &gMtxClear, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    Mtx* mtx = SkinMatrix_MtxFToNewMtx(gfxCtx, &mfTrans);
    assert(mtx != nullptr);

    if (mtx != nullptr) {
        gSPMatrix(POLY_OPA_DISP++, mtx, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        Gfx_SetupDL_25Opa(gfxCtx);
        gSPDisplayList(POLY_OPA_DISP++, (Gfx*)(spot05_room_0DL_0084C8));
        gSPDisplayList(POLY_OPA_DISP++, (Gfx*)(spot05_room_0DL_001CD8));
        gSPDisplayList(POLY_OPA_DISP++, (Gfx*)(spot05_room_0DL_0015B0));
        gSPDisplayList(POLY_OPA_DISP++, (Gfx*)(spot05_room_0DL_007F00));
        gSPDisplayList(POLY_OPA_DISP++, (Gfx*)(spot05_room_0DL_002200));
    }

    CLOSE_DISPS(gfxCtx);
}

void RegisterSeamless() {
    COND_HOOK(AfterSceneCommands, true, AfterSceneCommands);
}

static RegisterShipInitFunc initFunc(RegisterSeamless);
