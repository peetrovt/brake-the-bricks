#pragma once

namespace btb {

// The playing field matches the Full HD background, in pixels.
constexpr float kFieldWidth = 1920.0f;
constexpr float kFieldHeight = 1080.0f;

// Every sprite in the art pack except bricks and the background has a 12 px glow around the part that collides.
constexpr float kGlowPadding = 12.0f;

constexpr int kPlayerDefaultLives = 3;

// The game advances in fixed steps, independent of the frame rate.
constexpr double kStepSeconds = 1.0 / 240.0;

}  // namespace btb
