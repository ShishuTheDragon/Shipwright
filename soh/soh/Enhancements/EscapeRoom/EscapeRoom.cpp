#include "EscapeRoom.h"

#include "global.h"
#include "../game-interactor/GameInteractor.h"
#include "../custom-message/CustomMessageTypes.h"
#include "../custom-message/CustomMessageManager.h"
#include "soh/ActorDB.h"
#include "overlays/actors/ovl_En_Kanban/z_en_kanban.h"

#include "ActorListHelpers.h"
#include "KakarikoVillage.h"
using namespace EscapeRoom;

namespace EscapeRoom::ZoraHouse {
    void Setup() {
        Find(ACTOR_EN_DAIKU_KAKARIKO).Delete();
        Spawn(ACTOR_EN_KZ, {-110, 0, 50}, {0, 7000, 0}, 0);
    }

    CustomMessage GetCustomMessage(u16 textId) {
        switch (textId) {
            // king zora
            case 0x401A: // in the way
            case 0x401C: // already moved
                return CustomMessage("Mweep?\x1B#Mweep&Mweepn't#", { QM_GREEN });

            case 0x401B: // showed the letter
            case 0x0227: // back door is blocked
                return CustomMessage("\x08             Mweep!");
        }
        return CustomMessage();
    }
}

namespace {
    CustomMessage GetCustomMessageForScene(u16 textId) {
        switch (gPlayState->sceneNum) {
        }
        return CustomMessage();
    }

    CustomMessage GetCustomMessage(u16 textId) {
        switch (textId) {
            // case 0x0301: // test sign
            // case TEXT_BEAN_SALESMAN_BUY_FOR_20:
                // return CustomMessage("Do you like my bomb flowers?");
            case 0x4005: return CustomMessage("Help! I'm a letter trapped in a bottle!");
        }

        CustomMessage msg;

        msg = KakarikoVillage::GetCustomMessage(textId);
        if (msg != CustomMessage())
            return msg;

        msg = ZoraHouse::GetCustomMessage(textId);
        if (msg != CustomMessage())
            return msg;

        char buf[64];
        snprintf(buf, sizeof(buf), "err: missing string for 0x%04X", textId);
        return CustomMessage(buf);
    }
}

void EscapeRoom_RegisterHooks() {
    static bool gAfterSceneInit = false;

    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](int16_t sceneNum) {
        gAfterSceneInit = true;
    });

    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameFrameUpdate>([]() {
        if (gAfterSceneInit) {
            switch (gPlayState->sceneNum) {
                case SCENE_KAKARIKO_VILLAGE: KakarikoVillage::Setup(); break;
                case SCENE_POTION_SHOP_KAKARIKO: ZoraHouse::Setup(); break;
            }
            gAfterSceneInit = false;
        }
    });
}

//gSaveContext.entranceIndex = ENTR_HYRULE_FIELD_PAST_BRIDGE_SPAWN;
//gSaveContext.ship.maskMemory = PLAYER_MASK_NONE;

CustomMessage EscapeRoom_GetCustomMessage(u16 textId) {
    CustomMessage msg = GetCustomMessage(textId);
    msg.AutoFormat();
    return msg;
}
