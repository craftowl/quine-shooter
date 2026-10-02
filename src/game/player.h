#pragma once

#include "raylib.h"

#include "core/input.h"

namespace stg {

// 自機の開始位置（当たり判定の中心。480×640 の座標）。復活の位置も同じ（requirements.md §1）。
// 仮の定数で、T12 で JSON へ移す
inline constexpr Vector2 PLAYER_START_POSITION = {240.0f, 560.0f};

struct Player {
    Vector2 position = PLAYER_START_POSITION;  // 当たり判定の中心
};

// 入力に合わせて dt 秒ぶん動かし、位置を movable_area の中に収める。
// 斜めも縦横と同じ速さにする。反対のキーを同時に押したら、その軸は動かない
void update_player(Player& player, const InputState& input, const Rectangle& movable_area, float dt);

}  // namespace stg
