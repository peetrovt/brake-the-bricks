# Brake The Bricks (C++)

A C++17 remake of the Java/Swing game in `../BrakeTheBricks`, using GLFW + OpenGL 2.1 and the Full HD art pack
in `assets/images` (see its README).

## Build and run

```sh
sudo apt install libglfw3-dev libgl1-mesa-dev   # once
cmake -S . -B build
cmake --build build -j
./build/brake_the_bricks
```

The field is 1920x1080. The window opens at 80% of the screen and keeps the aspect ratio when resized.

## Controls

| Key | Action |
| --- | --- |
| Left / Right | Move the paddle |
| Enter | Start / pause / resume / next level |
| Space | Launch, fire lasers, release a caught ball |
| H or F1 | Help |
| F11 | Fullscreen |
| Esc | Exit |

Debug keys: Q clears the level, W/E shrinks/grows the paddle, R drops a random power-up, 1-9 and 0 drop each
power-up in turn.

## Rules

- Five levels: rectangle, wall, triangle, rhombus, hexagon. The ball gets faster each level.
- Some bricks take two hits (they show cracks after the first). Metal bricks never break and are not needed to
  finish a level.
- 10 points per broken brick, 5 per crack, 10 per caught power-up. One broken brick in seven drops a power-up.
- Where the ball hits the paddle sets its angle: the ends send it off at up to 60 degrees.

| Power-up | Effect |
| --- | --- |
| Extra life | +1 life |
| Multiball | Every ball splits into 3 (up to 24 balls) |
| Expand / Shrink | Paddle one size wider / narrower (200, 300, 420 px) until a life is lost |
| Laser | 12 s: Space shoots two bolts |
| Fireball | 8 s: balls smash through breakable bricks without bouncing |
| Slow | 10 s: balls at 65% speed |
| Barrier | 15 s: a wall below the paddle bounces balls back |
| Catch | 15 s: balls stick to the paddle; Space releases them |
| Bonus | +500 points |

Running effects are shown top right with a bar for the time left.

## Layout

- `src/Game.*`: game states, collisions, power-ups, scoring, drawing
- `src/Level.*`: the five brick layouts and per-level speed and brick mix
- `src/Ball.*`, `src/Player.*`, `src/Brick.*`, `src/PowerUp.*`: the game objects
- `src/Renderer.*`: OpenGL drawing, PNG textures, pixel font
- `assets/images/`: the art pack
- `third_party/stb/`: `stb_image.h` for PNG loading
