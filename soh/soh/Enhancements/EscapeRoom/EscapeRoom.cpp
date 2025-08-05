#include "EscapeRoom.h"

#include "global.h"
#include "../game-interactor/GameInteractor.h"
#include "../custom-message/CustomMessageTypes.h"
#include "../custom-message/CustomMessageManager.h"

namespace {
    constexpr s16 LargeGrayRock = 1;
    constexpr s16 BombFlowerBase = -1;

    namespace SceneFlags {
        constexpr s16 BoulderAroundChicken = 0x10;
    }

    Actor* Spawn(s16 actorId, Vec3f pos, Vec3s rot, s16 params) {
        return Actor_Spawn(&gPlayState->actorCtx, gPlayState, actorId, pos.x, pos.y, pos.z, rot.x, rot.y, rot.z, params, false);
    }

    Actor* Spawn(s16 actorId, Vec3f pos, s16 params) {
        return Spawn(actorId, pos, {0, 0, 0}, params);
    }

    void Delete(ActorCategory cat, s16 actorId, Vec3f pos) {
        Actor* begin = gPlayState->actorCtx.actorLists[cat].head;
        for (Actor* iter = begin; iter != nullptr; iter = iter->next) {
            if (iter->id != actorId) continue;
            if (Math_Vec3f_DistXYZ(&iter->world.pos, &pos) >= 1) continue;
            Actor_Kill(iter);
        }
    }
}

namespace {
    bool mAfterSceneInit = false;

    void SetupKakarikoVillage() {
        // rocks blocking the graveyard
        Spawn(ACTOR_EN_ISHI, {1887, 189, 1381}, LargeGrayRock);
        Spawn(ACTOR_EN_ISHI, {1916, 189, 1446}, LargeGrayRock);
        Spawn(ACTOR_EN_ISHI, {1857, 189, 1306}, LargeGrayRock);

        // gate blocking hyrule field
        Spawn(ACTOR_BG_GATE_SHUTTER, {-2140, 137, 1050}, {0, 17074, 0}, -1);

        // the chicken hiding in a bean spot
        Spawn(ACTOR_OBJ_BEAN, {295, 160, 1053}, 0);
        if (!(gSaveContext.infTable[25] & 0x0200)) {
            Delete(ACTORCAT_PROP, ACTOR_EN_NIW, {-1697, 80, 870});
            Spawn(ACTOR_OBJ_MAKEKINSUTA, {295, 160, 1053}, 0x4000);
        }

        // rock covering chicken instead of crate
        Delete(ACTORCAT_BG, ACTOR_OBJ_KIBAKO2, {-60, 0, -46});
        Spawn(ACTOR_OBJ_BOMBIWA, {-70, 0, -40}, SceneFlags::BoulderAroundChicken);

        // bean salesman
        Spawn(ACTOR_EN_MS, {-537, 200, -319}, {0, 552, 0}, 0);
        Spawn(ACTOR_EN_BOMBF, {-576, 200, -343}, BombFlowerBase);
        Spawn(ACTOR_EN_BOMBF, {-472, 200, -358}, BombFlowerBase);
        Spawn(ACTOR_EN_BOMBF, {-522, 200, -365}, BombFlowerBase);
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
            }
            mAfterSceneInit = false;
        }
    });
}

//gSaveContext.entranceIndex = ENTR_HYRULE_FIELD_PAST_BRIDGE_SPAWN;
//gSaveContext.ship.maskMemory = PLAYER_MASK_NONE;

CustomMessage EscapeRoom_GetCustomMessage(u16 textId) {
    CustomMessage msg;
    switch (textId) {
        case TEXT_BEAN_SALESMAN_BUY_FOR_20:
            msg = CustomMessage("Do you like my bomb flowers?");
            break;
        default:
            return CustomMessage();
    }
    msg.AutoFormat();
    return msg;
}
