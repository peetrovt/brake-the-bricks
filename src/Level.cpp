#include "Level.h"

#include "GameConstants.h"

namespace btb {
namespace {

constexpr float kStepX = Brick::kWidth + Brick::kGap;
constexpr float kStepY = Brick::kHeight + Brick::kGap;
constexpr float kTop = 110.0f;

}  // namespace

const std::vector<LevelInfo>& levels() {
    // Rows hold at most 14 bricks, the width of the field. Rows whose length differs by an odd number
    // are offset by half a brick, which gives the slanted edges of the triangle and the rhombus.
    static const std::vector<LevelInfo> all = {
        {"rectangle", {14, 14, 14, 14, 14}, 700.0f, 10, 0},
        {"wall", {12, 12, 12, 12, 12, 12, 12, 12}, 760.0f, 20, 0},
        {"triangle", {14, 13, 12, 11, 10, 9, 8, 7}, 820.0f, 25, 3},
        {"rhombus", {2, 5, 8, 11, 14, 11, 8, 5, 2}, 880.0f, 30, 4},
        {"hexagon", {8, 10, 12, 14, 14, 14, 12, 10, 8}, 940.0f, 35, 5},
    };
    return all;
}

std::vector<Brick> buildLevel(int index) {
    const LevelInfo& level = levels()[index];
    std::vector<Brick> bricks;
    float y = kTop;
    for (size_t row = 0; row < level.rows.size(); ++row) {
        const int columns = level.rows[row];
        const float x = (kFieldWidth - (columns * kStepX - Brick::kGap)) * 0.5f;
        const auto colour = static_cast<BrickColour>(row % kBrickColourCount);
        for (int column = 0; column < columns; ++column) {
            bricks.push_back(Brick::make(x + column * kStepX, y, colour, level.toughPercent, level.metalPercent));
        }
        y += kStepY;
    }
    return bricks;
}

}  // namespace btb
