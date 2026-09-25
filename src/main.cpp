#include "raylib.h"

#include <algorithm>

#include "core/fixed_step.h"
#include "debug/debug_overlay.h"
#include "render/text_renderer.h"

namespace stg {
namespace {

constexpr int SCREEN_WIDTH = 480;
constexpr int SCREEN_HEIGHT = 640;
constexpr const char* WINDOW_TITLE = "2DShooting";

void frame();  // 1フレーム分の処理（design.md §1）

// 拡大率を 1/4 単位で決める（0.25 刻み。tasks.md T05）。使える大きさに 3:4 のまま収まる最大の値を、切り下げて返す。
// 1/4 単位で持つと、ウィンドウ（120 × 160 の倍数）とフォント（3 の倍数）の大きさが必ず整数になる
constexpr int SCALE_QUARTERS_MIN = 4;  // 拡大率 1.0。これより小さいと、フォントが 12px を下回る（ADR 0002）

int choose_scale_quarters(int available_width, int available_height) {
    const int fit = std::min(available_width * 4 / SCREEN_WIDTH, available_height * 4 / SCREEN_HEIGHT);
    return std::max(fit, SCALE_QUARTERS_MIN);
}

}  // namespace
}  // namespace stg

// プラットフォームの違いは、このブロックの中だけに書く（ADR 0004、AGENT.md）
#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
namespace stg {
namespace {

// ブラウザの表示領域に収まる大きさでウィンドウ（canvas）を作り、拡大率を 1/4 単位で返す
int open_window() {
    const int available_width = emscripten_run_script_int("window.innerWidth");
    const int available_height = emscripten_run_script_int("window.innerHeight");
    const int quarters = choose_scale_quarters(available_width, available_height);
    InitWindow(SCREEN_WIDTH * quarters / 4, SCREEN_HEIGHT * quarters / 4, WINDOW_TITLE);
    return quarters;
}

void run_main_loop() {
    emscripten_set_main_loop(frame, 0, 1);
}

}  // namespace
}  // namespace stg
#else
namespace stg {
namespace {

// モニターに収まる大きさでウィンドウを作り、拡大率を 1/4 単位で返す。
// InitWindow の前にはモニターの大きさを取れないので、隠して作ってから大きさと位置を直し、表示する
int open_window() {
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, WINDOW_TITLE);

    const int monitor = GetCurrentMonitor();
    const int monitor_width = GetMonitorWidth(monitor);
    const int monitor_height = GetMonitorHeight(monitor);
    // タスクバー、メニューバー、タイトルバーの分として、高さの 10% を使わない
    const int quarters = choose_scale_quarters(monitor_width, monitor_height * 9 / 10);

    const int width = SCREEN_WIDTH * quarters / 4;
    const int height = SCREEN_HEIGHT * quarters / 4;
    SetWindowSize(width, height);
    const Vector2 monitor_position = GetMonitorPosition(monitor);
    SetWindowPosition(static_cast<int>(monitor_position.x) + (monitor_width - width) / 2,
                      static_cast<int>(monitor_position.y) + (monitor_height - height) / 2);
    ClearWindowState(FLAG_WINDOW_HIDDEN);
    return quarters;
}

void run_main_loop() {
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        frame();
    }
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
FixedStep fixed_step;
render::TextRenderer text_renderer;

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

}  // namespace
}  // namespace stg

int main() {
    // ウィンドウの大きさは起動時に決めて固定する（ADR 0008）
    const int scale_quarters = stg::open_window();
    const float scale = static_cast<float>(scale_quarters) / 4.0f;
    TraceLog(LOG_INFO, "SCREEN: Scale %.2f, window %d x %d", static_cast<double>(scale), GetScreenWidth(),
             GetScreenHeight());
    // Esc で終了しない（requirements.md §1）。InitWindow の中で Esc に戻されるので、InitWindow の後に呼ぶ
    SetExitKey(KEY_NULL);

    // 読めなかったときのエラー画面は T07 で作る。それまでは終了する
    const char* font_path = TextFormat("%s%s", GetApplicationDirectory(), stg::FONT_FILE);
    if (!stg::text_renderer.load(font_path, scale)) {
        CloseWindow();
        return 1;
    }
#ifdef DEBUG
    stg::debug::init_overlay();
#endif
    stg::run_main_loop();
#ifdef DEBUG
    stg::debug::shutdown_overlay();
#endif
    stg::text_renderer.unload();
    CloseWindow();
    return 0;
}
