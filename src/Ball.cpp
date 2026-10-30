#include "Ball.h"

#include <cmath>

namespace btb {
namespace {

constexpr float kMinVertical = 0.3f;

}  // namespace

void Ball::setAngle(float angle) {
    dx = std::sin(angle);
    dy = -std::cos(angle);
}

void Ball::rotate(float angle) {
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    const float newDx = dx * c - dy * s;
    dy = dx * s + dy * c;
    dx = newDx;
    keepSteep();
}

void Ball::keepSteep() {
    if (std::abs(dy) >= kMinVertical) {
        return;
    }
    dy = dy < 0.0f ? -kMinVertical : kMinVertical;
    dx = std::copysign(std::sqrt(1.0f - dy * dy), dx);
}

void Ball::bounceOff(const Rect& rect) {
    // How far the ball would have to move in each direction to stop overlapping.
    const float left = x + kRadius - rect.x;
    const float right = rect.x + rect.w - (x - kRadius);
    const float up = y + kRadius - rect.y;
    const float down = rect.y + rect.h - (y - kRadius);

    if (std::min(left, right) < std::min(up, down)) {
        if (left < right) {
            x -= left;
            dx = -std::abs(dx);
        } else {
            x += right;
            dx = std::abs(dx);
        }
    } else {
        if (up < down) {
            y -= up;
            dy = -std::abs(dy);
        } else {
            y += down;
            dy = std::abs(dy);
        }
    }
    keepSteep();
}

}  // namespace btb
