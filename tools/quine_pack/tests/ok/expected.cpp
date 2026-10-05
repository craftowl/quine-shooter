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
        "   R\"(@\\\"'\\\\#%&*+,-./:;   "
        " <=>?[]^_`{|}~)\";int(main "
        " )(){r$eturn(0);}R\"$(@\\\"' "
        "\\\\#%&*+,-./:;<=>?[]^_`{|}~"
        ")\";i nt(main)(){retur n(0)"
        " ;}R\" (@          \\\" '\\\\# ",
    },
    {
        "zako_cut", 12, 4,
        // マスク
        "..########.."
        ".####$$####."
        "############"
        ".##.####.##.",
        // テキスト
        "  Vector2z  "
        " ako_$$cut_ "
        "move(Vector2"
        " p, floa tt ",
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
