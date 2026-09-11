#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/Enhancements/cosmetics/cosmeticsTypes.h"
#include "soh/ShipInit.hpp"

extern "C" {
#include "textures/parameter_static/parameter_static.h"
#include "soh_assets.h"
#include "functions.h"
#include "macros.h"
#include "variables.h"
extern PlayState* gPlayState;
extern const char* _gAmmoDigit0Tex[];
Gfx* Gfx_TextureIA8(Gfx* displayListHead, void* texture, s16 textureWidth, s16 textureHeight, s16 rectLeft, s16 rectTop,
                    s16 rectWidth, s16 rectHeight, u16 dsdx, u16 dtdy);
float OTRGetDimensionFromLeftEdge(float v);
float OTRGetDimensionFromRightEdge(float v);
void FrameInterpolation_RecordOpenChild(const void* a, int b);
void FrameInterpolation_RecordCloseChild(void);
}

#define CVAR_IVAN_COOP_MODE_NAME CVAR_ENHANCEMENT("IvanCoopModeEnabled")
#define CVAR_IVAN_COOP_MODE_VALUE CVarGetInteger(CVAR_IVAN_COOP_MODE_NAME, 0)

#define CVAR_IVAN_SEPARATE_LOADOUT_NAME CVAR_ENHANCEMENT("IvanSeparateLoadout")
#define CVAR_IVAN_SEPARATE_LOADOUT_VALUE CVarGetInteger(CVAR_IVAN_SEPARATE_LOADOUT_NAME, 0)

static const s16 cButtonSize = 16;
static const s16 cButtonTexStep = 512 * 32 / cButtonSize;
static const s16 itemIconSize = 16;
static const s16 itemIconTexStep = 512 * 32 / itemIconSize;
static const s16 itemSpacing = 16;
static const s16 dPadSize = 32;
static const s16 dPadTexStep = 512;
static const s16 dPadCenterX = 64;
static const s16 cButtonsCenterX = 256;
static const s16 dPadCenterY = 208;
static const s16 cButtonsCenterY = 208;
static const s16 naviLabelYOffset = 4;
static const s16 naviLabelXOffset = 8;
static const s16 ivanEquipAnimFrames = 10; // mirrors vanilla's sEquipMoveTimer

enum class IvanItemIndex : u8 {
    CLeft,
    CDown,
    CRight,
    DPadUp,
    DPadDown,
    DPadLeft,
    DPadRight,
};

struct IvanEquipAnim {
    bool active;
    s16 item;
    s16 startX, startY;
    s16 framesLeft;
};

struct IvanEquipButton {
    u16 button;
    IvanItemIndex slot;
};

static const IvanEquipButton equipButtons[] = {
    { BTN_CLEFT, IvanItemIndex::CLeft },      { BTN_CDOWN, IvanItemIndex::CDown },
    { BTN_CRIGHT, IvanItemIndex::CRight },    { BTN_DUP, IvanItemIndex::DPadUp },
    { BTN_DDOWN, IvanItemIndex::DPadDown },   { BTN_DLEFT, IvanItemIndex::DPadLeft },
    { BTN_DRIGHT, IvanItemIndex::DPadRight },
};

static IvanEquipAnim sIvanEquipAnim[ARRAY_COUNT(gSaveContext.ship.ivanButtonItems)];

static s16 IvanIconItem(s16 itemId) {
    switch (itemId) {
        case ITEM_ARROW_FIRE:
            return ITEM_BOW_ARROW_FIRE;
        case ITEM_ARROW_ICE:
            return ITEM_BOW_ARROW_ICE;
        case ITEM_ARROW_LIGHT:
            return ITEM_BOW_ARROW_LIGHT;
        default:
            return itemId;
    }
}

static bool IsAmmoItem(s16 itemId) {
    if ((itemId >= ITEM_BOW_ARROW_FIRE) && (itemId <= ITEM_BOW_ARROW_LIGHT)) {
        return true;
    }

    switch (itemId) {
        case ITEM_STICK:
        case ITEM_NUT:
        case ITEM_BOMB:
        case ITEM_BOW:
        case ITEM_SLINGSHOT:
        case ITEM_BOMBCHU:
        case ITEM_BEAN:
            return true;
        default:
            return false;
    }
}

static bool IvanCanUseItem(s16 itemId) {
    switch (itemId) {
        case ITEM_STICK:
        case ITEM_NUT:
        case ITEM_BOMB:
        case ITEM_BOMBCHU:
        case ITEM_BOW:
        case ITEM_ARROW_FIRE:
        case ITEM_ARROW_ICE:
        case ITEM_ARROW_LIGHT:
        case ITEM_SLINGSHOT:
        case ITEM_OCARINA_FAIRY:
        case ITEM_OCARINA_TIME:
        case ITEM_HOOKSHOT:
        case ITEM_LONGSHOT:
        case ITEM_DINS_FIRE:
        case ITEM_NAYRUS_LOVE:
        case ITEM_FARORES_WIND:
        case ITEM_HAMMER:
        case ITEM_BOOMERANG:
        case ITEM_LENS:
        case ITEM_BEAN:
            return true;
        default:
            return false;
    }
}

// Converts the cursor's grid vertex to HUD space, the inverse of vanilla's x-160 / 120-y mapping.
static void IvanSeedEquipAnim(PlayState* play, u8 slot, s16 item) {
    s16 cursorSlot = play->pauseCtx.cursorSlot[PAUSE_ITEM];
    s16 idx = cursorSlot * 4;
    const s16 halfQuad = 16; // itemVtx is the quad's top-left; the sprite draws centered
    sIvanEquipAnim[slot].active = true;
    sIvanEquipAnim[slot].item = item;
    sIvanEquipAnim[slot].startX = (s16)(play->pauseCtx.itemVtx[idx].v.ob[0] + 160 + halfQuad);
    sIvanEquipAnim[slot].startY = (s16)(120 - play->pauseCtx.itemVtx[idx].v.ob[1] + halfQuad);
    sIvanEquipAnim[slot].framesLeft = ivanEquipAnimFrames;
}

static void OnKaleidoUpdate() {
    PlayState* play = gPlayState;

    if (play->pauseCtx.state == 6 && play->pauseCtx.pageIndex == PAUSE_ITEM) {
        u16 cursorItem = play->pauseCtx.cursorItem[PAUSE_ITEM];

        if (cursorItem != PAUSE_ITEM_NONE) {
            u32 ivanButtons = play->state.input[1].press.button;
            if (cursorItem != ITEM_SOLD_OUT && cursorItem != ITEM_NONE) {
                // En_Partner only polls Ivan's D-pad slots when DpadEquips is on.
                bool dpadEquips = CVarGetInteger(CVAR_ENHANCEMENT("DpadEquips"), 0) != 0;

                for (size_t i = 0; i < ARRAY_COUNT(equipButtons); i++) {
                    if (!dpadEquips && equipButtons[i].slot >= IvanItemIndex::DPadUp) {
                        continue;
                    }
                    if (CHECK_BTN_ALL(ivanButtons, equipButtons[i].button)) {
                        if (!IvanCanUseItem(cursorItem)) {
                            Audio_PlaySfxGeneral(NA_SE_SY_ERROR, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale,
                                                 &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
                            break;
                        }
                        u8 targetSlot = (u8)equipButtons[i].slot;

                        // Honor P1's "Allow unequipping Items" toggle (see ItemUnequip.cpp).
                        if (CVarGetInteger(CVAR_ENHANCEMENT("ItemUnequip"), 0) &&
                            gSaveContext.ship.ivanButtonItems[targetSlot] == cursorItem) {
                            gSaveContext.ship.ivanButtonItems[targetSlot] = ITEM_NONE;
                            sIvanEquipAnim[targetSlot].active = false;
                            Audio_PlaySfxGeneral(NA_SE_SY_DECIDE, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale,
                                                 &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
                            break;
                        }

                        // Swap with the slot already holding this item, like P1's item binding.
                        u8 displacedItem = gSaveContext.ship.ivanButtonItems[targetSlot];
                        for (size_t j = 0; j < ARRAY_COUNT(gSaveContext.ship.ivanButtonItems); j++) {
                            if (j != targetSlot && gSaveContext.ship.ivanButtonItems[j] == cursorItem) {
                                gSaveContext.ship.ivanButtonItems[j] = displacedItem;
                                sIvanEquipAnim[j].active = false;
                            }
                        }
                        gSaveContext.ship.ivanButtonItems[targetSlot] = cursorItem;
                        IvanSeedEquipAnim(play, targetSlot, cursorItem);
                        Audio_PlaySfxGeneral(NA_SE_SY_DECIDE, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale,
                                             &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
                        break;
                    }
                }
            }
        }
    }
}

static Gfx* DrawWideTextureRectCentered(Gfx* displayListHead, Vec3s center, s16 size, s16 texStep) {
    s32 x = center.x - (size / 2);
    s32 y = center.y - (size / 2);

    gSPWideTextureRectangle(displayListHead++, x << 2, y << 2, (x + size) << 2, (y + size) << 2, G_TX_RENDERTILE, 0, 0,
                            texStep << 1, texStep << 1);
    return displayListHead;
}

static Gfx* DrawAmmoCountAt(Gfx* displayListHead, s16 itemId, s16 x, s16 y, s16 alpha) {
    if (!GameInteractor_Should(VB_DRAW_AMMO_COUNT, IsAmmoItem(itemId), &itemId)) {
        return displayListHead;
    }

    if ((itemId >= ITEM_BOW_ARROW_FIRE) && (itemId <= ITEM_BOW_ARROW_LIGHT)) {
        itemId = ITEM_BOW;
    }

    s16 ammo = AMMO(itemId);
    if (ammo < 0) {
        ammo = 0;
    }

    s16 tens = ammo / 10;
    s16 ones = ammo % 10;

    gDPSetPrimColor(displayListHead++, 0, 0, 255, 255, 255, alpha);
    if (((itemId == ITEM_BOW) && (ammo == CUR_CAPACITY(UPG_QUIVER))) ||
        ((itemId == ITEM_BOMB) && (ammo == CUR_CAPACITY(UPG_BOMB_BAG))) ||
        ((itemId == ITEM_SLINGSHOT) && (ammo == CUR_CAPACITY(UPG_BULLET_BAG))) ||
        ((itemId == ITEM_STICK) && (ammo == CUR_CAPACITY(UPG_STICKS))) ||
        ((itemId == ITEM_NUT) && (ammo == CUR_CAPACITY(UPG_NUTS))) || ((itemId == ITEM_BOMBCHU) && (ammo == 50)) ||
        ((itemId == ITEM_BEAN) && (ammo == 15)) || GameInteractor_Should(VB_COLOR_AMMO_GREEN, false, itemId)) {
        gDPSetPrimColor(displayListHead++, 0, 0, 120, 255, 0, alpha);
    }
    if (ammo == 0) {
        gDPSetPrimColor(displayListHead++, 0, 0, 100, 100, 100, alpha);
    }
    if (tens != 0) {
        displayListHead =
            Gfx_TextureIA8(displayListHead, (u8*)_gAmmoDigit0Tex[tens], 8, 8, x, y, 8, 8, 1 << 10, 1 << 10);
    }
    displayListHead =
        Gfx_TextureIA8(displayListHead, (u8*)_gAmmoDigit0Tex[ones], 8, 8, x + 6, y, 8, 8, 1 << 10, 1 << 10);

    return displayListHead;
}

// Callers must check active first and skip the slot's static icon while the flight is running.
static Gfx* IvanDrawEquipAnim(Gfx* displayListHead, u8 slot, Vec3s target, s16 alpha) {
    IvanEquipAnim& anim = sIvanEquipAnim[slot];
    float frac = (float)anim.framesLeft / ivanEquipAnimFrames;
    Vec3s pos = {
        (s16)(target.x + (anim.startX - target.x) * frac),
        (s16)(target.y + (anim.startY - target.y) * frac),
        0,
    };
    s16 item = IvanIconItem(anim.item);

    gDPPipeSync(displayListHead++);
    gDPLoadTextureBlock(displayListHead++, gItemIcons[item], G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, 0,
                        G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
                        G_TX_NOLOD);
    gDPSetPrimColor(displayListHead++, 0, 0, 255, 255, 255, alpha);
    displayListHead = DrawWideTextureRectCentered(displayListHead, pos, itemIconSize, itemIconTexStep);

    if (anim.framesLeft > 0) {
        anim.framesLeft--;
    }
    if (anim.framesLeft == 0) {
        anim.active = false;
    }
    return displayListHead;
}

// Draws one slot's equipped icon at `center`, or its in-flight equip animation while one is running.
static Gfx* IvanDrawSlotItem(Gfx* displayListHead, u8 slot, Vec3s center, s16 alpha) {
    if (sIvanEquipAnim[slot].active) {
        return IvanDrawEquipAnim(displayListHead, slot, center, alpha);
    }

    s16 item = IvanIconItem(gSaveContext.ship.ivanButtonItems[slot]);
    if (item == ITEM_NONE) {
        return displayListHead;
    }

    gDPPipeSync(displayListHead++);
    gDPLoadTextureBlock(displayListHead++, gItemIcons[item], G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, 0,
                        G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
                        G_TX_NOLOD);
    gDPSetPrimColor(displayListHead++, 0, 0, 255, 255, 255, alpha);
    displayListHead = DrawWideTextureRectCentered(displayListHead, center, itemIconSize, itemIconTexStep);
    displayListHead =
        DrawAmmoCountAt(displayListHead, item, center.x - (itemIconSize / 2), center.y - (itemIconSize / 2), alpha);
    return displayListHead;
}

struct Centers {
    s16 dpadX, dpadY;
    s16 cButtonsX, cButtonsY;
};

static Centers GetCenters() {
    s16 bottomMargin = CVarGetInteger(CVAR_COSMETIC("HUD.Margin.B"), 0);
    s16 leftMargin = CVarGetInteger(CVAR_COSMETIC("HUD.Margin.L"), 0);
    s16 rightMargin = CVarGetInteger(CVAR_COSMETIC("HUD.Margin.R"), 0);

    Centers centers;

    /* dpadX/Y */ {
        s16 posType = CVarGetInteger(CVAR_COSMETIC("Ivan.Dpad.PosType"), ORIGINAL_LOCATION);
        bool useMargins = CVarGetInteger(CVAR_COSMETIC("Ivan.Dpad.UseMargins"), 0) != 0;
        s16 yMargin = useMargins ? bottomMargin : 0;

        if (posType == ORIGINAL_LOCATION) {
            centers.dpadX = (s16)(dPadCenterX + (useMargins ? leftMargin : 0));
            centers.dpadY = (s16)(dPadCenterY + yMargin);
        } else {
            s16 posX = CVarGetInteger(CVAR_COSMETIC("Ivan.Dpad.PosX"), 0);
            centers.dpadY = (s16)(CVarGetInteger(CVAR_COSMETIC("Ivan.Dpad.PosY"), 0) + yMargin);
            switch (posType) {
                case ANCHOR_LEFT:
                    centers.dpadX = (s16)OTRGetDimensionFromLeftEdge(posX + (useMargins ? leftMargin : 0));
                    break;
                case ANCHOR_RIGHT:
                    centers.dpadX = (s16)OTRGetDimensionFromRightEdge(posX + (useMargins ? rightMargin : 0));
                    break;
                case ANCHOR_NONE:
                    centers.dpadX = posX;
                    break;
                case HIDDEN:
                default:
                    centers.dpadX = -9999;
                    break;
            }
        }
    }

    /* cButtonsX/Y */ {
        s16 posType = CVarGetInteger(CVAR_COSMETIC("Ivan.CButtons.PosType"), ORIGINAL_LOCATION);
        bool useMargins = CVarGetInteger(CVAR_COSMETIC("Ivan.CButtons.UseMargins"), 0) != 0;
        s16 yMargin = useMargins ? bottomMargin : 0;

        if (posType == ORIGINAL_LOCATION) {
            centers.cButtonsX = (s16)(cButtonsCenterX + (useMargins ? rightMargin : 0));
            centers.cButtonsY = (s16)(cButtonsCenterY + yMargin);
        } else {
            s16 posX = CVarGetInteger(CVAR_COSMETIC("Ivan.CButtons.PosX"), 0);
            centers.cButtonsY = (s16)(CVarGetInteger(CVAR_COSMETIC("Ivan.CButtons.PosY"), 0) + yMargin);
            switch (posType) {
                case ANCHOR_LEFT:
                    centers.cButtonsX = (s16)OTRGetDimensionFromLeftEdge(posX + (useMargins ? leftMargin : 0));
                    break;
                case ANCHOR_RIGHT:
                    centers.cButtonsX = (s16)OTRGetDimensionFromRightEdge(posX + (useMargins ? rightMargin : 0));
                    break;
                case ANCHOR_NONE:
                    centers.cButtonsX = posX;
                    break;
                case HIDDEN:
                default:
                    centers.cButtonsX = -9999;
                    break;
            }
        }
    }
    return centers;
}

static void OnInterfaceDraw() {
    PlayState* play = gPlayState;
    InterfaceContext* interfaceCtx = &gPlayState->interfaceCtx;

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL_39Overlay(play->state.gfxCtx);

    gDPPipeSync(OVERLAY_DISP++);
    gDPSetCombineMode(OVERLAY_DISP++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    gDPSetEnvColor(OVERLAY_DISP++, 0, 0, 0, 255);

    Color_RGB8 primary = { 255, 255, 255 };
    if (CVarGetInteger(CVAR_COSMETIC("Ivan.IdlePrimary.Changed"), 0)) {
        primary = CVarGetColor24(CVAR_COSMETIC("Ivan.IdlePrimary.Value"), (Color_RGB8){ 255, 255, 255 });
    }
    Color_RGB8 secondary = { 0, 255, 0 };
    if (CVarGetInteger(CVAR_COSMETIC("Ivan.IdleSecondary.Changed"), 0)) {
        secondary = CVarGetColor24(CVAR_COSMETIC("Ivan.IdleSecondary.Value"), (Color_RGB8){ 0, 255, 0 });
    }
    Color_RGB8 tint = {
        (u8)(((u16)primary.r + (u16)secondary.r) / 2),
        (u8)(((u16)primary.g + (u16)secondary.g) / 2),
        (u8)(((u16)primary.b + (u16)secondary.b) / 2),
    };

    // Follow the life meter, not the C-button alphas, which dim for restrictions Ivan ignores.
    s16 ivanHudAlpha = interfaceCtx->healthAlpha;

    gDPSetPrimColor(OVERLAY_DISP++, 0, 0, tint.r, tint.g, tint.b, ivanHudAlpha);

    Centers clusters = GetCenters();

    Vec3s centers[4] = {
        { (s16)(clusters.cButtonsX - itemSpacing), clusters.cButtonsY, 0 }, // C-Left
        { clusters.cButtonsX, (s16)(clusters.cButtonsY + itemSpacing), 0 }, // C-Down
        { (s16)(clusters.cButtonsX + itemSpacing), clusters.cButtonsY, 0 }, // C-Right
        { clusters.cButtonsX, (s16)(clusters.cButtonsY - itemSpacing), 0 }, // C-Up
    };

    gDPLoadTextureBlock(OVERLAY_DISP++, gButtonBackgroundTex, G_IM_FMT_IA, G_IM_SIZ_8b, 32, 32, 0,
                        G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
                        G_TX_NOLOD);
    for (size_t i = 0; i < ARRAY_COUNT(centers); i++) {
        OVERLAY_DISP = DrawWideTextureRectCentered(OVERLAY_DISP, centers[i], cButtonSize, cButtonTexStep);
    }

    if (CVarGetInteger(CVAR_ENHANCEMENT("DpadEquips"), 0) != 0) {
        Vec3s dpadCenters[4] = {
            { clusters.dpadX, (s16)(clusters.dpadY - itemSpacing), 0 }, // Dpad-Up
            { clusters.dpadX, (s16)(clusters.dpadY + itemSpacing), 0 }, // Dpad-Down
            { (s16)(clusters.dpadX - itemSpacing), clusters.dpadY, 0 }, // Dpad-Left
            { (s16)(clusters.dpadX + itemSpacing), clusters.dpadY, 0 }, // Dpad-Right
        };

        gDPLoadTextureBlock(OVERLAY_DISP++, gDPadTex, G_IM_FMT_IA, G_IM_SIZ_16b, dPadSize, dPadSize, 0,
                            G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
                            G_TX_NOLOD);
        Vec3s dpadTexCenter = { clusters.dpadX, clusters.dpadY, 0 };
        OVERLAY_DISP = DrawWideTextureRectCentered(OVERLAY_DISP, dpadTexCenter, dPadSize, dPadTexStep);

        for (size_t i = 0; i < ARRAY_COUNT(dpadCenters); i++) {
            u8 slot = (u8)((size_t)IvanItemIndex::DPadUp + i);
            OVERLAY_DISP = IvanDrawSlotItem(OVERLAY_DISP, slot, dpadCenters[i], ivanHudAlpha);
        }
    }

    // Only the three item-bearing C slots; centers[3] is C-Up, which carries the Navi label instead.
    for (size_t i = 0; i < 3; i++) {
        u8 slot = (u8)((size_t)IvanItemIndex::CLeft + i);
        OVERLAY_DISP = IvanDrawSlotItem(OVERLAY_DISP, slot, centers[i], ivanHudAlpha);
    }

    s16 cUpLeftX = centers[3].x - (cButtonSize / 2);
    s16 cUpLeftY = centers[3].y - (cButtonSize / 2);

    gDPPipeSync(OVERLAY_DISP++);
    gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 255, 255, ivanHudAlpha);
    gDPSetCombineLERP(OVERLAY_DISP++, PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0, PRIMITIVE,
                      ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0);
    gDPLoadTextureBlock_4b(OVERLAY_DISP++, (void*)gNaviCUpENGTex, G_IM_FMT_IA, 32, 8, 0, G_TX_NOMIRROR | G_TX_WRAP,
                           G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
    gSPWideTextureRectangle(OVERLAY_DISP++, (cUpLeftX - naviLabelXOffset) << 2, (cUpLeftY + naviLabelYOffset) << 2,
                            (cUpLeftX - naviLabelXOffset + 32) << 2, (cUpLeftY + naviLabelYOffset + 8) << 2,
                            G_TX_RENDERTILE, (31 << 5), 0, -(1 << 10), 1 << 10);

    gDPPipeSync(OVERLAY_DISP++);

    CLOSE_DISPS(play->state.gfxCtx);
}

static void RegisterIvanSeparateLoadout() {
    bool enabled = CVAR_IVAN_COOP_MODE_VALUE != 0 && CVAR_IVAN_SEPARATE_LOADOUT_VALUE != 0;
    COND_HOOK(OnKaleidoUpdate, enabled, OnKaleidoUpdate);
    COND_HOOK(OnInterfaceDraw, enabled, OnInterfaceDraw);
}

static RegisterShipInitFunc initFunc(RegisterIvanSeparateLoadout,
                                     { CVAR_IVAN_COOP_MODE_NAME, CVAR_IVAN_SEPARATE_LOADOUT_NAME });
