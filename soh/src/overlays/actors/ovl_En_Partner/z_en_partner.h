#ifndef Z_EN_PARTNER_H
#define Z_EN_PARTNER_H

#include <libultraship/libultra.h>
#include "global.h"
#include <overlays/actors/ovl_En_Boom/z_en_boom.h>

struct EnPartner;

typedef void (*EnPartnerActionFunc)(struct EnPartner*, PlayState*);

typedef struct EnPartner {
    Actor actor;

    SkelAnime skelAnime;

    Vec3s jointTable[15];
    Vec3s morphTable[15];

    ColliderCylinder collider;
    ColliderCylinder weaponCollider;

    Color_RGBAf innerColor;
    Color_RGBAf outerColor;
    LightInfo lightInfoGlow;
    LightNode* lightNodeGlow;
    LightInfo lightInfoNoGlow;
    LightNode* lightNodeNoGlow;

    f32 yVelocity;

    u8 canMove;
    u8 usedItem;
    u8 usedItemButton;
    u8 usedSpell;
    u8 damageTimer;
    s16 magicTimer;

    u8 shouldDraw;
    s16 itemTimer;
    s16 beanCooldownTimer;

    GetItemEntry entry;
    WeaponInfo stickWeaponInfo;

    EnBoom* boomerangActor;
    Actor* hookshotTarget;
} EnPartner;

#ifdef __cplusplus
extern "C" {
#endif
void EnPartner_Init(Actor* thisx, PlayState* play);
void EnPartner_Destroy(Actor* thisx, PlayState* play);
void EnPartner_Update(Actor* thisx, PlayState* play);
void EnPartner_Draw(Actor* thisx, PlayState* play);

extern EnPartner* gIvanActor;
extern f32 gIvanCamYaw;
extern f32 gIvanCamPitch;

// Returns XZ distance from actor to the nearest player (Link or Ivan).
// Use this instead of actor->xzDistToPlayer when gating collision checks,
// so that Ivan can interact with objects even when Link is far away.
static inline f32 Actor_XZDistToNearestPlayer(Actor* actor) {
    if (gIvanActor != NULL) {
        f32 distToIvan = Actor_WorldDistXZToActor(actor, &gIvanActor->actor);
        if (distToIvan < actor->xzDistToPlayer) {
            return distToIvan;
        }
    }
    return actor->xzDistToPlayer;
}
#ifdef __cplusplus
}
#endif

#endif
