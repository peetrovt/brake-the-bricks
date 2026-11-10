#pragma once

#include "Renderer.h"

#include <string>

namespace btb {

// A horizontal sprite strip. The numbers come from assets/atlas.json.
struct AnimationInfo {
    int frameWidth;
    int frameHeight;
    int frames;
    float fps;
    bool loop;
    float anchorX;  // inside a frame; lands on the position passed to draw()
    float anchorY;
};

class Animation {
public:
    // Throws std::runtime_error if the image cannot be loaded.
    Animation(const std::string& path, const AnimationInfo& info) : texture_(path), info_(info) {}

    bool finished(float age) const { return !info_.loop && age >= info_.frames / info_.fps; }
    int frameAt(float age) const;
    // `rotation` is in radians, clockwise on screen, around the anchor.
    void draw(float age, float x, float y, float rotation = 0.0f, gfx::Blend blend = gfx::Blend::Normal) const;

private:
    gfx::Texture texture_;
    AnimationInfo info_;
};

}  // namespace btb
