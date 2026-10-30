#include "Player.h"

#include "GameConstants.h"

#include <algorithm>

namespace btb {

void Player::reset() {
    size_ = PaddleSize::Normal;
    centerX_ = kFieldWidth * 0.5f;
}

void Player::move(int direction, float seconds) {
    centerX_ += direction * kSpeed * seconds;
    keepInsideField();
}

void Player::grow() {
    if (size_ != PaddleSize::Wide) {
        size_ = static_cast<PaddleSize>(static_cast<int>(size_) + 1);
        keepInsideField();
    }
}

void Player::shrink() {
    if (size_ != PaddleSize::Short) {
        size_ = static_cast<PaddleSize>(static_cast<int>(size_) - 1);
    }
}

float Player::width() const {
    switch (size_) {
        case PaddleSize::Short:
            return 200.0f;
        case PaddleSize::Wide:
            return 420.0f;
        default:
            return 300.0f;
    }
}

void Player::keepInsideField() {
    const float half = width() * 0.5f;
    centerX_ = std::clamp(centerX_, half, kFieldWidth - half);
}

}  // namespace btb
