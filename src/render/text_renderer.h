#pragma once

#include "raylib.h"

#include "core/load_errors.h"

namespace stg::render {

// 文字の大きさ。480×640 の座標での px（ADR 0008）
inline constexpr int FONT_SIZE = 12;

// 480×640 の座標で文字を描く。フォントを拡大率に合わせた大きさで読み込み、Camera2D で拡大して描く（ADR 0008）
class TextRenderer {
public:
    // 12 × scale px でフォントを読み込み、Camera2D の拡大率を scale にする。12 × scale は整数でなければならない。
    // font_path は実行ファイルの場所からの相対パス（ADR 0006 §6）。読めなかったら、パスと理由を errors に足す。
    // scale は、480×640 の 1px が実際の画面の何ピクセルか。BeginMode2D は raylib の高 DPI の倍率を掛けないので、
    // 高 DPI の画面でも、Camera2D の中の座標は実際のピクセルになる
    [[nodiscard]] bool load(const char* font_path, float scale, LoadErrors& errors);
    // 読み込んでいなければ何もしない
    void unload();

    // BeginMode2D と EndMode2D。この間の座標は 480×640
    void begin();
    void end();

    // position は 480×640 の座標。画面のピクセルに合わせて丸めて描く
    void draw_text(const char* text, Vector2 position, Color color) const;
    // 描いたときの幅と高さ（480×640 の座標）
    [[nodiscard]] Vector2 measure_text(const char* text) const;

private:
    Font font_{};
    Camera2D camera_{};
};

}  // namespace stg::render
