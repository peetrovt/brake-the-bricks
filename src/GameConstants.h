#pragma once

#include <array>

namespace btb {

// The playing field, in pixels. The original Swing frame was 500 x 600, minus 6 px of borders
// and a 23 px menu bar; the background image is still drawn 500 px wide and clipped on the right.
constexpr int kBackgroundWidth = 500;
constexpr int kVisibleWidth = 494;
constexpr int kGameHeight = 577;
constexpr int kBottom = kGameHeight - 10;

constexpr int kBallSize = 15;
constexpr int kFallingObjectSize = 30;
constexpr int kPlayerDefaultLives = 3;

// Level layout: the shapes step by kBrickLayoutUnit, which is half a brick image wide.
constexpr int kBrickLayoutUnit = 20;
constexpr int kOffsetSide = 40;
constexpr int kOffsetTop = 70;
constexpr int kBrickGap = 2;

struct LevelInfo {
    const char* figure;
    int bricks;
    int tickMs;  // time per game step; lower is faster
};

constexpr std::array<LevelInfo, 5> kLevels = {{
    {"rectangle", 50, 9},
    {"square", 100, 8},
    {"triangle", 80, 7},
    {"rhombus", 80, 6},
    {"hexagon", 96, 5},
}};

}  // namespace btb
