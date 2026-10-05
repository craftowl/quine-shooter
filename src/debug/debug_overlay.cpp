#include "debug/debug_overlay.h"

#ifdef DEBUG

#include "imgui.h"
#include "raylib.h"
#include "rlImGui.h"

#include "debug/allocation_counter.h"
#include "debug/enemy_text_list.h"

namespace stg::debug {

void init_overlay() {
    rlImGuiSetup(true);
    // imgui.ini を書き出さない（作業ディレクトリにファイルを作らない）
    ImGui::GetIO().IniFilename = nullptr;
}

void shutdown_overlay() {
    rlImGuiShutdown();
}

void draw_overlay(const char* reload_status) {
    rlImGuiBegin();
    ImGui::SetNextWindowPos(ImVec2(8.0f, 8.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("FPS: %d", GetFPS());
    ImGui::Text("Frame: %.2f ms", GetFrameTime() * 1000.0f);
    // 前のフレームの確保回数と、起動後の違反の累計（ADR 0007）
    ImGui::Text("Alloc/frame: %d", last_frame_allocations());
    ImGui::Text("Alloc violations: %d", allocation_violations());
    // 調整値のホットリロードの最後の結果（tasks.md の T12）。失敗の理由は長いので、折り返す
    if (reload_status[0] != '\0') {
        ImGui::PushTextWrapPos(320.0f);
        ImGui::TextUnformatted("Params:");
        ImGui::TextUnformatted(reload_status);
        ImGui::PopTextWrapPos();
    }
    ImGui::End();
    // 埋め込んだ敵の体のテキスト（tasks.md の T15b）
    draw_enemy_text_list();
    rlImGuiEnd();
}

}  // namespace stg::debug

#endif  // DEBUG
