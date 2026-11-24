#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <span>

#include "raylib.h"

#include "enemies/enemy_text.h"

namespace stg {

// 敵1体が持てるマスの数の上限（ボスの上限 60×24。ADR 0002）
inline constexpr std::size_t MAX_ENEMY_CELLS = 60 * 24;

// 敵1体（tasks.md T16 の決定5）。雑魚とボスで同じ型を使う
struct Enemy {
    EnemyText text{};                                 // 体のテキスト（埋め込んだデータを指す）
    Vector2 position{};                               // マスクの左上の角（480×640 の座標）
    std::array<bool, MAX_ENEMY_CELLS> alive_cells{};  // 行 × 列数 ＋ 列 の番号で引く。'#' と '$' だけが初めに true
    int alive_cell_count = 0;                         // alive_cells の true の数
};

// 敵のプール（tasks.md T16 の決定5）。BulletPool と同じく、生きている敵を配列の前に詰めて持ち、
// 消すときは、消す敵の場所に末尾の敵を移して数を1減らす。動かし方は敵ごとに違うので、update は持たず、
// 使う側が enemies() を回して動かす。確保はしない。使う側のメンバーとして、静的な領域に置く（ADR 0007）
template <std::size_t Capacity>
class EnemyPool {
public:
    // 置く。満杯なら置かずに false を返す。text の列数 × 行数は MAX_ENEMY_CELLS 以下
    [[nodiscard]] bool spawn(const EnemyText& text, Vector2 position) {
        assert(text.columns > 0 && text.rows > 0);
        const std::size_t cell_count = static_cast<std::size_t>(text.columns) * static_cast<std::size_t>(text.rows);
        assert(cell_count <= MAX_ENEMY_CELLS && text.mask.size() == cell_count && text.text.size() == cell_count);
        if (count_ == Capacity) {
            return false;
        }
        Enemy& enemy = enemies_[count_];
        enemy.text = text;
        enemy.position = position;
        enemy.alive_cells.fill(false);
        enemy.alive_cell_count = 0;
        for (std::size_t index = 0; index < cell_count; ++index) {
            const char cell = text.mask[index];
            if (cell == '#' || cell == '$') {
                enemy.alive_cells[index] = true;
                ++enemy.alive_cell_count;
            }
        }
        ++count_;
        return true;
    }

    // 条件に当たる敵を消す。敵を前から1体ずつ一度だけ predicate に渡し、true が返った敵を消す
    // （末尾の敵をそこへ移し、移してきた敵も同じ回で渡す）
    template <typename Predicate>
    void remove_if(Predicate predicate) {
        std::size_t index = 0;
        while (index < count_) {
            if (!predicate(static_cast<const Enemy&>(enemies_[index]))) {
                ++index;
                continue;
            }
            // 末尾の敵をここへ移す。移してきた敵はまだ渡していないので、index を進めずに、次の回で渡す
            --count_;
            enemies_[index] = enemies_[count_];
        }
    }

    // すべて消す
    void clear() { count_ = 0; }

    // 生きている敵だけ（配列の前から count() 個）。並びの順は、消すたびに変わる。
    // 書き換えられる形は、動かすときと文字を壊すとき（T16b）に使う
    [[nodiscard]] std::span<Enemy> enemies() { return std::span<Enemy>(enemies_.data(), count_); }
    [[nodiscard]] std::span<const Enemy> enemies() const { return std::span<const Enemy>(enemies_.data(), count_); }

    [[nodiscard]] std::size_t count() const { return count_; }

    [[nodiscard]] static constexpr std::size_t capacity() { return Capacity; }

private:
    std::array<Enemy, Capacity> enemies_{};
    std::size_t count_ = 0;
};

}  // namespace stg
