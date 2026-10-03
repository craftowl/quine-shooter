#include "game/player.h"

#include <algorithm>
#include <cassert>

namespace stg {
namespace {

// 斜めに動くときに、縦と横に掛ける値（1/√2）
constexpr float DIAGONAL_FACTOR = 0.70710678f;

// 負の向きのキーと正の向きのキーから、軸の向き（-1、0、1）を決める。両方押したら 0
float axis(bool negative, bool positive) {
    return (positive ? 1.0f : 0.0f) - (negative ? 1.0f : 0.0f);
}

}  // namespace

void update_player(Player& player, const InputState& input, const PlayerParams& params, const Rectangle& movable_area,
                   float dt) {
    assert(movable_area.width >= 0.0f && movable_area.height >= 0.0f);
    float dx = axis(input.left, input.right);
    float dy = axis(input.up, input.down);
    if (dx != 0.0f && dy != 0.0f) {
        dx *= DIAGONAL_FACTOR;
        dy *= DIAGONAL_FACTOR;
    }
    const float distance = (input.slow ? params.slow_speed : params.normal_speed) * dt;
    player.position.x = std::clamp(player.position.x + dx * distance, movable_area.x, movable_area.x + movable_area.width);
    player.position.y = std::clamp(player.position.y + dy * distance, movable_area.y, movable_area.y + movable_area.height);
}

}  // namespace stg
