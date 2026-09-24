#pragma once

namespace stg {

// update() に渡す固定の時間（ADR 0003）
inline constexpr float FIXED_DT = 1.0f / 60.0f;

// 1フレームで消化する時間の上限（ADR 0003）
inline constexpr float MAX_FRAME_TIME = 0.25f;

// 溜まった時間から、そのフレームで update() を回す回数を決める
class FixedStep {
public:
    // frame_time 秒だけ時間を進め、このフレームで update() を回す回数を返す。
    // frame_time は MAX_FRAME_TIME で打ち切る
    [[nodiscard]] int advance(float frame_time);

private:
    float accumulator_ = 0.0f;
};

}  // namespace stg
