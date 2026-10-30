#pragma once

#include "Brick.h"

#include <vector>

namespace btb {

struct LevelInfo {
    const char* figure;
    std::vector<int> rows;  // bricks per row, top to bottom; each row is centred
    float ballSpeed;        // pixels per second
    int toughPercent;       // chance of a 2-hit brick
    int metalPercent;       // chance of an unbreakable brick
};

const std::vector<LevelInfo>& levels();

// Lays out the bricks of level `index` (0 .. levels().size() - 1).
std::vector<Brick> buildLevel(int index);

}  // namespace btb
