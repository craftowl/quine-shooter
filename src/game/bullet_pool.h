#pragma once

#include <array>
#include <cstddef>
#include <span>

#include "raylib.h"

namespace stg {

// 弾1つ（tasks.md T14 の決定3）。見た目（文字と色）は持たず、描く側が決める
struct Bullet {
    Vector2 position{};  // 弾の文字の中心（480×640 の座標）
    Vector2 velocity{};  // px/秒
};

// 弾のプール（tasks.md T14 の決定2・3・5・10・12）。自機の弾と敵弾で共通に使い、容量は Capacity で変える。
// 生きている弾を配列の前に詰めて持ち、消すときは、消す弾の場所に末尾の弾を移して数を1減らす。
// 確保はしない。使う側のメンバーとして、静的な領域に置く（ADR 0007）
template <std::size_t Capacity>
class BulletPool {
public:
    // 撃つ。満杯なら撃たずに false を返す（満杯は起こりうるので、バグとして止めない。ADR 0006）
    [[nodiscard]] bool spawn(Vector2 position, Vector2 velocity) {
        if (count_ == Capacity) {
            return false;
        }
        bullets_[count_] = Bullet{position, velocity};
        ++count_;
        return true;
    }

    // dt 秒ぶん動かし、中心が alive_area の外に出た弾を消す。中心がちょうど辺の上にある弾は消さない
    void update(float dt, const Rectangle& alive_area) {
        std::size_t index = 0;
        while (index < count_) {
            Bullet& bullet = bullets_[index];
            bullet.position.x += bullet.velocity.x * dt;
            bullet.position.y += bullet.velocity.y * dt;
            if (is_inside(bullet.position, alive_area)) {
                ++index;
                continue;
            }
            // 末尾の弾をここへ移す。移してきた弾はまだ動かしていないので、index を進めずに、次の回で動かして確かめる
            --count_;
            bullets_[index] = bullets_[count_];
        }
    }

    // すべて消す
    void clear() { count_ = 0; }

    // 生きている弾だけ（配列の前から count() 個）。並びの順は、消すたびに変わる
    [[nodiscard]] std::span<const Bullet> bullets() const { return std::span<const Bullet>(bullets_.data(), count_); }

    [[nodiscard]] std::size_t count() const { return count_; }

    [[nodiscard]] static constexpr std::size_t capacity() { return Capacity; }

private:
    [[nodiscard]] static bool is_inside(Vector2 position, const Rectangle& area) {
        return position.x >= area.x && position.x <= area.x + area.width && position.y >= area.y &&
               position.y <= area.y + area.height;
    }

    std::array<Bullet, Capacity> bullets_{};
    std::size_t count_ = 0;
};

}  // namespace stg
