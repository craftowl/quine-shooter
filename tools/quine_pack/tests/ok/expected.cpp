// quine_pack が生成したファイル。手で直さない（tools/quine_pack/quine_pack.cmake、ADR 0002）
#include "enemies/enemy_text.h"

namespace stg {

namespace {

constexpr EnemyText ENEMY_TEXTS[] = {
    {
        "boss_ok", 26, 6,
        // マスク
        "...####################..."
        ".########################."
        ".#####$############$#####."
        "##########################"
        "####.################.####"
        ".####.##..........##.####.",
        // テキスト
        "   Vector2 boss_ok_move   "
        " (Vector2 p, float t) { r "
        " eturn$ Vector2{p.x$ + t, "
        " p.y}; } float boss_ok_ang"
        "le(f loat a) { return  a *"
        "  DEG 2R          AD  + P ",
    },
    {
        "zako_ok", 16, 6,
        // マスク
        "....########...."
        "..############.."
        ".###$######$###."
        "################"
        ".##..######..##."
        "...##......##...",
        // テキスト
        "    float za    "
        "  ko_ok(float   "
        " s) ${ retu$rn  "
        "s * 0.5f; } floa"
        " t   zako_o  k( "
        "   fl      oa   ",
    },
};

}  // namespace

std::span<const EnemyText> all_enemy_texts() {
    return ENEMY_TEXTS;
}

}  // namespace stg
