#pragma once

#include "Rect.h"

namespace btb {

// The stones of the art pack. Jade always drops a power-up, gold is worth extra points and obsidian never breaks.
enum class BrickType { Sandstone, Limestone, Granite, Jade, Gold, Obsidian };
constexpr int kBrickTypeCount = 6;
constexpr int kMaxBrickVariants = 4;
constexpr int kMaxBrickHits = 3;

// How many hits a type takes, which is also how many sprites (uncracked plus crack stages) each variant has.
int brickMaxHits(BrickType type);
// How many differently carved sprites a type has.
int brickVariants(BrickType type);

struct Brick {
    static constexpr float kWidth = 128.0f;
    static constexpr float kHeight = 48.0f;

    float x;
    float y;
    BrickType type;
    int variant;
    int hitsLeft;
    float hitSeconds = -1.0f;  // time since the last hit that did not break it; negative when not animating

    // `toughPercent` and `metalPercent` are the chances of a multi-hit brick and of an obsidian brick.
    static Brick make(float x, float y, int toughPercent, int metalPercent);

    // Returns true when the hit destroys the brick. `smash` (fireball) breaks tough bricks at once.
    bool hit(bool smash);

    bool unbreakable() const { return type == BrickType::Obsidian; }
    int cracks() const { return brickMaxHits(type) - hitsLeft; }
    Rect rect() const { return {x, y, kWidth, kHeight}; }
    float centerX() const { return x + kWidth * 0.5f; }
    float centerY() const { return y + kHeight * 0.5f; }
};

}  // namespace btb
