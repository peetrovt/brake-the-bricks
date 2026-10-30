#include "Ball.h"

#include "Player.h"
#include "Random.h"

#include <algorithm>

namespace btb {

void Ball::reset(const Player& player) {
    followPlayer(player);
    y_ = player.y() - kBallSize;
    xdir_ = randomInt(2) == 0 ? -1 : 1;
    ydir_ = -1;
    speed_ = kBaseSpeed;
    stopped_ = false;
}

void Ball::followPlayer(const Player& player) {
    x_ = player.x() + player.width() / 2 - kBallSize / 2;
}

void Ball::move() {
    x_ = std::min(x_ + speed_ * xdir_, kVisibleWidth - kBallSize);
    y_ += speed_ * ydir_;

    if (x_ <= 0) {
        xdir_ = 1;
    }
    if (x_ >= kVisibleWidth - kBallSize) {
        xdir_ = -1;
    }
    if (y_ <= 0) {
        ydir_ = 1;
    }
}

void Ball::accelerate() {
    speed_ = std::min(speed_ + 1, kMaxSpeed);
}

void Ball::slowDown() {
    speed_ = std::max(speed_ - 1, kBaseSpeed);
}

void Ball::stop() {
    if (!stopped_) {
        stoppedXDir_ = xdir_;
        stoppedYDir_ = ydir_;
        xdir_ = 0;
        ydir_ = 0;
        stopped_ = true;
    }
}

void Ball::resume() {
    if (stopped_) {
        xdir_ = stoppedXDir_;
        ydir_ = stoppedYDir_;
        stopped_ = false;
    }
}

}  // namespace btb
