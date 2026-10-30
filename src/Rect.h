#pragma once

#include <algorithm>

namespace btb {

struct Rect {
    float x, y, w, h;

    // Rectangles that only touch along an edge do not intersect.
    bool intersects(const Rect& other) const {
        return x < other.x + other.w && other.x < x + w && y < other.y + other.h && other.y < y + h;
    }

    bool intersectsCircle(float cx, float cy, float radius) const {
        float dx = cx - std::clamp(cx, x, x + w);
        float dy = cy - std::clamp(cy, y, y + h);
        return dx * dx + dy * dy < radius * radius;
    }
};

}  // namespace btb
