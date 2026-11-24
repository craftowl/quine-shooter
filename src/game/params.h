#pragma once

#include "raylib.h"

#include <optional>
#include <string>

namespace stg {

// 調整値（data/params.json）。キーの単位と範囲は design.md §5 の「params.json のキー」
struct PlayerParams {
    float normal_speed = 0.0f;  // px/秒
    float slow_speed = 0.0f;    // px/秒（Shift を押している間）
    Vector2 start_position{};   // 480×640 の座標（当たり判定の中心。復活の位置も同じ）
};

struct ShotParams {
    float interval = 0.0f;      // 秒（撃ってから次に撃つまで。tasks.md T14 の決定7）
    float bullet_speed = 0.0f;  // px/秒（上向き）
};

// 仮の雑魚 zako_dummy（tasks.md T16 の決定4）。T21 で置き換えるか消す
struct ZakoDummyParams {
    float speed = 0.0f;  // px/秒（下向き）
};

// 敵ごとの値。敵 ID ごとに入れ子にする
struct EnemiesParams {
    ZakoDummyParams zako_dummy;
};

struct Params {
    PlayerParams player;
    ShotParams shot;
    EnemiesParams enemies;
};

// text（params.json の中身）の型と範囲を確かめて解釈する。失敗したら、理由（英語。キーの名前を含む）を
// error に入れ、TraceLog にも出す。知らないキーは、失敗にせず TraceLog に警告を出す。
// 確保してよい範囲の中で呼ぶ（起動時は起動の間、読み直しは AllocationAllowed の中。ADR 0007）
[[nodiscard]] std::optional<Params> parse_params(const char* text, std::string& error);

}  // namespace stg
