#pragma once

#include "GameConstants.h"
#include "Rect.h"

namespace btb {

enum class FallingObjectKind { SmallerPlayer, BiggerPlayer, ExtraLife, BallSpeedUp, BallSpeedDown };
constexpr int kFallingObjectKindCount = 5;

// A bonus that drops from a broken brick and takes effect when the paddle catches it.
struct FallingObject {
    int x;
    int y;
    int speed;  // pixels per game step
    FallingObjectKind kind;

    // A random bonus at (x, y), moved left if it would stick out of the right border.
    static FallingObject makeRandom(int x, int y);

    void move() { y += speed; }
    Rect rect() const { return {x, y, kFallingObjectSize, kFallingObjectSize}; }
};

}  // namespace btb
