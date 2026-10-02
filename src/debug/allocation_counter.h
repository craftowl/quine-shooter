#pragma once

// 動的確保の回数を数え、確保してよい場面（起動時とデータの読み込み時）の外での確保を assert で止める（ADR 0007）。
// 数えるのは C++ の operator new による確保だけ。malloc（raylib、ImGui）は数えない

namespace stg::debug {

#ifdef DEBUG

// 確保してよい範囲（データの読み込み）。このオブジェクトがあるスコープの中では確保してよい。
// 入れ子にできる。コピーとムーブはできない
class AllocationAllowed {
public:
    AllocationAllowed();
    ~AllocationAllowed();
    AllocationAllowed(const AllocationAllowed&) = delete;
    AllocationAllowed& operator=(const AllocationAllowed&) = delete;
};

// 確保が OS のライブラリ（ゲームのコードの外）から来たかを判定する関数を渡す。
// main.cpp のプラットフォームの #if の中で作って渡す。渡さなければ、すべてゲームの確保とみなす。
// macOS では、OS のライブラリ（GPU のドライバー、CoreText など）の確保も、置き換えた operator new を通るため
void set_os_allocation_filter(bool (*is_os_allocation)());

// 起動の終わり。run_main_loop の直前に1回呼ぶ。これより後は、AllocationAllowed の外で確保すると違反になる。
// また、これより後は、OS のライブラリの確保を数えない
void end_startup_allocations();

// フレームの最初に呼ぶ。前のフレームの回数を確定する
void begin_frame_allocations();

// 前のフレームの確保回数（確保してよい場面の確保も含む。OS のライブラリの確保は含まない）
[[nodiscard]] int last_frame_allocations();

// 起動後の違反の累計
[[nodiscard]] int allocation_violations();

#else

// Release では何もしない。呼ぶ側を #ifdef DEBUG で囲まずに書けるように置く
class AllocationAllowed {
public:
    AllocationAllowed() {}  // 空のコンストラクタを書いて、使っていない変数の警告を出さない
    AllocationAllowed(const AllocationAllowed&) = delete;
    AllocationAllowed& operator=(const AllocationAllowed&) = delete;
};

#endif  // DEBUG

}  // namespace stg::debug
