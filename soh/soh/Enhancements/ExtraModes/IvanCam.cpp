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
    ImGui::Image(reinterpret_cast<ImTextureID>(fb), avail);
}
