#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/ShipInit.hpp"
#include "soh/ResourceManagerHelpers.h"
#include "overlays/actors/ovl_En_Holl/z_en_holl.h"
#include "scenes/overworld/spot05/spot05_room_0.h"
#include "soh_assets.h"

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

void AfterSceneCommands(int sceneNum) {
    if (sceneNum == SCENE_LOST_WOODS) {
        Gfx* gfx = ResourceMgr_LoadGfxByName(spot10_room_8DL_0017C8);
        if (gfx[0].words.w1 == 0xbeefbeef) {
            // G_VTX_OTR_HASH: scenes/shared/spot10_scene/spot10_room_8Vtx_0012F8
            //
            SkipCommand(gfx[81]);        // gsSP2Triangles(0, 1, 2, , 0, 3, 4, )
            SkipCommand(gfx[82]);        // gsSP2Triangles(3, 5, 6, , 7, 8, 9, )
            KeepSecondTriangle(gfx[83]); // gsSP2Triangles(10, 8, 2, , 2, 11, 12, )
            gfx[0].words.w1++;
        }
    }
}

namespace {
void NoOp(Actor* thisx, PlayState* play) {
}
} // namespace

// PatchedHoll, a variant of EnHoll (the black planes in Lost Woods that fade as the player draws near) but without any
// room-swapping logic.
namespace {
// Constants from z_en_holl.c:
const f32 triggerDists[4] = { 200.0f, 150.0f, 100.0f, 50.0f };
const f32 planeYMin = -50.0f;
const f32 planeYMax = 200.0f;
const f32 planeHalfWidth = 100.0f;

// func_80A58DD4 but without the room-swapping logic:
void PatchedHoll_Update(EnHoll* thisx, PlayState* play) {
    Player* player = GET_PLAYER(play);

    Vec3f vec;
    Actor_WorldToActorCoords(&thisx->actor, &vec, &player->actor.world.pos);
    thisx->side = (vec.z < 0.0f) ? 0 : 1;

    f32 absZ = fabsf(vec.z);
    if (vec.y > planeYMin && vec.y < planeYMax && fabsf(vec.x) < planeHalfWidth && absZ <= triggerDists[1]) {
        thisx->planeAlpha = (255.0f / (triggerDists[2] - triggerDists[3])) * (absZ - triggerDists[3]);
        thisx->planeAlpha = CLAMP(thisx->planeAlpha, 0, 255);
    }
}

void SpawnPatchedHoll(Vec3f pos, Vec3s rot) {
    EnHoll* holl = (EnHoll*)Actor_Spawn(&gPlayState->actorCtx, gPlayState, ACTOR_EN_HOLL, pos.x, pos.y, pos.z, rot.x,
                                        rot.y, rot.z, 0, false);
    holl->actionFunc = PatchedHoll_Update;
    holl->actor.destroy = NoOp;
}
} // namespace

// MeadowPreview, a custom prop that renders the Sacred Forest Meadow scene DLs.
namespace {
// The point within the meadow to use as the pivot for rendering the preview:
Vec3f meadowEntrancePivot = { 203, 0, -2177 };

extern "C" void MeadowPreview_Draw(Actor* thisx, PlayState* play) {
    GraphicsContext* gfxCtx = play->state.gfxCtx;

    Vec3f trans;
    Math_Vec3f_Sum(&thisx->world.pos, &meadowEntrancePivot, &trans);

    MtxF mfTrans;
    SkinMatrix_SetTranslate(&mfTrans, trans.x, trans.y, trans.z);

    OPEN_DISPS(gfxCtx);

    gSPMatrix(POLY_OPA_DISP++, &gMtxClear, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    Mtx* mtx = SkinMatrix_MtxFToNewMtx(gfxCtx, &mfTrans);

    if (mtx != nullptr) {
        gSPMatrix(POLY_OPA_DISP++, mtx, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        Gfx_SetupDL_25Opa(gfxCtx);

        gSPDisplayList(POLY_OPA_DISP++, (Gfx*)spot05_room_0DL_007620);
        gSPDisplayList(POLY_OPA_DISP++, (Gfx*)spot05_room_0DL_006EC8);
        gSPDisplayList(POLY_OPA_DISP++, (Gfx*)spot05_room_0DL_001CD8);
        gSPDisplayList(POLY_OPA_DISP++, (Gfx*)spot05_room_0DL_0015B0);
        gSPDisplayList(POLY_OPA_DISP++, (Gfx*)spot05_room_0DL_007F00);
        gSPDisplayList(POLY_OPA_DISP++, (Gfx*)spot05_room_0DL_002200);
        gSPDisplayList(POLY_OPA_DISP++, (Gfx*)spot05_room_0DL_003D88);
    }

    CLOSE_DISPS(gfxCtx);
}

void SpawnMeadowPreview(Vec3f pos) {
    // Use a patched version of Crater Smoke Cone (a prop with no banking and no init logic)
    Actor* prop =
        Actor_Spawn(&gPlayState->actorCtx, gPlayState, ACTOR_BG_SPOT17_FUNEN, pos.x, pos.y, pos.z, 0, 0, 0, 0, false);
    prop->update = NoOp;
    prop->draw = MeadowPreview_Draw;
}
} // namespace

void Nothing(Actor* thisx, PlayState* play) {
}

extern "C" void DrawStuff(Actor* thisx, PlayState* play) {
    auto gfxCtx = play->state.gfxCtx;

    OPEN_DISPS(gfxCtx);

    if (play->sceneNum == SCENE_LOST_WOODS) {
        gSPMatrix(POLY_OPA_DISP++, &gMtxClear, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        Gfx_SetupDL_25Opa(gfxCtx);
        gSPDisplayList(POLY_OPA_DISP++, (Gfx*)gLostWoodsExitDL);
    }

    CLOSE_DISPS(gfxCtx);
}

void OnSceneSpawnActors() {
    if (gPlayState->sceneNum == SCENE_LOST_WOODS && gPlayState->roomCtx.curRoom.num == 8) {
        SpawnPatchedHoll({ 800, 0, -2800 }, { 0, 0, 0 });

        Actor* actor =
            Actor_Spawn(&gPlayState->actorCtx, gPlayState, ACTOR_BG_SPOT17_FUNEN, 0, 0, 0, 0, 0, 0, 0, false);
        actor->update = Nothing;
        actor->draw = DrawStuff;

        SpawnMeadowPreview({ 800, 0, -3000 });
    }
}

void RegisterSeamless() {
    COND_HOOK(AfterSceneCommands, true, AfterSceneCommands);
    COND_HOOK(OnSceneSpawnActors, true, OnSceneSpawnActors);
}

static RegisterShipInitFunc initFunc(RegisterSeamless);
