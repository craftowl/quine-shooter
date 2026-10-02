#pragma once

#include "raylib.h"

#include "core/input.h"
#include "game/player.h"
#include "render/text_renderer.h"

namespace stg {

// ゲームの状態と、その更新と描画（tasks.md T12a）
class Game {
public:
    // 文字の大きさから自機の動ける範囲を決め、開始位置に置く。text は読み込み済み
    void start(const render::TextRenderer& text);
    void update(const InputState& input, float dt);
    // text.begin() と text.end() の間で呼ぶ（begin と end は main.cpp が呼ぶ。どちらも const ではないため）
    void draw(const render::TextRenderer& text) const;

private:
    Player player_;
    Rectangle player_movable_area_{};  // 当たり判定の中心が動ける範囲。start で、自機の文字の大きさから決める
};

}  // namespace stg
