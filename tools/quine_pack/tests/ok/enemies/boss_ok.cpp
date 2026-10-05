// テスト用の正しい入力（ボス）。1つの範囲の複数の関数と、切り捨てを確かめる
// QUINE-BEGIN boss_ok
Vector2 boss_ok_move(Vector2 p, float t) {
    return Vector2{p.x + t, p.y};
}

float boss_ok_angle(float a) { return a * DEG2RAD
    + PI / 2; }

float boss_ok_scale(float v) {
    return v * 0.5f + 1;
}
// QUINE-END
