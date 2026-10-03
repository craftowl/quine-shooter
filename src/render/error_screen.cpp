#include "render/error_screen.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstring>

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

// 1行に写せる文字数の上限。これより長い行は、ここで切って次の行に送る
constexpr std::size_t MAX_LINE_LENGTH = 255;

// text の先頭 length 文字を描いたときの幅（px）。毎フレーム呼ぶので確保しない（ADR 0007）
int measure_prefix(const char* text, std::size_t length, int font_size) {
    char line[MAX_LINE_LENGTH + 1];
    assert(length <= MAX_LINE_LENGTH);
    std::memcpy(line, text, length);
    line[length] = '\0';
    return MeasureText(line, font_size);
}

// text を max_width（px）に収まるように折り返して描き、使った行数を返す。
// 空白の位置で折り返し、空白のない長い語は、収まるところで切る（tasks.md の T12b）
int draw_wrapped(const char* text, int x, int y, int max_width, int font_size, int line_height) {
    int lines = 0;
    const char* rest = text;
    while (*rest != '\0') {
        const std::size_t total = std::min(std::strlen(rest), MAX_LINE_LENGTH);
        // 空白の手前で切って収まる、いちばん長い長さを探す
        std::size_t fit = 0;
        for (std::size_t end = 1; end <= total; ++end) {
            const bool at_break = end == total || rest[end] == ' ';
            if (at_break && measure_prefix(rest, end, font_size) <= max_width) {
                fit = end;
            }
        }
        // 最初の語だけで収まらないときは、文字の単位で切る（少なくとも1文字は描く）
        if (fit == 0) {
            fit = 1;
            while (fit < total && measure_prefix(rest, fit + 1, font_size) <= max_width) {
                ++fit;
            }
        }
        char line[MAX_LINE_LENGTH + 1];
        std::memcpy(line, rest, fit);
        line[fit] = '\0';
        DrawText(line, x, y + line_height * lines, font_size, RAYWHITE);
        ++lines;
        rest += fit;
        while (*rest == ' ') {
            ++rest;
        }
    }
    return lines;
}

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
        // 理由は長くなりうるので、画面の右の余白の手前で折り返す
        const int reason_x = x + REASON_INDENT * unit;
        const int max_width = GetRenderWidth() - reason_x - MARGIN * unit;
        next_line(draw_wrapped(error.reason.c_str(), reason_x, y, max_width, font_size, LINE_HEIGHT * unit));
    }
    next_line(1);
    DrawText(quit_hint, x, y, font_size, RAYWHITE);

    EndMode2D();
}

}  // namespace stg::render
