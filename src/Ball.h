#pragma once

#include "Rect.h"

namespace btb {

struct Ball {
    static constexpr float kRadius = 16.0f;

    float x = 0.0f;  // centre
    float y = 0.0f;
    float dx = 0.0f;  // unit direction; the speed is shared by all balls
    float dy = -1.0f;
    bool stuck = true;  // rides on the paddle until launched
    float stuckOffset = 0.0f;  // from the paddle centre

    // `angle` is in radians from straight up, positive to the right.
    void setAngle(float angle);
    // Turns the direction by `angle` radians.
    void rotate(float angle);
    // Keeps the ball from bouncing almost horizontally for ever.
    void keepSteep();
    // Moves the ball out of `rect`, which it overlaps, along the shallower axis and bounces it off that side.
    void bounceOff(const Rect& rect);

    Rect rect() const { return {x - kRadius, y - kRadius, 2.0f * kRadius, 2.0f * kRadius}; }
};

}  // namespace btb
