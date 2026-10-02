#pragma once

#include "core/load_errors.h"

namespace stg::render {

// 起動時に読めなかったファイルと理由を、raylib に組み込まれたフォントで描く（ADR 0006 §6）。
// BeginDrawing と EndDrawing の間で呼ぶ。quit_hint は閉じ方の案内（デスクトップ版と Web 版で変わる）。
// scale は、480×640 の 1px が実際の画面の何ピクセルか
void draw_error_screen(const LoadErrors& errors, const char* quit_hint, float scale);

}  // namespace stg::render
