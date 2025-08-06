#include "EscapeRoom.h"

#include "global.h"
#include "../game-interactor/GameInteractor.h"
#include "../custom-message/CustomMessageTypes.h"
#include "../custom-message/CustomMessageManager.h"
#include "soh/ActorDB.h"
#include "overlays/actors/ovl_En_Kanban/z_en_kanban.h"

#include "ActorListHelpers.h"
using namespace EscapeRoom;

namespace {
    namespace Ishi {
        constexpr s16 LargeGrayRock = 1;
    }
    namespace Bombf {
        constexpr s16 FlowerBase = -1;
    }
    namespace Makekinsuta {
        constexpr s16 BeanSpotChicken = 0x4000;
    }
    namespace Niw {
        constexpr s16 HideInACrate = 4;
    }
    namespace KV_SceneFlag {
        constexpr s16 BoulderAroundChicken = 0x10;
    }
    namespace Signs {
        constexpr s16 TellsTruth = 0x030A;
        constexpr s16 TellsLies = 0x030B;
    }
}

namespace {
    bool mAfterSceneInit = false;

    void SetupKakarikoVillage() {
        // rocks blocking the graveyard
        Spawn(ACTOR_EN_ISHI, {1887, 189, 1381}, Ishi::LargeGrayRock);
        Spawn(ACTOR_EN_ISHI, {1916, 189, 1446}, Ishi::LargeGrayRock);
        Spawn(ACTOR_EN_ISHI, {1857, 189, 1306}, Ishi::LargeGrayRock);

        // gate blocking hyrule field
        Spawn(ACTOR_BG_GATE_SHUTTER, {-2140, 137, 1050}, {0, 17074, 0}, -1);

        // the chicken hiding in a bean spot
        // (formerly the one by the entrance)
        Spawn(ACTOR_OBJ_BEAN, {295, 160, 1053}, 0);
        if (!(gSaveContext.infTable[25] & 0x0200)) {
            Find(ACTOR_EN_NIW, {-1697, 80, 870}).Delete();
            Spawn(ACTOR_OBJ_MAKEKINSUTA, {295, 160, 1053}, Makekinsuta::BeanSpotChicken);
        }
        Spawn(ACTOR_EN_CS, {330, 160, 1080}, {0, -22965, 0}, 0);

        // the chicken in a hurty crate
        // (formerly just chillin’ by Anju)
        Spawn(ACTOR_EN_KANBAN, {746, 65, 1597}, {0, -21013, 0}, Signs::TellsLies);
        Spawn(ACTOR_EN_KANBAN, {830, 65, 1580}, {0, 27250, 0}, Signs::TellsTruth);
        Find(ACTOR_EN_NIW, {796, 80, 1639}).SetParams(Niw::HideInACrate);
        Spawn(ACTOR_OBJ_KIBAKO2, {796, 80, 1639}, -6);

        // the chicken in a rock
        // (formerly the one in a crate)
        Find(ACTOR_OBJ_KIBAKO2, {-60, 0, -46}).Delete();
        Spawn(ACTOR_OBJ_BOMBIWA, {-70, 0, -40}, KV_SceneFlag::BoulderAroundChicken);

        // bean salesman
        Spawn(ACTOR_EN_MS, {-537, 200, -319}, {0, 552, 0}, 0);
        Spawn(ACTOR_EN_BOMBF, {-576, 200, -343}, Bombf::FlowerBase);
        Spawn(ACTOR_EN_BOMBF, {-472, 200, -358}, Bombf::FlowerBase);
        Spawn(ACTOR_EN_BOMBF, {-522, 200, -365}, Bombf::FlowerBase);
    }

    void SetupZoraHouse() {
        Find(ACTOR_EN_DAIKU_KAKARIKO).Delete();
        Spawn(ACTOR_EN_KZ, {-110, 0, 50}, {0, 7000, 0}, 0);
    }

    CustomMessage GetCustomMessage(u16 textId) {
        switch (textId) {
            case TEXT_BEAN_SALESMAN_BUY_FOR_20:
                return CustomMessage("Do you like my bomb flowers?");

            // case 0x0301: // test sign

            // rooftop man
            case 0x5050:
                return CustomMessage("Do some parkour!");
            case 0x5055:
                return CustomMessage("\x08             Parkour!\x0E\x48");
            case 0x5051:
                return CustomMessage("Sweet moves!^Anyway, this washed up while I was sleeping. You can have it!");
            case 0x0099:
                return CustomMessage("You found a letter in a bottle!&Don't give it to any strange hands.");
            case 0x5056:
                return CustomMessage("Maybe ask around town, see if anyone knows how to read.");
            case 0x4005:
                return CustomMessage("Help! I'm a letter trapped in a bottle!");

            // the chicken hiding in a bean spot
            case 0x2022:
                return CustomMessage("I wish I was small and icky. Then I could crawl into this soil!");
            case 0x2028:
                return CustomMessage("Eww, I saw a chicken!");
            case 0x002F:
                return CustomMessage("Yep, that's ground.");

            // the chicken in a hurty crate
            case Signs::TellsTruth: {
                EnKanban* other = Find(ACTOR_EN_KANBAN, Signs::TellsLies).Single<EnKanban>();
                if (other->partFlags != 0xFFFF)
                    return CustomMessage("One sign tells the truth and the other got wrecked, LOL!", TEXTBOX_TYPE_WOODEN);
                return CustomMessage("One sign always tells the truth and the other always lies.", TEXTBOX_TYPE_WOODEN);
            }
            case Signs::TellsLies:
                return CustomMessage("Tip: Rolling into boxes is a good idea!", TEXTBOX_TYPE_WOODEN);

            // king zora
            case 0x401A:
            case 0x401C:
                return CustomMessage("Mweep?\x1B#Mweep&Mweepn't#", { QM_GREEN });
            case 0x401B:
                return CustomMessage("Mweep!");

            default:
                char buf[64];
                snprintf(buf, sizeof(buf), "err: missing string for 0x%04X", textId);
                return CustomMessage(buf);
        }
    }
}

void EscapeRoom_RegisterHooks() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](int16_t sceneNum) {
        mAfterSceneInit = true;
    });

    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameFrameUpdate>([]() {
        if (mAfterSceneInit) {
            switch (gPlayState->sceneNum) {
                case SCENE_KAKARIKO_VILLAGE: SetupKakarikoVillage(); break;
                case SCENE_POTION_SHOP_KAKARIKO: SetupZoraHouse(); break;
            }
            mAfterSceneInit = false;
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
