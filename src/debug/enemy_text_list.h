#pragma once

#ifdef DEBUG

namespace stg::debug {

// ImGui のウィンドウに、埋め込んだ敵をすべて出す（ID、大きさ、テキストとマスクを行ごとに並べて）。
// draw_overlay の中（rlImGuiBegin と rlImGuiEnd の間）で呼ぶ
void draw_enemy_text_list();

}  // namespace stg::debug

#endif  // DEBUG
