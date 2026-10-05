#include "enemies/zako_dummy.h"

namespace stg {

// 仮の雑魚の挙動（tasks.md の T15b）。体のテキストは、この範囲を data/masks/zako_dummy.txt に流し込んだもの
// QUINE-BEGIN zako_dummy
Vector2 zako_dummy_velocity(float speed) {
    return Vector2{0, speed};
}
// QUINE-END

}  // namespace stg
