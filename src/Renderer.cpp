#include "Renderer.h"

#include "GameConstants.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include <GL/gl.h>
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <stdexcept>
#include <unordered_map>

namespace gfx {
namespace {

constexpr float kVirtualWidth = btb::kFieldWidth;
constexpr float kVirtualHeight = btb::kFieldHeight;
constexpr float kPi = 3.14159265358979f;
constexpr int kCircleSegments = 32;

void color(const Color& c) { glColor4f(c.r, c.g, c.b, c.a); }

using Glyph = std::array<const char*, 7>;

const std::unordered_map<char, Glyph>& font() {
    static const std::unordered_map<char, Glyph> glyphs = {
        {'A', {"01110", "10001", "10001", "11111", "10001", "10001", "10001"}},
        {'B', {"11110", "10001", "10001", "11110", "10001", "10001", "11110"}},
        {'C', {"01110", "10001", "10000", "10000", "10000", "10001", "01110"}},
        {'D', {"11110", "10001", "10001", "10001", "10001", "10001", "11110"}},
        {'E', {"11111", "10000", "10000", "11110", "10000", "10000", "11111"}},
        {'F', {"11111", "10000", "10000", "11110", "10000", "10000", "10000"}},
        {'G', {"01110", "10001", "10000", "10111", "10001", "10001", "01111"}},
        {'H', {"10001", "10001", "10001", "11111", "10001", "10001", "10001"}},
        {'I', {"01110", "00100", "00100", "00100", "00100", "00100", "01110"}},
        {'J', {"00111", "00010", "00010", "00010", "00010", "10010", "01100"}},
        {'K', {"10001", "10010", "10100", "11000", "10100", "10010", "10001"}},
        {'L', {"10000", "10000", "10000", "10000", "10000", "10000", "11111"}},
        {'M', {"10001", "11011", "10101", "10101", "10001", "10001", "10001"}},
        {'N', {"10001", "10001", "11001", "10101", "10011", "10001", "10001"}},
        {'O', {"01110", "10001", "10001", "10001", "10001", "10001", "01110"}},
        {'P', {"11110", "10001", "10001", "11110", "10000", "10000", "10000"}},
        {'Q', {"01110", "10001", "10001", "10001", "10101", "10010", "01101"}},
        {'R', {"11110", "10001", "10001", "11110", "10100", "10010", "10001"}},
        {'S', {"01111", "10000", "10000", "01110", "00001", "00001", "11110"}},
        {'T', {"11111", "00100", "00100", "00100", "00100", "00100", "00100"}},
        {'U', {"10001", "10001", "10001", "10001", "10001", "10001", "01110"}},
        {'V', {"10001", "10001", "10001", "10001", "10001", "01010", "00100"}},
        {'W', {"10001", "10001", "10001", "10101", "10101", "10101", "01010"}},
        {'X', {"10001", "10001", "01010", "00100", "01010", "10001", "10001"}},
        {'Y', {"10001", "10001", "01010", "00100", "00100", "00100", "00100"}},
        {'Z', {"11111", "00001", "00010", "00100", "01000", "10000", "11111"}},
        {'0', {"01110", "10001", "10011", "10101", "11001", "10001", "01110"}},
        {'1', {"00100", "01100", "00100", "00100", "00100", "00100", "01110"}},
        {'2', {"01110", "10001", "00001", "00010", "00100", "01000", "11111"}},
        {'3', {"11111", "00010", "00100", "00010", "00001", "10001", "01110"}},
        {'4', {"00010", "00110", "01010", "10010", "11111", "00010", "00010"}},
        {'5', {"11111", "10000", "11110", "00001", "00001", "10001", "01110"}},
        {'6', {"00110", "01000", "10000", "11110", "10001", "10001", "01110"}},
        {'7', {"11111", "00001", "00010", "00100", "01000", "01000", "01000"}},
        {'8', {"01110", "10001", "10001", "01110", "10001", "10001", "01110"}},
        {'9', {"01110", "10001", "10001", "01111", "00001", "00010", "01100"}},
        {':', {"00000", "01100", "01100", "00000", "01100", "01100", "00000"}},
        {'-', {"00000", "00000", "00000", "11111", "00000", "00000", "00000"}},
        {'.', {"00000", "00000", "00000", "00000", "00000", "01100", "01100"}},
        {'!', {"00100", "00100", "00100", "00100", "00100", "00000", "00100"}},
        {'/', {"00001", "00010", "00010", "00100", "01000", "01000", "10000"}},
        {'+', {"00000", "00100", "00100", "11111", "00100", "00100", "00000"}},
        {',', {"00000", "00000", "00000", "00000", "01100", "00100", "01000"}},
        {'(', {"00010", "00100", "01000", "01000", "01000", "00100", "00010"}},
        {')', {"01000", "00100", "00010", "00010", "00010", "00100", "01000"}},
    };
    return glyphs;
}

void drawGlyphs(const std::string& text, float x, float y, float scale) {
    const auto& glyphs = font();
    glBegin(GL_QUADS);
    for (size_t i = 0; i < text.size(); ++i) {
        auto it = glyphs.find(static_cast<char>(std::toupper(static_cast<unsigned char>(text[i]))));
        if (it == glyphs.end()) {
            continue;
        }
        float glyphX = x + i * 6.0f * scale;
        for (int row = 0; row < 7; ++row) {
            for (int col = 0; col < 5; ++col) {
                if (it->second[row][col] != '1') {
                    continue;
                }
                float px = glyphX + col * scale;
                float py = y + row * scale;
                glVertex2f(px, py);
                glVertex2f(px + scale, py);
                glVertex2f(px + scale, py + scale);
                glVertex2f(px, py + scale);
            }
        }
    }
    glEnd();
}

}  // namespace

void beginFrame(int framebufferWidth, int framebufferHeight) {
    framebufferWidth = std::max(framebufferWidth, 1);
    framebufferHeight = std::max(framebufferHeight, 1);

    float scale = std::min(framebufferWidth / kVirtualWidth, framebufferHeight / kVirtualHeight);
    int width = static_cast<int>(std::lround(kVirtualWidth * scale));
    int height = static_cast<int>(std::lround(kVirtualHeight * scale));

    glViewport(0, 0, framebufferWidth, framebufferHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glViewport((framebufferWidth - width) / 2, (framebufferHeight - height) / 2, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, kVirtualWidth, kVirtualHeight, 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void fillRect(const Rect& rect, const Color& c) {
    color(c);
    glBegin(GL_QUADS);
    glVertex2f(rect.x, rect.y);
    glVertex2f(rect.x + rect.w, rect.y);
    glVertex2f(rect.x + rect.w, rect.y + rect.h);
    glVertex2f(rect.x, rect.y + rect.h);
    glEnd();
}

void fillCircle(float cx, float cy, float radius, const Color& c) {
    color(c);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= kCircleSegments; ++i) {
        float angle = 2.0f * kPi * i / kCircleSegments;
        glVertex2f(cx + std::cos(angle) * radius, cy + std::sin(angle) * radius);
    }
    glEnd();
}

Texture::Texture(const std::string& path) {
    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if (!pixels) {
        throw std::runtime_error("Cannot load image " + path + ": " + stbi_failure_reason());
    }
    // Premultiplied alpha, so filtering never bleeds the colour of transparent pixels into the edges.
    for (size_t i = 0; i < static_cast<size_t>(width) * height * 4; i += 4) {
        for (int c = 0; c < 3; ++c) {
            pixels[i + c] = static_cast<stbi_uc>((pixels[i + c] * pixels[i + 3] + 127) / 255);
        }
    }

    glGenTextures(1, &id_);
    glBindTexture(GL_TEXTURE_2D, id_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // Mipmaps keep the Full HD art smooth in a smaller window.
    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(pixels);
}

Texture::~Texture() {
    glDeleteTextures(1, &id_);
}

void drawImage(const Texture& texture, const Rect& dest, float alpha) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture.id());
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(alpha, alpha, alpha, alpha);  // premultiplied
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(dest.x, dest.y);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(dest.x + dest.w, dest.y);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(dest.x + dest.w, dest.y + dest.h);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(dest.x, dest.y + dest.h);
    glEnd();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}

float textWidth(const std::string& text, float scale) {
    if (text.empty()) {
        return 0.0f;
    }
    return (text.size() * 6.0f - 1.0f) * scale;
}

void drawText(const std::string& text, float x, float y, float scale, const Color& c) {
    float shadow = std::max(1.0f, scale * 0.5f);
    color({0.0f, 0.0f, 0.0f, 0.7f * c.a});
    drawGlyphs(text, x + shadow, y + shadow, scale);
    color(c);
    drawGlyphs(text, x, y, scale);
}

}  // namespace gfx
