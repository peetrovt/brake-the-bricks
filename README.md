# Brake The Bricks (C++)

A C++17 port of the Java/Swing game in `../BrakeTheBricks`, using GLFW + OpenGL 2.1 and the original images.

## Build and run

```sh
sudo apt install libglfw3-dev libgl1-mesa-dev   # once
cmake -S . -B build
cmake --build build -j
./build/brake_the_bricks
```

## Controls

| Key | Action |
| --- | --- |
| Left / Right | Move the paddle |
| Enter | Start / pause / resume / next level |
| H or F1 | Help |
| Esc | Exit |

Debug keys from the original: Q clears the level, A/Z stops/resumes the ball, W/E shrinks/grows the paddle, R drops a random bonus.

## Layout

- `src/Game.*`: game states, collisions, scoring, drawing (was `MainGame.java`)
- `src/Level.*`: the five brick layouts (was `*Level.java`)
- `src/Ball.*`, `src/Player.*`, `src/Brick.*`, `src/FallingObject.*`: the game objects
- `src/Renderer.*`: OpenGL drawing, PNG textures, pixel font
- `assets/images/`: the original artwork
- `third_party/stb/`: `stb_image.h` for PNG loading
