#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/ShipInit.hpp"
#include "soh/ResourceManagerHelpers.h"
#include "soh/resource/type/Scene.h"
#include <libultraship/libultra.h>
#include "global.h"
#include "z64scene.h"
#include <soh/resource/type/scenecommand/SetMesh.h>
#include <soh/resource/type/scenecommand/SetActorList.h>

#define CVAR_NAME CVAR_ENHANCEMENT("IvanCoop.RenderEveryRoom")
#define CVAR_VALUE CVarGetInteger(CVAR_NAME, 0)

extern "C" PlayState* gPlayState;
extern "C" uintptr_t gSegments[NUM_SEGMENTS];

extern s32 OTRScene_ExecuteCommands(PlayState* play, SOH::Scene* scene);

extern "C" Actor* Actor_Spawn(ActorContext* actorCtx, PlayState* play, s16 actorId, f32 posX, f32 posY, f32 posZ, s16 rotX, s16 rotY, s16 rotZ, s16 params);

#define MAX_ROOMS 40
Room allRooms[MAX_ROOMS];
bool loadedActors[MAX_ROOMS];

extern "C" s32 OTRfunc_8009728C(PlayState* play, RoomContext* roomCtx, s32 roomNum);

Actor* gRenderHelper = nullptr;

static bool ReadyAndSupported() {
    if (!gPlayState || gPlayState->roomCtx.status != 0)
        return false;

    return true;
    switch (gPlayState->sceneNum) {
        case SCENE_KOKIRI_FOREST:
            return true;
    }
    return false;
}

static void LoadOtherRooms() {
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
}

static void Nothing(Actor* thisx, PlayState* play) {
}

static void DrawOtherRooms(Actor*, PlayState* play) {
    for (s8 roomNum = 0; roomNum < play->numRooms; roomNum++) {
        if (roomNum == play->roomCtx.curRoom.num && play->roomCtx.status == 0)
            continue;
        if (roomNum == play->roomCtx.prevRoom.num)
            continue;
        Room_Draw(play, &allRooms[roomNum], 3);
    }
}

static void SetupRenderHelper() {
    assert(ReadyAndSupported());
    assert(!gRenderHelper);

    LoadOtherRooms();

    gRenderHelper = Actor_Spawn(&gPlayState->actorCtx, gPlayState, ACTOR_BG_SPOT17_FUNEN, 0, 0, 0, 0, 0, 0, 0);
    gRenderHelper->update = Nothing;
    gRenderHelper->draw = DrawOtherRooms;
    gRenderHelper->room = -1;
}

static void ClearData() {
    gRenderHelper = nullptr;

    memset(allRooms, 0, sizeof(allRooms));
    for (s8 i = 0; i < MAX_ROOMS; i++)
        allRooms[i].num = -1;
}

static void KillRenderHelper() {
    assert(ReadyAndSupported());

    if (gRenderHelper) {
        Actor_Kill(gRenderHelper);
        gRenderHelper = nullptr;
    }

    ClearData();
}

static void OnSceneInit(int) {
    ClearData();
}

static void AfterSceneCommands(int) {
    if (ReadyAndSupported() && !gRenderHelper)
        SetupRenderHelper();
}

static void RegisterLostWoodsMultiViewSupport() {
    if (ReadyAndSupported()) {
        if (CVAR_VALUE)
            SetupRenderHelper();
        else
            KillRenderHelper();
    }

    if (!CVAR_VALUE)
        ClearData();

    COND_HOOK(OnSceneInit, CVAR_VALUE, [](int){ ClearData(); });
    COND_HOOK(AfterSceneCommands, CVAR_VALUE, AfterSceneCommands);
}

static RegisterShipInitFunc initFunc(RegisterLostWoodsMultiViewSupport, { CVAR_NAME });
