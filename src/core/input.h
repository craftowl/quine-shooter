#pragma once

namespace stg {

// 1フレームの最初に読んだ入力（design.md §1）。update() にはこれを渡し、update() の中ではキーを読まない
struct InputState {
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
    bool slow = false;  // Shift
};

// キーボードを読む（矢印キーと WASD、左右の Shift。requirements.md §1）
[[nodiscard]] InputState read_input();

}  // namespace stg
