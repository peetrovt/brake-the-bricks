#include "Animation.h"

#include <algorithm>

namespace btb {

int Animation::frameAt(float age) const {
    const int frame = static_cast<int>(std::max(age, 0.0f) * info_.fps);
    return info_.loop ? frame % info_.frames : std::min(frame, info_.frames - 1);
}

void Animation::draw(float age, float x, float y, float rotation, gfx::Blend blend) const {
    const float frameWidth = static_cast<float>(info_.frameWidth);
    const float frameHeight = static_cast<float>(info_.frameHeight);
    const gfx::Rect source{frameAt(age) * frameWidth, 0.0f, frameWidth, frameHeight};
    gfx::drawRegion(texture_, source, x, y, info_.anchorX, info_.anchorY, rotation, blend);
}

}  // namespace btb
