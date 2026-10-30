#pragma once

#include "Ball.h"
#include "Brick.h"
#include "GameConstants.h"
#include "Player.h"
#include "PowerUp.h"
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

    // Power-ups that last for a while.
    enum class Effect { Fireball, Laser, Barrier, Catch, Slow };
    static constexpr int kEffectCount = 5;

    struct Laser {
        float x;  // centre
        float y;  // top
    };

    // A short label that rises and fades where a power-up was caught.
    struct FloatingText {
        std::string text;
        float x;
        float y;
        float age;  // seconds
    };

    void startLevel();
    // Clears power-ups and puts a single ball on a normal-sized paddle.
    void resetPaddleAndBall();
    void launch();
    void step(int moveDirection);
    void moveBalls(float seconds);
    void moveLasers(float seconds);
    void movePowerUps(float seconds);
    void updateEffects(float seconds);
    // Damages brick `index` and returns true if it was destroyed (and removed).
    bool hitBrick(size_t index, bool smash);
    void fire();
    void releaseStuckBalls();
    void loseLife();
    void applyPowerUp(const PowerUp& powerUp);
    void showMessage(State state, const std::string& message);

    bool active(Effect effect) const { return effectSeconds_[static_cast<int>(effect)] > 0.0f; }
    float ballSpeed() const;
    bool levelCleared() const;

    void drawSprite(const gfx::Texture& texture, const Rect& visible, float alpha = 1.0f) const;
    void drawMessage(const std::string& text) const;
    void drawHud() const;
    void drawHelp() const;

    std::unique_ptr<gfx::Texture> background_;
    std::array<std::unique_ptr<gfx::Texture>, 3> paddleImages_;
    std::unique_ptr<gfx::Texture> ballImage_;
    std::unique_ptr<gfx::Texture> fireBallImage_;
    std::array<std::unique_ptr<gfx::Texture>, kBrickColourCount> brickImages_;
    std::array<std::unique_ptr<gfx::Texture>, kBrickColourCount> crackedBrickImages_;
    std::unique_ptr<gfx::Texture> metalBrickImage_;
    std::array<std::unique_ptr<gfx::Texture>, kPowerUpKindCount> powerUpImages_;

    Player player_;
    std::vector<Ball> balls_;
    std::vector<Brick> bricks_;
    std::vector<PowerUp> powerUps_;
    std::vector<Laser> lasers_;
    std::vector<FloatingText> floatingTexts_;
    std::array<float, kEffectCount> effectSeconds_{};  // time left of each effect
    float laserCooldown_ = 0.0f;

    State state_ = State::Ready;
    std::string message_;
    bool helpVisible_ = false;
    double pendingSeconds_ = 0.0;  // real time not yet turned into game steps
    int levelIndex_ = 0;
    int lives_ = kPlayerDefaultLives;
    int score_ = 0;
};

}  // namespace btb
