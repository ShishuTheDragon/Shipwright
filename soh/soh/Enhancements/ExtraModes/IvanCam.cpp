#include "soh/Enhancements/ExtraModes/IvanCam.h"

#include "fast/Fast3dWindow.h"
#include "ship/Context.h"
#include "soh/framebuffer_effects.h"

#include <imgui.h>
#include <memory>

void IvanCamWindow::InitElement() {
}

void IvanCamWindow::UpdateElement() {
}

void IvanCamWindow::DrawElement() {
    Fast::Fast3dWindow* window = (Fast::Fast3dWindow*)(&*Ship::Context::GetInstance()->GetWindow());

    if (gIvanFrameBuffer < 0) {
        ImGui::TextUnformatted("Ivan framebuffer not initialized.");
        return;
    }

    uintptr_t fb = window->GetFramebufferTextureId(gIvanFrameBuffer);
    if (!fb) {
        ImGui::TextUnformatted("Ivan framebuffer unavailable.");
        return;
    }

    ImVec2 avail = ImGui::GetContentRegionAvail();

    float uv_u0 = 0.0f, uv_u1 = 1.0f, uv_v1 = 1.0f;
    ImVec2 display_size = avail;
    if (avail.x > 0 && avail.y > 0) {
        float n64h_raw = (float)SCREEN_WIDTH * avail.y / avail.x;
        if (n64h_raw <= (float)SCREEN_HEIGHT) {
            // Wide panel: clip rows to match aspect ratio.
            uint32_t n64h = (uint32_t)(n64h_raw + 0.5f);
            gIvanViewportN64Height = n64h;
            uv_v1 = (float)n64h / SCREEN_HEIGHT;
        } else {
            // Portrait panel: full rows, center horizontal crop to fill panel.
            gIvanViewportN64Height = SCREEN_HEIGHT;
            float uv_u_width = (avail.x / avail.y) * ((float)SCREEN_HEIGHT / SCREEN_WIDTH);
            uv_u0 = 0.5f - uv_u_width * 0.5f;
            uv_u1 = 0.5f + uv_u_width * 0.5f;
        }
    }

    ImGui::Image(reinterpret_cast<ImTextureID>(fb), display_size, ImVec2(uv_u0, 0.0f), ImVec2(uv_u1, uv_v1));
}
