#pragma once

#include <string>
#include <vector>

namespace stg {

// 起動時に読めなかったファイル1つ分（ADR 0006 §6）
struct LoadError {
    std::string path;    // 実行ファイルの場所からの相対パス
    std::string reason;  // 読めなかった理由（英語）
};

// 起動時の読み込みの失敗を集める。1つ読めなくても残りの読み込みを続け、
// 読めなかったものをまとめてエラー画面に出す（tasks.md T07）。
// 確保は起動時の読み込みの間にだけ起きる（ADR 0007）
class LoadErrors {
public:
    void add(const char* path, const char* reason);
    [[nodiscard]] bool empty() const;
    [[nodiscard]] const std::vector<LoadError>& entries() const;

private:
    std::vector<LoadError> errors_;
};

}  // namespace stg
