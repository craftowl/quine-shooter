#include "render/error_screen.h"

#include <algorithm>
#include <cassert>
#include <cmath>

#include "raylib.h"

namespace stg::render {
namespace {

// 組み込みのフォントの文字の高さ（px）。raylib の DrawText は、文字間を fontSize / 10 にする
constexpr int BUILTIN_FONT_SIZE = 10;

// 並びの寸法。組み込みのフォントの 1 ドットを 1 とした単位
constexpr int MARGIN = 20;       // 画面の左と上の余白
constexpr int LINE_HEIGHT = 15;  // 1行の高さ（文字の高さ 10 と行間 5）
constexpr int PATH_INDENT = 12;  // ファイル名の字下げ（2文字分）
constexpr int REASON_INDENT = 24;

constexpr const char* TITLE = "Failed to start the game.";
constexpr const char* LIST_HEADING = "The following files could not be loaded:";

}  // namespace

void draw_error_screen(const LoadErrors& errors, const char* quit_hint, float scale) {
    assert(!errors.empty());
    assert(quit_hint != nullptr);

    // 組み込みのフォントは点で描かれた文字なので、整数倍でないとドットの幅がそろわない。
    // そこで拡大率を整数に四捨五入し、Camera2D を使わずに実際のピクセルの座標で描く。
    // 拡大率より大きくなるのは最大で 4/3 倍（拡大率 1.5 のとき 2）だが、いちばん長い行（40 文字ほど）でも画面の幅に収まる
    const int unit = std::max(1, static_cast<int>(std::lround(scale)));
    const int font_size = BUILTIN_FONT_SIZE * unit;

    // BeginDrawing は高 DPI の画面で座標を拡大するので、拡大しない Camera2D の中で描き、座標を実際のピクセルにそろえる
    Camera2D camera{};
    camera.zoom = 1.0f;
    BeginMode2D(camera);

    const int x = MARGIN * unit;
    int y = MARGIN * unit;
    const auto next_line = [&y, unit](int lines) { y += LINE_HEIGHT * unit * lines; };

    DrawText(TITLE, x, y, font_size, RAYWHITE);
    next_line(2);
    DrawText(LIST_HEADING, x, y, font_size, RAYWHITE);
    next_line(2);
    for (const LoadError& error : errors.entries()) {
        DrawText(error.path.c_str(), x + PATH_INDENT * unit, y, font_size, RAYWHITE);
        next_line(1);
        DrawText(error.reason.c_str(), x + REASON_INDENT * unit, y, font_size, RAYWHITE);
        next_line(1);
    }
    next_line(1);
    DrawText(quit_hint, x, y, font_size, RAYWHITE);

    EndMode2D();
}

}  // namespace stg::render
