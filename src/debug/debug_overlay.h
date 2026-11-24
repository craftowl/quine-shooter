#pragma once

#ifdef DEBUG

#include <cstddef>

namespace stg::debug {

// デバッグ表示に出すオブジェクトの数（tasks.md T14 の決定11）。敵や敵弾は、作るタスクで足す
struct ObjectCounts {
    std::size_t player_bullets = 0;
    std::size_t player_bullet_capacity = 0;
    std::size_t player_bullets_dropped = 0;  // 起動してから、満杯で撃てなかった回数の累計
    // 敵のプールにいる数と容量（tasks.md T16 の決定14）
    std::size_t zako = 0;
    std::size_t zako_capacity = 0;
    std::size_t boss = 0;
    std::size_t boss_capacity = 0;
};

// rlImGui を初期化する。InitWindow の後に1回呼ぶ
void init_overlay();

// rlImGui を片付ける。CloseWindow の前に1回呼ぶ
void shutdown_overlay();

// FPS、1フレームの時間、確保の回数、オブジェクトの数、調整値の最後の読み直しの結果（reload_status。空なら出さない）を
// ImGui で描く。render() の中、EndDrawing の前に呼ぶ
void draw_overlay(const char* reload_status, const ObjectCounts& counts);

}  // namespace stg::debug

#endif  // DEBUG
