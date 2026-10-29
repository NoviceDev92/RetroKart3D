# Handoff Report — Gameplay & AI Survey

**Agent**: `explorer_survey_2` (Gameplay & AI Explorer)  
**Date**: 2026-10-29  
**Recipient**: `orchestrator_1` / Lead Team  
**Artifact File**: `d:\Qt\RetroKart3D\.agents\teamwork\explorer_survey_2\survey_report.md`

---

## 1. Observation

### AI Opponents
- In `game_canvas.h:83–94`, kart state exists exclusively as single-player scalar fields (`kartX`, `kartZ`, `kartAngle`, `kartSpeed`, `maxSpeed`, `accel`, `brakeDecel`, `friction`, `turnRate`). No collections or opponent instances exist.
- In `game_canvas.cpp:121–242` (`generateDonutPlains`, `generateChocoValley`, `generateBowserCastle`), spline paths and polygons (`QPainterPath path`, `QPolygon castlePoly`) are created as local variables and destroyed when the function returns. No waypoints or centerlines are retained for AI navigation.

### Item System Self-Targeting Bugs
- In `game_canvas.cpp:602–604`:
  ```cpp
  case PlayerItem::INK_BLOOPER:
      inkTimer = 4.5;
      break;
  ```
  `inkTimer` is set directly on `this`, causing `renderInkOverlay()` (lines 988–1014) to draw ink splats covering the player's own windshield.
- In `game_canvas.cpp:593–600`:
  ```cpp
  case PlayerItem::GREEN_SHELL: {
      double fireDist = 28.0;
      double fx = kartX + std::cos(kartAngle) * fireDist;
      double fz = kartZ + std::sin(kartAngle) * fireDist;
      worldSprites.push_back({ fx, fz, 0.0, SpriteType::GREEN_SHELL, true, 0.0 });
      break;
  }
  ```
  The shell is placed as a static billboard sprite. There is no velocity vector, no motion integration in `updatePhysics` or `updateItemSystem`, and lines 488–504 omit `GREEN_SHELL` entirely from collision handling.
- In `game_canvas.cpp:584–591`:
  `BANANA` drops behind the kart with `dropDist = 24.0` without any owner identifier or immunity window. Reversing or turning sharply immediately triggers a self-spinout.

### Track Hazards
- In `game_canvas.cpp:244–304`, Bowser's Castle defines lava solely as a static background color and mask value (`#FF00FF`). No lava geysers exist.
- In `game_canvas.cpp:191–242`, Choco Valley has only static oil slicks and a banana. No chocolate fountains or rolling candies exist.

### HUD & Minimap
- `game_canvas.cpp` renders a cockpit hood, a 42×42 item slot at top center (`renderItemHUD()`, lines 907–986), an ink overlay, and a victory banner.
- No minimap implementation exists anywhere in `game_canvas.*` or `mainwindow.*`.

### Build & Toolchain
- Qt 6.11.1 MinGW 64-bit toolchain is verified at `D:\Qt\6.11.1\mingw_64\bin\qmake.exe` and `D:\Qt\Tools\mingw1310_64\bin\g++.exe`.

---

## 2. Logic Chain

1. **Why AI karts are absent**: Because the original project formulation deferred AI spline following to Week 3 (`PROJECT_FORMULATION.md:367`), the current implementation only modeled player-relative camera physics. Adding AI requires an `OpponentKart` data structure and persistent waypoints for each track.
2. **Why items fail to affect opponents**:
   - `INK_BLOOPER` was implemented solely as a visual demo on the player's camera.
   - `GREEN_SHELL` was treated as a static obstacle instead of a projectile.
   - Without opponents, all placed hazards (bananas) can only ever be encountered by the single player.
3. **Why hazards are missing**: The tracks currently use static mask brushes. Adding animated geysers requires stateful objects with cyclic timers and billboard height interpolation.
4. **Why Local Split-Screen is the optimal multiplayer choice**: Mode 7 ground mapping renders row-by-row on the CPU. Two 400×150 viewports equal exactly 60,000 pixels (the same as one 400×300 viewport), preserving 60 FPS performance without any risk of socket errors or firewall issues during a live showcase.

---

## 3. Caveats

- **Audio System**: Audio/sound effects were not in the scope of this survey and are currently not present in the codebase.
- **Hardware Acceleration**: The game intentionally avoids OpenGL in accordance with coursework constraints (`PROJECT_FORMULATION.md:9`). All proposals strictly preserve pure CPU pixel manipulation.

---

## 4. Conclusion

The RetroKart 3D engine is solid in its core Mode 7 mathematics, billboard rasterizer, and 60 FPS timer loop. However, the gameplay layer requires:
1. **AI Opponents**: Persistent track waypoints, an `OpponentKart` update loop with lookahead steering and rubber-banding, and procedural billboard rendering.
2. **Item System Overhaul**: Separation into active projectiles (`ShellProjectile`) with velocity and wall bounce, owner tracking and immunity timers for traps (`BANANA`), and redirecting `INK_BLOOPER` effects to opponents/Player 2.
3. **Track Hazards**: Animated lava geyser columns on Bowser Castle and chocolate fountains/candies on Choco Valley with height-aware collision.
4. **HUD Minimap**: A 72×72 top-left radar displaying cached track thumbnail and real-time kart dots.
5. **Multiplayer**: A 2-player local split-screen mode with independent viewports and multiplexed controls (WASD vs Arrow keys).

Detailed implementation blueprints and code snippets are documented in `survey_report.md`.

---

## 5. Verification Method

To verify these observations independently:
1. **AI Kart Absence**: Inspect `game_canvas.h:83–94` to confirm lack of opponent kart structures.
2. **Item Bugs**:
   - Run grep: `grep -n "inkTimer = 4.5" game_canvas.cpp` (Line 603).
   - Check shell spawning: `view_file` on `game_canvas.cpp:593–600`.
   - Check collision loop: `view_file` on `game_canvas.cpp:468–504` to confirm absence of `GREEN_SHELL`.
3. **Minimap Absence**: Search for "minimap" across all source files (`grep_search` returns 0 hits in code).
4. **Build Check**:
   ```powershell
   $env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH
   D:\Qt\6.11.1\mingw_64\bin\qmake.exe RetroKart3D.pro
   D:\Qt\Tools\mingw1310_64\bin\mingw32-make.exe -j4
   ```
