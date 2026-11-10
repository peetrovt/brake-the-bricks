#include "Level.h"

#include "GameConstants.h"

namespace btb {
namespace {

constexpr float kTop = 180.0f;

}  // namespace

const std::vector<LevelInfo>& levels() {
    // Rows hold at most 13 bricks, the width of the field. Rows whose length differs by an odd number
    // are offset by half a brick, which gives the slanted edges of the triangle and the rhombus. A row of 0
    // leaves an empty line. More than 10 rows would reach the on-screen messages.
    static const std::vector<LevelInfo> all = {
        {"rectangle", {13, 13, 13, 13, 13}, 700.0f, 10, 0},
        {"wall", {11, 11, 11, 11, 11, 11, 11, 11}, 760.0f, 20, 0},
        {"triangle", {13, 12, 11, 10, 9, 8, 7, 6}, 820.0f, 25, 3},
        {"rhombus", {1, 4, 7, 10, 13, 10, 7, 4, 1}, 880.0f, 30, 4},
        {"hexagon", {7, 9, 11, 13, 13, 13, 11, 9, 7}, 940.0f, 35, 5},
        {"hourglass", {13, 11, 9, 7, 5, 5, 7, 9, 11, 13}, 1000.0f, 40, 6},
        {"arrow", {1, 3, 5, 7, 9, 11, 13, 3, 3, 3}, 1060.0f, 45, 7},
        {"cross", {3, 3, 3, 13, 13, 13, 13, 3, 3, 3}, 1120.0f, 50, 8},
        {"bars", {13, 0, 13, 13, 0, 13, 13, 0, 13, 13}, 1180.0f, 55, 9},
        {"fortress", {13, 13, 13, 13, 13, 13, 13, 13, 13, 13}, 1300.0f, 65, 10},
    };
    return all;
}

std::vector<Brick> buildLevel(int index) {
    const LevelInfo& level = levels()[index];
    std::vector<Brick> bricks;
    float y = kTop;
    for (size_t row = 0; row < level.rows.size(); ++row) {
        const int columns = level.rows[row];
        const float x = (kFieldWidth - columns * Brick::kWidth) * 0.5f;
        for (int column = 0; column < columns; ++column) {
            bricks.push_back(Brick::make(x + column * Brick::kWidth, y, level.toughPercent, level.metalPercent));
        }
        y += Brick::kHeight;
    }
    return bricks;
}

}  // namespace btb
