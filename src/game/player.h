#pragma once

#include "raylib.h"

#include "core/input.h"
#include "game/params.h"

namespace stg {

struct Player {
    Vector2 position{};  // 当たり判定の中心。開始位置は調整値（params.player.start_position）から
};

// 入力に合わせて dt 秒ぶん、調整値の速さで動かし、位置を movable_area の中に収める。
// 斜めも縦横と同じ速さにする。反対のキーを同時に押したら、その軸は動かない
void update_player(Player& player, const InputState& input, const PlayerParams& params, const Rectangle& movable_area,
                   float dt);

}  // namespace stg
