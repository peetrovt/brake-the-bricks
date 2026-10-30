#pragma once

#include "Ball.h"
#include "Brick.h"
#include "FallingObject.h"
#include "Player.h"
#include "Renderer.h"

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace btb {

class Game {
public:
    // Loads the images from `assetsDir`; the OpenGL context must be current.
    // Throws std::runtime_error if an image is missing.
    explicit Game(const std::string& assetsDir);

    // `key` is a GLFW key code.
    void onKeyPress(int key);
    // Advances the game by `seconds` of real time. `moveDirection` is -1, 0 or 1 from the arrow keys held.
    void update(double seconds, int moveDirection);
    void draw() const;

private:
    enum class State {
        Ready,          // before the first launch: the ball sits on the paddle
        Playing,
        Paused,
        LifeLost,       // the ball sits on the paddle until Enter
        LevelComplete,
        GameOver,
        Won,
    };

    void startLevel();
    void step(int moveDirection);
    void checkCollisions();
    void hitBrick(size_t index);
    void loseLife();
    void applyBonus(FallingObjectKind kind);
    void showMessage(State state, const std::string& message);

    void drawMessage(const std::string& text) const;
    void drawHud() const;
    void drawHelp() const;

    std::unique_ptr<gfx::Texture> background_;
    std::unique_ptr<gfx::Texture> playerImage_;
    std::array<std::unique_ptr<gfx::Texture>, kBrickColourCount> brickImages_;
    std::array<std::unique_ptr<gfx::Texture>, kFallingObjectKindCount> fallingObjectImages_;

    Player player_;
    Ball ball_;
    std::vector<Brick> bricks_;
    std::vector<FallingObject> fallingObjects_;

    State state_ = State::Ready;
    std::string message_;
    double bonusMessageSeconds_ = 0.0;  // the game holds still while "+1 Life" is shown
    bool helpVisible_ = false;
    double pendingMs_ = 0.0;  // real time not yet turned into game steps
    int levelIndex_ = 0;
    int lives_ = kPlayerDefaultLives;
    int score_ = 0;
};

}  // namespace btb
