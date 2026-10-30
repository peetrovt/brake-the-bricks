#include "Level.h"

#include "GameConstants.h"

namespace btb {
namespace {

constexpr int kStepX = Brick::kWidth + kBrickGap;
constexpr int kStepY = Brick::kHeight + kBrickGap;
// How far a row moves left when it grows by one brick on each side.
constexpr int kWidenShift = (kBrickLayoutUnit + kBrickGap / 2) * 2;

void addRow(std::vector<Brick>& bricks, int x, int y, int columns) {
    for (int i = 0; i < columns; ++i) {
        bricks.push_back(Brick::makeRandom(x + i * kStepX, y));
    }
}

// Rows of ten; used by both the rectangle and the square.
std::vector<Brick> grid(int count) {
    std::vector<Brick> bricks;
    for (int row = 0; row < count / 10; ++row) {
        addRow(bricks, kOffsetSide, kOffsetTop + row * kStepY, 10);
    }
    return bricks;
}

// Starts with ten columns and loses one every second row, each row shifted half a brick to the right.
std::vector<Brick> triangle(int count) {
    std::vector<Brick> bricks;
    int columns = 10;
    int x = kOffsetSide;
    int y = kOffsetTop;
    for (int row = 1; count > 0; ++row) {
        addRow(bricks, x, y, columns);
        count -= columns;
        y += kStepY;
        if (row % 2 == 0) {
            --columns;
            x += Brick::kWidth / 2 + kBrickGap / 2;
        }
    }
    return bricks;
}

// Centred rows that grow by two bricks for `growRows` rows, stay full until `shrinkAfterRow`, then shrink by two.
std::vector<Brick> diamond(int count, int columns, int x, int growRows, int shrinkAfterRow) {
    std::vector<Brick> bricks;
    int y = kOffsetTop;
    for (int row = 0; count > 0; ++row) {
        addRow(bricks, x, y, columns);
        count -= columns;
        if (row < growRows) {
            columns += 2;
            x -= kWidenShift;
        } else if (row > shrinkAfterRow) {
            columns -= 2;
            x += kWidenShift;
        }
        y += kStepY;
    }
    return bricks;
}

}  // namespace

std::vector<Brick> buildLevel(int index) {
    const int count = kLevels[index].bricks;
    switch (index) {
        case 2:
            return triangle(count);
        case 3:
            return diamond(count, 2, kBackgroundWidth / 2 - kBrickLayoutUnit * 2, 4, 6);
        case 4:
            return diamond(count, 4, kBackgroundWidth / 2 - 2 * kBrickLayoutUnit - 3 * kBrickGap - kOffsetSide, 3, 7);
        default:
            return grid(count);
    }
}

}  // namespace btb
