// この関数は QUINE-BEGIN の外に置く（行の途中なのでマーカーではない）
// QUINE-BEGIN lexical_ok
float lexical_ok(float x) {
    float a = 0 + 1 + 2 + 0.5 + .5 + 0.50 + 2.00 + 1. + 2.0f + .5f + 0.f - 1;
    float b = PI + DEG2RAD + RAD2DEG + atan2f(x, a);
    Vector2 _ = Vector2{a, b};
    return _.x / b;
}
// QUINE-END
