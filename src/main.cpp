#include "raylib.h"

#include <algorithm>
#include <cassert>
#include <cmath>

#include "core/fixed_step.h"
#include "core/load_errors.h"
#include "debug/debug_overlay.h"
#include "render/error_screen.h"
#include "render/text_renderer.h"

namespace stg {
namespace {

constexpr int SCREEN_WIDTH = 480;
constexpr int SCREEN_HEIGHT = 640;
constexpr const char* WINDOW_TITLE = "2DShooting";

void frame();        // 1フレーム分の処理（design.md §1）
void error_frame();  // 起動時に読めなかったファイルがあるときの、1フレーム分の処理（tasks.md T07）

// 拡大率を 1/4 単位で決める（0.25 刻み。tasks.md T05）。拡大率は、480×640 の 1px が実際の画面の何ピクセルになるか。
// 使える大きさ（実際のピクセル）に 3:4 のまま収まる最大の値を、切り下げて返す。
// 1/4 単位で持つと、ウィンドウ（120 × 160 の倍数）とフォント（3 の倍数）の大きさが必ず整数になる
constexpr int SCALE_QUARTERS_MIN = 4;  // 拡大率 1.0。これより小さいと、フォントが 12px を下回る（ADR 0002）

// pixel_ratio は、OS やブラウザの座標（ポイント、CSS px）の 1 が、実際の何ピクセルか。
// 最小の拡大率は、OS やブラウザの座標で 1.0 にする（高 DPI の画面で文字が小さくなりすぎないように）
int choose_scale_quarters(int available_width, int available_height, float pixel_ratio) {
    const int min_quarters =
        std::max(SCALE_QUARTERS_MIN, static_cast<int>(std::ceil(static_cast<float>(SCALE_QUARTERS_MIN) * pixel_ratio)));
    const int fit = std::min(available_width * 4 / SCREEN_WIDTH, available_height * 4 / SCREEN_HEIGHT);
    return std::max(fit, min_quarters);
}

// デスクトップ版のウィンドウを、モニターに収まる大きさで作り、拡大率を 1/4 単位で返す。
// InitWindow の前にはモニターの大きさを取れないので、隠して作ってから大きさと位置を直し、表示する。
// high_dpi のときは、ウィンドウの大きさ（ポイント）の pixel_ratio 倍のピクセルで描く（macOS の Retina では 2 倍）
[[maybe_unused]] int open_desktop_window(bool high_dpi) {
    unsigned int flags = FLAG_WINDOW_HIDDEN;
    if (high_dpi) {
        flags |= FLAG_WINDOW_HIGHDPI;
    }
    SetConfigFlags(flags);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, WINDOW_TITLE);

    const float pixel_ratio = high_dpi ? GetWindowScaleDPI().x : 1.0f;
    const int monitor = GetCurrentMonitor();
    const int monitor_width = GetMonitorWidth(monitor);  // ポイント
    const int monitor_height = GetMonitorHeight(monitor);
    // タスクバー、メニューバー、タイトルバーの分として、高さの 10% を使わない
    const int quarters = choose_scale_quarters(static_cast<int>(static_cast<float>(monitor_width) * pixel_ratio),
                                               static_cast<int>(static_cast<float>(monitor_height * 9 / 10) * pixel_ratio),
                                               pixel_ratio);

    // ウィンドウの大きさはポイントで渡す
    const int width = static_cast<int>(std::lround(static_cast<float>(SCREEN_WIDTH * quarters / 4) / pixel_ratio));
    const int height = static_cast<int>(std::lround(static_cast<float>(SCREEN_HEIGHT * quarters / 4) / pixel_ratio));
    SetWindowSize(width, height);
    const Vector2 monitor_position = GetMonitorPosition(monitor);
    SetWindowPosition(static_cast<int>(monitor_position.x) + (monitor_width - width) / 2,
                      static_cast<int>(monitor_position.y) + (monitor_height - height) / 2);
    ClearWindowState(FLAG_WINDOW_HIDDEN);
    TraceLog(LOG_INFO, "SCREEN: Pixel ratio %.2f", static_cast<double>(pixel_ratio));
    return quarters;
}

// Esc では閉じないので（SetExitKey）、閉じるボタンを押すまで回る
[[maybe_unused]] void run_desktop_loop(void (*frame_func)()) {
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        frame_func();
    }
}

}  // namespace
}  // namespace stg

// プラットフォームの違いは、このブロックの中だけに書く（ADR 0004、AGENT.md）
#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
namespace stg {
namespace {

// ブラウザの表示領域に収まる大きさでウィンドウ（canvas）を作り、拡大率を 1/4 単位で返す。
// canvas の中身は実際のピクセルの大きさにし、表示の大きさ（CSS）をその 1/devicePixelRatio にする。
// raylib の高 DPI の設定は Web では効かないので、raylib からは、canvas の中身の大きさのウィンドウに見える
int open_window() {
    const double pixel_ratio = emscripten_get_device_pixel_ratio();
    const int available_width = static_cast<int>(emscripten_run_script_int("window.innerWidth") * pixel_ratio);
    const int available_height = static_cast<int>(emscripten_run_script_int("window.innerHeight") * pixel_ratio);
    const int quarters = choose_scale_quarters(available_width, available_height, static_cast<float>(pixel_ratio));

    const int width = SCREEN_WIDTH * quarters / 4;
    const int height = SCREEN_HEIGHT * quarters / 4;
    InitWindow(width, height, WINDOW_TITLE);
    if (emscripten_set_element_css_size("#canvas", width / pixel_ratio, height / pixel_ratio) != EMSCRIPTEN_RESULT_SUCCESS) {
        TraceLog(LOG_WARNING, "SCREEN: Failed to set the CSS size of the canvas");
    }
    TraceLog(LOG_INFO, "SCREEN: Pixel ratio %.2f", pixel_ratio);
    return quarters;
}

// エラー画面の閉じ方の案内。Web 版には閉じるボタンがないので、タブを閉じるまで出し続ける（ADR 0006 §6）
constexpr const char* QUIT_HINT = "Close the tab to quit.";

// タブを閉じるまで回る
void run_main_loop(void (*frame_func)()) {
    emscripten_set_main_loop(frame_func, 0, 1);
}

}  // namespace
}  // namespace stg
#elif defined(__APPLE__)
namespace stg {
namespace {

// macOS は、Retina の画面で実際のピクセルで描く
int open_window() {
    return open_desktop_window(true);
}

constexpr const char* QUIT_HINT = "Close the window to quit.";

void run_main_loop(void (*frame_func)()) {
    run_desktop_loop(frame_func);
}

}  // namespace
}  // namespace stg
#else
namespace stg {
namespace {

// Windows は高 DPI の設定を使わない。GLFW が DPI に対応したアプリとして動くので、OS は引き伸ばさず、
// モニターの大きさもウィンドウの大きさもピクセルになる（見込み。Windows では確かめていない。T10）
int open_window() {
    return open_desktop_window(false);
}

constexpr const char* QUIT_HINT = "Close the window to quit.";

void run_main_loop(void (*frame_func)()) {
    run_desktop_loop(frame_func);
}

}  // namespace
}  // namespace stg
#endif

namespace stg {
namespace {

constexpr const char* FONT_FILE = "assets/fonts/JetBrainsMono-Regular.ttf";  // 実行ファイルの場所から（ADR 0006）

// 雛形の確認用に、画面を横切る文字を1つと、四隅と中央の目印を描く。ゲームの中身ができたら消す
constexpr float MARKER_SPEED = 120.0f;  // 1秒に進む px

float marker_x = 0.0f;
float screen_scale = 1.0f;  // 480×640 の 1px が実際の画面の何ピクセルか。起動時に決める
FixedStep fixed_step;
render::TextRenderer text_renderer;
LoadErrors load_errors;

void update(float dt) {
    marker_x += MARKER_SPEED * dt;
    if (marker_x > static_cast<float>(SCREEN_WIDTH)) {
        marker_x = 0.0f;
    }
}

void draw_corner_marks() {
    const char* mark = "+";
    const Vector2 size = text_renderer.measure_text(mark);
    const float right = static_cast<float>(SCREEN_WIDTH) - size.x;
    const float bottom = static_cast<float>(SCREEN_HEIGHT) - size.y;
    text_renderer.draw_text(mark, {0.0f, 0.0f}, RAYWHITE);
    text_renderer.draw_text(mark, {right, 0.0f}, RAYWHITE);
    text_renderer.draw_text(mark, {0.0f, bottom}, RAYWHITE);
    text_renderer.draw_text(mark, {right, bottom}, RAYWHITE);
    text_renderer.draw_text(mark, {right / 2.0f, bottom / 2.0f}, RAYWHITE);
}

void render() {
    BeginDrawing();
    ClearBackground(BLACK);
    text_renderer.begin();
    draw_corner_marks();
    text_renderer.draw_text("@", {marker_x, static_cast<float>(SCREEN_HEIGHT) / 2.0f}, RAYWHITE);
    text_renderer.end();
#ifdef DEBUG
    debug::draw_overlay();
#endif
    EndDrawing();
}

void frame() {
    const int steps = fixed_step.advance(GetFrameTime());
    for (int i = 0; i < steps; ++i) {
        update(FIXED_DT);
    }
    render();
}

// ゲームの処理（update）は回さず、エラー画面を描くだけ
void error_frame() {
    BeginDrawing();
    ClearBackground(BLACK);
    render::draw_error_screen(load_errors, QUIT_HINT, screen_scale);
    EndDrawing();
}

}  // namespace
}  // namespace stg

int main() {
    // ウィンドウの大きさは起動時に決めて固定する（ADR 0008）
    const int scale_quarters = stg::open_window();
    const float scale = static_cast<float>(scale_quarters) / 4.0f;
    stg::screen_scale = scale;
    TraceLog(LOG_INFO, "SCREEN: Scale %.2f, window %d x %d, render %d x %d", static_cast<double>(scale),
             GetScreenWidth(), GetScreenHeight(), GetRenderWidth(), GetRenderHeight());
    // Esc で終了しない（requirements.md §1）。InitWindow の中で Esc に戻されるので、InitWindow の後に呼ぶ
    SetExitKey(KEY_NULL);

    // 起動時に読むファイル。1つ読めなくても残りを読み続け、読めなかったものをまとめてエラー画面に出す（ADR 0006 §6）。
    // params.json とウェーブ定義は、T12 と T20 でここに足す
    const bool loaded = stg::text_renderer.load(stg::FONT_FILE, scale, stg::load_errors);

    if (loaded) {
#ifdef DEBUG
        stg::debug::init_overlay();
#endif
        stg::run_main_loop(stg::frame);
#ifdef DEBUG
        stg::debug::shutdown_overlay();
#endif
    } else {
        // ゲームの処理を始めずに、閉じるまでエラー画面を出し続ける
        assert(!stg::load_errors.empty());
        TraceLog(LOG_WARNING, "STARTUP: %d file(s) could not be loaded, showing the error screen",
                 static_cast<int>(stg::load_errors.entries().size()));
        stg::run_main_loop(stg::error_frame);
    }

    stg::text_renderer.unload();
    CloseWindow();
    return loaded ? 0 : 1;
}
