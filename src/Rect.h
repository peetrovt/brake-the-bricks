#pragma once

namespace btb {

struct Rect {
    int x, y, w, h;

    // Same rule as java.awt.Rectangle: rectangles that only touch along an edge do not intersect.
    bool intersects(const Rect& other) const {
        return w > 0 && h > 0 && other.w > 0 && other.h > 0 && x < other.x + other.w && other.x < x + w &&
               y < other.y + other.h && other.y < y + h;
    }
};

}  // namespace btb
