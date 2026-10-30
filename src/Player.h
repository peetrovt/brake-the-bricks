#pragma once

#include "Rect.h"

namespace btb {

// Matches paddle-short.png, paddle.png and paddle-wide.png.
enum class PaddleSize { Short, Normal, Wide };

// The paddle.
class Player {
public:
    static constexpr float kHeight = 48.0f;
    static constexpr float kTop = 972.0f;
    static constexpr float kSpeed = 1200.0f;  // pixels per second

    Player() { reset(); }

    // Back to the normal size, centred.
    void reset();
    // `direction` is -1 (left), 0 or 1 (right).
    void move(int direction, float seconds);
    void grow();
    void shrink();

    PaddleSize size() const { return size_; }
    float centerX() const { return centerX_; }
    float width() const;
    // The collision box, without the sprite's glow.
    Rect rect() const { return {centerX_ - width() * 0.5f, kTop, width(), kHeight}; }

private:
    void keepInsideField();

    float centerX_ = 0.0f;
    PaddleSize size_ = PaddleSize::Normal;
};

}  // namespace btb
