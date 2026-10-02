#include "core/input.h"

#include "raylib.h"

namespace stg {

InputState read_input() {
    InputState input;
    input.left = IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A);
    input.right = IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D);
    input.up = IsKeyDown(KEY_UP) || IsKeyDown(KEY_W);
    input.down = IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S);
    input.slow = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    return input;
}

}  // namespace stg
