#pragma once

#include "Animation.h"
#include "Audio.h"
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
    // Loads the art from `dataDir` (the folder holding assets ); the OpenGL context must be current.
    // Throws std::runtime_error if an image is missing.
    explicit Game(const std::string& dataDir);

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

    // In the order of kSoundFiles in Game.cpp.
    enum class Sound {
        PaddleHit, WallBounce, BrickHit1, BrickHit2, BrickHit3, BrickBreak1, BrickBreak2, BrickBreak3, GoldBrickHit,
        BallLaunch, PowerUpSpawn, PowerUpCollect, PowerDownCurse, ExtraLife, Combo, LifeLost, LevelComplete, GameOver,
        UiClick, UiHover, UiPause, UiStoneDoor, AmbientLoop,
    };
    static constexpr int kSoundCount = 23;

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

    // A non-looping animation playing somewhere on the field.
    struct OneShot {
        const Animation* animation;
        float x;
        float y;
        float rotation;
        gfx::Blend blend;
        float age;  // seconds
    };

    struct Particle {
        const gfx::Texture* image;
        float x;  // centre
        float y;
        float vx;
        float vy;
        float life;  // seconds
        float brightness;
        gfx::Blend blend;
        bool fadeIn;  // otherwise it starts at full brightness
        float age;   // seconds
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
    // Damages brick `index`, plays its animations and returns true if it was destroyed (and removed).
    // (fromX, fromY) is where the blow came from, outside the brick.
    bool hitBrick(size_t index, bool smash, float fromX, float fromY);
    void spawn(const Animation& animation, float x, float y, float rotation = 0.0f,
               gfx::Blend blend = gfx::Blend::Normal);
    void shakeScreen(float pixels);
    Particle makeDustMote(float age) const;
    void updateVisuals(float seconds);
    void fire();
    // Returns true if any ball was stuck to the paddle.
    bool releaseStuckBalls();
    void loseLife();
    void applyPowerUp(const PowerUp& powerUp);
    void showMessage(State state, const std::string& message);
    // `x` is where on the field it happens, for the stereo position.
    void playSound(Sound sound, float x = kFieldWidth * 0.5f, float volume = 1.0f);
    // One of three numbered variants at random, so repeated hits do not sound mechanical.
    void playVariant(Sound first, float x, float volume = 1.0f);

    bool active(Effect effect) const { return effectSeconds_[static_cast<int>(effect)] > 0.0f; }
    float ballSpeed() const;
    bool levelCleared() const;

    void drawTorches() const;
    void drawParticles(const std::vector<Particle>& particles) const;
    void drawBrick(const Brick& brick) const;
    void drawBall(const Ball& ball) const;
    void drawMessage(const std::string& text) const;
    void drawBanner(const std::string& text) const;
    void drawHud() const;
    void drawHelp() const;

    audio::Mixer mixer_;
    std::array<int, kSoundCount> sounds_{};  // clip ids, -1 if the file could not be loaded

    std::unique_ptr<gfx::Font> font_;
    std::unique_ptr<gfx::Texture> background_;
    std::unique_ptr<gfx::Texture> torchGlow_;
    std::unique_ptr<gfx::Texture> dustMoteImage_;
    std::unique_ptr<gfx::Texture> emberImage_;
    std::array<std::unique_ptr<gfx::Texture>, 3> paddleImages_;
    std::unique_ptr<gfx::Texture> ballImage_;
    std::unique_ptr<gfx::Texture> trailImage_;
    // By type, variant and number of cracks; combinations the art pack does not have stay empty.
    std::array<std::array<std::array<std::unique_ptr<gfx::Texture>, kMaxBrickHits>, kMaxBrickVariants>, kBrickTypeCount>
        brickImages_;
    std::array<std::unique_ptr<gfx::Texture>, kPowerUpKindCount> powerUpImages_;
    std::unique_ptr<gfx::Texture> panelImage_;
    std::unique_ptr<gfx::Texture> bannerImage_;
    std::unique_ptr<gfx::Texture> lifeIcon_;

    std::array<std::unique_ptr<Animation>, kBrickTypeCount> shatterAnimations_;  // none for obsidian
    std::unique_ptr<Animation> hitFlash_;
    std::unique_ptr<Animation> obsidianClang_;
    std::unique_ptr<Animation> impactDust_;
    std::unique_ptr<Animation> impactSpark_;
    std::array<std::unique_ptr<Animation>, 3> paddleGlow_;
    std::unique_ptr<Animation> torchFlame_;
    std::unique_ptr<Animation> ballFire_;
    std::unique_ptr<Animation> ballGlowPulse_;
    std::unique_ptr<Animation> ballLost_;
    std::array<std::unique_ptr<Animation>, kPowerUpKindCount> powerUpAnimations_;
    std::unique_ptr<Animation> sparkleBurst_;

    Player player_;
    std::vector<Ball> balls_;
    std::vector<Brick> bricks_;
    std::vector<PowerUp> powerUps_;
    std::vector<Laser> lasers_;
    std::vector<FloatingText> floatingTexts_;
    std::array<float, kEffectCount> effectSeconds_{};  // time left of each effect
    float laserCooldown_ = 0.0f;
    int comboCount_ = 0;         // bricks broken in quick succession
    float comboSeconds_ = 0.0f;  // time left to break the next one and keep the combo going

    std::vector<OneShot> oneShots_;
    std::vector<Particle> dustMotes_;
    std::vector<Particle> embers_;
    std::vector<Particle> trail_;
    float clock_ = 0.0f;              // drives the looping animations
    float emberTimer_ = 0.0f;
    float sparkleTimer_ = 0.0f;
    float bannerSeconds_ = 0.0f;      // since the current message appeared
    float paddleGlowSeconds_ = -1.0f;  // since the last bounce; negative when not animating
    float shakeSeconds_ = 0.0f;       // time left
    float shakePixels_ = 0.0f;
    float shakeX_ = 0.0f;
    float shakeY_ = 0.0f;
    int trailSteps_ = 0;

    State state_ = State::Ready;
    std::string message_;
    bool helpVisible_ = false;
    double pendingSeconds_ = 0.0;  // real time not yet turned into game steps
    int levelIndex_ = 0;
    int lives_ = kPlayerDefaultLives;
    int score_ = 0;
};

}  // namespace btb
