#pragma once

#include "Rect.h"

namespace btb {

// Ordered by toughness: a hit turns a brick into the previous colour, and a green brick breaks.
enum class BrickColour { Green, Yellow, Orange, Red, Blue };
constexpr int kBrickColourCount = 5;

struct Brick {
    static constexpr int kWidth = 40;
    static constexpr int kHeight = 20;

    int x;
    int y;
    BrickColour colour;
    bool hasFallingObject;  // drops a bonus when hit

    // A brick of the given colour; one in ten carries a falling object.
    static Brick make(int x, int y, BrickColour colour);
    static Brick makeRandom(int x, int y);

    Rect rect() const { return {x, y, kWidth, kHeight}; }
    // The 1 px strip a ball moving in `ballYDir` hits: the top for 1 (down), just below the bottom for -1 (up).
    Rect horizontalEdge(int ballYDir) const;
    // The 1 px strip a ball moving in `ballXDir` hits: the left side for 1 (right), just past the right side for -1.
    Rect verticalEdge(int ballXDir) const;
};

}  // namespace btb
