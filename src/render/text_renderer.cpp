#include "render/text_renderer.h"

#include <cassert>
#include <cmath>

namespace stg::render {

bool TextRenderer::load(const char* font_path, float scale, LoadErrors& errors) {
    assert(scale > 0.0f);
    const float size = static_cast<float>(FONT_SIZE) * scale;
    const int pixel_size = static_cast<int>(size);
    // 拡大率を 0.25 刻みにしているので、12 × scale は整数になる（tasks.md T05）
    assert(static_cast<float>(pixel_size) == size);

    // LoadFontEx は理由を返さないので、ファイルがないことだけを先に確かめて、理由を分ける
    const char* full_path = TextFormat("%s%s", GetApplicationDirectory(), font_path);
    if (!FileExists(full_path)) {
        TraceLog(LOG_ERROR, "FONT: Failed to load %s: File not found", full_path);
        errors.add(font_path, "File not found");
        return false;
    }
    // 文字の一覧を渡さないと、ASCII の 95 文字（32〜126）を読み込む
    // TTF として読めないと、LoadFontEx は失敗を返さずに組み込みのフォントを返す（raylib 6.0 の LoadFontFromMemory）。
    // そこで、組み込みのフォントと同じテクスチャが返ってきたら、読めなかったとみなす
    Font font = LoadFontEx(full_path, pixel_size, nullptr, 0);
    if (!IsFontValid(font) || font.texture.id == 0 || font.texture.id == GetFontDefault().texture.id) {
        TraceLog(LOG_ERROR, "FONT: Failed to load %s: Not a valid font file", full_path);
        errors.add(font_path, "Not a valid font file");
        UnloadFont(font);
        return false;
    }
    // 読み込んだ大きさのまま描くので、補間しない
    SetTextureFilter(font.texture, TEXTURE_FILTER_POINT);

    font_ = font;
    camera_ = Camera2D{};
    camera_.zoom = scale;
    TraceLog(LOG_INFO, "FONT: Loaded %s at %d px (scale %.2f)", full_path, pixel_size, static_cast<double>(scale));
    return true;
}

void TextRenderer::unload() {
    // 読み込めなかったときは、片付けるものがない
    if (font_.texture.id == 0) {
        return;
    }
    UnloadFont(font_);
    font_ = Font{};
}

void TextRenderer::begin() {
    BeginMode2D(camera_);
}

void TextRenderer::end() {
    EndMode2D();
}

void TextRenderer::draw_text(const char* text, Vector2 position, Color color) const {
    // 読み込めていないと、raylib が組み込みのフォントで黙って描いてしまう
    assert(font_.texture.id != 0);
    // 画面のピクセルの境目に合わせる。ずれると、同じ文字でも位置によって線の太さが変わる
    const float zoom = camera_.zoom;
    const Vector2 snapped = {std::round(position.x * zoom) / zoom, std::round(position.y * zoom) / zoom};
    DrawTextEx(font_, text, snapped, static_cast<float>(FONT_SIZE), 0.0f, color);
}

Vector2 TextRenderer::measure_text(const char* text) const {
    assert(font_.texture.id != 0);
    return MeasureTextEx(font_, text, static_cast<float>(FONT_SIZE), 0.0f);
}

}  // namespace stg::render
