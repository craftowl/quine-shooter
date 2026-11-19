#include "game/game.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>

#include "core/window.h"

namespace stg {
namespace {

// 自機と当たり判定を描く文字（tasks.md T09 の仮の文字）。当たり判定の点は、自機の文字の中心に重ねる
constexpr char PLAYER_CHAR = 'A';
constexpr char HITBOX_CHAR = '.';
constexpr Color HITBOX_COLOR = RED;  // 自機の文字に重ねても見えるように、色を変える
// 自機の弾の文字と色（tasks.md T14 の決定9）。動かしてみて見えにくければ変える
constexpr char PLAYER_BULLET_CHAR = '|';
constexpr Color PLAYER_BULLET_COLOR = RAYWHITE;

// 文字の点や線の中心を position に合わせて描くときの、draw_text に渡す位置のずれ
Vector2 glyph_center_offset(const render::TextRenderer& text, char character) {
    const Rectangle bounds = text.glyph_bounds(character);
    return {bounds.x + bounds.width / 2.0f, bounds.y + bounds.height / 2.0f};
}

// 自機の文字が画面に収まるように、当たり判定の中心が動ける範囲を決める（tasks.md T09）
Rectangle compute_player_movable_area(const render::TextRenderer& text) {
    const Rectangle bounds = text.glyph_bounds(PLAYER_CHAR);
    const float half_width = bounds.width / 2.0f;
    const float half_height = bounds.height / 2.0f;
    const Rectangle area = {half_width, half_height, static_cast<float>(SCREEN_WIDTH) - bounds.width,
                            static_cast<float>(SCREEN_HEIGHT) - bounds.height};
    TraceLog(LOG_INFO, "PLAYER: Glyph %.2f x %.2f, movable area x %.2f-%.2f, y %.2f-%.2f", static_cast<double>(bounds.width),
             static_cast<double>(bounds.height), static_cast<double>(area.x), static_cast<double>(area.x + area.width),
             static_cast<double>(area.y), static_cast<double>(area.y + area.height));
    return area;
}

// 弾の中心がこの範囲の外に出たら、弾の文字が画面に映らない。画面を弾の文字の大きさの半分だけ広げる（tasks.md T14 の決定10）
Rectangle compute_bullet_alive_area(const render::TextRenderer& text, char character) {
    const Rectangle bounds = text.glyph_bounds(character);
    return {-bounds.width / 2.0f, -bounds.height / 2.0f, static_cast<float>(SCREEN_WIDTH) + bounds.width,
            static_cast<float>(SCREEN_HEIGHT) + bounds.height};
}

// character の点や線の中心が center に来るように描く
void draw_char_centered(const render::TextRenderer& text, char character, Vector2 center, Color color) {
    const char chars[] = {character, '\0'};
    const Vector2 offset = glyph_center_offset(text, character);
    text.draw_text(chars, {center.x - offset.x, center.y - offset.y}, color);
}

}  // namespace

int shot_interval_updates(float interval, float dt) {
    // interval は params.json の範囲（0 より大きく 1 以下）。dt が 1/60 なら、回数は 60 を超えない
    assert(interval > 0.0f && interval <= 1.0f && dt > 0.0f);
    const long updates = std::lround(interval / dt);
    return updates < 1 ? 1 : static_cast<int>(updates);
}

void Game::start(const render::TextRenderer& text, const Params& params) {
    player_movable_area_ = compute_player_movable_area(text);
    // 開始位置が動ける範囲の外でも（画面の端など）、最初のフレームから範囲の中に描くように収めておく
    const Rectangle& area = player_movable_area_;
    const Vector2 start = params.player.start_position;
    player_ = Player{{std::clamp(start.x, area.x, area.x + area.width), std::clamp(start.y, area.y, area.y + area.height)}};

    // 自機の弾（tasks.md T14）。弾の中心を、自機の文字の上端に置いて撃つ（決定9）
    shot_spawn_offset_y_ = -text.glyph_bounds(PLAYER_CHAR).height / 2.0f;
    bullet_alive_area_ = compute_bullet_alive_area(text, PLAYER_BULLET_CHAR);
    bullet_draw_offset_ = glyph_center_offset(text, PLAYER_BULLET_CHAR);
    const Rectangle bullet_bounds = text.glyph_bounds(PLAYER_BULLET_CHAR);
    const Rectangle& alive = bullet_alive_area_;
    TraceLog(LOG_INFO, "SHOT: Glyph %.2f x %.2f, spawn offset y %.2f, alive area x %.2f-%.2f, y %.2f-%.2f",
             static_cast<double>(bullet_bounds.width), static_cast<double>(bullet_bounds.height),
             static_cast<double>(shot_spawn_offset_y_), static_cast<double>(alive.x),
             static_cast<double>(alive.x + alive.width), static_cast<double>(alive.y),
             static_cast<double>(alive.y + alive.height));
    player_bullets_.clear();
    // 最初の update() で撃つように、撃つ回数に達した状態にしておく（決定8）
    updates_since_shot_ = std::numeric_limits<int>::max();
}

void Game::update(const InputState& input, const Params& params, float dt) {
    update_player(player_, input, params.player, player_movable_area_, dt);

    // 飛んでいる弾を先に動かし、撃った弾は撃った位置に描かれるようにする
    player_bullets_.update(dt, bullet_alive_area_);
    // 撃つ回数は毎回求める。ホットリロードで interval が変わっても、次の判定から反映される（決定7）
    if (updates_since_shot_ >= shot_interval_updates(params.shot.interval, dt)) {
        const Vector2 position = {player_.position.x, player_.position.y + shot_spawn_offset_y_};
        // 満杯なら撃たない（決定5）。撃てなかった回数を数えるのは、デバッグ機能のあるビルドだけ
        [[maybe_unused]] const bool spawned = player_bullets_.spawn(position, {0.0f, -params.shot.bullet_speed});
#ifdef DEBUG
        if (!spawned) {
            ++player_bullets_dropped_;
        }
#endif
        updates_since_shot_ = 0;
    }
    ++updates_since_shot_;
}

void Game::draw(const render::TextRenderer& text) const {
    draw_char_centered(text, PLAYER_CHAR, player_.position, RAYWHITE);
    draw_char_centered(text, HITBOX_CHAR, player_.position, HITBOX_COLOR);

    const char bullet_chars[] = {PLAYER_BULLET_CHAR, '\0'};
    for (const Bullet& bullet : player_bullets_.bullets()) {
        text.draw_text(bullet_chars, {bullet.position.x - bullet_draw_offset_.x, bullet.position.y - bullet_draw_offset_.y},
                       PLAYER_BULLET_COLOR);
    }
}

#ifdef DEBUG
debug::ObjectCounts Game::object_counts() const {
    return {player_bullets_.count(), player_bullets_.capacity(), player_bullets_dropped_};
}
#endif

}  // namespace stg
