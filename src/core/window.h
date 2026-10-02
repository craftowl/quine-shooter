#pragma once

namespace stg {

// 画面の座標の大きさ。表示するときは座標だけを拡大する（ADR 0008）
inline constexpr int SCREEN_WIDTH = 480;
inline constexpr int SCREEN_HEIGHT = 640;

inline constexpr const char* WINDOW_TITLE = "2DShooting";

// 拡大率を 1/4 単位で決める（0.25 刻み。tasks.md T05）。拡大率は、480×640 の 1px が実際の画面の何ピクセルになるか。
// 使える大きさ（実際のピクセル）に 3:4 のまま収まる最大の値を、切り下げて返す。
// 1/4 単位で持つと、ウィンドウ（120 × 160 の倍数）とフォント（3 の倍数）の大きさが必ず整数になる。
// pixel_ratio は、OS やブラウザの座標（ポイント、CSS px）の 1 が、実際の何ピクセルか。
// 最小の拡大率は、OS やブラウザの座標で 1.0 にする（高 DPI の画面で文字が小さくなりすぎないように）
int choose_scale_quarters(int available_width, int available_height, float pixel_ratio);

// デスクトップ版のウィンドウを、モニターに収まる大きさで作り、拡大率を 1/4 単位で返す。
// high_dpi のときは、ウィンドウの大きさ（ポイント）の pixel_ratio 倍のピクセルで描く（macOS の Retina では 2 倍）
int open_desktop_window(bool high_dpi);

// デスクトップ版のメインループ。Esc では閉じないので（SetExitKey）、閉じるボタンを押すまで回る
void run_desktop_loop(void (*frame_func)());

}  // namespace stg
