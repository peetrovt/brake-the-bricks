#include "PowerUp.h"

#include "GameConstants.h"
#include "Random.h"

#include <algorithm>
#include <array>
#include <numeric>

namespace btb {
namespace {

// Relative drop chances, in PowerUpKind order.
constexpr std::array<int, kPowerUpKindCount> kWeights = {
    2,  // ExtraLife
    4,  // Multiball
    4,  // Expand
    3,  // Shrink
    3,  // Laser
    2,  // Fireball
    3,  // Slow
    2,  // Barrier
    3,  // Catch
    4,  // Bonus
};

}  // namespace

PowerUp PowerUp::make(float x, float y, PowerUpKind kind) {
    return {std::clamp(x, kRadius, kFieldWidth - kRadius), y, randomFloat(300.0f, 450.0f), kind};
}

PowerUp PowerUp::makeRandom(float x, float y) {
    int roll = randomInt(std::accumulate(kWeights.begin(), kWeights.end(), 0));
    int kind = 0;
    while (roll >= kWeights[kind]) {
        roll -= kWeights[kind];
        ++kind;
    }
    return make(x, y, static_cast<PowerUpKind>(kind));
}

const char* powerUpName(PowerUpKind kind) {
    switch (kind) {
        case PowerUpKind::ExtraLife:
            return "+1 Life";
        case PowerUpKind::Multiball:
            return "Multiball";
        case PowerUpKind::Expand:
            return "Expand";
        case PowerUpKind::Shrink:
            return "Shrink";
        case PowerUpKind::Laser:
            return "Laser";
        case PowerUpKind::Fireball:
            return "Fireball";
        case PowerUpKind::Slow:
            return "Slow";
        case PowerUpKind::Barrier:
            return "Barrier";
        case PowerUpKind::Catch:
            return "Catch";
        case PowerUpKind::Bonus:
            return "+500";
    }
    return "";
}

const char* powerUpDescription(PowerUpKind kind) {
    switch (kind) {
        case PowerUpKind::ExtraLife:
            return "One more life";
        case PowerUpKind::Multiball:
            return "Every ball splits in 3";
        case PowerUpKind::Expand:
            return "Wider paddle";
        case PowerUpKind::Shrink:
            return "Narrower paddle";
        case PowerUpKind::Laser:
            return "Space shoots lasers";
        case PowerUpKind::Fireball:
            return "Balls smash through";
        case PowerUpKind::Slow:
            return "Slower balls";
        case PowerUpKind::Barrier:
            return "Wall below the paddle";
        case PowerUpKind::Catch:
            return "Ball sticks, Space fires";
        case PowerUpKind::Bonus:
            return "500 points";
    }
    return "";
}

}  // namespace btb
