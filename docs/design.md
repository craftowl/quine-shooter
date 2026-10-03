# 設計（design.md）

> 状態: 骨子（2026-09-22）。雛形の作成に必要な部分だけを決めてある。「未決」の項目は実装が進んだ週に決める。2026-09-23 に、AGENT.md の見直しに合わせて §2〜§5 を更新した。同日、文書の見直し（ADR 0002 の更新など）に合わせて §4 と §5 を更新した。2026-09-24 に、ゲームの規則（requirements.md §1）に合わせて、§5 に調整値の一覧を足した。同日、§2 のコード例に `SetExitKey(KEY_NULL)` を足した（raylib 6.0 では、何もしないと Esc で `WindowShouldClose()` が真になり、requirements.md §1 の「Esc ではゲームを終了しない」に反するため）。また、§5 の当たり判定を決める時期を「敵弾を作るとき」から「自機の弾と敵の文字の判定を作るとき」に変えた（当たり判定が最初に要るのは、敵弾より先に作る自機の弾と敵の文字の判定のため）。同じく、§5 の押した瞬間の入力の扱いを決める時期を「入力の処理を作るとき」から「押した瞬間の入力を最初に使うとき（シーン遷移）」に変えた（最初に作る入力の処理は自機の移動で、押し続けている間しか使わないため）。T01 の実装のあと、§2 のコード例の関数を、グローバルな `static` 関数から名前空間 `stg` の中の無名名前空間に移した（AGENT.md の「すべてのコードを名前空間 `stg` に入れる」に合わせるため）。T02 で、§3 に `cmake/check_sources.cmake` を足した。T05 の前に、§5 のウィンドウの大きさの決め方と Web 版でファイルを開く基準を決め（人が決めた）、§3 に `assets/README.md` を足した。2026-09-25 に、§5 の高 DPI の試行にかける時間の上限を決めた（人が決めた）。2026-10-02 に、§1 の1フレームの流れを、フレームの最初に読んだ入力を `update()` に渡す形に直した（T09。人が採用した）。同日、§5 の Windows 版をビルドするコンパイラを MSVC に決めた（T10。人が決めた）。同日、§5 の調整値の JSON の形、ホットリロードの方式、失敗したときの知らせ方を決め、params.json のキーの表を足した（T12。人が決めた）。2026-10-03 に、§4 の quine_pack を何で書くかと、動かすタイミング、埋め込みの形、マスクの扱いを決めた（T15。人が決めた）。

## 1. ゲームループ

更新は 1/60 秒の固定間隔、描画は毎フレーム。詳細は [ADR 0003](decisions/0003-game-loop.md)。

1フレームの流れ:

1. 入力を読む（`read_input()` が `InputState` を返す）
2. 溜まった時間の分だけ `update(input, FIXED_DT)` を回す。どの回にも、1 で読んだ同じ入力を渡す
3. `render()` で描く

## 2. プラットフォームの切り替え

デスクトップと Web ではメインループの形が違う。この差は `main.cpp` の先頭にある1つの `#if` に閉じ込める（[ADR 0004](decisions/0004-platforms.md)、AGENT.md）。

```cpp
// main.cpp（main() のほかは、名前空間 stg に入れる。AGENT.md）
namespace stg {
namespace {
void frame();   // 1フレーム分の処理（上の 1〜3）
}
}

// プラットフォームの違いは、このブロックの中だけに書く
#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
namespace stg {
namespace {
void run_main_loop() { emscripten_set_main_loop(frame, 0, 1); }
}
}
#else
namespace stg {
namespace {
void run_main_loop() {
    SetTargetFPS(60);
    while (!WindowShouldClose()) frame();
}
}
}
#endif

int main() {
    // ウィンドウの大きさは起動時に決めて固定する（ADR 0008）
    InitWindow(window_width, window_height, "...");
    // Esc で終了しない（requirements.md §1）。InitWindow の中で終了キーが Esc に戻されるので、InitWindow の後に呼ぶ
    SetExitKey(KEY_NULL);
    stg::run_main_loop();
    CloseWindow();
}
```

## 3. ディレクトリ構成

```
2DShooting/
  CMakeLists.txt
  README.md
  cmake/
    check_sources.cmake   # ソースの検査（AGENT.md の「検査」）。ビルドのたびに実行する
  .gitignore
  .github/workflows/      # Windows 版と Web 版のビルド、Web 版の公開（GitHub Pages）
  docs/
    AGENT.md              # プロジェクト原則
    requirements.md       # 何を作るか
    design.md             # このファイル
    tasks.md              # 実装タスク
    decisions/            # ADR
    verification/         # 受け入れ基準の検証ログ
  src/
    main.cpp              # 起動とメインループ
    core/                 # 時間、入力、固定ステップ
    render/               # 文字の描画
    game/                 # シーン、自機、弾、当たり判定
    enemies/              # 敵の挙動（QUINE マーカーで囲む）
    debug/                # デバッグ機能（Debug と RelWithDebInfo のみ）
  data/
    params.json           # 調整値
    waves/                # ウェーブ定義
    masks/                # 敵の形のマスク（*.txt）
  assets/
    README.md             # 素材の出典とライセンスの一覧
    fonts/                # JetBrains Mono と OFL.txt
    sounds/
  tools/
    quine_pack/           # ビルド時に QUINE 範囲を切り出して検査し、マスクに流し込むツール（CMake スクリプト）と、そのテスト
  web/
    shell.html            # Web 版の HTML テンプレート
```

## 4. Quine の敵の生成（ビルド時）

1. `src/enemies/` の `.h` と `.cpp`（サブディレクトリを含む）から `QUINE-BEGIN` と `QUINE-END` の行にはさまれた範囲を切り出す（マーカーの行は含めない）。同じ敵 ID の範囲が2つあれば、ビルドエラーにする
2. 範囲の中を検査する。ASCII 以外の文字、制御文字（タブ、CR、LF のほか）、コメント、文字列と文字のリテラル、`\`、認めていない数値や大文字の名前があれば、ビルドエラーにする。敵のソースとマスクの先頭の BOM も、ビルドエラーにする
3. `data/masks/<敵ID>.txt` を検査する。範囲とマスクが1対1で対応しているか、使える文字、行の長さ、`#` の数、大きさの上限（雑魚かボスかは敵 ID で決まる）のどれかに違反があれば、ビルドエラーにする
4. 空白・タブ・改行が続くところを空白1つに置き換え、範囲の最初と最後の空白を取り除く
5. マスクの `#` に詰める
6. 結果をビルドディレクトリに生成する `.cpp` に書き、実行ファイルに埋め込む

ルールの詳細は [ADR 0002](decisions/0002-quine-enemy-text.md)。

決定（2026-10-03）：`tools/quine_pack/quine_pack.cmake`（CMake スクリプト）で書き、入力（`src/enemies/` のソースと `data/masks/` のマスク）が変わったときだけ動かす。C++ で書くと、Web 版のビルド（emcmake）では em++ でコンパイルされるので、ビルドの途中でそのままでは実行できない。CMake スクリプトなら、この問題は起きず、依存も増えない。`src/enemies/` の外の QUINE マーカーは、`cmake/check_sources.cmake` が止める。埋め込みの型と関数の形、テストのしかたは [tasks.md](tasks.md) の T15。

マスクは実行時には読まない。直したらビルドし直す（ホットリロードでは変わらない）。`data/` ごと配布物に入るのは許す。

## 5. 未決の設計

「決める時期」の列に書いた時期に決める。

| 項目 | 決める時期 | 候補 |
| --- | --- | --- |
| ウィンドウの大きさの決め方（デスクトップと Web。[ADR 0008](decisions/0008-screen-scaling.md)） | 決定（2026-09-24）：画面（デスクトップはモニターの幅と高さの 90%、Web はブラウザの表示領域）に収まる最大の大きさ。拡大率は 0.25 刻みで切り下げ、最小 1.0。詳しくは [tasks.md](tasks.md) の T05 | — |
| 高 DPI の試行にかける時間の上限（[ADR 0008](decisions/0008-screen-scaling.md)） | 決定（2026-09-25）：半日（AI の実装と人の確認を合わせて 4 時間）。詳しくは [tasks.md](tasks.md) の T06 | — |
| Web 版でファイルを開く基準（[ADR 0006](decisions/0006-error-handling.md) の「実行ファイルの場所を基準に開く」が、Web では成り立たない） | 決定（2026-09-24）：`GetApplicationDirectory()` の結果をそのまま使う（Web では `"/"` を返す）。ADR 0006 §6 と AGENT.md に書いた | — |
| 書式（インデントなど） | 決定（2026-09-24）：AI が決めて完了報告に書く。clang-format は入れない | — |
| 押した瞬間の入力の扱い（`update()` が0回や2回呼ばれるフレーム） | 押した瞬間の入力を最初に使うとき（シーン遷移） | 押した瞬間を次の `update()` まで持ち越す、`update()` ごとに押しているかどうかの変化から判定する |
| Windows 版をビルドするコンパイラ（[ADR 0004](decisions/0004-platforms.md)。例外と警告の設定に関係する。決めたら、コンパイラの最低バージョンを [ADR 0001](decisions/0001-tech-stack.md) に書く） | 決定（2026-10-02）：MSVC（Visual Studio 2026 以上）。ジェネレータは Visual Studio。詳しくは [tasks.md](tasks.md) の T10 | — |
| シーン管理 | シーン遷移を作るとき | `std::variant`、仮想関数、列挙型と switch |
| エンティティ管理 | 弾を作るとき | 種類ごとのプール、ECS 風の配列、継承（[ADR 0007](decisions/0007-dynamic-allocation.md) により、出現のたびに `new` する形は使えない。先に確保したオブジェクトを使う） |
| 当たり判定 | 自機の弾と敵の文字の判定を作るとき | 円と円、空間分割（グリッド）の有無 |
| 描画の分離 | 描画が増えてきたとき | 描画コマンドのキュー、各オブジェクトの `draw()` |
| `Draw*` を呼んでよいディレクトリ（検査に加える） | 描画の分離を決めたとき | 描画の分離の方式に合わせて決める |
| 調整値の JSON の形（乱数のシードの置き場所を含む）とホットリロードの方式 | 決定（2026-10-02）：種類ごとの入れ子、キーは snake_case、座標は `{"x", "y"}`、単位と範囲は下の表。シードは `random.seed`。ホットリロードは 0.5 秒ごとに中身を読み、前と違えば読み直す（Debug と RelWithDebInfo のみ。Web 版はしない）。詳しくは [tasks.md](tasks.md) の T12 | — |
| ホットリロードに失敗したときの知らせ方 | 決定（2026-10-02）：前の値を使い続け、ログとデバッグ表示に出す。詳しくは [tasks.md](tasks.md) の T12 | — |

### 調整値の JSON に置く値

[requirements.md](requirements.md) §1「ゲームの流れと規則」で、JSON に書くと決めた値。ファイルの分け方やキーの名前は、上の表の「調整値の JSON の形」で決める。

- 自機：残機の数、復活したあとの無敵の時間、当たり判定の大きさ、通常と低速の速さ、開始位置（復活の位置と同じ。T12 で足した）
- ショット：段階の数、各段階の弾の並び
- アイテム：落ちる速さ、取れる範囲の大きさ、段階が上限のときに取ったときの点
- 進行：ウェーブの制限時間、「STAGE n」と「ALL CLEAR」の表示時間
- 敵ごと：HP、文字を1つ壊したときの点、倒したときの点、挙動の値（移動や弾の速さなど。[ADR 0002](decisions/0002-quine-enemy-text.md) のルール5）
- ボス：攻撃が変わる HP の割合
- そのほか：音量、乱数のシード（AGENT.md）

ウェーブの中身（どの敵がどこに出るか）は、調整値ではなくウェーブ定義（`data/waves/`）に書く。

### params.json のキー

`data/params.json` の形の決まり（2026-10-02 に決めた。[tasks.md](tasks.md) の T12）。

- 上の一覧の分け方で入れ子にする（`player`、`shot`、`item`、`progress`、`enemies`、`boss`、`audio`、`random`）。一覧の「そのほか」は、音量を `audio`、乱数のシードを `random` に分ける。キーは snake_case
- 座標は `{"x": …, "y": …}`（480×640 の座標）
- 値は、使うタスクで足す。足すときは、この表に単位と範囲を書く。表にあるキーは、すべて必須
- 乱数のシードは `random.seed` に置く（T12 で置き場所だけ決めた）。キーと表の行は T13 で足す
- キーがない、型が違う、範囲の外のどれかなら、読み込みの失敗にする。知らないキーは、ログに警告を出すだけにする

| キー | 型 | 単位 | 範囲 | 足したタスク |
| --- | --- | --- | --- | --- |
| `player.normal_speed` | 数 | px/秒（480×640 の座標） | 0 より大きく 4800 以下 | T12 |
| `player.slow_speed` | 数 | px/秒（Shift を押している間） | 0 より大きく 4800 以下 | T12 |
| `player.start_position` | 座標 | 480×640 の座標（当たり判定の中心。復活の位置も同じ） | x は 0〜480、y は 0〜640（端を含む） | T12 |
