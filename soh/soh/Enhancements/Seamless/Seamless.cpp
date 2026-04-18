#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/ShipInit.hpp"
#include "macros.h"
#include "soh/resource/type/Scene.h"
#include <soh/ResourceManagerHelpers.h>
#include "align_asset_macro.h"
#include <functions.h>
#include <scenes/overworld/spot05/spot05_room_0.h>
#include <scenes/overworld/spot10/spot10_room_8.h>
#include "global.h"
#include <soh/resource/type/Array.h>
#include <fast/lus_gbi.h>

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

#define dspot10_room_8DL_0017C8 "__OTR__scenes/shared/spot10_scene/spot10_room_8DL_0017C8"
static const ALIGN_ASSET(2) char spot10_room_8DL_0017C8[] = dspot10_room_8DL_0017C8;

#define dspot05_room_0Vtx_008118 "__OTR__scenes/shared/spot05_scene/spot05_room_0Vtx_008118"
static const ALIGN_ASSET(2) char spot05_room_0Vtx_008118[] = dspot05_room_0Vtx_008118;

#define dspot10_room_8Vtx_0012F8 "__OTR__scenes/shared/spot10_scene/spot10_room_8Vtx_0012F8"
static const ALIGN_ASSET(2) char spot10_room_8Vtx_0012F8[] = dspot10_room_8Vtx_0012F8;

void AfterSceneCommands(int sceneNum) {
    // if (sceneNum == SCENE_LOST_WOODS)
    {
        //G_VTX_OTR_HASH: scenes/shared/spot05_scene/spot05_room_0Vtx_008118

        Gfx* gfx = ResourceMgr_LoadGfxByName(spot10_room_8DL_0017C8);
        if (gfx[0].words.w1 == 0xbeefbeef) {
                                             // G_VTX_OTR_HASH: scenes/shared/spot10_scene/spot10_room_8Vtx_0012F8
                                             //
            SkipCommand(gfx[81]);         // gsSP2Triangles(0, 1, 2, , 0, 3, 4, )
            SkipCommand(gfx[82]);         // gsSP2Triangles(3, 5, 6, , 7, 8, 9, )
            KeepSecondTriangle(gfx[83]);  // gsSP2Triangles(10, 8, 2, , 2, 11, 12, )
            gfx[0].words.w1++;
        }
    }

    // if (sceneNum == SCENE_SACRED_FOREST_MEADOW)
    {
        Gfx* gfx = ResourceMgr_LoadGfxByName(spot05_room_0DL_0084C8);
        if (gfx[0].words.w1 == 0xbeefbeef) { // 30 is the center // 16, 17, 19, 20, 23 are the borders
            SkipCommand(gfx[32]);
            SkipCommand(gfx[33]);
            SkipCommand(gfx[34]);
            SkipCommand(gfx[35]);
            SkipCommand(gfx[36]);
            SkipCommand(gfx[37]);
            SkipCommand(gfx[38]);
            SkipCommand(gfx[39]);
            KeepSecondTriangle(gfx[40]);
            KeepFirstTriangle(gfx[44]);
            SkipCommand(gfx[45]);
            gfx[0].words.w1++;
        }
    }
}

namespace {
    f32 x = 1003;
    f32 y = 0;
    f32 z = -5177;
}

bool onceOnly = false;

/*
860 0 -2800
740 0 -2800     leftbase
700 120 -2800   leftupper
800 180 -2800   <- peak         +180y
740 0 -2800
2
860 0 -2800     rightbase  +60x +0y
900 120 -2800   rightupper +100x +120y

16: -194 184 2322
17: -295 108 2322
19: -127 -4 2322
20: -110 115 2322
23: -250 -10 2322
40: -250 -10 2322
41: -127 -4 2322

 16: -200 180
 17: -300 120
 19: -140 0    and also 41
 20: -100 120
 23: -260 0
*/

Fast::F3DVtx_t simple(Vec3s ob, Vec3s tc, Vec3s n) {
    Fast::F3DVtx_t out;
    out.ob[0] = ob.x;
    out.ob[1] = ob.y;
    out.ob[2] = ob.z;
    out.tc[0] = tc.x;
    out.tc[1] = tc.y;
    out.cn[0] = n.x;
    out.cn[1] = n.y;
    out.cn[2] = n.z;
    out.cn[3] = 255;
    out.flag = 0;
    return out;
}

#define fixywixy_len 16
static Fast::F3DVtx_t fixywixy[fixywixy_len] = {
    simple({700, 120, -2800}, {2171,512}, {113,218,12}),
    simple({855,   0, -3000}, {3355,0}, {200,86,61}),
    simple({800, 180, -2800}, {1536,512}, {255,138,21}),
    simple({740,   0, -2800}, {2789,512}, {82,87,5}),
    simple({860,   0, -2800}, {3355,512}, {175,88,5}),
    simple({710, 120, -3000}, {2141,0}, {74,205,79}),
    simple({715,  58, -3000}, {2457,0}, {96,25,66}),
    simple({860,   0, -2800}, {283,512}, {175,88,5}),
    simple({900, 120, -2800}, {901,512}, {143,220,12}),
    simple({752,   0, -3000}, {2789,0}, {57,85,62}),
    simple({855,   0, -3000}, {283,0}, {200,86,61}),
    simple({800, 187, -3068}, {1529,-174}, {252,166,78}),
    simple({742, 164, -3055}, {1825,-124}, {38,157,55}),
    simple({853, 169, -3048}, {1197,-142}, {221,157,57}),
    simple({891, 123, -3000}, {915,0}, {187,204,83}),
    simple({887,  64, -3000}, {586,0}, {160,27,67}),
};

static Gfx foxywoxy[] = {
    {0x20100000,0x00000000}, // G_SETTIMG_OTR_HASH
    {0xd7784161,0x4e740dec}, // scenes/shared/spot10_scene/spot10_sceneTex_00F230
    gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0x0000, G_TX_LOADTILE, 0, G_TX_NOMIRROR | G_TX_CLAMP, 4, G_TX_NOLOD, G_TX_NOMIRROR | G_TX_WRAP, 4, G_TX_NOLOD),
    gsDPLoadSync(),
    gsDPLoadBlock(G_TX_LOADTILE, 0, 0, 255, 512),
    gsDPPipeSync(),
    gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 4, 0x0000, G_TX_RENDERTILE, 0, G_TX_NOMIRROR | G_TX_CLAMP, 4, G_TX_NOLOD, G_TX_NOMIRROR | G_TX_WRAP, 4, G_TX_NOLOD),
    gsDPSetTileSize(G_TX_RENDERTILE, 0, 0, 0x003C, 0x003C),
    { _SHIFTL(G_VTX_OTR_HASH, 24, 8) | _SHIFTL(fixywixy_len, 12, 8) | _SHIFTL(0 + fixywixy_len, 1, 7), (uintptr_t)(void*)fixywixy },
    {0,0}, // hash doesn't matter as it has a ptr>0xFFFFF
    gsSP1Triangle(2, 12, 11, 0),
    gsSP2Triangles(13, 2, 11, 0, 14, 8, 13, 0),
    gsSP2Triangles(15, 8, 14, 0, 10, 7, 15, 0),
    gsSP2Triangles(3, 9, 6, 0, 12, 0, 5, 0),
    gsSP2Triangles(5, 0, 6, 0, 0, 12, 2, 0),
    gsSP2Triangles(15, 7, 8, 0, 3, 6, 0, 0),
    gsSP1Triangle(2, 13, 8, 0),
    gsSP2Triangles(1, 3, 4, 0, 1, 9, 3, 0),
    gsDPPipeSync(),
    gsSPEndDisplayList(),
};

void set(std::shared_ptr<SOH::Array>& res, s32 vertIndex, Vec3s values) {
    res->Vertices[vertIndex].v.ob[0] = values.x;
    res->Vertices[vertIndex].v.ob[1] = values.y;
    res->Vertices[vertIndex].v.ob[2] = values.z;
}

extern "C" void SeamlessHook_DrawNextScene() {
    auto play = gPlayState;
    auto gfxCtx = play->state.gfxCtx;

    // auto BLAH = ResourceMgr_LoadGfxByName(spot10_room_8DL_0017C8);
    // auto toMatch = std::static_pointer_cast<SOH::Array>(ResourceMgr_GetResourceByNameHandlingMQ(spot10_room_8Vtx_0012F8));

    // int XY = 1; // 54

    // if (!onceOnly) {
    //     auto verts = (Fast::F3DVtx*)(((char*)toMatch->Vertices.data()) + 0x150);
    //     for (int i = 0; i < 4; i++) {
    //         auto& AX = verts[i].v;
    //         printf("%d: simple({%d,%d,%d}, {%d,%d}, {%d,%d,%d}), // a=%d, f=%d\n",
    //             (int)i,
    //             (int)AX.ob[0], (int)AX.ob[1], (int)AX.ob[2],
    //             (int)AX.tc[0], (int)AX.tc[1],
    //             (int)AX.cn[0], (int)AX.cn[1], (int)AX.cn[2],
    //             (int)AX.cn[3], (int)AX.flag);
    //     }
    //     onceOnly = true;
    // }

    //     for (int i = 0; i < toMatch->Vertices.size(); i++) {
    //         auto& AX = toMatch->Vertices[i].v.ob;
    //         if (AX[2] == -2800) {
    //             printf("%d %d %d\n", (int)AX[0], (int)AX[1], (int)AX[2]);
    //         }
    //     }

    // int y = 1;

    auto gfx = std::static_pointer_cast<SOH::Array>(ResourceMgr_GetResourceByNameHandlingMQ(spot05_room_0Vtx_008118));
    // set(gfx, 16, {-200, 180, 2300 });
    // set(gfx, 17, {-300, 120, 2300 });
    // set(gfx, 19, {-140,   0, 2300 });
    // set(gfx, 20, {-100, 120, 2300 });
    // set(gfx, 23, {-260,   0, 2300 });
    // set(gfx, 40, {-260,   0, 2300 });
    // set(gfx, 41, {-140,   0, 2300 });


    if (!onceOnly) {
        for (int i = 0; i < gfx->Vertices.size(); i++) {
            auto& AX = gfx->Vertices[i].v.ob;
            // if (AX[2] == 2322) // i == 16 || i == 17 || i == 19 || i == 20 || i == 23)
                printf("%d: %d %d %d\n", (int)i, (int)AX[0] + (int)x, (int)AX[1] + (int)y, (int)AX[2] + (int)z);
        }
        onceOnly = true;
    }

    OPEN_DISPS(gfxCtx);

    MtxF mfTrans;
    if (play->sceneNum == SCENE_LOST_WOODS) {
        SkinMatrix_SetTranslate(&mfTrans, x, y, z);
    } else if (play->sceneNum == SCENE_SACRED_FOREST_MEADOW) {
        SkinMatrix_SetTranslate(&mfTrans, -x, -y, -z);
    } else {
        return;
    }

    if (play->sceneNum == SCENE_LOST_WOODS) {
        assert((uintptr_t)(void*)fixywixy > 0xFFFFF);
        gSPMatrix(POLY_OPA_DISP++, &gMtxClear, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        Gfx_SetupDL_25Opa(gfxCtx);
        gSPDisplayList(POLY_OPA_DISP++, foxywoxy);
    }

    gSPMatrix(POLY_OPA_DISP++, &gMtxClear, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    Mtx* mtx = SkinMatrix_MtxFToNewMtx(gfxCtx, &mfTrans);
    assert(mtx != nullptr);

    if (mtx != nullptr) {
        gSPMatrix(POLY_OPA_DISP++, mtx, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        Gfx_SetupDL_25Opa(gfxCtx);

        if (play->sceneNum == SCENE_LOST_WOODS) {
            gSPDisplayList(POLY_OPA_DISP++, (Gfx*)spot05_room_0DL_0084C8);
            gSPDisplayList(POLY_OPA_DISP++, (Gfx*)spot05_room_0DL_007620);
            gSPDisplayList(POLY_OPA_DISP++, (Gfx*)spot05_room_0DL_006EC8);
            gSPDisplayList(POLY_OPA_DISP++, (Gfx*)spot05_room_0DL_001CD8);
            gSPDisplayList(POLY_OPA_DISP++, (Gfx*)spot05_room_0DL_0015B0);
            gSPDisplayList(POLY_OPA_DISP++, (Gfx*)spot05_room_0DL_007F00);
            gSPDisplayList(POLY_OPA_DISP++, (Gfx*)spot05_room_0DL_002200);
            gSPDisplayList(POLY_OPA_DISP++, (Gfx*)spot05_room_0DL_003D88);
        } else if (play->sceneNum == SCENE_SACRED_FOREST_MEADOW) {
            gSPDisplayList(POLY_OPA_DISP++, (Gfx*)(spot10_room_8DL_002630));
            gSPDisplayList(POLY_OPA_DISP++, foxywoxy);
        }
    }

    CLOSE_DISPS(gfxCtx);
}

int restoreHUD = 0;

void OnPlayDrawEnd() {
    auto interfaceCtx = &gPlayState->interfaceCtx;

    if (restoreHUD > 0) {
        restoreHUD--;

        interfaceCtx->aAlpha = interfaceCtx->bAlpha = interfaceCtx->cLeftAlpha =
            interfaceCtx->cDownAlpha = interfaceCtx->cRightAlpha = interfaceCtx->dpadUpAlpha = interfaceCtx->dpadDownAlpha =
                interfaceCtx->dpadLeftAlpha = interfaceCtx->dpadRightAlpha = interfaceCtx->healthAlpha =
                    interfaceCtx->startAlpha = interfaceCtx->magicAlpha = 255;
    }
}

void OnPlayDestroy() {
    int XXX = 1;
    // restoreHUD = 1;
}

void OnExitGame(int) {
}

void OnLoadGame(int) {
}

void OnPlayerUpdate() {
    auto play = gPlayState;

    if (play->transitionTrigger == TRANS_TRIGGER_START) {
        restoreHUD = 20;
    }
}

void RegisterSeamless() {
    COND_HOOK(OnPlayerUpdate, true, OnPlayerUpdate);
    COND_HOOK(AfterSceneCommands, true, AfterSceneCommands);
    COND_HOOK(OnLoadGame, true, OnLoadGame);
    COND_HOOK(OnPlayDrawEnd, true, OnPlayDrawEnd);
    COND_HOOK(OnPlayDestroy, true, OnPlayDestroy);
    COND_HOOK(OnExitGame, true, OnExitGame);
}

static RegisterShipInitFunc initFunc(RegisterSeamless);
