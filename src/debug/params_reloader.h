#pragma once

#ifdef DEBUG

#include "raylib.h"

#include <string>

#include "game/params.h"

namespace stg::debug {

// ゲームのログの段階。起動時に SetTraceLogLevel で設定する。raylib 6.0 には今の段階を取る関数がないので、
// ホットリロードで一時的に段階を上げたあと、この値に戻す（tasks.md の T12）
inline constexpr int GAME_LOG_LEVEL = LOG_INFO;

// poll の結果。成功と失敗のほかに「変わっていない」も返す（ADR 0006）
enum class ReloadResult { Unchanged, Reloaded, Failed };

// params.json のホットリロード（tasks.md の T12）。0.5 秒ごとに中身を読み、前に読んだ中身と違えば、
// 読んだテキストをそのまま parse_params に渡す。読み直しは確保してよい範囲の中で行う（ADR 0007）
class ParamsReloader {
public:
    // 起動時に1回。path（実行ファイルの場所を含むパス）と、起動時に main.cpp が読んで解釈したテキストを覚える。
    // ファイルは読まないので、失敗しない
    void start(const char* path, const char* text);
    // フレームの最初（update() の外）に呼ぶ。0.5 秒ごとに中身を読み、前に読んだ中身と違えば（または前に読めなかったら）
    // 読み直す。読み直しに成功したら params を書き換えて Reloaded。失敗したら params はそのままで Failed。
    // 読めないとき（ファイルがない、または空）も Failed
    [[nodiscard]] ReloadResult poll(Params& params);
    // 最後の読み直しの結果（英語）。まだなければ空文字列
    [[nodiscard]] const char* last_status() const;

private:
    std::string path_;
    std::string last_text_;      // 前に読んだ中身（読み直しに失敗した中身も覚える）
    bool last_unreadable_ = false;  // 前に読んだとき、読めなかったか
    double next_check_time_ = 0.0;
    std::string status_;
};

}  // namespace stg::debug

#endif  // DEBUG
