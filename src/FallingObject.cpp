#include "FallingObject.h"

#include "Random.h"

#include <algorithm>

namespace btb {

FallingObject FallingObject::makeRandom(int x, int y) {
    return {std::min(x, kVisibleWidth - kFallingObjectSize), y, randomInt(2) + 1,
            static_cast<FallingObjectKind>(randomInt(kFallingObjectKindCount))};
}

}  // namespace btb
