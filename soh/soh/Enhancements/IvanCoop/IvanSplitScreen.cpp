#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/ShipInit.hpp"
#include "soh/OTRGlobals.h"
#include "soh/Enhancements/ExtraModes/IvanCoop.h"

#include <cstring>

extern "C" {
#include "macros.h"
#include "variables.h"
#include "functions.h"
#include <overlays/actors/ovl_En_Partner/z_en_partner.h>
#include <overlays/actors/ovl_Object_Kankyo/z_object_kankyo.h>
extern PlayState* gPlayState;
void FrameInterpolation_RecordOpenChild(const void* a, int b);
void FrameInterpolation_RecordCloseChild(void);
int16_t OTRGetRectDimensionFromLeftEdge(float v);
int16_t OTRGetRectDimensionFromRightEdge(float v);
}

#define CVAR_NAME CVAR_ENHANCEMENT("IvanCoop.SplitScreen")
#define CVAR_IVAN_MODE CVAR_ENHANCEMENT("IvanCoopModeEnabled")

static bool IsEnabled() {
    return CVarGetInteger(CVAR_IVAN_MODE, 0) &&
           CVarGetInteger(CVAR_NAME, 0) == IVAN_SPLIT_SCREEN_METHOD2;
}

static Gfx ivan_opa[0x2FC0];
static Gfx ivan_xlu[0x1000];

static MtxF sIvanViewProjMtxF;

// Head of Link's XLU display list (just past Ivan's spliced-in stub). OnPlayDrawEnd scans
// from here to recenter the Lens of Truth overlay into Link's left half.
static Gfx* sLinkXluStart = NULL;

// Reserved gSPSegment slot at the head of Link's XLU stream that re-asserts Link's billboard
// matrix on segment 0x01. Ivan's spliced XLU sublist leaves seg 0x01 pointing at Ivan's
// billboard, so Link's XLU geometry would otherwise inherit it. Link's billboard matrix isn't
// built until later in Play_Draw, so the slot is reserved here and patched in OnPlayDrawEnd.
// (Vanilla never sets seg 0x01 on XLU because the OPA pass's value persists into XLU; the
// interleaved OPA/XLU sublists of the two cameras break that assumption.)
static Gfx* sLinkXluBillboardSeg = NULL;

// The snow effect (Object_Kankyo, params 3) runs its whole simulation inside its draw
// function and anchors every flake relative to play->view. Rendering the world twice would
// otherwise advance Link's flakes against Ivan's camera, flinging them onto the near plane
// (the giant stretched quad). Give Ivan a private snow state and swap it in around his pass.
static ObjectKankyoEffect sIvanSnowEffects[ARRAY_COUNT(((ObjectKankyo*)0)->effects)] = {};
static u8 sIvanSnowCount = 0;

static ObjectKankyo* FindSnowActor(PlayState* play) {
    Actor* actor = Actor_Find(&play->actorCtx, ACTOR_OBJECT_KANKYO, ACTORCAT_ITEMACTION);
    if (actor != NULL && actor->params == 3) {
        return (ObjectKankyo*)actor;
    }
    return NULL;
}

static void SwapSnowState(ObjectKankyo* snow, PlayState* play) {
    ObjectKankyoEffect tmpEffects[ARRAY_COUNT(snow->effects)];
    memcpy(tmpEffects, snow->effects, sizeof(tmpEffects));
    memcpy(snow->effects, sIvanSnowEffects, sizeof(snow->effects));
    memcpy(sIvanSnowEffects, tmpEffects, sizeof(sIvanSnowEffects));

    u8 tmpCount = play->envCtx.unk_EE[2];
    play->envCtx.unk_EE[2] = sIvanSnowCount;
    sIvanSnowCount = tmpCount;
}

static void SetIvansCameraAndViewport(EnPartner* ivan) {
    const f32 camDist = 90.0f;
    const f32 lookAtHeight = 40.0f;
    const f32 fovy = 60.0f;

    // Convenience variables:
    PlayState* play = gPlayState;

    // Read some (currently) global variables:
    Vec3f ivanPos = ivan->actor.world.pos;
    s16 yaw = (s16)gIvanCamYaw;
    s16 pitch = (s16)gIvanCamPitch;

    // Setup camera position and direction:
    play->view.up.x = 0.0f;
    play->view.up.y = 1.0f;
    play->view.up.z = 0.0f;
    play->view.eye.x = ivanPos.x - Math_SinS(yaw) * Math_CosS(pitch) * camDist;
    play->view.eye.y = ivanPos.y + lookAtHeight + Math_SinS(pitch) * camDist;
    play->view.eye.z = ivanPos.z - Math_CosS(yaw) * Math_CosS(pitch) * camDist;
    play->view.lookAt.x = ivanPos.x;
    play->view.lookAt.y = ivanPos.y + lookAtHeight;
    play->view.lookAt.z = ivanPos.z;
    play->view.fovy = fovy;

    // Apply collision to camera position:
    Vec3f ivanCamResult;
    CollisionPoly* ivanCamPoly = NULL;
    s32 ivanCamBgId = 0;
    if (BgCheck_CameraLineTest1(&play->colCtx, &play->view.lookAt, &play->view.eye, &ivanCamResult, &ivanCamPoly, 1, 1,
                                1, -1, &ivanCamBgId)) {
        play->view.eye.x = ivanCamResult.x + COLPOLY_GET_NORMAL(ivanCamPoly->normal.x);
        play->view.eye.y = ivanCamResult.y + COLPOLY_GET_NORMAL(ivanCamPoly->normal.y);
        play->view.eye.z = ivanCamResult.z + COLPOLY_GET_NORMAL(ivanCamPoly->normal.z);
    }

    // Set viewport to right half of screen:
    play->view.viewport.leftX = SCREEN_WIDTH / 2;
    play->view.viewport.rightX = SCREEN_WIDTH;
}

static void RenderEverything() {
    // Convenience variables:
    PlayState* play = gPlayState;
    GraphicsContext* gfxCtx = play->state.gfxCtx;

    // Emit fog for both display lists
    OPEN_DISPS(gfxCtx);
    POLY_OPA_DISP = Play_SetFog(play, POLY_OPA_DISP);
    POLY_XLU_DISP = Play_SetFog(play, POLY_XLU_DISP);
    CLOSE_DISPS(gfxCtx);

    // Compute Ivan's view/projection matrices and emit the viewport scissor
    func_800AA460(&play->view, play->view.fovy, play->view.zNear, play->lightCtx.fogFar);
    func_800AAA50(&play->view, 15);

    // MirroredWorld: flip projection and invert culling for Ivan's pass
    if (CVarGetInteger(CVAR_ENHANCEMENT("MirroredWorld"), 0)) {
        OPEN_DISPS(gfxCtx);
        gSPSetExtraGeometryMode(POLY_OPA_DISP++, G_EX_INVERT_CULLING);
        gSPSetExtraGeometryMode(POLY_XLU_DISP++, G_EX_INVERT_CULLING);
        gSPMatrix(POLY_OPA_DISP++, play->view.projectionFlippedPtr, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION);
        gSPMatrix(POLY_XLU_DISP++, play->view.projectionFlippedPtr, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION);
        gSPMatrix(POLY_OPA_DISP++, play->view.viewingPtr, G_MTX_NOPUSH | G_MTX_MUL | G_MTX_PROJECTION);
        gSPMatrix(POLY_XLU_DISP++, play->view.viewingPtr, G_MTX_NOPUSH | G_MTX_MUL | G_MTX_PROJECTION);
        CLOSE_DISPS(gfxCtx);
    }

    // Build billboard matrix and view-projection matrix (mirror z_play.c:1471-1496)
    Matrix_MtxToMtxF(&play->view.viewing, &play->billboardMtxF);
    Matrix_MtxToMtxF(&play->view.projection, &play->viewProjectionMtxF);
    Matrix_Mult(&play->viewProjectionMtxF, MTXMODE_NEW);
    Matrix_Mult(&play->billboardMtxF, MTXMODE_APPLY);
    Matrix_Get(&play->viewProjectionMtxF);

    // Widen frustum X row for half-width viewport so edge actors aren't culled
    play->viewProjectionMtxF.xx *= 0.5f;
    play->viewProjectionMtxF.xy *= 0.5f;
    play->viewProjectionMtxF.xz *= 0.5f;
    play->viewProjectionMtxF.xw *= 0.5f;

    play->billboardMtxF.mf[0][3] = play->billboardMtxF.mf[1][3] = play->billboardMtxF.mf[2][3] =
        play->billboardMtxF.mf[3][0] = play->billboardMtxF.mf[3][1] = play->billboardMtxF.mf[3][2] = 0.0f;
    Matrix_Transpose(&play->billboardMtxF);
    play->billboardMtx =
        Matrix_MtxFToMtx(MATRIX_CHECKFLOATS(&play->billboardMtxF), (Mtx*)Graph_Alloc(gfxCtx, sizeof(Mtx)));

    // Emit segment registers
    OPEN_DISPS(gfxCtx);
    gSPSegment(POLY_OPA_DISP++, 0x01, (uintptr_t)play->billboardMtx);
    gSPSegment(POLY_XLU_DISP++, 0x01, (uintptr_t)play->billboardMtx);
    gSPSegment(POLY_OPA_DISP++, 0x02, (uintptr_t)play->sceneSegment);
    gSPSegment(POLY_XLU_DISP++, 0x02, (uintptr_t)play->sceneSegment);
    CLOSE_DISPS(gfxCtx);

    // Skybox first call
    if (play->skyboxId && (play->skyboxId != SKYBOX_UNSET_1D) && !play->envCtx.skyboxDisabled) {
        if ((play->skyboxId == SKYBOX_NORMAL_SKY) || (play->skyboxId == SKYBOX_CUTSCENE_MAP)) {
            Environment_UpdateSkybox(play, play->skyboxId, &play->envCtx, &play->skyboxCtx);
            SkyboxDraw_Draw(&play->skyboxCtx, gfxCtx, play->skyboxId, play->envCtx.skyboxBlend,
                            play->view.eye.x, play->view.eye.y, play->view.eye.z);
        } else if (play->skyboxCtx.unk_140 == 0) {
            SkyboxDraw_Draw(&play->skyboxCtx, gfxCtx, play->skyboxId, 0,
                            play->view.eye.x, play->view.eye.y, play->view.eye.z);
        }
    }

    if (!play->envCtx.sunMoonDisabled) {
        Environment_DrawSunAndMoon(play);
    }

    Environment_DrawSkyboxFilters(play);

    // Draw-only — Environment_UpdateLightningStrike runs in Play_Update, not here
    Environment_DrawLightning(play, 0);

    // Set up lights then draw world geometry
    Lights* lights = LightContext_NewLights(&play->lightCtx, gfxCtx);
    Lights_BindAll(lights, play->lightCtx.listHead, NULL);
    Lights_Draw(lights, gfxCtx);

    Scene_Draw(play);
    Room_Draw(play, &play->roomCtx.curRoom, 3);
    Room_Draw(play, &play->roomCtx.prevRoom, 3);

    // Skybox second call (camera-quake path)
    if ((play->skyboxCtx.unk_140 != 0) && (GET_ACTIVE_CAM(play)->setting != CAM_SET_PREREND_FIXED)) {
        Vec3f quakeOffset;
        Camera_GetSkyboxOffset(&quakeOffset, GET_ACTIVE_CAM(play));
        SkyboxDraw_Draw(&play->skyboxCtx, gfxCtx, play->skyboxId, 0,
                        play->view.eye.x + quakeOffset.x, play->view.eye.y + quakeOffset.y,
                        play->view.eye.z + quakeOffset.z);
    }

    if (play->envCtx.unk_EE[1] != 0) {
        Environment_DrawRain(play, &play->view, gfxCtx);
    }

    Environment_FillScreen(gfxCtx, 0, 0, 0, play->unk_11E18, FILL_SCREEN_OPA);

    // Draw actors (Lens of Truth forced off for Ivan's pass).
    u8 savedLensActive = play->actorCtx.lensActive;
    play->actorCtx.lensActive = false;

    // Camera-billboarded draw functions (e.g. En_Light's flame) orient toward the active
    // camera's cached direction. Point it at Ivan's view for this pass so the flame faces
    // Ivan rather than Link, then restore Link's direction afterward.
    Camera* activeCam = GET_ACTIVE_CAM(play);
    Vec3s savedCamDir = activeCam->camDir;
    activeCam->camDir.y = (s16)gIvanCamYaw;
    activeCam->camDir.x = (s16)gIvanCamPitch;

    func_800315AC(play, &play->actorCtx);

    activeCam->camDir = savedCamDir;
    play->actorCtx.lensActive = savedLensActive;

    // Gameplay tints (skip MREG debug block per REFACTOR.md)
    switch (play->envCtx.fillScreen) {
        case 1:
            Environment_FillScreen(gfxCtx, play->envCtx.screenFillColor[0], play->envCtx.screenFillColor[1],
                                   play->envCtx.screenFillColor[2], play->envCtx.screenFillColor[3],
                                   FILL_SCREEN_OPA | FILL_SCREEN_XLU);
            break;
        default:
            break;
    }

    if (play->envCtx.sandstormState != SANDSTORM_OFF) {
        Environment_DrawSandstorm(play, play->envCtx.sandstormState);
    }
}

static bool did = false;

static void OnPlayDrawBegin() {
    did = false;
    sLinkXluBillboardSeg = NULL;

    EnPartner* ivan = GetIvanActor(gPlayState);
    if (ivan == NULL)
        return;

    PlayState* play = gPlayState;

    // The original game skips all world rendering when paused (R_PAUSE_MENU_MODE >= 3
    // jumps to overlay elements via goto). KaleidoScope draws full-screen, so Ivan's
    // half needs no world render during pause.
    // if ((play->pauseCtx.state != 0) || (play->pauseCtx.debugState != 0))
        // return;

    if (R_PAUSE_MENU_MODE == 2 || R_PAUSE_MENU_MODE == 3)
        return;

    s16 camSetting = GET_ACTIVE_CAM(play)->setting;
    if (camSetting == CAM_SET_PREREND_FIXED || camSetting == CAM_SET_PREREND_PIVOT)
        return;

    did = true;

    GraphicsContext* gfxCtx = play->state.gfxCtx;

    // Save the real GfxPool arenas and original camera/viewport:
    TwoHeadGfxArena savedOpa = gfxCtx->polyOpa;
    TwoHeadGfxArena savedXlu = gfxCtx->polyXlu;
    View savedView = play->view;
    MtxF linkViewProjectionMtxF = play->viewProjectionMtxF;

    // To avoid buffer overflow, replace the GfxPool arenas with our own separate buffers:
    THGA_Ct(&gfxCtx->polyOpa, ivan_opa, sizeof(ivan_opa));
    THGA_Ct(&gfxCtx->polyXlu, ivan_xlu, sizeof(ivan_xlu));

    // Render the world from Ivan’s view, with Ivan's private snow state swapped in:
    ObjectKankyo* snow = FindSnowActor(play);
    if (snow != NULL) {
        SwapSnowState(snow, play);
    }

    SetIvansCameraAndViewport(ivan);
    RenderEverything();

    if (snow != NULL) {
        SwapSnowState(snow, play);
    }

    // Capture the live widened Ivan view-projection so OnPlayDrawEnd can re-project
    // actors and OR-in Ivan's cull verdict. Same matrix func_800315AC used at line 171.
    sIvanViewProjMtxF = play->viewProjectionMtxF;

    // Our own separate display lists will “called” from the real display list, so
    // we need to add the “return” statement:
    OPEN_DISPS(gfxCtx);
    gSPEndDisplayList(POLY_OPA_DISP++);
    gSPEndDisplayList(POLY_XLU_DISP++);
    CLOSE_DISPS(gfxCtx);

    // Check for overflow while still pointing at the Ivan buffers
    // TODO: do something if crashed (e.g. skip rendering)
    if (THGA_IsCrash(&gfxCtx->polyOpa)) {
        SPDLOG_ERROR("IvanSplitScreen: ivan_opa buffer overflow");
    }
    if (THGA_IsCrash(&gfxCtx->polyXlu)) {
        SPDLOG_ERROR("IvanSplitScreen: ivan_xlu buffer overflow");
    }

    // Restore the real GfxPool arenas and original camera/viewport:
    gfxCtx->polyOpa = savedOpa;
    gfxCtx->polyXlu = savedXlu;
    play->view = savedView;
    play->viewProjectionMtxF = linkViewProjectionMtxF;

    // “Call” our separate display lists:
    OPEN_DISPS(gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, ivan_opa);
    gSPDisplayList(POLY_XLU_DISP++, ivan_xlu);
    // Reserve a slot to re-assert Link's billboard on seg 0x01 (patched in OnPlayDrawEnd once
    // Link's billboard matrix has been built). The placeholder is never executed: the display
    // list isn't processed until after both hooks have run this frame.
    sLinkXluBillboardSeg = POLY_XLU_DISP;
    gSPSegment(POLY_XLU_DISP++, 0x01, 0);
    CLOSE_DISPS(gfxCtx);

    // Everything Link's Play_Draw appends to POLY_XLU after this point starts here.
    sLinkXluStart = gfxCtx->polyXlu.p;

    // Lastly, before closing off, set Link’s viewport to left half of screen:
    play->view.viewport.rightX = SCREEN_WIDTH / 2;
}

// The Lens of Truth overlay (Actor_DrawLensOverlay, z_actor.c) is a 2D screen-space texture
// rectangle baked around SCREEN_WIDTH/2; texrects ignore the viewport/projection, so it lands
// at screen center straddling the split instead of inside Link's half. We can't change the
// decomp emit, so we translate the already-emitted G_TEXRECT_WIDE commands in Link's XLU stream
// to recenter the circle in Link's left half. Pure translation (no X scaling, dsdx untouched)
// keeps the lens a true circle.
static void RecenterLensOverlay() {
    if (sLinkXluStart == NULL)
        return;

    GraphicsContext* gfxCtx = gPlayState->state.gfxCtx;
    Gfx* end = gfxCtx->polyXlu.p;

    // Move the circle's center from SCREEN_WIDTH/2 to the center of Link's visible half
    // [leftEdge, SCREEN_WIDTH/2]. leftEdge is in game coords (negative under widescreen);
    // in encoded (<<2) rect units the shift reduces to a clean integer.
    s16 leftEdge = OTRGetRectDimensionFromLeftEdge(0);
    s32 dx = 2 * (s32)leftEdge - SCREEN_WIDTH;

    for (Gfx* g = sLinkXluStart; g < end; g++) {
        if (((g->words.w0 >> 24) & 0xFF) != (u32)G_TEXRECT_WIDE)
            continue;

        // A wide texrect is 3 consecutive Gfx:
        //   [0] w0 = (op<<24)   | xh,   w1 = yh
        //   [1] w0 = (tile<<24) | xl,   w1 = yl
        //   [2] w0 = (s<<16)|t,         w1 = (dsdx<<16)|dtdy
        // The lens emits the only full-height (yl=0, yh=SCREEN_HEIGHT<<2) wide rects here.
        Gfx* g0 = g;
        Gfx* g1 = g + 1;
        if (g1 >= end || (g0->words.w1 & 0x00FFFFFF) != (u32)(SCREEN_HEIGHT << 2) ||
            (g1->words.w1 & 0x00FFFFFF) != 0)
            continue;

        // Sign-extend the 24-bit X fields, translate, write back preserving op/tile bits.
        s32 xh = (s32)(g0->words.w0 & 0x00FFFFFF);
        s32 xl = (s32)(g1->words.w0 & 0x00FFFFFF);
        if (xh & 0x00800000) xh -= 0x01000000;
        if (xl & 0x00800000) xl -= 0x01000000;
        g0->words.w0 = (g0->words.w0 & ~(uintptr_t)0x00FFFFFF) | ((uintptr_t)(xh + dx) & 0x00FFFFFF);
        g1->words.w0 = (g1->words.w0 & ~(uintptr_t)0x00FFFFFF) | ((uintptr_t)(xl + dx) & 0x00FFFFFF);

        g += 2; // skip the remaining two Gfx of this rect
    }
}

static void OnPlayDrawEnd() {
    if (!did)
        return;

    // Patch the reserved slot now that Play_Draw has built Link's billboard matrix. This is the
    // exact same Mtx that Link's OPA pass uses for seg 0x01, so Link's XLU billboards match.
    if (sLinkXluBillboardSeg != NULL) {
        Gfx* gfxP = sLinkXluBillboardSeg;
        gSPSegment(gfxP, 0x01, (uintptr_t)gPlayState->billboardMtx);
    }

    RecenterLensOverlay();

    gPlayState->view.viewport.rightX = SCREEN_WIDTH;

    // Encode Link's half-viewport X remap into viewProjectionMtxF so func_8002C124
    // places the Z-target lock-on triangles in the left half rather than screen center.
    // Replicates Method 1's: spBC.x = spBC.x / 2 - viewportWidth / 2.
    // viewportWidth accounts for widescreen (leftEdge goes negative beyond 4:3 bounds).
    f32 leftEdge = (f32)OTRGetRectDimensionFromLeftEdge(0);
    f32 viewportWidth = (f32)(SCREEN_WIDTH / 2) - leftEdge;
    f32 xShift = viewportWidth / (f32)SCREEN_WIDTH;
    MtxF* m = &gPlayState->viewProjectionMtxF;
    m->xx = m->xx * 0.5f - xShift * m->wx;
    m->xy = m->xy * 0.5f - xShift * m->wy;
    m->xz = m->xz * 0.5f - xShift * m->wz;
    m->xw = m->xw * 0.5f - xShift * m->ww;

    // Union update-culling: re-project each actor through Ivan's captured matrix and OR-in
    // its cull verdict so actors visible only in Ivan's half keep updating. func_800315AC
    // already set/cleared the flag from Link's frustum this frame; we never clear here.
    // Ship_CalcShouldDrawAndUpdate runs the vanilla cull first, so it covers both CVar paths.
    ActorContext* actorCtx = &gPlayState->actorCtx;
    for (s32 i = 0; i < ARRAY_COUNT(actorCtx->actorLists); i++) {
        for (Actor* actor = actorCtx->actorLists[i].head; actor != NULL; actor = actor->next) {
            Vec3f projPos;
            f32 projW;
            SkinMatrix_Vec3fMtxFMultXYZW(&sIvanViewProjMtxF, &actor->world.pos, &projPos, &projW);

            bool shouldDraw = false;
            bool shouldUpdate = false;
            Ship_CalcShouldDrawAndUpdate(gPlayState, actor, &projPos, projW, &shouldDraw, &shouldUpdate);
            if (shouldUpdate)
                actor->flags |= ACTOR_FLAG_INSIDE_CULLING_VOLUME;

            // Per-viewport audio loudness: func_800315AC registered each actor's SFX against
            // &actor->projectedPos (the audio system holds the pointer and reads it on later
            // frames). projectedPos currently holds Link's projection; if Ivan's viewport hears
            // the actor louder (closer in projected space), swap in Ivan's so the SFX pans/volumes
            // toward whichever half is nearer. Mirrors Method 1's z_actor.c:3109-3114.
            f32 linkDistSq = SQ(actor->projectedPos.x) + SQ(actor->projectedPos.y) + SQ(actor->projectedPos.z);
            f32 ivanDistSq = SQ(projPos.x) + SQ(projPos.y) + SQ(projPos.z);
            if (ivanDistSq < linkDistSq) {
                actor->projectedPos = projPos;
                actor->projectedW = projW;
            }
        }
    }
}

static void RegisterIvanSplitScreen() {
    COND_HOOK(OnPlayDrawBegin, IsEnabled(), OnPlayDrawBegin);
    COND_HOOK(OnPlayDrawEnd, IsEnabled(), OnPlayDrawEnd);
}

static RegisterShipInitFunc initFunc(RegisterIvanSplitScreen, { CVAR_IVAN_MODE, CVAR_NAME });
