#pragma once

#include "Rect.h"

namespace btb {

// One per powerup-*.png in the art pack.
enum class PowerUpKind { ExtraLife, Multiball, Expand, Shrink, Laser, Fireball, Slow, Barrier, Catch, Bonus };
constexpr int kPowerUpKindCount = 10;

// A bonus orb that drops from a broken brick and takes effect when the paddle catches it.
struct PowerUp {
    static constexpr float kRadius = 48.0f;

    float x;  // centre
    float y;
    float speed;  // pixels per second
    PowerUpKind kind;

    static PowerUp make(float x, float y, PowerUpKind kind);
    // A random kind; helpful power-ups are more likely than harmful ones.
    static PowerUp makeRandom(float x, float y);

    Rect rect() const { return {x - kRadius, y - kRadius, 2.0f * kRadius, 2.0f * kRadius}; }
};

const char* powerUpName(PowerUpKind kind);
const char* powerUpDescription(PowerUpKind kind);

}  // namespace btb
