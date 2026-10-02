#include "game/game.h"

#include "core/window.h"

namespace stg {
namespace {

// 自機と当たり判定を描く文字（tasks.md T09 の仮の文字）。当たり判定の点は、自機の文字の中心に重ねる
constexpr char PLAYER_CHAR = 'A';
constexpr char HITBOX_CHAR = '.';
constexpr Color HITBOX_COLOR = RED;  // 自機の文字に重ねても見えるように、色を変える

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

// character の点や線の中心が center に来るように描く
void draw_char_centered(const render::TextRenderer& text, char character, Vector2 center, Color color) {
    const char chars[] = {character, '\0'};
    const Vector2 offset = glyph_center_offset(text, character);
    text.draw_text(chars, {center.x - offset.x, center.y - offset.y}, color);
}

}  // namespace

void Game::start(const render::TextRenderer& text) {
    player_ = Player{};
    player_movable_area_ = compute_player_movable_area(text);
}

void Game::update(const InputState& input, float dt) {
    update_player(player_, input, player_movable_area_, dt);
}

void Game::draw(const render::TextRenderer& text) const {
    draw_char_centered(text, PLAYER_CHAR, player_.position, RAYWHITE);
    draw_char_centered(text, HITBOX_CHAR, player_.position, HITBOX_COLOR);
}

}  // namespace stg
