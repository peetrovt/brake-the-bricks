#pragma once

#include "Rect.h"

namespace btb {

// The paddle.
class Player {
public:
    static constexpr int kDefaultWidth = 100;
    static constexpr int kHeight = 16;
    static constexpr int kSpeed = 2;  // pixels per game step

    Player() { reset(); }

    // Back to the default size, centred above the bottom.
    void reset();
    // `direction` is -1 (left), 0 or 1 (right).
    void move(int direction);
    void makeBigger();
    void makeSmaller();

    int x() const { return x_; }
    int y() const { return y_; }
    int width() const { return width_; }

    // The 1 px high top surface the ball bounces off.
    Rect topEdge() const { return {x_, y_, width_, 1}; }
    // The side a ball moving in `ballXDir` runs into: the left side for 1, the right side for -1.
    Rect sideEdge(int ballXDir) const;

private:
    int x_ = 0;
    int y_ = 0;
    int width_ = kDefaultWidth;
};

}  // namespace btb
