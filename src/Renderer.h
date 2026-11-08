#pragma once

#include <array>
#include <string>

// Minimal immediate-mode 2D drawing on top of OpenGL 2.1.
// Coordinates are in a virtual screen of the playing field's size with the origin in the top-left corner;
// the window is letterboxed to keep the aspect ratio.
namespace gfx {

struct Color {
    float r, g, b, a;
};

struct Rect {
    float x, y, w, h;
};

void beginFrame(int framebufferWidth, int framebufferHeight);

void fillRect(const Rect& rect, const Color& color);
void fillCircle(float cx, float cy, float radius, const Color& color);

// A PNG uploaded to the GPU. Create and destroy it while the OpenGL context is current.
class Texture {
public:
    // Throws std::runtime_error if the file cannot be read or decoded.
    explicit Texture(const std::string& path);
    ~Texture();
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    unsigned id() const { return id_; }

private:
    unsigned id_ = 0;
};

// Draws the whole texture stretched over `dest`.
void drawImage(const Texture& texture, const Rect& dest, float alpha = 1.0f);

// Text uses a built-in 5x7 pixel font; `scale` is the size of one font pixel. Lowercase is drawn as uppercase.
float textWidth(const std::string& text, float scale);
// `y` is the top of the text. A dark shadow is drawn below and to the right so it reads on any background.
void drawText(const std::string& text, float x, float y, float scale, const Color& color);

// A TrueType font baked into a texture. Create and destroy it while the OpenGL context is current.
class Font {
public:
    // Throws std::runtime_error if the file cannot be read or is not a usable font.
    explicit Font(const std::string& path);
    ~Font();
    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;

    float width(const std::string& text, float size) const;
    // Same placement and shadow as the pixel font's drawText. `size` is the line height. Only printable ASCII is drawn.
    void draw(const std::string& text, float x, float y, float size, const Color& color) const;

private:
    // Positions are relative to the pen on the baseline, at the size the font was baked.
    struct Glyph {
        float x0, y0, x1, y1;
        float u0, v0, u1, v1;
        float advance;
    };

    const Glyph* glyph(char c) const;
    void drawGlyphs(const std::string& text, float x, float baseline, float ratio) const;

    unsigned id_ = 0;
    float ascent_ = 0.0f;
    std::array<Glyph, 95> glyphs_{};
};

}  // namespace gfx
