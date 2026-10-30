#pragma once

#include "GameConstants.h"
#include "Rect.h"

namespace btb {

class Player;

class Ball {
public:
    // Puts the ball on top of the paddle, heading up in a random horizontal direction, at the base speed.
    void reset(const Player& player);
    // Keeps the ball centred on the paddle while it waits for launch.
    void followPlayer(const Player& player);
    void move();

    void accelerate();
    void slowDown();
    // Debug keys: freeze the ball in place and release it again.
    void stop();
    void resume();

    void setYDir(int ydir) { ydir_ = ydir; }
    void flipXDir() { xdir_ = -xdir_; }
    void flipYDir() { ydir_ = -ydir_; }

    int x() const { return x_; }
    int y() const { return y_; }
    int xDir() const { return xdir_; }
    int yDir() const { return ydir_; }
    Rect rect() const { return {x_, y_, kBallSize, kBallSize}; }

private:
    static constexpr int kBaseSpeed = 2;
    static constexpr int kMaxSpeed = 3;

    int x_ = 0;
    int y_ = 0;
    int xdir_ = 1;
    int ydir_ = -1;
    int speed_ = kBaseSpeed;  // pixels per game step along each axis
    bool stopped_ = false;
    int stoppedXDir_ = 0;
    int stoppedYDir_ = 0;
};

}  // namespace btb
