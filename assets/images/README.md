# Brick Breaker – Full HD asset pack (1920x1080)

All sprites are PNG with transparency. Sizes are 3x the original 40x20 brick grid.

| File | Size | Notes |
|---|---|---|
| background.png | 1920x1080 | Space nebula, darker centre for readability |
| brick-<color>.png | 120x60 | red, orange, yellow, green, cyan, blue, purple |
| brick-<color>-cracked.png | 120x60 | 2nd-hit state for multi-hit bricks |
| brick-metal.png | 120x60 | Unbreakable / armored brick |
| paddle.png / paddle-wide.png / paddle-short.png | 324x72 / 444x72 / 224x72 | Visible bar 300/420/200 x 48, 12px glow padding each side |
| ball.png / ball-fire.png | 56x56 | Visible ball 32px, 12px glow padding |
| powerup-*.png | 120x120 | Visible orb 96px, 12px glow padding |

Glow padding: use the inner size for collisions, draw the sprite offset by -12px.

## Power-ups (replacing the old icons)

| Old | New | Effect |
|---|---|---|
| heart | powerup-extra-life | +1 life |
| star | powerup-multiball | Split into 3 balls |
| grenade | powerup-fireball | Ball smashes through bricks (use ball-fire.png) |
| blue shield | powerup-barrier | Temporary wall below the paddle |
| orange shield | powerup-laser | Paddle shoots lasers |
| – | powerup-expand | Wider paddle (paddle-wide.png) |
| – | powerup-shrink | Narrower paddle (paddle-short.png) – negative |
| – | powerup-slow | Slows the ball |
| – | powerup-catch | Ball sticks to paddle, release with fire key |
| – | powerup-bonus | Score bonus |
