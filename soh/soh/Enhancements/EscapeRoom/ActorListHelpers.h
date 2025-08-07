#include "libultraship/libultra/types.h"
#include "z64actor.h"
#include "z64math.h"
#include <optional>

namespace EscapeRoom {
    Actor* Spawn(s16 actorId, Vec3f pos, Vec3s rot, s16 params);

    Actor* Spawn(s16 actorId, Vec3f pos, s16 params);

    class Find {
        s16 actorId{0};
        std::optional<Vec3f> pos;
        f32 posTolerance{1};
        std::optional<s16> params;

        Actor* head();
        bool matches(Actor* actor);

        template <typename F>
        void each(F&& func) {
            for (Actor* iter = head(); iter != nullptr; iter = iter->next) {
                if (matches(iter))
                    func(iter);
            }
        }

    public:
        [[nodiscard]] explicit Find(s16 actorId_);
        [[nodiscard]] explicit Find(s16 actorId_, Vec3f pos_);
        [[nodiscard]] explicit Find(s16 actorId_, s16 params_);

        Actor* Single();

        void SetParams(s16 params);
        void Move(Vec3f newPos);
        void Move(Vec3f newPos, Vec3f newRot);
        void Delete();
        void ReplaceWith(s16 newActorId);

        template<typename T>
        T* Single() {
            return (T*)Single();
        }
    };

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
    namespace Tsubo {
        constexpr s16 HeartPot = 19715;
    }
    namespace Signs {
        constexpr s16 TellsTruth = 0x030A;
        constexpr s16 TellsLies = 0x030B;
    }
}
