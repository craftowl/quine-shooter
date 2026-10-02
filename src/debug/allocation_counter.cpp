#include "debug/allocation_counter.h"

#ifdef DEBUG

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <new>

#include "raylib.h"

namespace stg::debug {
namespace {

// 初めは「起動時」。main() より前のグローバルな変数の初期化も、起動時の確保として扱う
std::atomic<bool> startup_finished{false};
// 1フレームの回数と違反の累計。音のスレッドなど、ほかのスレッドから確保されても数えられるように atomic にする
std::atomic<int> frame_allocations{0};
std::atomic<int> last_frame_count{0};
std::atomic<int> violation_count{0};
// AllocationAllowed の入れ子の深さ。ガードを置いたスレッドの中だけで確保を許す
thread_local int allowed_depth = 0;
// OS のライブラリの確保かを判定する関数（set_os_allocation_filter）
std::atomic<bool (*)()> os_allocation_filter{nullptr};
// 判定の中で起きた確保は数えない（判定を繰り返さない）
thread_local bool in_os_allocation_filter = false;

// 起動の後に、OS のライブラリから来た確保か
bool is_os_allocation_after_startup() {
    if (!startup_finished.load(std::memory_order_relaxed)) {
        return false;
    }
    bool (*const filter)() = os_allocation_filter.load(std::memory_order_relaxed);
    if (filter == nullptr) {
        return false;
    }
    in_os_allocation_filter = true;
    const bool result = filter();
    in_os_allocation_filter = false;
    return result;
}

void count_allocation(std::size_t size) {
    if (in_os_allocation_filter || is_os_allocation_after_startup()) {
        return;
    }
    frame_allocations.fetch_add(1, std::memory_order_relaxed);
    if (!startup_finished.load(std::memory_order_relaxed) || allowed_depth > 0) {
        return;
    }
    violation_count.fetch_add(1, std::memory_order_relaxed);
    TraceLog(LOG_ERROR, "ALLOC: %zu bytes allocated outside startup and data loading (ADR 0007)", size);
    // 呼び出し履歴から、確保したコードが分かる。RelWithDebInfo では止まらず、数えるだけ
    assert(false && "allocation outside startup and data loading");
}

// 確保に失敗したら強制終了する（AGENT.md：メモリ不足は強制終了してよい）。
// 数えるのは、呼ぶ側の operator new で行う（下の「置き換え」）
void* allocate(std::size_t size, bool nothrow) {
    void* const ptr = std::malloc(size == 0 ? 1 : size);
    if (ptr == nullptr && !nothrow) {
        std::abort();
    }
    return ptr;
}

// アラインメント指定版。aligned_alloc は環境によってないので、malloc で多めに取って位置をずらし、
// malloc が返した元の位置を、返す位置の直前に置く
void* allocate_aligned(std::size_t size, std::align_val_t alignment, bool nothrow) {
    const std::size_t align = static_cast<std::size_t>(alignment);
    assert(align != 0 && (align & (align - 1)) == 0);
    const std::size_t extra = align - 1 + sizeof(void*);
    if (size > SIZE_MAX - extra) {
        if (nothrow) {
            return nullptr;
        }
        std::abort();
    }
    void* const raw = std::malloc(size + extra);
    if (raw == nullptr) {
        if (nothrow) {
            return nullptr;
        }
        std::abort();
    }
    const std::uintptr_t start = reinterpret_cast<std::uintptr_t>(raw) + sizeof(void*);
    const std::uintptr_t aligned = (start + align - 1) & ~static_cast<std::uintptr_t>(align - 1);
    void* const ptr = reinterpret_cast<void*>(aligned);
    std::memcpy(static_cast<char*>(ptr) - sizeof(void*), &raw, sizeof(void*));
    return ptr;
}

void deallocate_aligned(void* ptr) {
    if (ptr == nullptr) {
        return;
    }
    void* raw = nullptr;
    std::memcpy(&raw, static_cast<char*>(ptr) - sizeof(void*), sizeof(void*));
    std::free(raw);
}

}  // namespace

AllocationAllowed::AllocationAllowed() {
    ++allowed_depth;
}

AllocationAllowed::~AllocationAllowed() {
    assert(allowed_depth > 0);
    --allowed_depth;
}

void set_os_allocation_filter(bool (*is_os_allocation)()) {
    os_allocation_filter.store(is_os_allocation, std::memory_order_relaxed);
}

void end_startup_allocations() {
    assert(!startup_finished.load(std::memory_order_relaxed));
    startup_finished.store(true, std::memory_order_relaxed);
}

void begin_frame_allocations() {
    last_frame_count.store(frame_allocations.exchange(0, std::memory_order_relaxed), std::memory_order_relaxed);
}

int last_frame_allocations() {
    return last_frame_count.load(std::memory_order_relaxed);
}

int allocation_violations() {
    return violation_count.load(std::memory_order_relaxed);
}

}  // namespace stg::debug

// グローバルな operator new と operator delete の置き換え。名前空間の外に置く決まり（C++ の規格）
// 解放は数えない（ADR 0007 が禁じているのは確保だけ）。
// 数える処理は operator new の中で直接呼ぶ。OS の確保かの判定（main.cpp）は、呼び出し履歴の operator new の段を目印にする。
// 確保の処理の中で数えると、最適化で operator new からの末尾呼び出しになったとき、operator new の段が消える

void* operator new(std::size_t size) {
    stg::debug::count_allocation(size);
    return stg::debug::allocate(size, false);
}

void* operator new[](std::size_t size) {
    stg::debug::count_allocation(size);
    return stg::debug::allocate(size, false);
}

void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
    stg::debug::count_allocation(size);
    return stg::debug::allocate(size, true);
}

void* operator new[](std::size_t size, const std::nothrow_t&) noexcept {
    stg::debug::count_allocation(size);
    return stg::debug::allocate(size, true);
}

void* operator new(std::size_t size, std::align_val_t alignment) {
    stg::debug::count_allocation(size);
    return stg::debug::allocate_aligned(size, alignment, false);
}

void* operator new[](std::size_t size, std::align_val_t alignment) {
    stg::debug::count_allocation(size);
    return stg::debug::allocate_aligned(size, alignment, false);
}

void* operator new(std::size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept {
    stg::debug::count_allocation(size);
    return stg::debug::allocate_aligned(size, alignment, true);
}

void* operator new[](std::size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept {
    stg::debug::count_allocation(size);
    return stg::debug::allocate_aligned(size, alignment, true);
}

void operator delete(void* ptr) noexcept {
    std::free(ptr);
}

void operator delete[](void* ptr) noexcept {
    std::free(ptr);
}

void operator delete(void* ptr, const std::nothrow_t&) noexcept {
    std::free(ptr);
}

void operator delete[](void* ptr, const std::nothrow_t&) noexcept {
    std::free(ptr);
}

void operator delete(void* ptr, std::size_t) noexcept {
    std::free(ptr);
}

void operator delete[](void* ptr, std::size_t) noexcept {
    std::free(ptr);
}

void operator delete(void* ptr, std::align_val_t) noexcept {
    stg::debug::deallocate_aligned(ptr);
}

void operator delete[](void* ptr, std::align_val_t) noexcept {
    stg::debug::deallocate_aligned(ptr);
}

void operator delete(void* ptr, std::align_val_t, const std::nothrow_t&) noexcept {
    stg::debug::deallocate_aligned(ptr);
}

void operator delete[](void* ptr, std::align_val_t, const std::nothrow_t&) noexcept {
    stg::debug::deallocate_aligned(ptr);
}

void operator delete(void* ptr, std::size_t, std::align_val_t) noexcept {
    stg::debug::deallocate_aligned(ptr);
}

void operator delete[](void* ptr, std::size_t, std::align_val_t) noexcept {
    stg::debug::deallocate_aligned(ptr);
}

#endif  // DEBUG
