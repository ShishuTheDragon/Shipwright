#include "ActorListHelpers.h"

#include "global.h"
#include "soh/ActorDB.h"

namespace EscapeRoom {
    Actor* Spawn(s16 actorId, Vec3f pos, Vec3s rot, s16 params) {
        return Actor_Spawn(&gPlayState->actorCtx, gPlayState, actorId, pos.x, pos.y, pos.z, rot.x, rot.y, rot.z, params, false);
    }

    Actor* Spawn(s16 actorId, Vec3f pos, s16 params) {
        return Spawn(actorId, pos, {0, 0, 0}, params);
    }

    Actor* Find::head() {
        s32 cat = ActorDB::Instance->RetrieveEntry(actorId).entry.category;
        return gPlayState->actorCtx.actorLists[cat].head;
    }

    bool Find::matches(Actor* actor) {
        if (actor->id != actorId)
            return false;
        if (pos.has_value() && Math_Vec3f_DistXYZ(&actor->world.pos, &*pos) >= 1)
            return false;
        if (params.has_value() && actor->params != *params)
            return false;
        return true;
    }

    Find::Find(s16 actorId_)
        : actorId{actorId_} {}

    Find::Find(s16 actorId_, Vec3f pos_)
        : actorId{actorId_}, pos(pos_) {}

    Find::Find(s16 actorId_, s16 params_)
        : actorId{actorId_}, params(params_) {}

    Actor* Find::Single() {
        for (Actor* iter = head(); iter != nullptr; iter = iter->next) {
            if (matches(iter))
                return iter;
        }
        return nullptr;
    }

    void Find::SetParams(s16 params) {
        each([&](Actor* actor) {
            actor->params = params;
        });
    }

    void Find::Move(Vec3f newPos, Vec3f newRot) {
        each([&](Actor* actor) {
            Math_Vec3f_Copy(&actor->world.pos, &newPos);
            actor->world.rot.x = newRot.x;
            actor->world.rot.y = newRot.y;
            actor->world.rot.z = newRot.z;
        });
    }

    void Find::Delete() {
        each(Actor_Kill);
    }
}
