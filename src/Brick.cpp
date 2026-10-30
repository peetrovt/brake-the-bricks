#include "Brick.h"

#include "Random.h"

namespace btb {

Brick Brick::make(int x, int y, BrickColour colour) {
    return {x, y, colour, randomInt(10) == 1};
}

Brick Brick::makeRandom(int x, int y) {
    return make(x, y, static_cast<BrickColour>(randomInt(kBrickColourCount)));
}

Rect Brick::horizontalEdge(int ballYDir) const {
    if (ballYDir == 1) {
        return {x, y, kWidth, 1};
    }
    if (ballYDir == -1) {
        return {x, y + kHeight, kWidth, 1};
    }
    return {0, 0, 0, 0};
}

Rect Brick::verticalEdge(int ballXDir) const {
    if (ballXDir == 1) {
        return {x, y, 1, kHeight};
    }
    if (ballXDir == -1) {
        return {x + kWidth, y, 1, kHeight};
    }
    return {0, 0, 0, 0};
}

}  // namespace btb
