#include "functions.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/ObjectExtension/ObjectExtension.h"
#include "soh/ShipInit.hpp"
#include "macros.h"

#define CVAR_NAME CVAR_ENHANCEMENT("IvanCoop.NoOcarinaFreeze")
#define CVAR_VALUE CVarGetInteger(CVAR_NAME, 0)

extern "C" {
extern PlayState* gPlayState;
s32 Player_InflictDamage(PlayState* play, s32 damage);
}

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

static bool didCancelOcarina = false;

static void CancelOcarinaForDamage(Player* player) {
    auto overlay = Ship::Context::GetInstance()->GetWindow()->GetGui()->GetGameOverlay();
    overlay->TextDrawNotification(3.0f, true, "NoOcarinaFreeze PRE cleanup");
    player->stateFlags1 &= ~PLAYER_STATE1_IN_CUTSCENE;
    didCancelOcarina = true;
}

static void OnPlayerShouldUpdate(void* actorPtr, bool* /*result*/) {
    Player* player = (Player*)actorPtr;
    if (!(player->stateFlags2 & PLAYER_STATE2_OCARINA_PLAYING))
        return;
    if (!(player->cylinder.base.acFlags & AC_HIT))
        return;

    CancelOcarinaForDamage(player);
}

static void OnPlayerSfx(s16 sfxId) {
    if (didCancelOcarina && sfxId == NA_SE_PL_DAMAGE) {
        didCancelOcarina = false;

        auto overlay = Ship::Context::GetInstance()->GetWindow()->GetGui()->GetGameOverlay();
        overlay->TextDrawNotification(3.0f, true, "NoOcarinaFreeze POST cleanup");

        auto play = gPlayState;
        auto player = GET_PLAYER(play);

        play->msgCtx.ocarinaMode = OCARINA_MODE_04;
        Message_CloseTextbox(play);
    }
}

static s32 NoOcarinaFreezePlayerDamage(PlayState* play, s32 damage) {
    if (CVAR_VALUE) {
        Player* player = GET_PLAYER(play);
        if (player->stateFlags2 & PLAYER_STATE2_OCARINA_PLAYING)
            CancelOcarinaForDamage(player);
    }
    return Player_InflictDamage(play, damage);
}

static void SetDamagePlayerOverride() {
    if (gPlayState != nullptr)
        gPlayState->damagePlayer = NoOcarinaFreezePlayerDamage;
}

static void RegisterNoOcarinaFreeze() {
    COND_HOOK(OnActorInit, CVAR_VALUE, [](void* refActor) {
        Actor* actor = static_cast<Actor*>(refActor);
        AddFlagIfNeeded(actor);
    });

    COND_HOOK(OnPlayerSfx, true, [](s16 sfxId) {
        OnPlayerSfx(sfxId);
    });

    COND_ID_HOOK(OnActorInit, ACTOR_PLAYER, CVAR_VALUE, [](void* /*refActor*/) {
        SetDamagePlayerOverride();
    });

    COND_ID_HOOK(ShouldActorUpdate, ACTOR_PLAYER, CVAR_VALUE, OnPlayerShouldUpdate);

    SyncAllActors(CVAR_VALUE);
}

static RegisterShipInitFunc initFunc(RegisterNoOcarinaFreeze, { CVAR_NAME });
