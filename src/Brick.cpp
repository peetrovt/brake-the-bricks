#include "Brick.h"

#include "Random.h"

namespace btb {
namespace {

// One breakable brick in seven is jade, the rate power-ups have always dropped at.
constexpr int kJadePercent = 14;
constexpr int kGoldPercent = 6;

BrickType rollType(int toughPercent, int metalPercent) {
    if (randomInt(100) < metalPercent) {
        return BrickType::Obsidian;
    }
    const int special = randomInt(100);
    if (special < kJadePercent) {
        return BrickType::Jade;
    }
    if (special < kJadePercent + kGoldPercent) {
        return BrickType::Gold;
    }
    if (randomInt(100) >= toughPercent) {
        return BrickType::Sandstone;
    }
    // The harder the level, the more of its tough bricks are the 3-hit kind.
    return randomInt(100) < toughPercent ? BrickType::Granite : BrickType::Limestone;
}

}  // namespace

int brickMaxHits(BrickType type) {
    switch (type) {
        case BrickType::Limestone:
            return 2;
        case BrickType::Granite:
            return 3;
        default:
            return 1;
    }
}

int brickVariants(BrickType type) {
    switch (type) {
        case BrickType::Jade:
        case BrickType::Gold:
            return 2;
        case BrickType::Obsidian:
            return 1;
        default:
            return kMaxBrickVariants;
    }
}

Brick Brick::make(float x, float y, int toughPercent, int metalPercent) {
    const BrickType type = rollType(toughPercent, metalPercent);
    return {x, y, type, randomInt(brickVariants(type)), brickMaxHits(type)};
}

bool Brick::hit(bool smash) {
    if (unbreakable()) {
        return false;
    }
    if (smash || hitsLeft <= 1) {
        return true;
    }
    --hitsLeft;
    return false;
}

}  // namespace btb
