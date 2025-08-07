#include "KakarikoVillage.h"

#include "global.h"
#include "overlays/actors/ovl_En_Kanban/z_en_kanban.h"
#include "soh/Enhancements/custom-message/CustomMessageManager.h"
#include "soh/Enhancements/EscapeRoom/ActorListHelpers.h"

namespace EscapeRoom::KakarikoVillage {
    namespace SceneFlag {
        constexpr s16 BoulderAroundChicken = 0x10;
    }

    CustomMessage TellsTruthSignMessage() {
        EnKanban* other = Find(ACTOR_EN_KANBAN, Signs::TellsLies).Single<EnKanban>();
        if (other->partFlags != 0xFFFF)
            return CustomMessage("One sign tells the truth and the other got wrecked, LOL!", TEXTBOX_TYPE_WOODEN);
        else
            return CustomMessage("One sign always tells the truth and the other always lies.", TEXTBOX_TYPE_WOODEN);
    }

    void Setup() {
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
        Spawn(ACTOR_OBJ_BOMBIWA, {-70, 0, -40}, SceneFlag::BoulderAroundChicken);

        // ladder skulltula
        Find(ACTOR_EN_SW, 0x5004).Move({5, 350, -160});

        // outside zora house
        Spawn(ACTOR_EN_ZO, {118, 320, -652}, {0, -13638, 0}, 7);
        Find(ACTOR_OBJ_TSUBO, {284, 200, -356}).Move({196, 320, -824});
        Find(ACTOR_OBJ_TSUBO, {255, 200, -366}).Move({154, 320, -837});
        Find(ACTOR_OBJ_TSUBO, {222, 200, -377}).Move({126, 320, -852});

        // bean salesman
        Spawn(ACTOR_EN_MS, {-537, 200, -319}, {0, 552, 0}, 0);
        Spawn(ACTOR_EN_BOMBF, {-576, 200, -343}, Bombf::FlowerBase);
        Spawn(ACTOR_EN_BOMBF, {-472, 200, -358}, Bombf::FlowerBase);
        Spawn(ACTOR_EN_BOMBF, {-522, 200, -365}, Bombf::FlowerBase);
    }

    CustomMessage GetCustomMessage(u16 textId) {
        switch (textId) {
            // rooftop man
            case 0x5050: return CustomMessage("Do some parkour!");
            case 0x5055: return CustomMessage("\x08             Parkour!\x0E\x48");
            case 0x5051: return CustomMessage("Sweet moves!^Anyway, this washed up while I was sleeping. You can have it!");
            case 0x0099: return CustomMessage("You found a letter in a bottle!&Don't give it to any strange hands.");
            case 0x5056: return CustomMessage("Maybe ask around town, see if anyone knows how to read.");

            // the chicken hiding in a bean spot
            case 0x2022: return CustomMessage("I wish I was small and icky. Then I could crawl into this soil!");
            case 0x2028: return CustomMessage("Eww, I saw a chicken!");
            case 0x002F: return CustomMessage("Yep, that's ground.");

            // outside zora house
            case 0x4021: return CustomMessage("Have you heard of #Zoras#? They're like #fishes# but #tall#.^Remember that. It could be the difference between #life# or #death#.", { QM_BLUE, QM_GREEN, QM_RED, QM_YELLOW, QM_PINK });

            // the chicken in a hurty crate
            case Signs::TellsTruth: return TellsTruthSignMessage();
            case Signs::TellsLies: return CustomMessage("Tip: Rolling into boxes is a good idea!", TEXTBOX_TYPE_WOODEN);
        }
        return CustomMessage();
    }
}
