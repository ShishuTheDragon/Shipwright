# Ivan (EnPartner) viewpoint render in ImGui — notes

## Goal
Render Ivan (EnPartner) viewpoint into an ImGui window so P2 can see their own view. Use a secondary framebuffer and a custom View; then display its texture in ImGui.

## Key locations
- Main render path: `soh/src/code/z_play.c` `Play_Draw`
- View helpers: `soh/src/code/z_view.c` (`func_800AB9EC`, `func_800AB944`, `View_Init`, etc.)
- Framebuffer utilities: `soh/soh/framebuffer_effects.c` / `.h`
- Pause Link example: `soh/src/overlays/misc/ovl_kaleido_scope/z_kaleido_equipment.c` uses `gsSPSetFB` / `gsSPResetFB`
- ImGui game draw: `libultraship/src/ship/window/gui/Gui.cpp` uses `GetGfxFrameBuffer()`
- Fast renderer: `libultraship/src/fast/interpreter.cpp` has `CreateFrameBuffer`, `mRapi->GetFramebufferTextureId`
- Ivan actor: `soh/src/overlays/actors/ovl_En_Partner/z_en_partner.c`, header `soh/src/overlays/actors/ovl_En_Partner/z_en_partner.h`
- Actor ID: `soh/soh/ActorDB.cpp` registers EnPartner as `ACTORCAT_ITEMACTION` (id in `gEnPartnerId`)

## Plan (high-level)
1) Add a new framebuffer ID (e.g. `gIvanFrameBuffer`) in `soh/soh/framebuffer_effects.h/.c` and initialize in `FB_CreateFramebuffers()`.
2) In `Play_Draw`, add a helper that renders the world using a supplied `View` + target framebuffer. Call it once for main view and once for Ivan view.
3) Build Ivan camera:
   - Find Ivan actor: `Actor_Find(&play->actorCtx, gEnPartnerId, ACTORCAT_ITEMACTION)`
   - Eye = `ivan->actor.world.pos`
   - LookAt = eye + forward vector from `ivan->actor.world.rot.y` (or use camera-based direction if desired)
   - Use a temporary `View` (do not mutate `play->view`), call `View_Init`, `func_800AA358`, `func_800AA460`, `func_800AAA50`, and `func_800AB9EC`.
4) Expose framebuffer texture to ImGui:
   - Add `Interpreter::GetFramebufferTextureId(int fb)` that returns `mRapi->GetFramebufferTextureId(fb)`.
   - Add window-level accessor in `Fast3dWindow` (e.g. `GetFramebufferTextureId(int fb)`)
5) Add ImGui window (new GUI class) and show the FB texture using `ImGui::Image` similar to `Gui.cpp`.

## Suggested rendering helper outline
- `RenderWorldToFramebuffer(PlayState* play, View* view, int fb)`
- Use `gsSPSetFB(WORK_DISP++, fb)` before world draw and `gsSPResetFB(WORK_DISP++)` after.
- Reuse the same world draw path (skybox/rooms/actors) from `Play_Draw` by refactoring into helper.

## Implementation notes
- Follow the pause Link draw example for how to set temporary FB.
- Avoid mutating `play->view` when rendering Ivan; use a local `View`.
- Consider smaller FB dimensions for performance.
- Consider CVar toggle to show/hide Ivan viewport window.

## Example view setup snippet
```
View ivanView;
View_Init(&ivanView, gfxCtx);
SET_FULLSCREEN_VIEWPORT(&ivanView);
Vec3f eye = ivan->actor.world.pos;
Vec3f lookAt = {
    eye.x + Math_SinS(ivan->actor.world.rot.y) * 100.0f,
    eye.y + 10.0f,
    eye.z + Math_CosS(ivan->actor.world.rot.y) * 100.0f
};
Vec3f up = {0.0f, 1.0f, 0.0f};
func_800AA358(&ivanView, &eye, &lookAt, &up);
func_800AA460(&ivanView, play->view.fovy, play->view.zNear, play->lightCtx.fogFar);
func_800AAA50(&ivanView, 15);
```

## Files touched (anticipated)
- `soh/soh/framebuffer_effects.h`
- `soh/soh/framebuffer_effects.c`
- `soh/src/code/z_play.c`
- `libultraship/src/fast/interpreter.cpp`
- `libultraship/src/fast/Fast3dWindow.cpp`
- `libultraship/include/fast/Fast3dWindow.h`
- New ImGui window in `soh/soh/SohGui` (plus registration in `SohGui.cpp`)

## Next Steps
- [x] Add a new framebuffer ID and allocation for Ivan in `soh/soh/framebuffer_effects.h` and `soh/soh/framebuffer_effects.c` (initialize in `FB_CreateFramebuffers()`).
- [x] Decide ImGui integration details: no toggle or MSAA; window name "Ivan Cam"; default size 320x240; texture source is the Ivan framebuffer; implementation lives in `soh/soh/Enhancements/ExtraModes/IvanCam.cpp`.
- [x] Create and register an ImGui window in `soh/soh/Enhancements/ExtraModes/IvanCam.cpp`
- [x] Display the Ivan framebuffer texture.
- [x] Add framebuffer texture accessors (`Interpreter::GetFramebufferTextureId(int fb)` and `Fast3dWindow::GetFramebufferTextureId(int fb)`).
- [ ] Refactor `Play_Draw` in `soh/src/code/z_play.c` to render the world via a helper that accepts a `View*` + target framebuffer; call it for main view and Ivan view.
- [ ] Build Ivan’s camera using a temporary `View` (find Ivan actor, compute eye/lookAt/up, call `View_Init` and the view setup helpers).
