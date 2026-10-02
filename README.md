# 2DShooting

[![Web](https://github.com/craftowl/quine-shooter/actions/workflows/web.yml/badge.svg)](https://github.com/craftowl/quine-shooter/actions/workflows/web.yml)
[![Windows](https://github.com/craftowl/quine-shooter/actions/workflows/windows.yml/badge.svg)](https://github.com/craftowl/quine-shooter/actions/workflows/windows.yml)

**ブラウザで遊ぶ：[https://craftowl.github.io/quine-shooter/](https://craftowl.github.io/quine-shooter/)**

自分のソースコードでできた敵を撃ち崩す弾幕シューティング（C++20 + raylib）。

仕様と設計は [docs/](docs/) にある。

## ビルド（macOS 版）

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

ビルドすると、`assets/`（フォントなど）が実行ファイルの横にコピーされる。ゲームは実行ファイルの場所を基準にファイルを開くので、実行ファイルを別の場所へ移すときは、`assets/` も一緒に移す。

## ビルド（Windows 版）

### 必要なもの

- Visual Studio 2026（C++ によるデスクトップ開発のワークロード）。Build Tools for Visual Studio 2026 でもよい
- CMake 4.2 以上（Visual Studio 2026 のジェネレータ `Visual Studio 18 2026` が使える版）
- git（依存のライブラリを取得するのに使う）
- 最初の設定のときにネットワーク（raylib を GitHub から取得する）

CMake が 4.2 より古いと、設定の段階で「ジェネレータが見つからない」というエラーになる。ジェネレータは CMakeLists.txt を読む前に選ばれるので、CMakeLists.txt では古い CMake を見分けられない。

ビルドを確かめた環境：GitHub Actions の `windows-2025-vs2026`（Visual Studio 2026、CMake 4.4.3）

### 手順

Visual Studio のジェネレータ（既定）は、1つのディレクトリに3つの構成を持つ。構成はビルドのときに `--config` で選ぶ。

```sh
cmake -S . -B build/windows

# Debug（開発用）
cmake --build build/windows --config Debug
.\build\windows\Debug\2DShooting.exe

# RelWithDebInfo（計測とプレイテスト用）
cmake --build build/windows --config RelWithDebInfo

# Release（配布用）
cmake --build build/windows --config Release
```

実行ファイルは `build/windows/<構成>/2DShooting.exe` にでき、その横に `assets/` がコピーされる。C++ のランタイムは静的にリンクするので、開発ツールの入っていない Windows でも、`2DShooting.exe` と `assets/` を一緒に置けば起動できる。Release だけは、起動したときにコンソールのウィンドウを出さない。

## ビルド（Web 版）

### 必要なもの

- macOS 版と同じもの（CMake 3.25 以上、git、ネットワーク）
- Emscripten 6.0.9（emsdk で入れる）

ビルドを確かめた環境：macOS 27.0、Emscripten 6.0.9（emsdk）、CMake 4.2.1

### emsdk を入れる（最初の1回だけ）

`<emsdk の置き場所>` は好きな場所でよい（例：`~/emsdk`）。

```sh
git clone --branch 6.0.9 https://github.com/emscripten-core/emsdk.git <emsdk の置き場所>
cd <emsdk の置き場所>
./emsdk install 6.0.9
./emsdk activate 6.0.9
```

### 手順

emsdk の設定はシェルごとに読み込む必要がある。ビルドするシェルで、先に `emsdk_env.sh` を読み込む。

```sh
source <emsdk の置き場所>/emsdk_env.sh

# Debug（ほかの構成も、デスクトップ版と同じく -DCMAKE_BUILD_TYPE で選ぶ）
emcmake cmake -S . -B build/web-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/web-debug

# 手元のブラウザで開く（サーバーが立ち、ブラウザが開く。終わるときは Ctrl+C）
emrun build/web-debug/2DShooting.html
```

`assets/` は `2DShooting.data` にまとめられる。サーバーに置くときは、`2DShooting.html`、`.js`、`.wasm`、`.data` を同じ場所に置く。

`2DShooting.html` をファイルとして直接開くと、ブラウザが wasm の読み込みを止めることが多い。`emrun` のように、サーバーから開く。

ブラウザが `http://localhost` を開けないとき（常に HTTPS で接続する設定など）は、IP アドレスで開く。

```sh
emrun --no-browser --hostname 127.0.0.1 build/web-debug/2DShooting.html
# ブラウザで http://127.0.0.1:6931/2DShooting.html を開く
```
