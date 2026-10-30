#pragma once

#include "Brick.h"

#include <vector>

namespace btb {

// Lays out the bricks of level `index` (0 .. kLevels.size() - 1) with random colours.
std::vector<Brick> buildLevel(int index);

}  // namespace btb
