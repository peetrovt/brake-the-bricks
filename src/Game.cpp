#include "Game.h"

#include "GameConstants.h"
#include "Level.h"
#include "Random.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>

namespace btb {
namespace {

constexpr float kPi = 3.14159265358979f;
// Hitting the ball with the very end of the paddle sends it off at this angle from vertical.
constexpr float kMaxBounceAngle = kPi / 3.0f;
constexpr float kMultiballSpread = 0.4f;  // radians between the split balls
constexpr size_t kMaxBalls = 24;
constexpr int kPowerUpDropChance = 7;  // one broken brick in this many drops a power-up
constexpr float kSlowFactor = 0.65f;

constexpr float kLaserSpeed = 1500.0f;
constexpr float kLaserWidth = 6.0f;
constexpr float kLaserHeight = 28.0f;
constexpr float kLaserCooldown = 0.2f;

constexpr float kBarrierY = 1050.0f;
constexpr float kBarrierHeight = 8.0f;

constexpr float kFloatingTextSeconds = 1.2f;

// Keeps a long stall (window dragged, debugger) from replaying seconds of game at once.
constexpr int kMaxStepsPerUpdate = 60;

// In Effect order: how long each lasts, and the power-up whose icon shows it in the HUD.
constexpr std::array<float, 5> kEffectSeconds = {8.0f, 12.0f, 15.0f, 15.0f, 10.0f};
constexpr std::array<PowerUpKind, 5> kEffectIcons = {PowerUpKind::Fireball, PowerUpKind::Laser, PowerUpKind::Barrier,
                                                      PowerUpKind::Catch, PowerUpKind::Slow};

const gfx::Color kMessageColor{0.13f, 0.83f, 0.93f, 1.0f};
const gfx::Color kHudColor{0.91f, 0.47f, 0.98f, 1.0f};
const gfx::Color kWhite{1.0f, 1.0f, 1.0f, 1.0f};
const gfx::Color kBarrierColor{0.08f, 0.72f, 0.65f, 1.0f};
const gfx::Color kLaserGlowColor{1.0f, 0.54f, 0.12f, 0.35f};
const gfx::Color kLaserColor{1.0f, 0.93f, 0.7f, 1.0f};

const char* brickColourName(int colour) {
    static const std::array<const char*, kBrickColourCount> names = {"red",  "orange", "yellow", "green",
                                                                     "cyan", "blue",   "purple"};
    return names[colour];
}

const char* powerUpFile(int kind) {
    static const std::array<const char*, kPowerUpKindCount> files = {
        "powerup-extra-life.png", "powerup-multiball.png", "powerup-expand.png", "powerup-shrink.png",
        "powerup-laser.png",      "powerup-fireball.png",  "powerup-slow.png",   "powerup-barrier.png",
        "powerup-catch.png",      "powerup-bonus.png",
    };
    return files[kind];
}

// Points the ball up, steeper the closer to the paddle centre it was hit.
void aimFromPaddle(Ball& ball, float offset, const Player& player) {
    const float t = std::clamp(offset / (player.width() * 0.5f), -1.0f, 1.0f);
    ball.setAngle(t * kMaxBounceAngle);
}

void followPaddle(Ball& ball, const Player& player) {
    const float half = player.width() * 0.5f;
    ball.stuckOffset = std::clamp(ball.stuckOffset, -half, half);
    ball.x = std::clamp(player.centerX() + ball.stuckOffset, Ball::kRadius, kFieldWidth - Ball::kRadius);
    ball.y = Player::kTop - Ball::kRadius;
}

}  // namespace

Game::Game(const std::string& assetsDir) {
    auto load = [&](const std::string& name) {
        return std::make_unique<gfx::Texture>(assetsDir + "/images/" + name);
    };

    background_ = load("background.png");
    paddleImages_[static_cast<int>(PaddleSize::Short)] = load("paddle-short.png");
    paddleImages_[static_cast<int>(PaddleSize::Normal)] = load("paddle.png");
    paddleImages_[static_cast<int>(PaddleSize::Wide)] = load("paddle-wide.png");
    ballImage_ = load("ball.png");
    fireBallImage_ = load("ball-fire.png");
    for (int colour = 0; colour < kBrickColourCount; ++colour) {
        brickImages_[colour] = load(std::string("brick-") + brickColourName(colour) + ".png");
        crackedBrickImages_[colour] = load(std::string("brick-") + brickColourName(colour) + "-cracked.png");
    }
    metalBrickImage_ = load("brick-metal.png");
    for (int kind = 0; kind < kPowerUpKindCount; ++kind) {
        powerUpImages_[kind] = load(powerUpFile(kind));
    }

    startLevel();
    showMessage(State::Ready, "Press Enter to Start!");
}

void Game::startLevel() {
    bricks_ = buildLevel(levelIndex_);
    resetPaddleAndBall();
    pendingSeconds_ = 0.0;
}

void Game::resetPaddleAndBall() {
    powerUps_.clear();
    lasers_.clear();
    floatingTexts_.clear();
    effectSeconds_.fill(0.0f);
    player_.reset();
    balls_.assign(1, Ball{});
    followPaddle(balls_.front(), player_);
}

void Game::showMessage(State state, const std::string& message) {
    state_ = state;
    message_ = message;
}

void Game::launch() {
    state_ = State::Playing;
    releaseStuckBalls();
}

void Game::onKeyPress(int key) {
    if (key == GLFW_KEY_H || key == GLFW_KEY_F1) {
        helpVisible_ = !helpVisible_;
        return;
    }
    if (helpVisible_) {
        return;
    }

    switch (key) {
        case GLFW_KEY_ENTER:
        case GLFW_KEY_KP_ENTER:
            switch (state_) {
                case State::Ready:
                case State::LifeLost:
                    launch();
                    break;
                case State::Paused:
                    state_ = State::Playing;
                    break;
                case State::Playing:
                    showMessage(State::Paused, "PAUSE");
                    break;
                case State::GameOver:
                    // Like the original, a new game continues from the level that was lost.
                    score_ = 0;
                    lives_ = kPlayerDefaultLives;
                    startLevel();
                    launch();
                    break;
                case State::Won:
                    levelIndex_ = 0;
                    score_ = 0;
                    lives_ = kPlayerDefaultLives;
                    startLevel();
                    launch();
                    break;
                case State::LevelComplete:
                    startLevel();
                    launch();
                    break;
            }
            break;

        case GLFW_KEY_SPACE:
            if (state_ == State::Ready || state_ == State::LifeLost) {
                launch();
            } else if (state_ == State::Playing) {
                fire();
            }
            break;

        // Debug keys.
        case GLFW_KEY_Q:
            bricks_.erase(std::remove_if(bricks_.begin(), bricks_.end(), [](const Brick& b) { return !b.metal; }),
                          bricks_.end());
            break;
        case GLFW_KEY_W:
            player_.shrink();
            break;
        case GLFW_KEY_E:
            player_.grow();
            break;
        case GLFW_KEY_R:
            powerUps_.push_back(PowerUp::makeRandom(player_.centerX(), Player::kTop - 400.0f));
            break;
        default:
            // 1..9 and 0 drop each power-up kind in turn.
            if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
                const int kind = (key - GLFW_KEY_0 + kPowerUpKindCount - 1) % kPowerUpKindCount;
                powerUps_.push_back(
                    PowerUp::make(player_.centerX(), Player::kTop - 400.0f, static_cast<PowerUpKind>(kind)));
            }
            break;
    }
}

void Game::update(double seconds, int moveDirection) {
    if (helpVisible_) {
        return;
    }

    pendingSeconds_ += seconds;
    int steps = 0;
    while (pendingSeconds_ >= kStepSeconds && steps < kMaxStepsPerUpdate) {
        pendingSeconds_ -= kStepSeconds;
        ++steps;
        step(moveDirection);
    }
    if (steps == kMaxStepsPerUpdate) {
        pendingSeconds_ = 0.0;
    }
}

void Game::step(int moveDirection) {
    const float seconds = static_cast<float>(kStepSeconds);
    switch (state_) {
        case State::Ready:
        case State::LifeLost:
            player_.move(moveDirection, seconds);
            for (Ball& ball : balls_) {
                followPaddle(ball, player_);
            }
            break;
        case State::Playing:
            player_.move(moveDirection, seconds);
            moveBalls(seconds);
            if (balls_.empty()) {
                loseLife();
                return;
            }
            moveLasers(seconds);
            movePowerUps(seconds);
            updateEffects(seconds);
            for (auto it = floatingTexts_.begin(); it != floatingTexts_.end();) {
                it->age += seconds;
                it->y -= 60.0f * seconds;
                it = it->age > kFloatingTextSeconds ? floatingTexts_.erase(it) : it + 1;
            }
            if (levelCleared()) {
                if (levelIndex_ < static_cast<int>(levels().size()) - 1) {
                    ++levelIndex_;
                    showMessage(State::LevelComplete, "Press Enter for next Level!");
                } else {
                    showMessage(State::Won, "WIN!");
                }
            }
            break;
        default:
            break;
    }
}

void Game::moveBalls(float seconds) {
    const float distance = ballSpeed() * seconds;
    const Rect paddle = player_.rect();
    const float r = Ball::kRadius;

    for (auto it = balls_.begin(); it != balls_.end();) {
        Ball& ball = *it;
        if (ball.stuck) {
            followPaddle(ball, player_);
            ++it;
            continue;
        }

        ball.x += ball.dx * distance;
        ball.y += ball.dy * distance;

        if (ball.x < r) {
            ball.x = r;
            ball.dx = std::abs(ball.dx);
        } else if (ball.x > kFieldWidth - r) {
            ball.x = kFieldWidth - r;
            ball.dx = -std::abs(ball.dx);
        }
        if (ball.y < r) {
            ball.y = r;
            ball.dy = std::abs(ball.dy);
        }
        if (active(Effect::Barrier) && ball.dy > 0.0f && ball.y + r > kBarrierY) {
            ball.y = kBarrierY - r;
            ball.dy = -std::abs(ball.dy);
        }

        if (ball.dy > 0.0f && paddle.intersectsCircle(ball.x, ball.y, r)) {
            const float offset = ball.x - player_.centerX();
            if (ball.y < paddle.y + paddle.h * 0.5f) {
                ball.y = paddle.y - r;
                if (active(Effect::Catch)) {
                    ball.stuck = true;
                    ball.stuckOffset = offset;
                } else {
                    aimFromPaddle(ball, offset, player_);
                }
            } else {
                // Caught by the end of the paddle, too low to bounce up: knock it sideways.
                ball.dx = std::copysign(std::abs(ball.dx), offset);
            }
        }

        // A fireball smashes every breakable brick it touches; otherwise the ball bounces off one brick per step.
        for (size_t i = 0; i < bricks_.size();) {
            if (!bricks_[i].rect().intersectsCircle(ball.x, ball.y, r)) {
                ++i;
                continue;
            }
            if (active(Effect::Fireball) && !bricks_[i].metal) {
                hitBrick(i, true);
                continue;
            }
            ball.bounceOff(bricks_[i].rect());
            hitBrick(i, false);
            break;
        }

        if (ball.y - r > kFieldHeight) {
            it = balls_.erase(it);
        } else {
            ++it;
        }
    }
}

bool Game::hitBrick(size_t index, bool smash) {
    Brick& brick = bricks_[index];
    if (brick.metal) {
        return false;
    }
    if (!brick.hit(smash)) {
        score_ += 5;
        return false;
    }

    const float x = brick.centerX();
    const float y = brick.centerY();
    bricks_.erase(bricks_.begin() + static_cast<std::ptrdiff_t>(index));
    score_ += 10;
    if (randomInt(kPowerUpDropChance) == 0) {
        powerUps_.push_back(PowerUp::makeRandom(x, y));
    }
    return true;
}

void Game::moveLasers(float seconds) {
    for (auto it = lasers_.begin(); it != lasers_.end();) {
        it->y -= kLaserSpeed * seconds;
        const Rect bolt{it->x - kLaserWidth * 0.5f, it->y, kLaserWidth, kLaserHeight};
        bool gone = bolt.y + bolt.h < 0.0f;
        for (size_t i = 0; i < bricks_.size() && !gone; ++i) {
            if (bolt.intersects(bricks_[i].rect())) {
                hitBrick(i, false);  // metal bricks just absorb the bolt
                gone = true;
            }
        }
        it = gone ? lasers_.erase(it) : it + 1;
    }
}

void Game::movePowerUps(float seconds) {
    const Rect paddle = player_.rect();
    for (size_t i = 0; i < powerUps_.size();) {
        PowerUp& powerUp = powerUps_[i];
        powerUp.y += powerUp.speed * seconds;
        if (paddle.intersectsCircle(powerUp.x, powerUp.y, PowerUp::kRadius)) {
            const PowerUp caught = powerUp;
            powerUps_.erase(powerUps_.begin() + static_cast<std::ptrdiff_t>(i));
            applyPowerUp(caught);
        } else if (powerUp.y - PowerUp::kRadius > kFieldHeight) {
            powerUps_.erase(powerUps_.begin() + static_cast<std::ptrdiff_t>(i));
        } else {
            ++i;
        }
    }
}

void Game::updateEffects(float seconds) {
    for (int effect = 0; effect < kEffectCount; ++effect) {
        float& left = effectSeconds_[effect];
        if (left <= 0.0f) {
            continue;
        }
        left -= seconds;
        if (left <= 0.0f && static_cast<Effect>(effect) == Effect::Catch) {
            releaseStuckBalls();
        }
    }
    laserCooldown_ = std::max(0.0f, laserCooldown_ - seconds);
}

void Game::applyPowerUp(const PowerUp& powerUp) {
    score_ += 10;
    // Labels of power-ups caught close together stack upwards instead of overlapping.
    const auto recent = std::count_if(floatingTexts_.begin(), floatingTexts_.end(),
                                      [](const FloatingText& text) { return text.age < 0.4f; });
    floatingTexts_.push_back({powerUpName(powerUp.kind), powerUp.x, Player::kTop - 60.0f - 40.0f * recent, 0.0f});

    switch (powerUp.kind) {
        case PowerUpKind::ExtraLife:
            ++lives_;
            break;
        case PowerUpKind::Multiball: {
            releaseStuckBalls();
            std::vector<Ball> added;
            for (const Ball& ball : balls_) {
                for (float angle : {-kMultiballSpread, kMultiballSpread}) {
                    if (balls_.size() + added.size() >= kMaxBalls) {
                        break;
                    }
                    Ball copy = ball;
                    copy.rotate(angle);
                    added.push_back(copy);
                }
            }
            balls_.insert(balls_.end(), added.begin(), added.end());
            break;
        }
        case PowerUpKind::Expand:
            player_.grow();
            break;
        case PowerUpKind::Shrink:
            player_.shrink();
            break;
        case PowerUpKind::Laser:
            effectSeconds_[static_cast<int>(Effect::Laser)] = kEffectSeconds[static_cast<int>(Effect::Laser)];
            break;
        case PowerUpKind::Fireball:
            effectSeconds_[static_cast<int>(Effect::Fireball)] = kEffectSeconds[static_cast<int>(Effect::Fireball)];
            break;
        case PowerUpKind::Slow:
            effectSeconds_[static_cast<int>(Effect::Slow)] = kEffectSeconds[static_cast<int>(Effect::Slow)];
            break;
        case PowerUpKind::Barrier:
            effectSeconds_[static_cast<int>(Effect::Barrier)] = kEffectSeconds[static_cast<int>(Effect::Barrier)];
            break;
        case PowerUpKind::Catch:
            effectSeconds_[static_cast<int>(Effect::Catch)] = kEffectSeconds[static_cast<int>(Effect::Catch)];
            break;
        case PowerUpKind::Bonus:
            score_ += 500;
            break;
    }
}

void Game::fire() {
    releaseStuckBalls();
    if (active(Effect::Laser) && laserCooldown_ <= 0.0f) {
        const Rect paddle = player_.rect();
        const float y = Player::kTop - kLaserHeight;
        lasers_.push_back({paddle.x + 30.0f, y});
        lasers_.push_back({paddle.x + paddle.w - 30.0f, y});
        laserCooldown_ = kLaserCooldown;
    }
}

void Game::releaseStuckBalls() {
    for (Ball& ball : balls_) {
        if (!ball.stuck) {
            continue;
        }
        ball.stuck = false;
        aimFromPaddle(ball, ball.stuckOffset, player_);
        // Straight up would just bounce between the paddle and the ceiling.
        if (std::abs(ball.dx) < 0.17f) {
            ball.setAngle((randomInt(2) == 0 ? -1.0f : 1.0f) * randomFloat(0.26f, 0.52f));
        }
    }
}

void Game::loseLife() {
    if (--lives_ > 0) {
        resetPaddleAndBall();
        showMessage(State::LifeLost, "Lives left: " + std::to_string(lives_));
    } else {
        showMessage(State::GameOver, "GAME OVER");
    }
}

float Game::ballSpeed() const {
    const float speed = levels()[levelIndex_].ballSpeed;
    return active(Effect::Slow) ? speed * kSlowFactor : speed;
}

bool Game::levelCleared() const {
    return std::all_of(bricks_.begin(), bricks_.end(), [](const Brick& b) { return b.metal; });
}

void Game::drawSprite(const gfx::Texture& texture, const Rect& visible, float alpha) const {
    gfx::drawImage(texture,
                   {visible.x - kGlowPadding, visible.y - kGlowPadding, visible.w + 2.0f * kGlowPadding,
                    visible.h + 2.0f * kGlowPadding},
                   alpha);
}

void Game::draw() const {
    gfx::drawImage(*background_, {0.0f, 0.0f, kFieldWidth, kFieldHeight});

    for (const Brick& brick : bricks_) {
        const gfx::Texture& image = brick.metal     ? *metalBrickImage_
                                    : brick.cracked ? *crackedBrickImages_[static_cast<int>(brick.colour)]
                                                    : *brickImages_[static_cast<int>(brick.colour)];
        gfx::drawImage(image, {brick.x, brick.y, Brick::kWidth, Brick::kHeight});
    }

    if (active(Effect::Barrier)) {
        // Blinks during its last two seconds.
        const float left = effectSeconds_[static_cast<int>(Effect::Barrier)];
        const float alpha = left > 2.0f ? 1.0f : 0.55f + 0.45f * std::cos(left * 12.0f);
        gfx::Color glow = kBarrierColor;
        glow.a = 0.3f * alpha;
        gfx::fillRect({0.0f, kBarrierY - 6.0f, kFieldWidth, kBarrierHeight + 12.0f}, glow);
        gfx::Color core = kBarrierColor;
        core.a = 0.95f * alpha;
        gfx::fillRect({0.0f, kBarrierY, kFieldWidth, kBarrierHeight}, core);
    }

    for (const Laser& laser : lasers_) {
        gfx::fillRect({laser.x - kLaserWidth, laser.y - 4.0f, kLaserWidth * 2.0f, kLaserHeight + 8.0f}, kLaserGlowColor);
        gfx::fillRect({laser.x - kLaserWidth * 0.5f, laser.y, kLaserWidth, kLaserHeight}, kLaserColor);
    }
    for (const PowerUp& powerUp : powerUps_) {
        drawSprite(*powerUpImages_[static_cast<int>(powerUp.kind)], powerUp.rect());
    }
    drawSprite(*paddleImages_[static_cast<int>(player_.size())], player_.rect());
    const gfx::Texture& ballImage = active(Effect::Fireball) ? *fireBallImage_ : *ballImage_;
    for (const Ball& ball : balls_) {
        drawSprite(ballImage, ball.rect());
    }

    for (const FloatingText& text : floatingTexts_) {
        const float scale = 4.0f;
        gfx::drawText(text.text, text.x - gfx::textWidth(text.text, scale) * 0.5f, text.y, scale,
                      {1.0f, 0.95f, 0.6f, 1.0f - text.age / kFloatingTextSeconds});
    }

    if (state_ != State::Playing) {
        drawMessage(message_);
    }
    drawHud();
    if (helpVisible_) {
        drawHelp();
    }
}

void Game::drawMessage(const std::string& text) const {
    // As big as fits the field, up to 12 px per font pixel.
    const float scale = std::clamp(std::floor(1800.0f / gfx::textWidth(text, 1.0f)), 1.0f, 12.0f);
    const float x = (kFieldWidth - gfx::textWidth(text, scale)) * 0.5f;
    // Between the lowest brick row of the tallest level and the paddle.
    const float y = 840.0f - 3.5f * scale;
    gfx::drawText(text, x, y, scale, kMessageColor);
}

void Game::drawHud() const {
    const float scale = 4.0f;
    const float y = 24.0f;
    const float spacing = 60.0f;

    float x = 40.0f;
    for (const std::string& text : {"Score: " + std::to_string(score_), "Lives: " + std::to_string(lives_),
                                    "Level " + std::to_string(levelIndex_ + 1) + "/" +
                                        std::to_string(levels().size())}) {
        gfx::drawText(text, x, y, scale, kHudColor);
        x += gfx::textWidth(text, scale) + spacing;
    }

    const std::string help = "H - Help";
    float right = kFieldWidth - 40.0f - gfx::textWidth(help, scale);
    gfx::drawText(help, right, y, scale, {1.0f, 1.0f, 1.0f, 0.6f});

    // Running effects: an icon with a bar for the time left, right to left before the help hint.
    const float icon = 44.0f;
    right -= spacing;
    for (int effect = 0; effect < kEffectCount; ++effect) {
        const float left = effectSeconds_[effect];
        if (left <= 0.0f) {
            continue;
        }
        right -= icon;
        const gfx::Rect dest{right, 14.0f, icon, icon};
        // The orb fills 96 of the 120 px sprite.
        const float pad = icon * kGlowPadding / 96.0f;
        gfx::drawImage(*powerUpImages_[static_cast<int>(kEffectIcons[effect])],
                       {dest.x - pad, dest.y - pad, dest.w + 2.0f * pad, dest.h + 2.0f * pad});
        gfx::fillRect({right, dest.y + icon + 6.0f, icon, 5.0f}, {1.0f, 1.0f, 1.0f, 0.2f});
        gfx::fillRect({right, dest.y + icon + 6.0f, icon * left / kEffectSeconds[effect], 5.0f}, kWhite);
        right -= 16.0f;
    }
}

void Game::drawHelp() const {
    const gfx::Rect panel{360.0f, 110.0f, 1200.0f, 860.0f};
    gfx::fillRect({0.0f, 0.0f, kFieldWidth, kFieldHeight}, {0.0f, 0.0f, 0.0f, 0.55f});
    gfx::fillRect(panel, {0.04f, 0.04f, 0.13f, 0.94f});

    auto centred = [&](const std::string& text, float y, float scale, const gfx::Color& color) {
        gfx::drawText(text, panel.x + (panel.w - gfx::textWidth(text, scale)) * 0.5f, y, scale, color);
    };
    centred("Brake The Bricks 2.0", panel.y + 40.0f, 6.0f, kMessageColor);
    centred("Left / Right - Move    Enter - Start / Pause", panel.y + 120.0f, 3.0f, kWhite);
    centred("Space - Fire / Release    F11 - Fullscreen    Esc - Exit", panel.y + 160.0f, 3.0f, kWhite);

    const float icon = 64.0f;
    const float rowHeight = 100.0f;
    for (int kind = 0; kind < kPowerUpKindCount; ++kind) {
        const float x = panel.x + 80.0f + (kind / 5) * 560.0f;
        const float y = panel.y + 230.0f + (kind % 5) * rowHeight;
        const float pad = icon * kGlowPadding / 96.0f;
        gfx::drawImage(*powerUpImages_[kind], {x - pad, y - pad, icon + 2.0f * pad, icon + 2.0f * pad});
        const auto powerUp = static_cast<PowerUpKind>(kind);
        gfx::drawText(powerUpName(powerUp), x + icon + 24.0f, y + 4.0f, 4.0f, kMessageColor);
        gfx::drawText(powerUpDescription(powerUp), x + icon + 24.0f, y + 40.0f, 3.0f, kWhite);
    }

    centred("Debug: Q clear level, W/E paddle size, R random power-up, 1-0 drop power-up", panel.y + 790.0f, 2.0f,
            {1.0f, 1.0f, 1.0f, 0.6f});
    centred("H - Close help", panel.y + 820.0f, 2.0f, {1.0f, 1.0f, 1.0f, 0.6f});
}

}  // namespace btb
