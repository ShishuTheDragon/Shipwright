#pragma once

#include "soh/Enhancements/ExtraModes/IvanCoop/z_en_partner.h"

extern "C" {
extern PlayState* gPlayState;
extern s16 gEnPartnerId;
}

EnPartner* GetIvanActor(PlayState* play);
f32 XZDistToNearestPlayer(Actor* actor);
