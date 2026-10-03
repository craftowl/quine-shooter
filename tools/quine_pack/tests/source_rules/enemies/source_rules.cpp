// 範囲の外の日本語のコメントは違反にならない
// QUINE-BEGIN source_rules
float source_rules(float x) {
    float y = x * 2; →
    return y; // comment
    /* block */ /*/ TODO 3 */
    char c = 'a';
    const char* s = "s";
    int n = 1'0;
    int m = 1\
0;
    /\
/ hidden
    int k= 1;
}
// QUINE-END
