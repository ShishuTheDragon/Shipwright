#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/ShipInit.hpp"
#include "soh/ResourceManagerHelpers.h"
#include "soh/resource/type/Scene.h"
#include <libultraship/libultra.h>
#include "global.h"
#include <soh/resource/type/scenecommand/SetMesh.h>
#include <soh/resource/type/scenecommand/SetActorList.h>

extern "C" PlayState* gPlayState;
extern "C" uintptr_t gSegments[NUM_SEGMENTS];

extern s32 OTRScene_ExecuteCommands(PlayState* play, SOH::Scene* scene);

extern "C" Actor* Actor_Spawn(ActorContext* actorCtx, PlayState* play, s16 actorId, f32 posX, f32 posY, f32 posZ, s16 rotX, s16 rotY, s16 rotZ, s16 params);

#define MAX_ROOMS 40
Room allRooms[MAX_ROOMS];
bool loadedActors[MAX_ROOMS];

extern "C" s32 OTRfunc_8009728C(PlayState* play, RoomContext* roomCtx, s32 roomNum);

static void Nothing(Actor* thisx, PlayState* play) {
}

static void DrawStuff(Actor* thisx, PlayState* play) {
    if (play->sceneNum == SCENE_LOST_WOODS) {
        for (s8 roomNum = 0; roomNum < play->numRooms; roomNum++) {
            if (roomNum == play->roomCtx.curRoom.num && play->roomCtx.status == 0)
                continue;
            if (roomNum == play->roomCtx.prevRoom.num)
                continue;
            Room_Draw(play, &allRooms[roomNum], 3);
        }
    }
}

static bool needsLoading = false;

static void OnSceneInit(int) {
    memset(allRooms, 0, sizeof(allRooms));
    for (s8 i = 0; i < MAX_ROOMS; i++)
        allRooms[i].num = -1;

    memset(loadedActors, 0, sizeof(loadedActors));

    needsLoading = true;
}

static void AfterSceneCommands(int) {
    PlayState* play = gPlayState;
    RoomContext* roomCtx = &play->roomCtx;

    if (roomCtx->status != 0)
        return;

    if (!needsLoading) {
        // The curRoom was changed.
        play->numSetupActors = 0;
        return;
    }

    needsLoading = false;

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
    actor->draw = DrawStuff;
    actor->room = -1;
}

static void OnSceneSpawnActors() {
    auto play = gPlayState;
    if (play->sceneNum != SCENE_LOST_WOODS)
        return;


    RoomContext* roomCtx = &play->roomCtx;
    s8 alreadyLoaded = roomCtx->curRoom.num;
    loadedActors[alreadyLoaded] = true;

    for (s8 roomNum = 0; roomNum < play->numRooms; roomNum++) {
        if (loadedActors[roomNum])
            continue;

        auto roomData = std::static_pointer_cast<SOH::Scene>(ResourceMgr_GetResourceByNameHandlingMQ(play->roomList[roomNum].fileName));
        for (auto cmd : roomData->commands) {
            if (cmd->cmdId == SOH::SceneCommandID::EndMarker)
                break;
            if (cmd->cmdId == SOH::SceneCommandID::SetActorList) {
                auto cmdActor = (SOH::SetActorList*)cmd.get();
                play->numSetupActors = cmdActor->numActors;
                play->setupActorList = (ActorEntry*)cmdActor->GetRawPointer();
                loadedActors[roomNum] = true;
            }
        }
        break;
    }
}

static void OnActorSpawn(void* actorUnk) {
    auto play = gPlayState;
    if (play->sceneNum != SCENE_LOST_WOODS)
        return;

    Actor* actor = (Actor*)actorUnk;

    if (actor->id == ACTOR_EN_HOLL) {
        actor->room = 2;
    }

    actor->room = -1;
}

static void RegisterLostWoodsMultiViewSupport() {
    COND_ID_HOOK(OnSceneInit, SCENE_LOST_WOODS, true, OnSceneInit);
    COND_ID_HOOK(AfterSceneCommands, SCENE_LOST_WOODS, true, AfterSceneCommands);
    COND_HOOK(OnSceneSpawnActors, true, OnSceneSpawnActors);
    COND_HOOK(OnActorSpawn, true, OnActorSpawn);
}

static RegisterShipInitFunc initFunc(RegisterLostWoodsMultiViewSupport);
