#include "raylib.h"

#include "core/fixed_step.h"

namespace stg {
namespace {

void frame();  // 1フレーム分の処理（design.md §1）

}  // namespace
}  // namespace stg

// プラットフォームの違いは、このブロックの中だけに書く（ADR 0004、AGENT.md）
#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
namespace stg {
namespace {

void run_main_loop() {
    emscripten_set_main_loop(frame, 0, 1);
}

}  // namespace
}  // namespace stg
#else
namespace stg {
namespace {

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

constexpr int SCREEN_WIDTH = 480;
constexpr int SCREEN_HEIGHT = 640;

// 雛形の確認用に、画面を横切る文字を1つ描く。ゲームの中身ができたら消す
constexpr float MARKER_SPEED = 120.0f;  // 1秒に進む px
constexpr int MARKER_FONT_SIZE = 20;

float marker_x = 0.0f;
FixedStep fixed_step;

void update(float dt) {
    marker_x += MARKER_SPEED * dt;
    if (marker_x > static_cast<float>(SCREEN_WIDTH)) {
        marker_x = 0.0f;
    }
}

void render() {
    BeginDrawing();
    ClearBackground(BLACK);
    DrawText("@", static_cast<int>(marker_x), SCREEN_HEIGHT / 2, MARKER_FONT_SIZE, RAYWHITE);
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
    // ウィンドウの大きさの決め方は T05 で決める。それまでは 480×640 のまま
    InitWindow(stg::SCREEN_WIDTH, stg::SCREEN_HEIGHT, "2DShooting");
    // Esc で終了しない（requirements.md §1）。InitWindow の中で Esc に戻されるので、InitWindow の後に呼ぶ
    SetExitKey(KEY_NULL);
    stg::run_main_loop();
    CloseWindow();
    return 0;
}
