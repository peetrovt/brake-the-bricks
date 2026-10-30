#pragma once

#include "Rect.h"

namespace btb {

// The colours of the art pack, in the order the rows of a level use them.
enum class BrickColour { Red, Orange, Yellow, Green, Cyan, Blue, Purple };
constexpr int kBrickColourCount = 7;

struct Brick {
    static constexpr float kWidth = 120.0f;
    static constexpr float kHeight = 60.0f;
    static constexpr float kGap = 6.0f;

    float x;
    float y;
    BrickColour colour;
    int hitsLeft;   // 2 for a tough brick that has not been hit yet
    bool cracked;   // a tough brick that has been hit once
    bool metal;     // unbreakable, and not needed to finish the level

    // `toughPercent` and `metalPercent` are the chances of a 2-hit brick and of a metal brick.
    static Brick make(float x, float y, BrickColour colour, int toughPercent, int metalPercent);

    // Returns true when the hit destroys the brick. `smash` (fireball) breaks tough bricks at once.
    bool hit(bool smash);

    Rect rect() const { return {x, y, kWidth, kHeight}; }
    float centerX() const { return x + kWidth * 0.5f; }
    float centerY() const { return y + kHeight * 0.5f; }
};

}  // namespace btb
