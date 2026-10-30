#include "Game.h"

#include "GameConstants.h"
#include "Level.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>

namespace btb {
namespace {

constexpr int kLastLevel = static_cast<int>(kLevels.size()) - 1;
constexpr double kBonusMessageSeconds = 0.5;
// Keeps a long stall (window dragged, debugger) from replaying seconds of game at once.
constexpr int kMaxStepsPerUpdate = 100;

const gfx::Color kMessageColor{0.0f, 1.0f, 1.0f, 1.0f};
const gfx::Color kHudColor{1.0f, 0.0f, 1.0f, 1.0f};
const gfx::Color kWhite{1.0f, 1.0f, 1.0f, 1.0f};

}  // namespace

Game::Game(const std::string& assetsDir) {
    auto load = [&](const char* name) { return std::make_unique<gfx::Texture>(assetsDir + "/images/" + name); };

    background_ = load("background.png");
    playerImage_ = load("player2.png");

    brickImages_[static_cast<int>(BrickColour::Green)] = load("brick-green2.png");
    brickImages_[static_cast<int>(BrickColour::Yellow)] = load("brick-yellow2.png");
    brickImages_[static_cast<int>(BrickColour::Orange)] = load("brick-orange2.png");
    brickImages_[static_cast<int>(BrickColour::Red)] = load("brick-red2.png");
    brickImages_[static_cast<int>(BrickColour::Blue)] = load("brick-blue2.png");

    fallingObjectImages_[static_cast<int>(FallingObjectKind::SmallerPlayer)] = load("grenade_PNG1351.png");
    fallingObjectImages_[static_cast<int>(FallingObjectKind::BiggerPlayer)] = load("star_PNG1591.png");
    fallingObjectImages_[static_cast<int>(FallingObjectKind::ExtraLife)] = load("heart_PNG691.png");
    fallingObjectImages_[static_cast<int>(FallingObjectKind::BallSpeedUp)] = load("shield_PNG1281.png");
    fallingObjectImages_[static_cast<int>(FallingObjectKind::BallSpeedDown)] = load("shield_PNG1283.png");

    startLevel();
    showMessage(State::Ready, "Press Enter to Start!");
}

void Game::startLevel() {
    bricks_ = buildLevel(levelIndex_);
    fallingObjects_.clear();
    player_.reset();
    ball_.reset(player_);
    pendingMs_ = 0.0;
}

void Game::showMessage(State state, const std::string& message) {
    state_ = state;
    message_ = message;
}

void Game::onKeyPress(int key) {
    if (key == GLFW_KEY_H || key == GLFW_KEY_F1) {
        helpVisible_ = !helpVisible_;
        return;
    }
    if (helpVisible_ || bonusMessageSeconds_ > 0.0) {
        return;
    }

    switch (key) {
        case GLFW_KEY_ENTER:
        case GLFW_KEY_KP_ENTER:
            switch (state_) {
                case State::Ready:
                case State::Paused:
                case State::LifeLost:
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
                    state_ = State::Playing;
                    break;
                case State::Won:
                    levelIndex_ = 0;
                    score_ = 0;
                    lives_ = kPlayerDefaultLives;
                    startLevel();
                    state_ = State::Playing;
                    break;
                case State::LevelComplete:
                    startLevel();
                    state_ = State::Playing;
                    break;
            }
            break;

        // Debug keys from the original game.
        case GLFW_KEY_Q:
            bricks_.clear();
            break;
        case GLFW_KEY_A:
            ball_.stop();
            break;
        case GLFW_KEY_Z:
            ball_.resume();
            break;
        case GLFW_KEY_W:
            player_.makeSmaller();
            break;
        case GLFW_KEY_E:
            player_.makeBigger();
            break;
        case GLFW_KEY_R:
            fallingObjects_.push_back(FallingObject::makeRandom(ball_.x(), ball_.y()));
            break;
        default:
            break;
    }
}

void Game::update(double seconds, int moveDirection) {
    if (helpVisible_) {
        return;
    }
    if (bonusMessageSeconds_ > 0.0) {
        bonusMessageSeconds_ -= seconds;
        return;
    }

    const int stepMs = kLevels[levelIndex_].tickMs;
    pendingMs_ += seconds * 1000.0;
    int steps = 0;
    while (pendingMs_ >= stepMs && steps < kMaxStepsPerUpdate) {
        pendingMs_ -= stepMs;
        ++steps;
        step(moveDirection);
        if (bonusMessageSeconds_ > 0.0) {
            break;
        }
    }
    if (steps == kMaxStepsPerUpdate) {
        pendingMs_ = 0.0;
    }
}

void Game::step(int moveDirection) {
    switch (state_) {
        case State::Ready:
        case State::LifeLost:
            player_.move(moveDirection);
            ball_.followPlayer(player_);
            break;
        case State::Playing:
            ball_.move();
            player_.move(moveDirection);
            checkCollisions();
            for (FallingObject& object : fallingObjects_) {
                object.move();
            }
            break;
        default:
            break;
    }
}

void Game::checkCollisions() {
    if (ball_.y() >= kGameHeight - 2 * kBallSize) {
        loseLife();
        return;
    }

    if (ball_.rect().intersects(player_.topEdge())) {
        ball_.setYDir(-1);
        // Hitting the ball with the outer fifth of the paddle sends it back the way it came.
        if (ball_.xDir() == 1 && ball_.x() < player_.x() + player_.width() / 5) {
            ball_.flipXDir();
        }
        if (ball_.xDir() == -1 && ball_.x() > player_.x() + player_.width() * 4 / 5) {
            ball_.flipXDir();
        }
    }
    // The ball has been frozen with the debug key.
    if (ball_.xDir() == 0) {
        return;
    }
    if (ball_.rect().intersects(player_.sideEdge(ball_.xDir()))) {
        ball_.flipXDir();
    }

    // At most one brick per step.
    for (size_t i = 0; i < bricks_.size(); ++i) {
        if (ball_.rect().intersects(bricks_[i].rect())) {
            hitBrick(i);
            break;
        }
    }
    if (bricks_.empty()) {
        if (levelIndex_ < kLastLevel) {
            ++levelIndex_;
            showMessage(State::LevelComplete, "Press Enter for next Level!");
        } else {
            showMessage(State::Won, "WIN!");
        }
        return;
    }

    for (auto it = fallingObjects_.begin(); it != fallingObjects_.end();) {
        if (it->rect().intersects(player_.topEdge())) {
            score_ += 10;
            FallingObjectKind kind = it->kind;
            it = fallingObjects_.erase(it);
            applyBonus(kind);
        } else if (it->y > kGameHeight) {
            it = fallingObjects_.erase(it);
        } else {
            ++it;
        }
    }
}

void Game::hitBrick(size_t index) {
    const Brick brick = bricks_[index];
    if (brick.hasFallingObject) {
        fallingObjects_.push_back(FallingObject::makeRandom(ball_.x(), ball_.y()));
    }

    if (brick.colour != BrickColour::Green) {
        bricks_[index] = Brick::make(brick.x, brick.y, static_cast<BrickColour>(static_cast<int>(brick.colour) - 1));
        score_ += 5;
    } else {
        bricks_.erase(bricks_.begin() + static_cast<std::ptrdiff_t>(index));
        score_ += 10;
    }

    if (ball_.rect().intersects(brick.horizontalEdge(ball_.yDir()))) {
        ball_.flipYDir();
    }
    if (ball_.rect().intersects(brick.verticalEdge(ball_.xDir()))) {
        ball_.flipXDir();
    }
}

void Game::loseLife() {
    if (--lives_ > 0) {
        fallingObjects_.clear();
        player_.reset();
        ball_.reset(player_);
        showMessage(State::LifeLost, "Lives left: " + std::to_string(lives_));
    } else {
        showMessage(State::GameOver, "GAME OVER");
    }
}

void Game::applyBonus(FallingObjectKind kind) {
    switch (kind) {
        case FallingObjectKind::SmallerPlayer:
            player_.makeSmaller();
            break;
        case FallingObjectKind::BiggerPlayer:
            player_.makeBigger();
            break;
        case FallingObjectKind::ExtraLife:
            ++lives_;
            message_ = "+1 Life";
            bonusMessageSeconds_ = kBonusMessageSeconds;
            break;
        case FallingObjectKind::BallSpeedUp:
            ball_.accelerate();
            break;
        case FallingObjectKind::BallSpeedDown:
            ball_.slowDown();
            break;
    }
}

void Game::draw() const {
    gfx::drawImage(*background_, {0.0f, 0.0f, static_cast<float>(kBackgroundWidth), static_cast<float>(kGameHeight)});
    gfx::drawImage(*playerImage_, {static_cast<float>(player_.x()), static_cast<float>(player_.y()),
                                   static_cast<float>(player_.width()), static_cast<float>(Player::kHeight)});

    const float radius = kBallSize * 0.5f;
    gfx::fillCircle(ball_.x() + radius, ball_.y() + radius, radius, kWhite);

    for (const Brick& brick : bricks_) {
        gfx::drawImage(*brickImages_[static_cast<int>(brick.colour)],
                       {static_cast<float>(brick.x), static_cast<float>(brick.y), static_cast<float>(Brick::kWidth),
                        static_cast<float>(Brick::kHeight)});
    }
    for (const FallingObject& object : fallingObjects_) {
        gfx::drawImage(*fallingObjectImages_[static_cast<int>(object.kind)],
                       {static_cast<float>(object.x), static_cast<float>(object.y),
                        static_cast<float>(kFallingObjectSize), static_cast<float>(kFallingObjectSize)});
    }

    if (state_ != State::Playing || bonusMessageSeconds_ > 0.0) {
        drawMessage(message_);
    }
    drawHud();
    if (helpVisible_) {
        drawHelp();
    }
}

void Game::drawMessage(const std::string& text) const {
    // As big as fits the field, up to 5 px per font pixel.
    const float maxWidth = kVisibleWidth - 24.0f;
    const float scale = std::clamp(std::floor(maxWidth / gfx::textWidth(text, 1.0f)), 1.0f, 5.0f);
    const float x = (kVisibleWidth - gfx::textWidth(text, scale)) * 0.5f;
    const float y = kGameHeight * 2.0f / 3.0f - 7.0f * scale;
    gfx::drawText(text, x, y, scale, kMessageColor);
}

void Game::drawHud() const {
    const float scale = 2.0f;
    const std::string score = "Score: " + std::to_string(score_);
    gfx::drawText(score, 20.0f, 16.0f, scale, kHudColor);
    gfx::drawText("Lives: " + std::to_string(lives_), 20.0f + gfx::textWidth(score, scale) + 24.0f, 16.0f, scale,
                  kHudColor);
    gfx::drawText("H - Help", kVisibleWidth - 20.0f - gfx::textWidth("H - Help", scale), 16.0f, scale,
                  {1.0f, 1.0f, 1.0f, 0.6f});
}

void Game::drawHelp() const {
    static const std::array<const char*, 10> lines = {
        "Brake The Bricks 2.0",
        "",
        "Left / Right - Move",
        "Enter - Start / Pause",
        "Esc - Exit",
        "H - Close help",
        "",
        "Debug: Q clear level,",
        "A/Z stop/resume ball,",
        "W/E smaller/bigger, R bonus",
    };
    const float scale = 2.0f;
    const float lineHeight = 7.0f * scale + 10.0f;
    const float panelHeight = lines.size() * lineHeight + 30.0f;
    const float panelY = (kGameHeight - panelHeight) * 0.5f;

    gfx::fillRect({0.0f, 0.0f, static_cast<float>(kVisibleWidth), static_cast<float>(kGameHeight)},
                  {0.0f, 0.0f, 0.0f, 0.5f});
    gfx::fillRect({20.0f, panelY, kVisibleWidth - 40.0f, panelHeight}, {0.05f, 0.05f, 0.15f, 0.92f});
    for (size_t i = 0; i < lines.size(); ++i) {
        const float x = (kVisibleWidth - gfx::textWidth(lines[i], scale)) * 0.5f;
        gfx::drawText(lines[i], x, panelY + 20.0f + i * lineHeight, scale, i == 0 ? kMessageColor : kWhite);
    }
}

}  // namespace btb
