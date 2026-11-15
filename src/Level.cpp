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
        {"rectangle", {11, 11, 11, 11}, 600.0f, 5, 0},
        {"wall", {9, 9, 9, 9, 9, 9}, 640.0f, 10, 0},
        {"triangle", {11, 10, 9, 8, 7, 6, 5}, 680.0f, 15, 0},
        {"rhombus", {1, 4, 7, 10, 13, 10, 7, 4, 1}, 720.0f, 18, 2},
        {"hexagon", {5, 7, 9, 11, 11, 9, 7, 5}, 760.0f, 22, 2},
        {"hourglass", {13, 11, 9, 7, 5, 5, 7, 9, 11, 13}, 800.0f, 26, 3},
        {"arrow", {1, 3, 5, 7, 9, 11, 13, 3, 3, 3}, 840.0f, 30, 3},
        {"cross", {3, 3, 3, 13, 13, 13, 13, 3, 3, 3}, 870.0f, 34, 4},
        {"bars", {13, 0, 13, 13, 0, 13, 13, 0, 13, 13}, 900.0f, 38, 4},
        {"fortress", {13, 13, 13, 13, 13, 13, 13, 13, 13, 13}, 950.0f, 45, 5},
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
