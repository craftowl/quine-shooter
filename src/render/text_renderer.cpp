#include "render/text_renderer.h"

#include <cassert>
#include <cmath>

namespace stg::render {

bool TextRenderer::load(const char* font_path, float scale) {
    assert(scale > 0.0f);
    const float size = static_cast<float>(FONT_SIZE) * scale;
    const int pixel_size = static_cast<int>(size);
    // 拡大率を 0.25 刻みにしているので、12 × scale は整数になる（tasks.md T05）
    assert(static_cast<float>(pixel_size) == size);

    // 文字の一覧を渡さないと、ASCII の 95 文字（32〜126）を読み込む
    Font font = LoadFontEx(font_path, pixel_size, nullptr, 0);
    if (!IsFontValid(font) || font.texture.id == 0) {
        TraceLog(LOG_ERROR, "FONT: Failed to load %s", font_path);
        UnloadFont(font);
        return false;
    }
    // 読み込んだ大きさのまま描くので、補間しない
    SetTextureFilter(font.texture, TEXTURE_FILTER_POINT);

    font_ = font;
    camera_ = Camera2D{};
    camera_.zoom = scale;
    TraceLog(LOG_INFO, "FONT: Loaded %s at %d px (scale %.2f)", font_path, pixel_size, static_cast<double>(scale));
    return true;
}

void TextRenderer::unload() {
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
