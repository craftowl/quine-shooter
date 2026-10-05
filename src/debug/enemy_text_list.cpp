#include "debug/enemy_text_list.h"

#ifdef DEBUG

#include <cstddef>

#include "enemies/enemy_text.h"
#include "imgui.h"

namespace stg::debug {

void draw_enemy_text_list() {
    ImGui::SetNextWindowPos(ImVec2(8.0f, 160.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Enemy texts", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    for (const EnemyText& enemy : all_enemy_texts()) {
        ImGui::Text("%.*s (%dx%d)", static_cast<int>(enemy.id.size()), enemy.id.data(), enemy.columns, enemy.rows);
        // 行ごとに、テキストとマスクを並べる。文字列は作らず、埋め込んだデータの範囲をそのまま渡す（ADR 0007）
        for (int row = 0; row < enemy.rows; ++row) {
            const std::size_t start = static_cast<std::size_t>(row) * static_cast<std::size_t>(enemy.columns);
            const char* text = enemy.text.data() + start;
            const char* mask = enemy.mask.data() + start;
            ImGui::TextUnformatted(text, text + enemy.columns);
            ImGui::SameLine();
            ImGui::TextUnformatted(mask, mask + enemy.columns);
        }
        ImGui::Separator();
    }
    ImGui::End();
}

}  // namespace stg::debug

#endif  // DEBUG
