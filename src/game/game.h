#pragma once

#include <cstddef>

#include "raylib.h"

#include "core/input.h"
#include "game/bullet_pool.h"
#include "game/params.h"
#include "game/player.h"
#include "render/text_renderer.h"

#ifdef DEBUG
#include "debug/debug_overlay.h"
#endif

namespace stg {

// 自機の弾のプールの容量（tasks.md T14 の決定4）
inline constexpr std::size_t PLAYER_BULLET_CAPACITY = 256;

// 何回の update() ごとに撃つか（tasks.md T14 の決定7）。interval ÷ dt を、いちばん近い整数に丸める（最小 1）。
// interval と dt は 0 より大きい
[[nodiscard]] int shot_interval_updates(float interval, float dt);

// ゲームの状態と、その更新と描画（tasks.md T12a）
class Game {
public:
    // 文字の大きさから自機の動ける範囲と、弾を撃つ位置と消す範囲を決め、開始位置に置く。text は読み込み済み
    void start(const render::TextRenderer& text, const Params& params);
    void update(const InputState& input, const Params& params, float dt);
    // text.begin() と text.end() の間で呼ぶ（begin と end は main.cpp が呼ぶ。どちらも const ではないため）
    void draw(const render::TextRenderer& text) const;

#ifdef DEBUG
    // デバッグ表示に出すオブジェクトの数（tasks.md T14 の決定11）
    [[nodiscard]] debug::ObjectCounts object_counts() const;
#endif

private:
    Player player_;
    Rectangle player_movable_area_{};  // 当たり判定の中心が動ける範囲。start で、自機の文字の大きさから決める

    BulletPool<PLAYER_BULLET_CAPACITY> player_bullets_;
    int updates_since_shot_ = 0;        // 前に撃ってから呼ばれた update() の回数
    float shot_spawn_offset_y_ = 0.0f;  // 撃つ位置の、自機の中心からのずれ（自機の文字の上端まで）。start で決める
    Rectangle bullet_alive_area_{};     // 弾の中心がこの外に出たら消す。start で、弾の文字の大きさから決める
    Vector2 bullet_draw_offset_{};      // 弾の文字の中心を位置に合わせて描くときの、draw_text に渡す位置のずれ
#ifdef DEBUG
    std::size_t player_bullets_dropped_ = 0;  // 満杯で撃てなかった回数の累計（tasks.md T14 の決定5）
#endif
};

}  // namespace stg
