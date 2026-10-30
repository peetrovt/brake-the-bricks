#include "Brick.h"

#include "Random.h"

namespace btb {

Brick Brick::make(float x, float y, BrickColour colour, int toughPercent, int metalPercent) {
    const bool metal = randomInt(100) < metalPercent;
    const int hits = !metal && randomInt(100) < toughPercent ? 2 : 1;
    return {x, y, colour, hits, false, metal};
}

bool Brick::hit(bool smash) {
    if (metal) {
        return false;
    }
    if (smash || hitsLeft <= 1) {
        return true;
    }
    --hitsLeft;
    cracked = true;
    return false;
}

}  // namespace btb
