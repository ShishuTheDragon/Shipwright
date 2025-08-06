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
        void Move(Vec3f newPos, Vec3f newRot);
        void Delete();

        template<typename T>
        T* Single() {
            return (T*)Single();
        }
    };
}
