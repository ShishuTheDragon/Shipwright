#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/ShipInit.hpp"
#include "soh/ResourceManagerHelpers.h"
#include "soh/resource/type/Scene.h"
#include <libultraship/libultra.h>
#include "global.h"
#include "z64scene.h"
#include <soh/resource/type/scenecommand/SetMesh.h>
#include <soh/resource/type/scenecommand/SetActorList.h>

#define CVAR_NAME CVAR_DEVELOPER_TOOLS("RenderAllRooms")
#define CVAR_VALUE CVarGetInteger(CVAR_NAME, 0)

extern "C" PlayState* gPlayState;
extern "C" uintptr_t gSegments[NUM_SEGMENTS];

extern s32 OTRScene_ExecuteCommands(PlayState* play, SOH::Scene* scene);

extern "C" Actor* Actor_Spawn(ActorContext* actorCtx, PlayState* play, s16 actorId, f32 posX, f32 posY, f32 posZ, s16 rotX, s16 rotY, s16 rotZ, s16 params);

#define MAX_ROOMS 40
static Room allRooms[MAX_ROOMS];

static Actor* customDrawer = nullptr;

extern "C" s32 OTRfunc_8009728C(PlayState* play, RoomContext* roomCtx, s32 roomNum);

static bool SceneSupported(s16 sceneNum) {
    return true;
    switch (sceneNum) {
        case SCENE_KOKIRI_FOREST:
            return true;
    }
    return false;
}

static void Nothing(Actor* thisx, PlayState* play) {
}

static void DrawStuff(Actor* thisx, PlayState* play) {
    for (s8 roomNum = 0; roomNum < play->numRooms; roomNum++) {
        if (roomNum == play->roomCtx.curRoom.num && play->roomCtx.status == 0)
            continue;
        if (roomNum == play->roomCtx.prevRoom.num)
            continue;
        // gSPSegment(POLY_OPA_DISP++, 0x03, allRooms[roomNum].segment);
        Room_Draw(play, &allRooms[roomNum], 3);
    }
}

static bool needsLoading = false;

static void ResetThings() {
    memset(allRooms, 0, sizeof(allRooms));
    for (s8 i = 0; i < MAX_ROOMS; i++)
        allRooms[i].num = -1;

    needsLoading = true;
    customDrawer = nullptr;
}

static void DeleteThings() {
    if (customDrawer) {
        if (gPlayState)
            Actor_Kill(customDrawer);
        customDrawer = nullptr;
    }
}

static void SpawnThingsCore() {
    PlayState* play = gPlayState;
    RoomContext* roomCtx = &play->roomCtx;

    s8 alreadyLoaded = roomCtx->curRoom.num;
    allRooms[alreadyLoaded] = roomCtx->curRoom;

    for (s8 roomNum = 0; roomNum < play->numRooms; roomNum++) {
        if (roomNum == alreadyLoaded) {
            allRooms[roomNum].num = roomNum;
            allRooms[roomNum].segment = roomCtx->curRoom.segment;
            allRooms[roomNum].meshHeader = roomCtx->curRoom.meshHeader;
        }

        auto roomData = std::static_pointer_cast<SOH::Scene>(ResourceMgr_GetResourceByNameHandlingMQ(play->roomList[roomNum].fileName));
        for (auto cmd : roomData->commands) {
            if (cmd->cmdId == SOH::SceneCommandID::EndMarker)
                break;
            if (cmd->cmdId == SOH::SceneCommandID::SetMesh) {
                auto otrMesh = (SOH::SetMesh*)cmd.get();

                allRooms[roomNum].num = roomNum;
                allRooms[roomNum].segment = roomCtx->curRoom.segment;
                allRooms[roomNum].meshHeader = (MeshHeader*)otrMesh->GetRawPointer();
            }
        }
    }

    Actor* actor =
        Actor_Spawn(&gPlayState->actorCtx, gPlayState, ACTOR_BG_SPOT17_FUNEN, 0, 0, 0, 0, 0, 0, 0);
    actor->update = Nothing;
    actor->draw = Nothing;
    actor->room = -1;
    customDrawer = actor;
}

static void SpawnThingsMidScene() {
    if (!gPlayState)
        return;

    PlayState* play = gPlayState;
    RoomContext* roomCtx = &play->roomCtx;

    if (!SceneSupported(play->sceneNum))
        return;

    if (roomCtx->status != 0)
        return;

    if (needsLoading) {
        SpawnThingsCore();
        needsLoading = false;
    }
}

static void AfterSceneCommands(int) {
    PlayState* play = gPlayState;
    RoomContext* roomCtx = &play->roomCtx;

    if (!SceneSupported(play->sceneNum))
        return;

    if (roomCtx->status != 0)
        return;

    if (needsLoading) {
        SpawnThingsCore();
        needsLoading = false;
    }
}

static void OnPlayDrawEnd() {
    if (!gPlayState)
        return;
    if (!SceneSupported(gPlayState->sceneNum))
        return;
    if (needsLoading)
        return;
    DrawStuff(nullptr, gPlayState);
}

static void RegisterRenderAllRooms() {
    if (CVAR_VALUE) {
        ResetThings();
        SpawnThingsMidScene();
    } else {
        DeleteThings();
    }

    COND_HOOK(OnSceneInit, CVAR_VALUE, [](int) {
        ResetThings();
    });

    COND_HOOK(AfterSceneCommands, CVAR_VALUE, AfterSceneCommands);

    COND_HOOK(OnPlayDrawEnd, CVAR_VALUE, OnPlayDrawEnd);
}

static RegisterShipInitFunc initFunc(RegisterRenderAllRooms, { CVAR_NAME });
