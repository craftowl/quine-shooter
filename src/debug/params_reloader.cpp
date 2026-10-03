#include "debug/params_reloader.h"

#ifdef DEBUG

#include <cstring>

#include "debug/allocation_counter.h"

namespace stg::debug {
namespace {

constexpr double CHECK_INTERVAL = 0.5;  // 中身を読む間隔（秒）

}  // namespace

void ParamsReloader::start(const char* path, const char* text) {
    path_ = path;
    last_text_ = text;
    last_unreadable_ = false;
    next_check_time_ = GetTime() + CHECK_INTERVAL;
    status_.clear();
}

ReloadResult ParamsReloader::poll(Params& params) {
    const double now = GetTime();
    if (now < next_check_time_) {
        return ReloadResult::Unchanged;
    }
    next_check_time_ = now + CHECK_INTERVAL;

    // LoadFileText は読めるたびに INFO のログを出すので、読む間だけ段階を上げて、raylib のログを出さない
    SetTraceLogLevel(LOG_ERROR);
    char* text = LoadFileText(path_.c_str());
    SetTraceLogLevel(GAME_LOG_LEVEL);

    // 読めない（ファイルがない、または空）。ログは、読めなくなったときに1回だけ出す
    if (text == nullptr) {
        if (!last_unreadable_) {
            last_unreadable_ = true;
            AllocationAllowed allow;  // 表示の文を作る確保（データの読み込みの一部。ADR 0007）
            status_ = "Failed: params.json could not be read (missing or empty)";
            TraceLog(LOG_WARNING, "PARAMS: Reload failed: %s could not be read (missing or empty)", path_.c_str());
        }
        return ReloadResult::Failed;
    }

    // 前に読んだ中身と同じで、前は読めていたなら、何もしない（確保もしない）
    const bool unchanged = !last_unreadable_ && std::strcmp(text, last_text_.c_str()) == 0;
    if (unchanged) {
        UnloadFileText(text);
        return ReloadResult::Unchanged;
    }

    // 中身が変わったか、読めなかったあとに読めた。読んだテキストをそのまま解釈する。
    // 失敗した中身も覚え、次に中身が変わるまで読み直さない
    AllocationAllowed allow;  // 読み直しはデータの読み込み（ADR 0007）
    last_unreadable_ = false;
    last_text_ = text;
    UnloadFileText(text);
    std::string error;
    const std::optional<Params> loaded = parse_params(last_text_.c_str(), error);
    if (!loaded.has_value()) {
        status_ = "Failed: " + error;
        TraceLog(LOG_WARNING, "PARAMS: Reload failed, keeping the previous values: %s", error.c_str());
        return ReloadResult::Failed;
    }
    params = *loaded;
    status_ = "Reloaded";
    TraceLog(LOG_INFO, "PARAMS: Reloaded %s", path_.c_str());
    return ReloadResult::Reloaded;
}

const char* ParamsReloader::last_status() const {
    return status_.c_str();
}

}  // namespace stg::debug

#endif  // DEBUG
