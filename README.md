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

ビルドすると、`assets/`（フォントなど）と `data/`（調整値の `params.json` など）が実行ファイルの横に置かれる。ゲームは実行ファイルの場所を基準にファイルを開くので、実行ファイルを別の場所へ移すときは、`assets/` と `data/` も一緒に移す。

Debug と RelWithDebInfo では、実行ファイルの横の `data/` はリポジトリの `data/` へのシンボリックリンクになる。ゲームを動かしたまま `data/params.json` を書き換えて保存すると、0.5 秒ほどで読み直す（ホットリロード）。Release では `data/` はコピーで、ホットリロードはしない。

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

実行ファイルは `build/windows/<構成>/2DShooting.exe` にでき、その横に `assets/` と `data/` が置かれる。C++ のランタイムは静的にリンクするので、開発ツールの入っていない Windows でも、`2DShooting.exe` と `assets/` と `data/` を一緒に置けば起動できる。Debug と RelWithDebInfo の `data/` は、シンボリックリンクを作れればリポジトリの `data/` へのリンクになる（開発者モードでないと作れない。そのときはコピーになり、設定の段階で警告が出る）。Release だけは、起動したときにコンソールのウィンドウを出さない。

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

`assets/` と `data/` は `2DShooting.data` にまとめられる（Web 版ではホットリロードをしない）。サーバーに置くときは、`2DShooting.html`、`.js`、`.wasm`、`.data` を同じ場所に置く。

`2DShooting.html` をファイルとして直接開くと、ブラウザが wasm の読み込みを止めることが多い。`emrun` のように、サーバーから開く。

ブラウザが `http://localhost` を開けないとき（常に HTTPS で接続する設定など）は、IP アドレスで開く。

```sh
emrun --no-browser --hostname 127.0.0.1 build/web-debug/2DShooting.html
# ブラウザで http://127.0.0.1:6931/2DShooting.html を開く
```

## ボスの体の Quine

ボス（`boss_` で始まる ID の敵）の体のテキストは、`quines/<敵ID>.cpp` に置いた本物の C++ の Quine になっている。コンパイルして実行すると、行末の空白も含めて1バイトも違わない自分を出力する（[ADR 0009](docs/decisions/0009-boss-quine.md)）。Quine は、生成ツール `tools/quine_gen/` が、マスク `data/masks/<敵ID>.txt` から作る。ゲームは、Quine のファイルをそのまま体のテキストとして埋め込む。

手順は macOS と Linux の分だけを書く。どれもリポジトリの直下で打つ。

確かめた環境（2026-11-16）：macOS 27.2、Apple clang 21.0.0（Command Line Tools 27.2）、CMake 4.4.5。CI（GitHub Actions の `ubuntu-24.04` の g++。2026-11-17 に g++ 13.3.0 で確かめた）でも、`main` に push するたびに下の「まとめて確かめる」を動かす（手動でも動かせる）。

### 1つの Quine を確かめる

```sh
mkdir -p build/quine_try
# コンパイルして実行し、出力を元のファイルと比べる（cmp が何も出さなければ一致）
c++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -fno-exceptions -Wno-misleading-indentation quines/boss_dummy.cpp -o build/quine_try/boss_dummy
./build/quine_try/boss_dummy > build/quine_try/boss_dummy.cpp
cmp quines/boss_dummy.cpp build/quine_try/boss_dummy.cpp

# 出力をもう一度コンパイルして実行しても、同じものを出力する
c++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -fno-exceptions -Wno-misleading-indentation build/quine_try/boss_dummy.cpp -o build/quine_try/boss_dummy2
./build/quine_try/boss_dummy2 | cmp quines/boss_dummy.cpp -
```

コンパイルの設定はゲームと同じで、形に並べたことで避けられない字下げの警告（`-Wno-misleading-indentation`）だけを外している。設定の元は `tools/quine_gen/check_quine.cmake` の `QUINE_FLAGS` で、上のコマンドはそれを書き写したもの（変えるときは、元を先に変える）。

### まとめて確かめる

```sh
cmake -P tools/quine_gen/check_quine.cmake
```

生成ツールを `build/quine_gen` に作り、`quines/` のすべての `.cpp` について、マスクから生成し直すと同じになることと、上と同じ設定でコンパイルして実行すると自分を出力することを確かめる。最後に `check_quine: 1 件すべて合った` のように出れば成功で、合わなければ、どのファイルのどの確かめで合わなかったかを出して失敗する。

### 生成し直す

マスクを変えたら、Quine を生成し直す。生成し直さないと、形が合わないので、ゲームのビルドが quine_pack で止まる。

```sh
cmake -S tools/quine_gen -B build/quine_gen
cmake --build build/quine_gen
./build/quine_gen/quine_gen data/masks/boss_dummy.txt quines/boss_dummy.cpp
```

Quine は、行末の空白も中身の一部になっている。エディタで開いて保存するときに、行末の空白を消したり改行を変えたりしないようにする（`quines/.editorconfig` と `.gitattributes` で守っている）。
