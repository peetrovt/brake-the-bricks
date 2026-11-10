# Brake the Bricks: Ancient Temple theme

All art is made for a **1920×1080** screen. Every PNG has transparency except `background.png`.

## Layout used by the art

| Area | Pixels |
|---|---|
| HUD band (carved frieze) | y 0–104 |
| Side pillars (walls) | x 0–100 and x 1820–1920 |
| Play field | x 100–1820, y 108–1080 |
| Brick grid | 13 columns × 128 px, starting at x = 128, first row at y = 180, row height 48 |
| Paddle centre line | y ≈ 1010 |
| Torch sconces | (50, 560) and (1870, 560); also listed in `atlas.json → torch_flame.positions` |

## Folders

- **background/**: `background.png` (1920×1080), `torch_glow.png` (512, additive), plus two particles: `dust_mote.png` (floating dust) and `ember.png` (torch sparks).
- **bricks/**: 128×48. File names follow `{type}_{variant}[_crackN].png`.

  | Type | Hits | Files | Purpose |
  |---|---|---|---|
  | sandstone | 1 | a–d | basic brick |
  | limestone | 2 | a–d, plus `_crack1` | |
  | granite | 3 | a–d, plus `_crack1`, `_crack2` | |
  | jade | 1 | a–b | drops a power-up |
  | gold | 1 | a–b | bonus points |
  | obsidian | indestructible | a | bronze frame |

- **cracks/**: generic crack overlays (stage 1 and 2). Use these if you add brick types of your own.
- **paddle/**: three paddle sizes: short 176, normal 256 and long 352 (all 48 px tall).
- **ball/**: `ball.png` (32 px), `ball_glow.png` (96, additive) and `trail_particle.png`.
- **powerups/**: 64×64 tablets: expand, shrink, multiball, slow, fireball, life, laser, barrier, catch, bonus.
- **ui/**: `panel.png` (400×80 score/lives plaque), `banner.png` (960×240 for level clear / game over) and `life_icon.png`.
- **anim/**: horizontal sprite strips. Frame size, frame count, fps, loop, anchor point and a usage note for each one are in **`atlas.json`**.

## Animations and when to play them

| Event | Animation |
|---|---|
| Brick hit but not destroyed | Draw `brick_hit_flash` over the brick (blend "lighter"/additive) and nudge the brick by `shake_offsets`. Swap the brick to its next `_crackN` sprite. Spawn `impact_dust` at the contact point. |
| Brick destroyed | Hide the brick and play `brick_shatter_{type}` with its anchor on the brick centre (the anchor is 160,112 inside the 320×256 frame). Add a 4–8 px screen shake. |
| Gold brick | Play its shatter, plus `sparkle_burst` (additive). |
| Indestructible brick hit | Play `brick_clang_obsidian` in place of the brick sprite, plus `impact_spark`. |
| Ball hits paddle | Play `paddle_glow_{size}` in place of the paddle sprite, plus `impact_spark`. |
| Ball hits wall | `impact_spark` |
| Fireball power-up | Play `ball_fire` (loop). The trail points up in the sprite, so rotate it by `atan2(vy, vx) - 90°` to make it trail behind the ball. |
| Ball lost | `ball_lost` at the bottom of the screen |
| Falling power-up | `powerup_{name}` (loop with shimmer) |
| Every frame | `ball_glow_pulse` under the ball (additive), trail particles, `torch_flame` on each sconce, and `torch_glow` with its alpha flickering between 0.8 and 1.0. |
| Level clear | `sparkle_burst` at random spots, with `banner.png` scaling in |

To get frame *i* from a strip: `source rect = (i * frame_w, 0, frame_w, frame_h)`. Draw it so the frame's `anchor` point lands on the world position.

## Regenerating or tweaking

The `source/` folder has the Python scripts that drew everything (they need numpy, scipy and Pillow).
Colours are in `PAL` in `static_assets.py`, and glyphs are in `common.glyph`. To rebuild, run `python3 static_assets.py && python3 animations.py`.
