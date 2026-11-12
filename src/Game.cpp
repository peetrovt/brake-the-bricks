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

// Extra points for a gold brick, on top of the usual 10.
constexpr int kGoldBonus = 50;

constexpr float kTorchY = 560.0f;
constexpr std::array<float, 2> kTorchX = {50.0f, 1870.0f};
constexpr float kBallLostY = 1040.0f;
constexpr float kBannerCentreY = 800.0f;
constexpr float kPanelY = 12.0f;
constexpr float kPanelWidth = 400.0f;
constexpr float kPanelHeight = 80.0f;

constexpr float kShakeSeconds = 0.18f;
constexpr float kShakeBrick = 4.0f;
constexpr float kShakeGold = 8.0f;
constexpr int kDustMoteCount = 60;
constexpr float kEmberInterval = 0.08f;
constexpr int kStepsPerTrailParticle = 2;
constexpr float kBannerGrowSeconds = 0.35f;
constexpr float kSparkleInterval = 0.15f;
constexpr float kSparkleSeconds = 3.0f;  // how long a cleared level keeps sparkling

// brick_hit_flash in atlas.json: how far the brick is nudged on each frame.
constexpr std::array<std::array<float, 2>, 6> kHitShake = {
    {{0.0f, 0.0f}, {3.0f, -1.0f}, {-3.0f, 1.0f}, {2.0f, 0.0f}, {-1.0f, 0.0f}, {0.0f, 0.0f}}};

// In Game::Sound order; files in assets/sounds/.
constexpr std::array<const char*, 23> kSoundFiles = {
    "paddle_hit",     "wall_bounce",    "brick_hit_1",     "brick_hit_2",     "brick_hit_3", "brick_break_1",
    "brick_break_2",  "brick_break_3",  "gold_brick_hit",  "ball_launch",     "powerup_spawn", "powerup_collect",
    "powerdown_curse", "extra_life",    "combo",           "life_lost",       "level_complete", "game_over",
    "ui_click",       "ui_hover",       "ui_pause",        "ui_stone_door",   "ambient_temple_loop",
};
constexpr float kMusicVolume = 0.35f;
// How far left/right sounds at the walls are panned (1 would be fully to one side).
constexpr float kPanWidth = 0.6f;
// A combo: this many bricks broken, each within kComboSeconds of the last one.
constexpr int kComboBricks = 4;
constexpr float kComboSeconds = 0.6f;

const gfx::Color kMessageColor{1.0f, 0.82f, 0.42f, 1.0f};
const gfx::Color kHudColor{0.99f, 0.87f, 0.58f, 1.0f};
const gfx::Color kWhite{1.0f, 1.0f, 1.0f, 1.0f};
const gfx::Color kBarrierColor{1.0f, 0.72f, 0.26f, 1.0f};
const gfx::Color kLaserGlowColor{1.0f, 0.54f, 0.12f, 0.35f};
const gfx::Color kLaserColor{1.0f, 0.93f, 0.7f, 1.0f};

const char* brickTypeName(int type) {
    static const std::array<const char*, kBrickTypeCount> names = {"sandstone", "limestone", "granite",
                                                                   "jade",      "gold",      "obsidian"};
    return names[type];
}

// In PowerUpKind order; names the files in powerups/ and anim/.
const char* powerUpFile(int kind) {
    static const std::array<const char*, kPowerUpKindCount> names = {
        "life", "multiball", "expand", "shrink", "laser", "fireball", "slow", "barrier", "catch", "bonus",
    };
    return names[kind];
}

// Points the ball up, steeper the closer to the paddle centre it was hit.
void aimFromPaddle(Ball& ball, float offset, const Player& player) {
    const float t = std::clamp(offset / (player.width() * 0.5f), -1.0f, 1.0f);
    ball.setAngle(t * kMaxBounceAngle);
}

void followPaddle(Ball& ball, const Player& player) {
    const float half = player.width() * 0.5f;
    ball.stuckOffset = std::clamp(ball.stuckOffset, -half, half);
    ball.x = std::clamp(player.centerX() + ball.stuckOffset, kWallLeft + Ball::kRadius, kWallRight - Ball::kRadius);
    ball.y = Player::kTop - Ball::kRadius;
}

}  // namespace

Game::Game(const std::string& dataDir) {
    const std::string art = dataDir + "/assets/";
    auto load = [&](const std::string& name) { return std::make_unique<gfx::Texture>(art + name); };
    auto strip = [&](const std::string& name, const AnimationInfo& info) {
        return std::make_unique<Animation>(art + "anim/" + name + ".png", info);
    };

    font_ = std::make_unique<gfx::Font>(dataDir + "/assets/fonts/LiberationSans-Bold.ttf");

    background_ = load("background/background.png");
    torchGlow_ = load("background/torch_glow.png");
    dustMoteImage_ = load("background/dust_mote.png");
    emberImage_ = load("background/ember.png");
    paddleImages_[static_cast<int>(PaddleSize::Short)] = load("paddle/paddle_short.png");
    paddleImages_[static_cast<int>(PaddleSize::Normal)] = load("paddle/paddle_normal.png");
    paddleImages_[static_cast<int>(PaddleSize::Wide)] = load("paddle/paddle_long.png");
    ballImage_ = load("ball/ball.png");
    trailImage_ = load("ball/trail_particle.png");
    panelImage_ = load("ui/panel.png");
    bannerImage_ = load("ui/banner.png");
    lifeIcon_ = load("ui/life_icon.png");

    for (int type = 0; type < kBrickTypeCount; ++type) {
        const auto brickType = static_cast<BrickType>(type);
        for (int variant = 0; variant < brickVariants(brickType); ++variant) {
            const std::string base = std::string("bricks/") + brickTypeName(type) + "_" + static_cast<char>('a' + variant);
            brickImages_[type][variant][0] = load(base + ".png");
            for (int cracks = 1; cracks < brickMaxHits(brickType); ++cracks) {
                brickImages_[type][variant][cracks] = load(base + "_crack" + std::to_string(cracks) + ".png");
            }
        }
        if (brickType != BrickType::Obsidian) {
            shatterAnimations_[type] =
                strip(std::string("brick_shatter_") + brickTypeName(type), {320, 256, 18, 30.0f, false, 160.0f, 112.0f});
        }
    }
    for (int kind = 0; kind < kPowerUpKindCount; ++kind) {
        const std::string name = std::string("powerup_") + powerUpFile(kind);
        powerUpImages_[kind] = load("powerups/" + name + ".png");
        powerUpAnimations_[kind] = strip(name, {64, 64, 12, 15.0f, true, 32.0f, 32.0f});
    }

    hitFlash_ = strip("brick_hit_flash", {128, 48, 6, 30.0f, false, 64.0f, 24.0f});
    obsidianClang_ = strip("brick_clang_obsidian", {128, 48, 8, 30.0f, false, 64.0f, 24.0f});
    impactDust_ = strip("impact_dust", {96, 96, 8, 24.0f, false, 48.0f, 67.0f});
    impactSpark_ = strip("impact_spark", {64, 64, 8, 30.0f, false, 32.0f, 32.0f});
    paddleGlow_[static_cast<int>(PaddleSize::Short)] = strip("paddle_glow_short", {176, 48, 8, 24.0f, false, 88.0f, 24.0f});
    paddleGlow_[static_cast<int>(PaddleSize::Normal)] = strip("paddle_glow_normal", {256, 48, 8, 24.0f, false, 128.0f, 24.0f});
    paddleGlow_[static_cast<int>(PaddleSize::Wide)] = strip("paddle_glow_long", {352, 48, 8, 24.0f, false, 176.0f, 24.0f});
    torchFlame_ = strip("torch_flame", {64, 128, 8, 12.0f, true, 32.0f, 118.0f});
    ballFire_ = strip("ball_fire", {96, 96, 8, 15.0f, true, 48.0f, 61.0f});
    ballGlowPulse_ = strip("ball_glow_pulse", {96, 96, 8, 12.0f, true, 48.0f, 48.0f});
    ballLost_ = strip("ball_lost", {128, 128, 14, 30.0f, false, 64.0f, 40.0f});
    sparkleBurst_ = strip("sparkle_burst", {256, 256, 16, 24.0f, false, 128.0f, 128.0f});

    for (int i = 0; i < kDustMoteCount; ++i) {
        // Part way through their lives, so the room is not empty at the start.
        dustMotes_.push_back(makeDustMote(randomFloat(0.0f, 4.0f)));
    }

    static_assert(kSoundFiles.size() == kSoundCount, "one file per Game::Sound");
    for (int i = 0; i < kSoundCount; ++i) {
        sounds_[i] = mixer_.load(dataDir + "/assets/sounds/" + kSoundFiles[i] + ".ogg");
    }
    mixer_.loop(sounds_[static_cast<int>(Sound::AmbientLoop)], kMusicVolume);

    startLevel();
    showMessage(State::Ready, "Press Enter to Start!");
}

void Game::startLevel() {
    bricks_ = buildLevel(levelIndex_);
    oneShots_.clear();
    trail_.clear();
    paddleGlowSeconds_ = -1.0f;
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
    bannerSeconds_ = 0.0f;
}

void Game::playSound(Sound sound, float x, float volume) {
    const float pan = (x - kFieldWidth * 0.5f) / (kFieldWidth * 0.5f) * kPanWidth;
    mixer_.play(sounds_[static_cast<int>(sound)], volume, pan);
}

void Game::playVariant(Sound first, float x, float volume) {
    playSound(static_cast<Sound>(static_cast<int>(first) + randomInt(3)), x, volume);
}

void Game::launch() {
    state_ = State::Playing;
    if (releaseStuckBalls()) {
        playSound(Sound::BallLaunch, player_.centerX());
    }
}

void Game::onKeyPress(int key) {
    if (key == GLFW_KEY_H || key == GLFW_KEY_F1) {
        helpVisible_ = !helpVisible_;
        playSound(Sound::UiHover);
        return;
    }
    if (key == GLFW_KEY_M) {
        mixer_.setMuted(!mixer_.muted());
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
                    playSound(Sound::UiStoneDoor);
                    launch();
                    break;
                case State::LifeLost:
                    launch();
                    break;
                case State::Paused:
                    state_ = State::Playing;
                    playSound(Sound::UiPause);
                    break;
                case State::Playing:
                    showMessage(State::Paused, "PAUSE");
                    playSound(Sound::UiPause);
                    break;
                case State::GameOver:
                    // Like the original, a new game continues from the level that was lost.
                    score_ = 0;
                    lives_ = kPlayerDefaultLives;
                    startLevel();
                    playSound(Sound::UiStoneDoor);
                    launch();
                    break;
                case State::Won:
                    levelIndex_ = 0;
                    score_ = 0;
                    lives_ = kPlayerDefaultLives;
                    startLevel();
                    playSound(Sound::UiStoneDoor);
                    launch();
                    break;
                case State::LevelComplete:
                    startLevel();
                    playSound(Sound::UiStoneDoor);
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
            bricks_.erase(std::remove_if(bricks_.begin(), bricks_.end(), [](const Brick& b) { return !b.unbreakable(); }),
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
            playSound(Sound::PowerUpSpawn, player_.centerX());
            break;
        default:
            // 1..9 and 0 drop each power-up kind in turn.
            if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
                const int kind = (key - GLFW_KEY_0 + kPowerUpKindCount - 1) % kPowerUpKindCount;
                powerUps_.push_back(
                    PowerUp::make(player_.centerX(), Player::kTop - 400.0f, static_cast<PowerUpKind>(kind)));
                playSound(Sound::PowerUpSpawn, player_.centerX());
            }
            break;
    }
}

void Game::update(double seconds, int moveDirection) {
    if (helpVisible_) {
        return;
    }

    updateVisuals(static_cast<float>(seconds));

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
                playSound(Sound::LevelComplete);
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
    const bool leaveTrail = ++trailSteps_ % kStepsPerTrailParticle == 0;

    for (auto it = balls_.begin(); it != balls_.end();) {
        Ball& ball = *it;
        if (ball.stuck) {
            followPaddle(ball, player_);
            ++it;
            continue;
        }

        ball.x += ball.dx * distance;
        ball.y += ball.dy * distance;

        if (leaveTrail) {
            trail_.push_back({trailImage_.get(), ball.x, ball.y, 0.0f, 0.0f, 0.22f, 0.7f, gfx::Blend::Additive, false, 0.0f});
        }

        if (ball.x < kWallLeft + r) {
            ball.x = kWallLeft + r;
            ball.dx = std::abs(ball.dx);
            spawn(*impactSpark_, kWallLeft, ball.y);
            playSound(Sound::WallBounce, kWallLeft);
        } else if (ball.x > kWallRight - r) {
            ball.x = kWallRight - r;
            ball.dx = -std::abs(ball.dx);
            spawn(*impactSpark_, kWallRight, ball.y);
            playSound(Sound::WallBounce, kWallRight);
        }
        if (ball.y < kCeiling + r) {
            ball.y = kCeiling + r;
            ball.dy = std::abs(ball.dy);
            spawn(*impactSpark_, ball.x, kCeiling);
            playSound(Sound::WallBounce, ball.x);
        }
        if (active(Effect::Barrier) && ball.dy > 0.0f && ball.y + r > kBarrierY) {
            ball.y = kBarrierY - r;
            ball.dy = -std::abs(ball.dy);
            spawn(*impactSpark_, ball.x, kBarrierY);
            playSound(Sound::WallBounce, ball.x);
        }

        if (ball.dy > 0.0f && paddle.intersectsCircle(ball.x, ball.y, r)) {
            const float offset = ball.x - player_.centerX();
            if (ball.y < paddle.y + paddle.h * 0.5f) {
                ball.y = paddle.y - r;
                paddleGlowSeconds_ = 0.0f;
                spawn(*impactSpark_, ball.x, paddle.y);
                playSound(Sound::PaddleHit, ball.x);
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
            if (active(Effect::Fireball) && !bricks_[i].unbreakable()) {
                hitBrick(i, true, ball.x, ball.y);
                continue;
            }
            ball.bounceOff(bricks_[i].rect());
            hitBrick(i, false, ball.x, ball.y);
            break;
        }

        if (ball.y - r > kFieldHeight) {
            spawn(*ballLost_, ball.x, kBallLostY);
            it = balls_.erase(it);
        } else {
            ++it;
        }
    }
}

bool Game::hitBrick(size_t index, bool smash, float fromX, float fromY) {
    Brick& brick = bricks_[index];
    const Rect rect = brick.rect();
    const float hitX = std::clamp(fromX, rect.x, rect.x + rect.w);
    const float hitY = std::clamp(fromY, rect.y, rect.y + rect.h);

    if (brick.unbreakable()) {
        brick.hitSeconds = 0.0f;
        spawn(*impactSpark_, hitX, hitY);
        playSound(Sound::GoldBrickHit, hitX, 0.7f);  // the metal clang
        return false;
    }
    if (!brick.hit(smash)) {
        score_ += 5;
        brick.hitSeconds = 0.0f;
        // The dust sprite billows upwards: turn that side away from the brick.
        spawn(*impactDust_, hitX, hitY, std::atan2(fromY - hitY, fromX - hitX) + kPi * 0.5f);
        playVariant(Sound::BrickHit1, hitX);
        return false;
    }

    const BrickType type = brick.type;
    const float x = brick.centerX();
    const float y = brick.centerY();
    bricks_.erase(bricks_.begin() + static_cast<std::ptrdiff_t>(index));
    score_ += 10;
    spawn(*shatterAnimations_[static_cast<int>(type)], x, y);
    playVariant(Sound::BrickBreak1, x);
    if (type == BrickType::Gold) {
        score_ += kGoldBonus;
        spawn(*sparkleBurst_, x, y, 0.0f, gfx::Blend::Additive);
        playSound(Sound::GoldBrickHit, x);
    }
    shakeScreen(type == BrickType::Gold ? kShakeGold : kShakeBrick);
    if (type == BrickType::Jade) {
        powerUps_.push_back(PowerUp::makeRandom(x, y));
        playSound(Sound::PowerUpSpawn, x);
    }

    comboCount_ = comboSeconds_ > 0.0f ? comboCount_ + 1 : 1;
    comboSeconds_ = kComboSeconds;
    if (comboCount_ % kComboBricks == 0) {
        playSound(Sound::Combo, x);
    }
    return true;
}

void Game::spawn(const Animation& animation, float x, float y, float rotation, gfx::Blend blend) {
    oneShots_.push_back({&animation, x, y, rotation, blend, 0.0f});
}

void Game::shakeScreen(float pixels) {
    // A weaker hit must not cut a stronger shake short.
    if (shakeSeconds_ <= 0.0f || pixels >= shakePixels_) {
        shakePixels_ = pixels;
        shakeSeconds_ = kShakeSeconds;
    }
}

Game::Particle Game::makeDustMote(float age) const {
    return {dustMoteImage_.get(),
            randomFloat(kWallLeft, kWallRight),
            randomFloat(kCeiling, kFieldHeight),
            randomFloat(-14.0f, 14.0f),
            randomFloat(-18.0f, 6.0f),
            randomFloat(5.0f, 10.0f),
            0.4f,
            gfx::Blend::Normal,
            true,
            age};
}

void Game::updateVisuals(float seconds) {
    clock_ += seconds;

    for (OneShot& shot : oneShots_) {
        shot.age += seconds;
    }
    oneShots_.erase(std::remove_if(oneShots_.begin(), oneShots_.end(),
                                   [](const OneShot& shot) { return shot.animation->finished(shot.age); }),
                    oneShots_.end());

    for (Brick& brick : bricks_) {
        if (brick.hitSeconds >= 0.0f) {
            brick.hitSeconds += seconds;
            if ((brick.unbreakable() ? obsidianClang_ : hitFlash_)->finished(brick.hitSeconds)) {
                brick.hitSeconds = -1.0f;
            }
        }
    }
    if (paddleGlowSeconds_ >= 0.0f) {
        paddleGlowSeconds_ += seconds;
        if (paddleGlow_[static_cast<int>(player_.size())]->finished(paddleGlowSeconds_)) {
            paddleGlowSeconds_ = -1.0f;
        }
    }

    auto advance = [seconds](std::vector<Particle>& particles) {
        for (Particle& p : particles) {
            p.age += seconds;
            p.x += p.vx * seconds;
            p.y += p.vy * seconds;
        }
    };
    auto removeDead = [](std::vector<Particle>& particles) {
        particles.erase(std::remove_if(particles.begin(), particles.end(),
                                       [](const Particle& p) { return p.age >= p.life; }),
                        particles.end());
    };
    advance(dustMotes_);
    advance(embers_);
    advance(trail_);
    removeDead(embers_);
    removeDead(trail_);
    for (Particle& mote : dustMotes_) {
        if (mote.age >= mote.life) {
            mote = makeDustMote(0.0f);
        }
    }

    emberTimer_ -= seconds;
    if (emberTimer_ <= 0.0f) {
        emberTimer_ = kEmberInterval;
        const float x = kTorchX[randomInt(static_cast<int>(kTorchX.size()))];
        embers_.push_back({emberImage_.get(), x + randomFloat(-12.0f, 12.0f), kTorchY - randomFloat(30.0f, 80.0f),
                           randomFloat(-25.0f, 25.0f), randomFloat(-150.0f, -60.0f), randomFloat(0.7f, 1.5f), 1.0f,
                           gfx::Blend::Additive, false, 0.0f});
    }

    bannerSeconds_ += seconds;
    sparkleTimer_ -= seconds;
    if ((state_ == State::LevelComplete || state_ == State::Won) && bannerSeconds_ < kSparkleSeconds &&
        sparkleTimer_ <= 0.0f) {
        sparkleTimer_ = kSparkleInterval;
        spawn(*sparkleBurst_, randomFloat(kWallLeft + 128.0f, kWallRight - 128.0f), randomFloat(kCeiling + 128.0f, 900.0f),
              0.0f, gfx::Blend::Additive);
    }

    shakeSeconds_ = std::max(0.0f, shakeSeconds_ - seconds);
    const float shake = shakePixels_ * shakeSeconds_ / kShakeSeconds;
    shakeX_ = shakeSeconds_ > 0.0f ? randomFloat(-shake, shake) : 0.0f;
    shakeY_ = shakeSeconds_ > 0.0f ? randomFloat(-shake, shake) : 0.0f;
}

void Game::moveLasers(float seconds) {
    for (auto it = lasers_.begin(); it != lasers_.end();) {
        it->y -= kLaserSpeed * seconds;
        const Rect bolt{it->x - kLaserWidth * 0.5f, it->y, kLaserWidth, kLaserHeight};
        bool gone = bolt.y + bolt.h < kCeiling;
        for (size_t i = 0; i < bricks_.size() && !gone; ++i) {
            if (bolt.intersects(bricks_[i].rect())) {
                const Rect rect = bricks_[i].rect();
                hitBrick(i, false, it->x, rect.y + rect.h + 1.0f);  // obsidian bricks just absorb the bolt
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
        if (left <= 0.0f && static_cast<Effect>(effect) == Effect::Catch && releaseStuckBalls()) {
            playSound(Sound::BallLaunch, player_.centerX());
        }
    }
    laserCooldown_ = std::max(0.0f, laserCooldown_ - seconds);
    comboSeconds_ = std::max(0.0f, comboSeconds_ - seconds);
}

void Game::applyPowerUp(const PowerUp& powerUp) {
    score_ += 10;
    // Labels of power-ups caught close together stack upwards instead of overlapping.
    const auto recent = std::count_if(floatingTexts_.begin(), floatingTexts_.end(),
                                      [](const FloatingText& text) { return text.age < 0.4f; });
    floatingTexts_.push_back({powerUpName(powerUp.kind), powerUp.x, Player::kTop - 60.0f - 40.0f * recent, 0.0f});
    switch (powerUp.kind) {
        case PowerUpKind::ExtraLife:
            playSound(Sound::ExtraLife, powerUp.x);
            break;
        case PowerUpKind::Shrink:
            playSound(Sound::PowerDownCurse, powerUp.x);
            break;
        default:
            playSound(Sound::PowerUpCollect, powerUp.x);
            break;
    }

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
    if (releaseStuckBalls()) {
        playSound(Sound::BallLaunch, player_.centerX());
    }
    if (active(Effect::Laser) && laserCooldown_ <= 0.0f) {
        lasers_.push_back({player_.centerX(), Player::kTop - kLaserHeight});
        laserCooldown_ = kLaserCooldown;
        playSound(Sound::UiClick, player_.centerX());
    }
}

bool Game::releaseStuckBalls() {
    bool released = false;
    for (Ball& ball : balls_) {
        if (!ball.stuck) {
            continue;
        }
        ball.stuck = false;
        released = true;
        aimFromPaddle(ball, ball.stuckOffset, player_);
        // Straight up would just bounce between the paddle and the ceiling.
        if (std::abs(ball.dx) < 0.17f) {
            ball.setAngle((randomInt(2) == 0 ? -1.0f : 1.0f) * randomFloat(0.26f, 0.52f));
        }
    }
    return released;
}

void Game::loseLife() {
    if (--lives_ > 0) {
        resetPaddleAndBall();
        showMessage(State::LifeLost, "Lives left: " + std::to_string(lives_));
        playSound(Sound::LifeLost);
    } else {
        showMessage(State::GameOver, "GAME OVER");
        playSound(Sound::GameOver);
    }
}

float Game::ballSpeed() const {
    const float speed = levels()[levelIndex_].ballSpeed;
    return active(Effect::Slow) ? speed * kSlowFactor : speed;
}

bool Game::levelCleared() const {
    return std::all_of(bricks_.begin(), bricks_.end(), [](const Brick& b) { return b.unbreakable(); });
}

void Game::drawTorches() const {
    for (size_t i = 0; i < kTorchX.size(); ++i) {
        // Between 0.8 and 1.0; the two torches flicker out of step.
        const float flicker = 0.9f + 0.06f * std::sin(clock_ * 11.0f + i * 2.0f) + 0.04f * std::sin(clock_ * 23.0f + i);
        const float half = torchGlow_->width() * 0.5f;
        gfx::drawImage(*torchGlow_, {kTorchX[i] - half, kTorchY - half, 2.0f * half, 2.0f * half}, flicker,
                       gfx::Blend::Additive);
        torchFlame_->draw(clock_ + i * 0.31f, kTorchX[i], kTorchY);
    }
}

void Game::drawParticles(const std::vector<Particle>& particles) const {
    for (const Particle& p : particles) {
        const float t = p.age / p.life;
        const float alpha = p.brightness * (p.fadeIn ? std::sin(std::clamp(t, 0.0f, 1.0f) * kPi) : 1.0f - t);
        const float half = p.image->width() * 0.5f * (1.0f - 0.4f * t);
        gfx::drawImage(*p.image, {p.x - half, p.y - half, 2.0f * half, 2.0f * half}, alpha, p.blend);
    }
}

void Game::drawBrick(const Brick& brick) const {
    const bool animating = brick.hitSeconds >= 0.0f;
    if (animating && brick.unbreakable()) {
        obsidianClang_->draw(brick.hitSeconds, brick.centerX(), brick.centerY());
        return;
    }
    float dx = 0.0f;
    float dy = 0.0f;
    if (animating) {
        const auto& nudge = kHitShake[hitFlash_->frameAt(brick.hitSeconds)];
        dx = nudge[0];
        dy = nudge[1];
    }
    const gfx::Texture& image = *brickImages_[static_cast<int>(brick.type)][brick.variant][brick.cracks()];
    gfx::drawImage(image, {brick.x + dx, brick.y + dy, Brick::kWidth, Brick::kHeight});
    if (animating) {
        hitFlash_->draw(brick.hitSeconds, brick.centerX() + dx, brick.centerY() + dy, 0.0f, gfx::Blend::Additive);
    }
}

void Game::drawBall(const Ball& ball) const {
    ballGlowPulse_->draw(clock_, ball.x, ball.y, 0.0f, gfx::Blend::Additive);
    if (active(Effect::Fireball)) {
        // The flame points up in the sprite; this turns it to trail behind the ball.
        ballFire_->draw(clock_, ball.x, ball.y, std::atan2(ball.dy, ball.dx) - kPi * 0.5f);
    } else {
        gfx::drawImage(*ballImage_, {ball.x - Ball::kRadius, ball.y - Ball::kRadius, 2.0f * Ball::kRadius, 2.0f * Ball::kRadius});
    }
}

void Game::draw() const {
    // Under the shaken copy, so the edges it uncovers show the wall rather than black.
    gfx::drawImage(*background_, {0.0f, 0.0f, kFieldWidth, kFieldHeight});
    gfx::setOffset(shakeX_, shakeY_);
    gfx::drawImage(*background_, {0.0f, 0.0f, kFieldWidth, kFieldHeight});

    drawTorches();
    drawParticles(dustMotes_);
    drawParticles(embers_);

    for (const Brick& brick : bricks_) {
        drawBrick(brick);
    }

    if (active(Effect::Barrier)) {
        // Blinks during its last two seconds.
        const float left = effectSeconds_[static_cast<int>(Effect::Barrier)];
        const float alpha = left > 2.0f ? 1.0f : 0.55f + 0.45f * std::cos(left * 12.0f);
        const float width = kWallRight - kWallLeft;
        gfx::Color glow = kBarrierColor;
        glow.a = 0.3f * alpha;
        gfx::fillRect({kWallLeft, kBarrierY - 6.0f, width, kBarrierHeight + 12.0f}, glow);
        gfx::Color core = kBarrierColor;
        core.a = 0.95f * alpha;
        gfx::fillRect({kWallLeft, kBarrierY, width, kBarrierHeight}, core);
    }

    for (const Laser& laser : lasers_) {
        gfx::fillRect({laser.x - kLaserWidth, laser.y - 4.0f, kLaserWidth * 2.0f, kLaserHeight + 8.0f}, kLaserGlowColor);
        gfx::fillRect({laser.x - kLaserWidth * 0.5f, laser.y, kLaserWidth, kLaserHeight}, kLaserColor);
    }
    for (const PowerUp& powerUp : powerUps_) {
        powerUpAnimations_[static_cast<int>(powerUp.kind)]->draw(clock_, powerUp.x, powerUp.y);
    }

    const int paddleSize = static_cast<int>(player_.size());
    const Rect paddle = player_.rect();
    if (paddleGlowSeconds_ >= 0.0f) {
        paddleGlow_[paddleSize]->draw(paddleGlowSeconds_, player_.centerX(), paddle.y + paddle.h * 0.5f);
    } else {
        gfx::drawImage(*paddleImages_[paddleSize], {paddle.x, paddle.y, paddle.w, paddle.h});
    }

    drawParticles(trail_);
    for (const Ball& ball : balls_) {
        drawBall(ball);
    }
    for (const OneShot& shot : oneShots_) {
        shot.animation->draw(shot.age, shot.x, shot.y, shot.rotation, shot.blend);
    }

    gfx::setOffset(0.0f, 0.0f);

    for (const FloatingText& text : floatingTexts_) {
        const float size = 44.0f;
        font_->draw(text.text, text.x - font_->width(text.text, size) * 0.5f, text.y, size,
                    {1.0f, 0.95f, 0.6f, 1.0f - text.age / kFloatingTextSeconds});
    }

    if (state_ == State::LevelComplete || state_ == State::GameOver || state_ == State::Won) {
        drawBanner(message_);
    } else if (state_ != State::Playing) {
        drawMessage(message_);
    }
    drawHud();
    if (helpVisible_) {
        drawHelp();
    }
}

void Game::drawMessage(const std::string& text) const {
    // As big as fits the field, up to the size the font is sharp at.
    const float size = std::min(1600.0f / font_->width(text, 1.0f), 90.0f);
    const float x = (kFieldWidth - font_->width(text, size)) * 0.5f;
    // Between the lowest brick row of the tallest level and the paddle.
    const float y = 840.0f - 0.5f * size;
    font_->draw(text, x, y, size, kMessageColor);
}

void Game::drawBanner(const std::string& text) const {
    const float t = std::min(bannerSeconds_ / kBannerGrowSeconds, 1.0f);
    const float scale = 0.3f + 0.7f * (1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t));
    const float width = bannerImage_->width() * scale;
    const float height = bannerImage_->height() * scale;
    gfx::drawImage(*bannerImage_, {(kFieldWidth - width) * 0.5f, kBannerCentreY - height * 0.5f, width, height}, t);

    const float size = std::min(780.0f / font_->width(text, 1.0f), 90.0f) * scale;
    gfx::Color color = kMessageColor;
    color.a = t;
    // The capitals of this font sit just below the middle of the line.
    font_->draw(text, (kFieldWidth - font_->width(text, size)) * 0.5f, kBannerCentreY - 0.57f * size, size, color);
}

void Game::drawHud() const {
    const float size = 40.0f;
    const float spacing = 60.0f;
    const float textY = kPanelY + (kPanelHeight - size * 1.15f) * 0.5f;
    const float panelStep = kPanelWidth + 10.0f;

    auto panel = [&](int index) {
        const float x = 20.0f + index * panelStep;
        gfx::drawImage(*panelImage_, {x, kPanelY, kPanelWidth, kPanelHeight});
        return x;
    };
    auto centred = [&](float panelX, float width) { return panelX + (kPanelWidth - width) * 0.5f; };

    const std::string score = "Score: " + std::to_string(score_);
    font_->draw(score, centred(panel(0), font_->width(score, size)), textY, size, kHudColor);

    // Lives can pile up past what fits as a row of icons, so one icon and a count.
    const std::string lives = "x " + std::to_string(lives_);
    const float iconGap = 12.0f;
    const float iconSize = static_cast<float>(lifeIcon_->width());
    const float livesX = centred(panel(1), iconSize + iconGap + font_->width(lives, size));
    gfx::drawImage(*lifeIcon_, {livesX, kPanelY + (kPanelHeight - iconSize) * 0.5f, iconSize, iconSize});
    font_->draw(lives, livesX + iconSize + iconGap, textY, size, kHudColor);

    const std::string level = "Level " + std::to_string(levelIndex_ + 1) + "/" + std::to_string(levels().size());
    font_->draw(level, centred(panel(2), font_->width(level, size)), textY, size, kHudColor);

    const std::string help = "H - Help";
    float right = kFieldWidth - 40.0f - font_->width(help, size);
    font_->draw(help, right, textY, size, {1.0f, 1.0f, 1.0f, 0.6f});

    // Running effects: an icon with a bar for the time left, right to left before the help hint.
    const float icon = 44.0f;
    const float iconY = 22.0f;
    right -= spacing;
    for (int effect = 0; effect < kEffectCount; ++effect) {
        const float left = effectSeconds_[effect];
        if (left <= 0.0f) {
            continue;
        }
        right -= icon;
        gfx::drawImage(*powerUpImages_[static_cast<int>(kEffectIcons[effect])], {right, iconY, icon, icon});
        gfx::fillRect({right, iconY + icon + 6.0f, icon, 5.0f}, {1.0f, 1.0f, 1.0f, 0.2f});
        gfx::fillRect({right, iconY + icon + 6.0f, icon * left / kEffectSeconds[effect], 5.0f}, kWhite);
        right -= 16.0f;
    }
}

void Game::drawHelp() const {
    const gfx::Rect panel{360.0f, 110.0f, 1200.0f, 860.0f};
    gfx::fillRect({0.0f, 0.0f, kFieldWidth, kFieldHeight}, {0.0f, 0.0f, 0.0f, 0.55f});
    gfx::fillRect(panel, {0.13f, 0.09f, 0.05f, 0.96f});

    auto centred = [&](const std::string& text, float y, float size, const gfx::Color& color) {
        font_->draw(text, panel.x + (panel.w - font_->width(text, size)) * 0.5f, y, size, color);
    };
    centred("Brake The Bricks 2.0", panel.y + 30.0f, 66.0f, kMessageColor);
    centred("Left / Right - Move    Enter - Start / Pause", panel.y + 115.0f, 33.0f, kWhite);
    centred("Space - Fire / Release    M - Sound    F11 - Fullscreen    Esc - Exit", panel.y + 155.0f, 33.0f, kWhite);

    const float icon = 64.0f;
    const float rowHeight = 100.0f;
    for (int kind = 0; kind < kPowerUpKindCount; ++kind) {
        const float x = panel.x + 80.0f + (kind / 5) * 560.0f;
        const float y = panel.y + 230.0f + (kind % 5) * rowHeight;
        gfx::drawImage(*powerUpImages_[kind], {x, y, icon, icon});
        const auto powerUp = static_cast<PowerUpKind>(kind);
        font_->draw(powerUpName(powerUp), x + icon + 24.0f, y - 8.0f, 44.0f, kMessageColor);
        font_->draw(powerUpDescription(powerUp), x + icon + 24.0f, y + 40.0f, 33.0f, kWhite);
    }

    centred("Debug: Q clear level, W/E paddle size, R random power-up, 1-0 drop power-up", panel.y + 787.0f, 22.0f,
            {1.0f, 1.0f, 1.0f, 0.6f});
    centred("H - Close help", panel.y + 817.0f, 22.0f, {1.0f, 1.0f, 1.0f, 0.6f});
}

}  // namespace btb
