#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/ObjectExtension/ObjectExtension.h"
#include "soh/ShipInit.hpp"
#include "macros.h"

#define CVAR_NAME CVAR_ENHANCEMENT("IvanCoop.NoOcarinaFreeze")
#define CVAR_VALUE CVarGetInteger(CVAR_NAME, 0)

extern "C" PlayState* gPlayState;

// Tested:
// - breaking bushes as Ivan while Link is on ocarina
// - killing skulltulas
// - collecting pickups (mixed results)
//
// To test:
// - shooting fire arrows in Haunted Wasteland to spawn a chest without the "skip cutscenes" enhancements on. Does it break things?
// - sign restore animation
//
// Edge cases (not handled yet):
// - song-powered chests, like the Redead’s Grave
// - ENKANBAN_PIECE

// Presence of this data on an actor means we added ACTOR_FLAG_UPDATE_DURING_OCARINA to it
struct NoOcarinaFreezeMarker {};
static ObjectExtension::Register<NoOcarinaFreezeMarker> sNoOcarinaFreezeReg;

static void AddFlagIfNeeded(Actor* actor) {
    // Ignore actors that have the flag inherently
    if (actor->flags & ACTOR_FLAG_UPDATE_DURING_OCARINA)
        return;

    // Add the flag and add our marker
    actor->flags |= ACTOR_FLAG_UPDATE_DURING_OCARINA;
    ObjectExtension::GetInstance().Set(actor, NoOcarinaFreezeMarker{});
}

static void ClearFlagIfSet(Actor* actor) {
    // Ignore actors without our marker
    if (!ObjectExtension::GetInstance().Has<NoOcarinaFreezeMarker>(actor))
        return;

    // Remove the flag and remove our marker
    actor->flags &= ~ACTOR_FLAG_UPDATE_DURING_OCARINA;
    ObjectExtension::GetInstance().Remove<NoOcarinaFreezeMarker>(actor);
}

static void SyncAllActors(bool enable) {
    if (gPlayState == nullptr)
        return;

    for (size_t i = 0; i < ARRAY_COUNT(gPlayState->actorCtx.actorLists); i++) {
        Actor* actor = gPlayState->actorCtx.actorLists[i].head;
        while (actor != nullptr) {
            if (enable)
                AddFlagIfNeeded(actor);
            else
                ClearFlagIfSet(actor);
            actor = actor->next;
        }
    }
}

static void RegisterNoOcarinaFreeze() {
    COND_HOOK(OnActorInit, CVAR_VALUE, [](void* refActor) {
        Actor* actor = static_cast<Actor*>(refActor);
        AddFlagIfNeeded(actor);
    });

    SyncAllActors(CVAR_VALUE);
}

static RegisterShipInitFunc initFunc(RegisterNoOcarinaFreeze, { CVAR_NAME });
