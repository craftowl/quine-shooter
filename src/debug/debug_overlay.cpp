#include "debug/debug_overlay.h"

#ifdef DEBUG

#include "imgui.h"
#include "raylib.h"
#include "rlImGui.h"

namespace stg::debug {

void init_overlay() {
    rlImGuiSetup(true);
    // imgui.ini を書き出さない（作業ディレクトリにファイルを作らない）
    ImGui::GetIO().IniFilename = nullptr;
}

void shutdown_overlay() {
    rlImGuiShutdown();
}

void draw_overlay() {
    rlImGuiBegin();
    ImGui::SetNextWindowPos(ImVec2(8.0f, 8.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("FPS: %d", GetFPS());
    ImGui::Text("Frame: %.2f ms", GetFrameTime() * 1000.0f);
    ImGui::End();
    rlImGuiEnd();
}

}  // namespace stg::debug

#endif  // DEBUG
