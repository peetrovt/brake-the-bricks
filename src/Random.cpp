#include "Random.h"

#include <random>

namespace btb {
namespace {

std::mt19937& engine() {
    static std::mt19937 engine{std::random_device{}()};
    return engine;
}

}  // namespace

int randomInt(int bound) {
    return std::uniform_int_distribution<int>(0, bound - 1)(engine());
}

float randomFloat(float min, float max) {
    return std::uniform_real_distribution<float>(min, max)(engine());
}

}  // namespace btb
