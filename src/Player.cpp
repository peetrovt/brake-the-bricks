#include "Player.h"

#include "GameConstants.h"

#include <algorithm>

namespace btb {

void Player::reset() {
    width_ = kDefaultWidth;
    x_ = (kVisibleWidth - width_) / 2;
    y_ = kBottom - kHeight - 40;
}

void Player::move(int direction) {
    x_ = std::clamp(x_ + direction * kSpeed, 0, kVisibleWidth - width_);
}

void Player::makeBigger() {
    if (width_ < 150) {
        width_ += 50;
        x_ = std::min(x_, kVisibleWidth - width_);
    }
}

void Player::makeSmaller() {
    if (width_ >= 100) {
        width_ -= 50;
    }
}

Rect Player::sideEdge(int ballXDir) const {
    if (ballXDir == 1) {
        return {x_, y_, 1, kHeight};
    }
    if (ballXDir == -1) {
        return {x_ + width_ - 1, y_, 1, kHeight};
    }
    return {0, 0, 0, 0};
}

}  // namespace btb
