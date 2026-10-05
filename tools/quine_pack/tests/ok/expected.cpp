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
        "   Vector2boss_ok_move(   "
        " Vector2p,floatt){returnV "
        " ector$2{p.x+t,p.y}$;}flo "
        "atboss_ok_angle(floata){re"
        "turn a*DEG2RAD+PI/2;} floa"
        " tbos s_          ok _sca ",
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
        "    floatzak    "
        "  o_ok(floats)  "
        " {re$turns*$0.5 "
        "f;}floatzako_ok("
        " fl  oats){  re "
        "   tu      rn   ",
    },
};

}  // namespace

std::span<const EnemyText> all_enemy_texts() {
    return ENEMY_TEXTS;
}

}  // namespace stg
