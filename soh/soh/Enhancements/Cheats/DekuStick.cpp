#include <libultraship/bridge.h>
#include "soh/Enhancements/enhancementTypes.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/ShipInit.hpp"

extern "C" {
extern PlayState* gPlayState;
#include "macros.h"
}

#define CVAR_DEKU_STICK_NAME CVAR_CHEAT("DekuStick")
#define CVAR_DEKU_STICK_DEFAULT DEKU_STICK_NORMAL
#define CVAR_DEKU_STICK_VALUE CVarGetInteger(CVAR_DEKU_STICK_NAME, CVAR_DEKU_STICK_DEFAULT)

void RegisterDekuStickCheats() {
    COND_VB_SHOULD(VB_DEKU_STICK_BREAK, CVAR_DEKU_STICK_VALUE != DEKU_STICK_NORMAL, { *should = false; });
    COND_VB_SHOULD(VB_DEKU_STICK_BURN_OUT, CVAR_DEKU_STICK_VALUE != DEKU_STICK_NORMAL, { *should = false; });
    COND_VB_SHOULD(VB_DEKU_STICK_BURN_DOWN, CVAR_DEKU_STICK_VALUE != DEKU_STICK_NORMAL, { *should = false; });
    COND_VB_SHOULD(VB_DEKU_STICK_BE_ON_FIRE, CVAR_DEKU_STICK_VALUE == DEKU_STICK_UNBREAKABLE_AND_ALWAYS_ON_FIRE, {
        Player* player = GET_PLAYER(gPlayState);
        player->unk_860 = 200;  // Keeps the stick's flame lit
        player->unk_85C = 1.0f; // Ensures the stick is the proper length
        *should = true;
    });
}

static RegisterShipInitFunc initFunc(RegisterDekuStickCheats, { CVAR_DEKU_STICK_NAME });


// putting it here for now because i need to put it somewhere

void CheckForClicks() {
    if (Ship::Context::GetInstance()->GetControlDeck()->MouseGameInputBlocked()) {
        int x = 1;
    }

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        auto v = ImGui::GetMousePos();
        // printf("CLICK: %f %f\n", v.x, v.y);

        auto q = gPlayState->roomCtx.curRoom.meshHeader;

        auto XXX = 1;
    }
}

void RegisterDeleteFace() {
    COND_HOOK(OnGameFrameUpdate, true, CheckForClicks);
}

static RegisterShipInitFunc initFunc2(RegisterDeleteFace);

