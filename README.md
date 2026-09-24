# 2DShooting

自分のソースコードでできた敵を撃ち崩す弾幕シューティング（C++20 + raylib）。

仕様と設計は [docs/](docs/) にある。

## ビルド（デスクトップ版）

### 必要なもの

- CMake 3.25 以上
- C++20 に対応したコンパイラ
- git（依存のライブラリを取得するのに使う）
- 最初の設定のときにネットワーク（raylib を GitHub から取得する）

ビルドを確かめた環境：macOS 27.0、Apple clang 21.0.0（Command Line Tools 27）、CMake 4.2.1

### 手順

構成（`Debug`、`RelWithDebInfo`、`Release`）ごとに、別のディレクトリでビルドする。

```sh
# Debug（開発用）
cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug
./build/debug/2DShooting

# RelWithDebInfo（計測とプレイテスト用）
cmake -S . -B build/relwithdebinfo -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build/relwithdebinfo

# Release（配布用）
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release
```

`-DCMAKE_BUILD_TYPE` を省くと Debug になる。
