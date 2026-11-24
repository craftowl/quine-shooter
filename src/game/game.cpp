#include "game/game.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <optional>
#include <string_view>

#include "core/window.h"
#include "enemies/zako_dummy.h"

namespace stg {
namespace {

// 自機と当たり判定を描く文字（tasks.md T09 の仮の文字）。当たり判定の点は、自機の文字の中心に重ねる
constexpr char PLAYER_CHAR = 'A';
constexpr char HITBOX_CHAR = '.';
constexpr Color HITBOX_COLOR = RED;  // 自機の文字に重ねても見えるように、色を変える
// 自機の弾の文字と色（tasks.md T14 の決定9）。動かしてみて見えにくければ変える
constexpr char PLAYER_BULLET_CHAR = '|';
constexpr Color PLAYER_BULLET_COLOR = RAYWHITE;

// 敵（tasks.md T16 の決定3・6・7）。マスの大きさは ADR 0002 の 5.5 × 14px。幅は、文字の送り幅（拡大率によって
// 4.8〜5.45px）より少し広い定数にし、どの画面でも敵の大きさと当たり判定が同じになるようにする（ADR 0003）
constexpr float ENEMY_CELL_WIDTH = 5.5f;
constexpr float ENEMY_CELL_HEIGHT = 14.0f;
constexpr Color ENEMY_BODY_COLOR = GREEN;
constexpr Color ENEMY_WEAK_POINT_COLOR = YELLOW;  // 弱点の $（ADR 0002 のルール4）
// 仮の出し方（決定3）。T20 のウェーブ定義で置き換える
constexpr std::string_view ZAKO_ID = "zako_dummy";
constexpr std::string_view BOSS_ID = "boss_dummy";
constexpr Vector2 BOSS_SPAWN_POSITION = {24.0f, 24.0f};  // マスクの左上
constexpr float ZAKO_SPAWN_Y = 360.0f;                   // マスクの左上。x は横に等しい間隔で並べる
static_assert(ZAKO_SPAWN_COUNT == 3, "ENEMY: のログは3体の位置を出す");

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

// 埋め込んだ敵の体のテキスト。埋め込みはビルドのときに quine_pack が作るので、ないのはバグ（ADR 0006）
EnemyText find_embedded_enemy(std::string_view id) {
    const std::optional<EnemyText> text = find_enemy_text(id);
    assert(text.has_value());
    return *text;
}

// 敵の体（マスク全体の四角形）が、画面の外に完全に出たか（決定4）。4辺のどれから出ても true
bool is_enemy_off_screen(const Enemy& enemy) {
    const float width = static_cast<float>(enemy.text.columns) * ENEMY_CELL_WIDTH;
    const float height = static_cast<float>(enemy.text.rows) * ENEMY_CELL_HEIGHT;
    return enemy.position.x + width <= 0.0f || enemy.position.y + height <= 0.0f ||
           enemy.position.x >= static_cast<float>(SCREEN_WIDTH) || enemy.position.y >= static_cast<float>(SCREEN_HEIGHT);
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

    // 敵（tasks.md T16a）。位置はマスクの左上（決定6）
    zako_text_ = find_embedded_enemy(ZAKO_ID);
    boss_text_ = find_embedded_enemy(BOSS_ID);
    // 雑魚は、画面の幅から雑魚の幅 × 数を引いた残りを、数 ＋ 1 に等しく分けた間隔で並べる（決定3）
    const float zako_width = static_cast<float>(zako_text_.columns) * ENEMY_CELL_WIDTH;
    const float gap = (static_cast<float>(SCREEN_WIDTH) - zako_width * static_cast<float>(ZAKO_SPAWN_COUNT)) /
                      static_cast<float>(ZAKO_SPAWN_COUNT + 1);
    // 仮の配置の前提（雑魚が横に並んで画面に収まる）。マスの幅 5.5px では、雑魚のマスクが 30 列以上になると崩れる。
    // 雑魚のマスクの上限は 24 列（ADR 0002）なので、上限の中なら収まる（T20 で配置ごと置き換える）
    assert(gap >= 0.0f);
    for (std::size_t index = 0; index < ZAKO_SPAWN_COUNT; ++index) {
        zako_spawn_positions_[index] = {gap + static_cast<float>(index) * (zako_width + gap), ZAKO_SPAWN_Y};
    }
    const std::array<Vector2, ZAKO_SPAWN_COUNT>& zako = zako_spawn_positions_;
    TraceLog(LOG_INFO, "ENEMY: Cell %.2f x %.2f, boss at (%.2f, %.2f), zako at (%.2f, %.2f), (%.2f, %.2f), (%.2f, %.2f)",
             static_cast<double>(ENEMY_CELL_WIDTH), static_cast<double>(ENEMY_CELL_HEIGHT),
             static_cast<double>(BOSS_SPAWN_POSITION.x), static_cast<double>(BOSS_SPAWN_POSITION.y),
             static_cast<double>(zako[0].x), static_cast<double>(zako[0].y), static_cast<double>(zako[1].x),
             static_cast<double>(zako[1].y), static_cast<double>(zako[2].x), static_cast<double>(zako[2].y));
    zakos_.clear();
    bosses_.clear();
    spawn_zakos();
    spawn_boss();
}

void Game::spawn_zakos() {
    for (const Vector2 position : zako_spawn_positions_) {
        // T16 では容量に届かないので、満杯はバグとして止める（決定14。T20 で見直す）
        [[maybe_unused]] const bool spawned = zakos_.spawn(zako_text_, position);
        assert(spawned);
    }
}

void Game::spawn_boss() {
    [[maybe_unused]] const bool spawned = bosses_.spawn(boss_text_, BOSS_SPAWN_POSITION);
    assert(spawned);
}

void Game::update(const InputState& input, const Params& params, float dt) {
    update_player(player_, input, params.player, player_movable_area_, dt);

    // 敵を動かす（tasks.md T16 の決定4）。雑魚の速度は毎回求めるので、ホットリロードで速さを変えるとすぐ変わる。
    // ボスは動かない
    const Vector2 zako_velocity = zako_dummy_velocity(params.enemies.zako_dummy.speed);
    for (Enemy& zako : zakos_.enemies()) {
        zako.position.x += zako_velocity.x * dt;
        zako.position.y += zako_velocity.y * dt;
    }
    zakos_.remove_if([](const Enemy& zako) { return is_enemy_off_screen(zako); });
    // いなくなったら、同じ update() の中ですぐ置き直す（決定3）
    if (zakos_.count() == 0) {
        spawn_zakos();
    }
    if (bosses_.count() == 0) {
        spawn_boss();
    }

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
    // 後に描いたものが上に重なる。ボス → 雑魚 → 自機の弾 → 自機の順（tasks.md T16 の決定8）
    for (const Enemy& boss : bosses_.enemies()) {
        draw_enemy(text, boss);
    }
    for (const Enemy& zako : zakos_.enemies()) {
        draw_enemy(text, zako);
    }

    const char bullet_chars[] = {PLAYER_BULLET_CHAR, '\0'};
    for (const Bullet& bullet : player_bullets_.bullets()) {
        text.draw_text(bullet_chars, {bullet.position.x - bullet_draw_offset_.x, bullet.position.y - bullet_draw_offset_.y},
                       PLAYER_BULLET_COLOR);
    }

    draw_char_centered(text, PLAYER_CHAR, player_.position, RAYWHITE);
    draw_char_centered(text, HITBOX_CHAR, player_.position, HITBOX_COLOR);
}

void Game::draw_enemy(const render::TextRenderer& text, const Enemy& enemy) const {
    // 壊れていない '#' と '$' のマスの文字を、マスの左上に1文字ずつ描く（決定6・7）。マスの幅は文字の送り幅より
    // 少し広いので、文字列としてまとめて描かず、マスの格子に合わせる。空白は描かない
    const EnemyText& body = enemy.text;
    char chars[] = {'\0', '\0'};
    for (int row = 0; row < body.rows; ++row) {
        for (int column = 0; column < body.columns; ++column) {
            const std::size_t index = static_cast<std::size_t>(row) * static_cast<std::size_t>(body.columns) +
                                      static_cast<std::size_t>(column);
            if (!enemy.alive_cells[index] || body.text[index] == ' ') {
                continue;
            }
            chars[0] = body.text[index];
            const Color color = body.mask[index] == '$' ? ENEMY_WEAK_POINT_COLOR : ENEMY_BODY_COLOR;
            text.draw_text(chars,
                           {enemy.position.x + static_cast<float>(column) * ENEMY_CELL_WIDTH,
                            enemy.position.y + static_cast<float>(row) * ENEMY_CELL_HEIGHT},
                           color);
        }
    }
}

#ifdef DEBUG
debug::ObjectCounts Game::object_counts() const {
    return {.player_bullets = player_bullets_.count(),
            .player_bullet_capacity = player_bullets_.capacity(),
            .player_bullets_dropped = player_bullets_dropped_,
            .zako = zakos_.count(),
            .zako_capacity = zakos_.capacity(),
            .boss = bosses_.count(),
            .boss_capacity = bosses_.capacity()};
}
#endif

}  // namespace stg
