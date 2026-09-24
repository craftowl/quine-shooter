#pragma once

#ifdef DEBUG

namespace stg::debug {

// rlImGui を初期化する。InitWindow の後に1回呼ぶ
void init_overlay();

// rlImGui を片付ける。CloseWindow の前に1回呼ぶ
void shutdown_overlay();

// FPS と1フレームの時間を ImGui で描く。render() の中、EndDrawing の前に呼ぶ
void draw_overlay();

}  // namespace stg::debug

#endif  // DEBUG
