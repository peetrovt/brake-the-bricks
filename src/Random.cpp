#include "Random.h"

#include <random>

namespace btb {

int randomInt(int bound) {
    static std::mt19937 engine{std::random_device{}()};
    return std::uniform_int_distribution<int>(0, bound - 1)(engine);
}

}  // namespace btb
