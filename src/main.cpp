#include "raylib.h"

#include <cassert>
#include <optional>
#include <string>

#include "core/fixed_step.h"
#include "core/input.h"
#include "core/load_errors.h"
#include "core/window.h"
#include "debug/allocation_counter.h"
#include "debug/debug_overlay.h"
#include "debug/params_reloader.h"
#include "game/game.h"
#include "game/params.h"
#include "render/error_screen.h"
#include "render/text_renderer.h"

namespace stg {
namespace {

void frame();        // 1フレーム分の処理（design.md §1）
void error_frame();  // 起動時に読めなかったファイルがあるときの、1フレーム分の処理（tasks.md T07）

}  // namespace
}  // namespace stg

// プラットフォームの違いは、このブロックの中だけに書く（ADR 0004、AGENT.md）
#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
namespace stg {
namespace {

// ブラウザの表示領域に収まる大きさでウィンドウ（canvas）を作り、拡大率を 1/4 単位で返す。
// canvas の中身は実際のピクセルの大きさにし、表示の大きさ（CSS）をその 1/devicePixelRatio にする。
// raylib の高 DPI の設定は Web では効かないので、raylib からは、canvas の中身の大きさのウィンドウに見える
int open_window() {
    const double pixel_ratio = emscripten_get_device_pixel_ratio();
    const int available_width = static_cast<int>(emscripten_run_script_int("window.innerWidth") * pixel_ratio);
    const int available_height = static_cast<int>(emscripten_run_script_int("window.innerHeight") * pixel_ratio);
    const int quarters = choose_scale_quarters(available_width, available_height, static_cast<float>(pixel_ratio));

    const int width = SCREEN_WIDTH * quarters / 4;
    const int height = SCREEN_HEIGHT * quarters / 4;
    InitWindow(width, height, WINDOW_TITLE);
    if (emscripten_set_element_css_size("#canvas", width / pixel_ratio, height / pixel_ratio) != EMSCRIPTEN_RESULT_SUCCESS) {
        TraceLog(LOG_WARNING, "SCREEN: Failed to set the CSS size of the canvas");
    }
    TraceLog(LOG_INFO, "SCREEN: Pixel ratio %.2f", pixel_ratio);
    return quarters;
}

// エラー画面の閉じ方の案内。Web 版には閉じるボタンがないので、タブを閉じるまで出し続ける（ADR 0006 §6）
constexpr const char* QUIT_HINT = "Close the tab to quit.";

// タブを閉じるまで回る
void run_main_loop(void (*frame_func)()) {
    emscripten_set_main_loop(frame_func, 0, 1);
}

#ifdef DEBUG
// Web 版では、OS のライブラリの確保が operator new を通らないので、判定しない（ADR 0007）
constexpr bool (*OS_ALLOCATION_FILTER)() = nullptr;
// Web 版のファイルは .data に埋め込まれ、実行中に変わらないので、ホットリロードをしない（tasks.md の T12）
constexpr bool HOT_RELOAD_AVAILABLE = false;
#endif

}  // namespace
}  // namespace stg
#elif defined(__APPLE__)
#ifdef DEBUG
#include <dlfcn.h>
#include <execinfo.h>
#include <mach-o/dyld.h>
#include <mach-o/getsect.h>

#include <cstdint>
#include <cstring>
#endif
namespace stg {
namespace {

// macOS は、Retina の画面で実際のピクセルで描く
int open_window() {
    return open_desktop_window(true);
}

constexpr const char* QUIT_HINT = "Close the window to quit.";

void run_main_loop(void (*frame_func)()) {
    run_desktop_loop(frame_func);
}

#ifdef DEBUG
// 実行ファイルや共有ライブラリのコード（__TEXT）が置かれたアドレスの範囲
struct CodeRange {
    std::uintptr_t begin = 0;
    std::uintptr_t end = 0;

    [[nodiscard]] bool contains(const void* address) const {
        const auto value = reinterpret_cast<std::uintptr_t>(address);
        return begin <= value && value < end;
    }
};

CodeRange text_range(const mach_header* header) {
    unsigned long size = 0;
    const std::uint8_t* const data = getsegmentdata(reinterpret_cast<const mach_header_64*>(header), "__TEXT", &size);
    if (data == nullptr) {
        return {};
    }
    const auto begin = reinterpret_cast<std::uintptr_t>(data);
    return {begin, begin + size};
}

bool ends_with(const char* text, const char* suffix) {
    const std::size_t text_length = std::strlen(text);
    const std::size_t suffix_length = std::strlen(suffix);
    return text_length >= suffix_length && std::strcmp(text + text_length - suffix_length, suffix) == 0;
}

struct ImageRanges {
    CodeRange game;       // この実行ファイル
    CodeRange libcxx;     // libc++
    CodeRange libcxxabi;  // libc++abi
};

ImageRanges find_image_ranges() {
    ImageRanges ranges;
    Dl_info self{};
    if (dladdr(reinterpret_cast<void*>(&find_image_ranges), &self) != 0) {
        ranges.game = text_range(static_cast<const mach_header*>(self.dli_fbase));
    }
    for (std::uint32_t i = 0; i < _dyld_image_count(); ++i) {
        const char* const name = _dyld_get_image_name(i);
        if (name == nullptr) {
            continue;
        }
        if (ends_with(name, "/libc++.1.dylib")) {
            ranges.libcxx = text_range(_dyld_get_image_header(i));
        } else if (ends_with(name, "/libc++abi.dylib")) {
            ranges.libcxxabi = text_range(_dyld_get_image_header(i));
        }
    }
    return ranges;
}

// 呼び出し履歴の1段が、operator new の中か
enum class CounterFrame { OperatorNew, Other, Unknown };

// dladdr は遅い（この実行ファイルでは名前を順に探すので、1回 10µs ほど）。確保を数える仕組みの中の段のアドレスは
// 数十個しかないので、調べた結果を覚えておく
struct CachedFrame {
    const void* address = nullptr;
    CounterFrame kind = CounterFrame::Unknown;
};
constexpr int FRAME_CACHE_SIZE = 64;
thread_local CachedFrame frame_cache[FRAME_CACHE_SIZE];
thread_local int frame_cache_count = 0;

CounterFrame classify_counter_frame(const void* address) {
    for (int i = 0; i < frame_cache_count; ++i) {
        if (frame_cache[i].address == address) {
            return frame_cache[i].kind;
        }
    }
    Dl_info info{};
    if (dladdr(address, &info) == 0) {
        return CounterFrame::Unknown;
    }
    // dladdr は名前の先頭の _ を1つ除いて返す
    const bool is_operator_new = info.dli_sname != nullptr && (std::strncmp(info.dli_sname, "_Znwm", 5) == 0 ||
                                                               std::strncmp(info.dli_sname, "_Znam", 5) == 0);
    const CounterFrame kind = is_operator_new ? CounterFrame::OperatorNew : CounterFrame::Other;
    if (frame_cache_count < FRAME_CACHE_SIZE) {
        frame_cache[frame_cache_count] = {address, kind};
        ++frame_cache_count;
    }
    return kind;
}

// 確保が OS のライブラリから来たか（ADR 0007）。macOS では、OS のライブラリ（GPU のドライバー、CoreText など）の確保も、
// 置き換えた operator new を通る。呼び出し履歴を operator new からさかのぼり、libc++ を飛ばして、
// 最初に現れたのがこの実行ファイルならゲームの確保、それ以外なら OS の確保とする。
// libc++ を飛ばすのは、Debug では std::string の関数の実体が libc++ の中にあり、そこから operator new を呼ぶため。
// operator new の段を見つけるまでは classify_counter_frame で調べ、その先はアドレスの範囲で比べる
bool is_os_allocation() {
    static const ImageRanges ranges = find_image_ranges();
    constexpr int MAX_FRAMES = 32;
    void* frames[MAX_FRAMES];
    const int count = backtrace(frames, MAX_FRAMES);
    int i = 0;
    // operator new までは、確保を数える仕組みの中。dladdr は近くの名前を返すので、数える処理の段が
    // operator new の名前に見えることがある。operator new に見える段が続くときは、まとめて飛ばす
    bool passed_operator_new = false;
    for (; i < count; ++i) {
        // 数える仕組みの段は、必ずこの実行ファイルの中にある。外の段には dladdr を使わない
        const CounterFrame kind =
            ranges.game.contains(frames[i]) ? classify_counter_frame(frames[i]) : CounterFrame::Other;
        if (kind == CounterFrame::Unknown) {
            return false;  // 分からないときは、ゲームの確保とみなす（違反を見逃さない側に倒す）
        }
        if (kind == CounterFrame::OperatorNew) {
            passed_operator_new = true;
        } else if (passed_operator_new) {
            break;
        }
    }
    for (; i < count; ++i) {
        if (ranges.game.contains(frames[i])) {
            return false;
        }
        if (ranges.libcxx.contains(frames[i]) || ranges.libcxxabi.contains(frames[i])) {
            continue;
        }
        return true;
    }
    return false;
}

constexpr bool (*OS_ALLOCATION_FILTER)() = is_os_allocation;
// 実行ファイルの横の data/ はリポジトリへのリンクなので、編集したファイルを読み直せる（tasks.md の T12）
constexpr bool HOT_RELOAD_AVAILABLE = true;
#endif

}  // namespace
}  // namespace stg
#else
namespace stg {
namespace {

// Windows は高 DPI の設定を使わない。GLFW が DPI に対応したアプリとして動くので、OS は引き伸ばさず、
// モニターの大きさもウィンドウの大きさもピクセルになる（T10 で確かめた。ADR 0008）
int open_window() {
    return open_desktop_window(false);
}

constexpr const char* QUIT_HINT = "Close the window to quit.";

void run_main_loop(void (*frame_func)()) {
    run_desktop_loop(frame_func);
}

#ifdef DEBUG
// Windows で OS のライブラリの確保が operator new を通るかは、確かめていない（T10）。いまは判定しない
constexpr bool (*OS_ALLOCATION_FILTER)() = nullptr;
// 実行ファイルの横の data/ はリポジトリへのリンク（作れなければコピー）なので、読み直せる（tasks.md の T12）
constexpr bool HOT_RELOAD_AVAILABLE = true;
#endif

}  // namespace
}  // namespace stg
#endif

namespace stg {
namespace {

constexpr const char* FONT_FILE = "assets/fonts/JetBrainsMono-Regular.ttf";  // 実行ファイルの場所から（ADR 0006）
constexpr const char* PARAMS_FILE = "data/params.json";                    // 実行ファイルの場所から（ADR 0006）

float screen_scale = 1.0f;  // 480×640 の 1px が実際の画面の何ピクセルか。起動時に決める
FixedStep fixed_step;
render::TextRenderer text_renderer;
LoadErrors load_errors;
Params params;  // 調整値。起動時に読み、Debug と RelWithDebInfo ではホットリロードで書き換わる
Game game;
#ifdef DEBUG
debug::ParamsReloader params_reloader;
#endif

// 調整値を読む（ADR 0006 §6、tasks.md の T12）。起動時に1回だけ読み、ホットリロードにも同じテキストを渡す。
// 読めないか解釈に失敗したら、パスと理由を errors に足す
[[nodiscard]] bool load_params_file(LoadErrors& errors) {
    const std::string full_path = std::string(GetApplicationDirectory()) + PARAMS_FILE;
    char* text = LoadFileText(full_path.c_str());
    if (text == nullptr) {
        TraceLog(LOG_ERROR, "PARAMS: Failed to load %s: File not found or empty", full_path.c_str());
        errors.add(PARAMS_FILE, "File not found or empty");
        return false;
    }
    std::string error;
    const std::optional<Params> loaded = parse_params(text, error);
    if (!loaded.has_value()) {
        TraceLog(LOG_ERROR, "PARAMS: Failed to load %s", full_path.c_str());
        errors.add(PARAMS_FILE, error.c_str());
        UnloadFileText(text);
        return false;
    }
    params = *loaded;
#ifdef DEBUG
    if constexpr (HOT_RELOAD_AVAILABLE) {
        params_reloader.start(full_path.c_str(), text);
    }
#endif
    UnloadFileText(text);
    return true;
}

void render() {
    BeginDrawing();
    ClearBackground(BLACK);
    text_renderer.begin();
    game.draw(text_renderer);
    text_renderer.end();
#ifdef DEBUG
    if constexpr (HOT_RELOAD_AVAILABLE) {
        debug::draw_overlay(params_reloader.last_status());
    } else {
        debug::draw_overlay("");
    }
#endif
    EndDrawing();
}

void frame() {
#ifdef DEBUG
    debug::begin_frame_allocations();
#endif
#ifdef DEBUG
    // 調整値のホットリロード。update() の外、フレームの最初に行う（tasks.md の T12）
    if constexpr (HOT_RELOAD_AVAILABLE) {
        [[maybe_unused]] const debug::ReloadResult reload_result = params_reloader.poll(params);
    }
#endif
    // 入力はフレームの最初に1回読み、このフレームのすべての update() に渡す（design.md §1）
    const InputState input = read_input();
    const int steps = fixed_step.advance(GetFrameTime());
    for (int i = 0; i < steps; ++i) {
        game.update(input, params, FIXED_DT);
    }
    render();
}

// ゲームの処理（update）は回さず、エラー画面を描くだけ
void error_frame() {
    BeginDrawing();
    ClearBackground(BLACK);
    render::draw_error_screen(load_errors, QUIT_HINT, screen_scale);
    EndDrawing();
}

}  // namespace
}  // namespace stg

int main() {
#ifdef DEBUG
    // ログの段階を決めておく（ホットリロードは、一時的に上げた段階をこの値に戻す。tasks.md の T12）
    SetTraceLogLevel(stg::debug::GAME_LOG_LEVEL);
    // OS のライブラリの確保を、ゲームの確保と区別する（ADR 0007）
    stg::debug::set_os_allocation_filter(stg::OS_ALLOCATION_FILTER);
#endif
    // ウィンドウの大きさは起動時に決めて固定する（ADR 0008）
    const int scale_quarters = stg::open_window();
    const float scale = static_cast<float>(scale_quarters) / 4.0f;
    stg::screen_scale = scale;
    TraceLog(LOG_INFO, "SCREEN: Scale %.2f, window %d x %d, render %d x %d", static_cast<double>(scale),
             GetScreenWidth(), GetScreenHeight(), GetRenderWidth(), GetRenderHeight());
    // Esc で終了しない（requirements.md §1）。InitWindow の中で Esc に戻されるので、InitWindow の後に呼ぶ
    SetExitKey(KEY_NULL);

    // 起動時に読むファイル。1つ読めなくても残りを読み続け、読めなかったものをまとめてエラー画面に出す（ADR 0006 §6）。
    // ウェーブ定義は、T20 でここに足す
    const bool font_loaded = stg::text_renderer.load(stg::FONT_FILE, scale, stg::load_errors);
    const bool params_loaded = stg::load_params_file(stg::load_errors);
    const bool loaded = font_loaded && params_loaded;

    if (loaded) {
        stg::game.start(stg::text_renderer, stg::params);
#ifdef DEBUG
        stg::debug::init_overlay();
        // ここで起動が終わる。これより後は、データの読み込みの外で確保しない（ADR 0007）
        stg::debug::end_startup_allocations();
#endif
        stg::run_main_loop(stg::frame);
#ifdef DEBUG
        stg::debug::shutdown_overlay();
#endif
    } else {
        // ゲームの処理を始めずに、閉じるまでエラー画面を出し続ける
        assert(!stg::load_errors.empty());
        TraceLog(LOG_WARNING, "STARTUP: %d file(s) could not be loaded, showing the error screen",
                 static_cast<int>(stg::load_errors.entries().size()));
#ifdef DEBUG
        stg::debug::end_startup_allocations();
#endif
        stg::run_main_loop(stg::error_frame);
    }

    stg::text_renderer.unload();
    CloseWindow();
    return loaded ? 0 : 1;
}
