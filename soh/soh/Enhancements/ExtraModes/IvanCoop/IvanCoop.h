#pragma once

#include "soh/Enhancements/ExtraModes/IvanCoop/z_en_partner.h"

extern "C" {
extern PlayState* gPlayState;
extern s16 gEnPartnerId;
}

static inline EnPartner* GetIvanActor(PlayState* play) {
    return (EnPartner*)Actor_Find(&play->actorCtx, gEnPartnerId, ACTORCAT_ITEMACTION);
}

static inline f32 XZDistToNearestPlayer(Actor* actor) {
    EnPartner* ivan = GetIvanActor(gPlayState);
    if (ivan != NULL) {
        f32 distToIvan = Actor_WorldDistXZToActor(actor, &ivan->actor);
        if (distToIvan < actor->xzDistToPlayer) {
            return distToIvan;
        }
    }
    return actor->xzDistToPlayer;
}
