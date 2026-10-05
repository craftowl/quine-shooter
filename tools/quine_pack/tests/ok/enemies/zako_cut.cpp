// テストのサンプル（tasks.md の T15c1）。範囲がマスクの # より長く、切り捨てる。1つの範囲に関数を3つ書く
// QUINE-BEGIN zako_cut
Vector2 zako_cut_move(Vector2 p, float t) {
    return Vector2{p.x + t, p.y};
}

float zako_cut_angle(float a) { return a * DEG2RAD
    + PI / 2; }

float zako_cut_scale(float v) {
    return v * 0.5f + 1;
}
// QUINE-END
