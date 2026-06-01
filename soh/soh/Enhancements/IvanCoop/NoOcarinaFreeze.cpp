#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/ShipInit.hpp"
#include "z64actor.h"

#define CVAR_NAME CVAR_ENHANCEMENT("IvanCoop.NoOcarinaFreeze")
#define CVAR_VALUE CVarGetInteger(CVAR_NAME, 0)

static void RegisterNoOcarinaFreeze() {
    COND_HOOK(OnActorInit, CVAR_VALUE, [](void* refActor) {
        Actor* actor = static_cast<Actor*>(refActor);
        actor->flags |= ACTOR_FLAG_UPDATE_DURING_OCARINA;
    });
}

static RegisterShipInitFunc initFunc(RegisterNoOcarinaFreeze, { CVAR_NAME });
