#include "core/fixed_step.h"

#include <cassert>

namespace stg {

int FixedStep::advance(float frame_time) {
    assert(frame_time >= 0.0f);

    if (frame_time > MAX_FRAME_TIME) {
        frame_time = MAX_FRAME_TIME;
    }
    accumulator_ += frame_time;

    int steps = 0;
    while (accumulator_ >= FIXED_DT) {
        accumulator_ -= FIXED_DT;
        ++steps;
    }
    return steps;
}

}  // namespace stg
