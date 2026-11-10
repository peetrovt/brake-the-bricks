#pragma once

namespace btb {

// The playing field matches the Full HD background, in pixels.
constexpr float kFieldWidth = 1920.0f;
constexpr float kFieldHeight = 1080.0f;

// Inside the temple art: the pillars on either side and the carved frieze the HUD sits on.
constexpr float kWallLeft = 100.0f;
constexpr float kWallRight = 1820.0f;
constexpr float kCeiling = 108.0f;

constexpr int kPlayerDefaultLives = 3;

// The game advances in fixed steps, independent of the frame rate.
constexpr double kStepSeconds = 1.0 / 240.0;

}  // namespace btb
