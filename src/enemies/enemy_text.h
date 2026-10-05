#pragma once

#include <optional>
#include <span>
#include <string_view>

namespace stg {

// 敵の体に表示するテキスト（ADR 0002）。ビルドのときに quine_pack（tools/quine_pack/quine_pack.cmake）が、
// QUINE 範囲のコードをマスクに流し込んで作る
struct EnemyText {
    std::string_view id;    // 敵 ID
    int columns;            // マスクの列数
    int rows;               // マスクの行数
    std::string_view mask;  // columns*rows 文字。'#' '$' '.'（行の区切りなし）
    std::string_view text;  // columns*rows 文字。表示する文字。'#' はコードの文字、'$' は '$'、'.' は空白
};

// 埋め込んだすべての敵。定義は、ビルドのときに生成する enemy_text_data.cpp にある
[[nodiscard]] std::span<const EnemyText> all_enemy_texts();

// id の敵。なければ、TraceLog に id を出して nullopt を返す（ADR 0006）
[[nodiscard]] std::optional<EnemyText> find_enemy_text(std::string_view id);

}  // namespace stg
