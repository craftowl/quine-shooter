#include "core/window.h"

#include "raylib.h"

#include <algorithm>
#include <cmath>

namespace stg {
namespace {

constexpr int SCALE_QUARTERS_MIN = 4;  // 拡大率 1.0。これより小さいと、フォントが 12px を下回る（ADR 0002）

}  // namespace

int choose_scale_quarters(int available_width, int available_height, float pixel_ratio) {
    const int min_quarters =
        std::max(SCALE_QUARTERS_MIN, static_cast<int>(std::ceil(static_cast<float>(SCALE_QUARTERS_MIN) * pixel_ratio)));
    const int fit = std::min(available_width * 4 / SCREEN_WIDTH, available_height * 4 / SCREEN_HEIGHT);
    return std::max(fit, min_quarters);
}

// InitWindow の前にはモニターの大きさを取れないので、隠して作ってから大きさと位置を直し、表示する
int open_desktop_window(bool high_dpi) {
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

void run_desktop_loop(void (*frame_func)()) {
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        frame_func();
    }
}

}  // namespace stg
