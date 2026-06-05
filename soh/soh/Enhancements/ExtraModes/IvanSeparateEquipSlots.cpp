#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/ShipInit.hpp"
#include "soh/OTRGlobals.h"
#include "soh/framebuffer_effects.h"

#include "textures/parameter_static/parameter_static.h"
#include "soh_assets.h"
#include "functions.h"
#include "macros.h"
#include "variables.h"

extern "C" {
extern PlayState* gPlayState;
extern GameInfo* gGameInfo;
extern void* gItemIcons[158];
extern const char* _gAmmoDigit0Tex[];
Gfx* Gfx_TextureIA8(Gfx* displayListHead, void* texture, s16 textureWidth, s16 textureHeight, s16 rectLeft, s16 rectTop,
                    s16 rectWidth, s16 rectHeight, u16 dsdx, u16 dtdy);
}

static const s16 cButtonSize = 16;
static const s16 cButtonTexStep = 512 * 32 / cButtonSize;
static const s16 itemIconSize = 16;
static const s16 itemIconTexStep = 512 * 32 / itemIconSize;
static const s16 itemSpacing = 16;
static const s16 dPadSize = 32;
static const s16 dPadTexStep = 512;
static const s16 dPadCenterX = 80;
static const s16 cButtonsCenterX = 240;
static const s16 dPadCenterY = 200;
static const s16 cButtonsCenterY = 200;
static const s16 naviLabelYOffset = 4;
static const s16 naviLabelXOffset = 8;

enum class IvanItemIndex : u8 {
    CLeft = 0,
    CDown = 1,
    CRight = 2,
    ZL = 3,
    ZR = 4,
    CUp = 5,
    DPadUp = 6,
    DPadDown = 7,
    DPadLeft = 8,
    DPadRight = 9,
};

static Gfx* Gfx_WideTextureRectCentered(Gfx* displayListHead, Vec3s center, s16 size, s16 texStep) {
    s32 x = center.x - (size / 2);
    s32 y = center.y - (size / 2);

    gSPWideTextureRectangle(displayListHead++, x << 2, y << 2,
                            (x + size) << 2,
                            (y + size) << 2, G_TX_RENDERTILE, 0, 0,
                            texStep << 1, texStep << 1);
    return displayListHead;
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

void OnKaleidoUpdate() {
    auto play = gPlayState;

    if (play->pauseCtx.state == 6 && play->pauseCtx.pageIndex == PAUSE_ITEM) {
        auto cursorItem = play->pauseCtx.cursorItem[PAUSE_ITEM];

        if (cursorItem != PAUSE_ITEM_NONE) {
            u32 ivanButtons = play->state.input[1].press.button;
            if (cursorItem != ITEM_SOLD_OUT && cursorItem != ITEM_NONE) {
                struct IvanEquipButton {
                    u16 button;
                    IvanItemIndex slot;
                };
                static const IvanEquipButton kEquipButtons[] = {
                    { BTN_CLEFT, IvanItemIndex::CLeft },
                    { BTN_CDOWN, IvanItemIndex::CDown },
                    { BTN_CRIGHT, IvanItemIndex::CRight },
                    { BTN_ZL, IvanItemIndex::ZL },
                    { BTN_ZR, IvanItemIndex::ZR },
                    { BTN_CUP, IvanItemIndex::CUp },
                    { BTN_DUP, IvanItemIndex::DPadUp },
                    { BTN_DDOWN, IvanItemIndex::DPadDown },
                    { BTN_DLEFT, IvanItemIndex::DPadLeft },
                    { BTN_DRIGHT, IvanItemIndex::DPadRight },
                };

                for (size_t i = 0; i < ARRAY_COUNT(kEquipButtons); i++) {
                    if (CHECK_BTN_ALL(ivanButtons, kEquipButtons[i].button)) {
                        gSaveContext.ship.ivanItems[(u8)kEquipButtons[i].slot] = cursorItem;
                        Audio_PlaySoundGeneral(NA_SE_SY_DECIDE, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale,
                                               &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
                        break;
                    }
                }
            }
        }
    }
}

extern "C" void Ivan_DrawInventory() {
    if (gIvanFrameBuffer < 0) {
        return;
    }

    auto play = gPlayState;
    auto interfaceCtx = &gPlayState->interfaceCtx;

    OPEN_DISPS(play->state.gfxCtx);

    gsSPSetFBNoClearDepth(OVERLAY_DISP++, gIvanFrameBuffer);
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
    gDPSetPrimColor(OVERLAY_DISP++, 0, 0, tint.r, tint.g, tint.b, 255);

    Vec3s centers[4] = {
        { (s16)(cButtonsCenterX - itemSpacing), cButtonsCenterY, 0 }, // C-Left
        { cButtonsCenterX, (s16)(cButtonsCenterY + itemSpacing), 0 }, // C-Down
        { (s16)(cButtonsCenterX + itemSpacing), cButtonsCenterY, 0 }, // C-Right
        { cButtonsCenterX, (s16)(cButtonsCenterY - itemSpacing), 0 }, // C-Up
    };

    gDPLoadTextureBlock(OVERLAY_DISP++, gButtonBackgroundTex, G_IM_FMT_IA, G_IM_SIZ_8b, 32, 32, 0,
                    G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP,
                    G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
    for (int i = 0; i < 4; i++)
        OVERLAY_DISP = Gfx_WideTextureRectCentered(OVERLAY_DISP, centers[i], cButtonSize, cButtonTexStep);

    Vec3s dpadCenters[4] = {
        { dPadCenterX, (s16)(dPadCenterY - itemSpacing), 0 }, // Dpad-Up
        { dPadCenterX, (s16)(dPadCenterY + itemSpacing), 0 }, // Dpad-Down
        { (s16)(dPadCenterX - itemSpacing), dPadCenterY, 0 }, // Dpad-Left
        { (s16)(dPadCenterX + itemSpacing), dPadCenterY, 0 }, // Dpad-Right
    };

    gDPLoadTextureBlock(OVERLAY_DISP++, gDPadTex, G_IM_FMT_IA, G_IM_SIZ_16b, dPadSize, dPadSize, 0,
                        G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP,
                        G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
    OVERLAY_DISP = Gfx_WideTextureRectCentered(OVERLAY_DISP, { dPadCenterX, dPadCenterY, 0 }, dPadSize, dPadTexStep);

    for (size_t i = 0; i < 4; i++) {
        auto item = gSaveContext.ship.ivanItems[(u8)IvanItemIndex::DPadUp + i];
        if (item == ITEM_NONE) {
            continue;
        }

        gDPPipeSync(OVERLAY_DISP++);
        void* texture = gItemIcons[item];
        gDPLoadTextureBlock(OVERLAY_DISP++, texture, G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, 0,
                            G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP,
                            G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
        gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 255, 255, 255);
        OVERLAY_DISP = Gfx_WideTextureRectCentered(OVERLAY_DISP, dpadCenters[i], itemIconSize, itemIconTexStep);
    }

    for (size_t i = 0; i < 3; i++) {
        auto item = gSaveContext.ship.ivanItems[i];
        if (item == ITEM_NONE) {
            continue;
        }

        gDPPipeSync(OVERLAY_DISP++);
        void* texture = gItemIcons[item];
        gDPLoadTextureBlock(OVERLAY_DISP++, texture, G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, 0,
                            G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP,
                            G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
        gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 255, 255, 255);
        OVERLAY_DISP = Gfx_WideTextureRectCentered(OVERLAY_DISP, centers[i], itemIconSize, itemIconTexStep);
    }

    // C-Up item (index CUp=5): show at centers[3] (C-Up HUD position)
    {
        auto item = gSaveContext.ship.ivanItems[(u8)IvanItemIndex::CUp];
        if (item != ITEM_NONE) {
            gDPPipeSync(OVERLAY_DISP++);
            void* texture = gItemIcons[item];
            gDPLoadTextureBlock(OVERLAY_DISP++, texture, G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, 0,
                                G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP,
                                G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
            gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 255, 255, 255);
            OVERLAY_DISP = Gfx_WideTextureRectCentered(OVERLAY_DISP, centers[3], itemIconSize, itemIconTexStep);
        }
    }

    // ZL (index 3) and ZR (index 4): outside the D-pad/C-button clusters at the same height
    {
        Vec3s zlCenter = { (s16)(dPadCenterX - itemSpacing), itemSpacing / 2, 0 };
        Vec3s zrCenter = { (s16)(cButtonsCenterX + itemSpacing), itemSpacing / 2, 0 };
        Vec3s zlzrCenters[2] = { zlCenter, zrCenter };
        for (size_t i = 0; i < 2; i++) {
            auto item = gSaveContext.ship.ivanItems[(u8)IvanItemIndex::ZL + i];
            if (item == ITEM_NONE) {
                continue;
            }
            gDPPipeSync(OVERLAY_DISP++);
            void* texture = gItemIcons[item];
            gDPLoadTextureBlock(OVERLAY_DISP++, texture, G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, 0,
                                G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP,
                                G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
            gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 255, 255, 255);
            OVERLAY_DISP = Gfx_WideTextureRectCentered(OVERLAY_DISP, zlzrCenters[i], itemIconSize, itemIconTexStep);
        }
    }

    gDPPipeSync(OVERLAY_DISP++);
    gsSPResetFB(OVERLAY_DISP++);

    CLOSE_DISPS(play->state.gfxCtx);
}

void RegisterIvanSeparateEquipSlots() {
    COND_HOOK(OnKaleidoUpdate, true, OnKaleidoUpdate);
}

static RegisterShipInitFunc initFunc_Freezard(RegisterIvanSeparateEquipSlots);
