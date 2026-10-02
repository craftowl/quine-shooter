# ADR 0003: ゲームループの方式

- 日付: 2026-09-22
- 状態: 採用
- 更新: 2026-09-23（コード例の変数名を、design.md の関数 `frame()` と重ならない名前に変えた）
- 更新: 2026-10-02（コード例を、フレームの最初に読んだ入力を `update()` に渡す形に直した。T09）

## 決定

- 更新は **1/60 秒の固定間隔**、描画は毎フレーム行う
- 1フレームで消化する時間の上限を **0.25 秒** とする
- 描画位置の補間はしない

```cpp
constexpr float FIXED_DT = 1.0f / 60.0f;
constexpr float MAX_FRAME_TIME = 0.25f;

const InputState input = read_input();  // 入力はフレームの最初に1回だけ読む
float frame_time = GetFrameTime();
if (frame_time > MAX_FRAME_TIME) frame_time = MAX_FRAME_TIME;
accumulator += frame_time;
while (accumulator >= FIXED_DT) {
    update(input, FIXED_DT);  // どの回にも同じ入力を渡す
    accumulator -= FIXED_DT;
}
render();
```

## 理由

- 弾幕STGは当たり判定が遊びの中心になる。更新間隔が一定なら、フレームレートがぶれても弾の動きと判定がぶれない
- 同じ入力なら同じ結果になるので、仕様の受け入れ基準を再現性のある形で検証できる
- 上限がないと、処理落ちやウィンドウの移動で1フレームが長引いたときに更新回数が雪だるま式に増え、復帰できなくなる（spiral of death）
- 60fps 固定の2Dゲームでは補間の効果が小さいため、入れない

## 検討して見送ったもの

| 候補 | 見送った理由 |
| --- | --- |
| 可変タイムステップ（`dt` をそのまま使う） | 当たり判定がフレームレートに左右され、弾のすり抜けが起きうる |
| 描画位置の補間 | 60fps 固定の2Dでは見た目の差が小さく、状態を2つ持つ手間に見合わない |
