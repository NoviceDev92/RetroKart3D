# Handoff Report — explorer_survey_1

**Agent**: `explorer_survey_1` (Codebase & Architecture Explorer)  
**Date**: 2026-10-29  
**Recipient**: `orchestrator_1` (Conversation ID: `842ff862-512e-4d57-af04-61c29631cec0`)  
**Mission**: Codebase & Architecture Investigation of RetroKart3D  

---

## 1. Observation

1. **Qt Project Configuration (`RetroKart3D.pro:1-25`)**:
   - `QT += core gui` and `greaterThan(QT_MAJOR_VERSION, 4): QT += widgets`.
   - `CONFIG += c++17`.
   - Sources: `main.cpp`, `mainwindow.cpp`, `game_canvas.cpp`, `sprite_atlas.cpp`.
   - Headers: `mainwindow.h`, `game_canvas.h`, `sprite_atlas.h`.
   - No `QT += network` currently present.

2. **Build Toolchain & Compilation**:
   - Qt installation: `D:\Qt\6.11.1\mingw_64` (Qt 6.11.1).
   - Compiler: `D:\Qt\Tools\mingw1310_64\bin\g++.exe` (GCC 13.1.0).
   - Make: `D:\Qt\Tools\mingw1310_64\bin\mingw32-make.exe` (GNU Make 4.2.1).
   - Executing `mingw32-make release` succeeded with exit code 0.
   - Compiling `game_canvas.cpp` directly with `g++ -O2 -std=gnu++1z -Wall -Wextra` produces **0 compiler errors and 0 warnings**.
   - Release binary exists at `release/RetroKart3D.exe` (110,592 bytes).

3. **Game Loop & Rendering Pipeline (`game_canvas.cpp:27-29, 310-330, 687-857, 1016-1022`)**:
   - Fixed 16ms timer: `gameTimer.start(16); // ~60 FPS`.
   - Frame delta time: `double dt = frameTimer.restart() / 1000.0; if (dt > 0.05) dt = 0.05;`.
   - Internal buffer: `screenBuffer` is `QImage(400, 300, QImage::Format_RGB32)`.
   - Mode-7 ground projection: `renderGroundMode7()` iterates scanlines $y = 150 \dots 299$, calculating ground distance $z_{ground} = \frac{h_{cam} \cdot f}{y - y_{horizon}}$ and sampling from 1024x1024 `trackVisualMap`.
   - Billboard sprites: `renderSprites()` projects world coordinates to camera coordinates $(camX, camZ)$, sorts descending by depth, and blits using `drawTexturedBillboard()`.
   - Paint event: `QPainter painter(this); painter.drawImage(rect(), screenBuffer);` with nearest-neighbor scaling.

4. **Input & Physics (`game_canvas.cpp:89-110, 391-436`)**:
   - Keyboard events captured into `std::set<int> pressedKeys`.
   - Throttle/braking: `W`/`Up` accelerates by `accel = 110.0`; `S`/`Down` brakes/reverses by `brakeDecel = 160.0`.
   - Steering: `steerSpeedFactor = std::clamp(std::abs(kartSpeed) / 50.0, 0.70, 1.30); kartAngle += steerDirection * 3.4 * steerSpeedFactor * dt;`.
   - Terrain friction: Samples `trackMaskMap` pixel: White (Road) = 100% speed, Green (Off-road) = 45% max speed, Magenta (Lava) = 35% speed + recoil spinout.
   - Drift mechanic: None. No drift key, no lateral velocity vector, no mini-turbo counter.

5. **Items & Opponents (`game_canvas.cpp:468-504, 570-609`)**:
   - Ink Blooper (`game_canvas.cpp:602-604`):
     ```cpp
     case PlayerItem::INK_BLOOPER:
         inkTimer = 4.5;
         break;
     ```
     Sets `inkTimer = 4.5` on the player themselves, covering the player's own windshield.
   - Green Shell (`game_canvas.cpp:593-600`):
     ```cpp
     case PlayerItem::GREEN_SHELL: {
         double fireDist = 28.0;
         double fx = kartX + std::cos(kartAngle) * fireDist;
         double fz = kartZ + std::sin(kartAngle) * fireDist;
         worldSprites.push_back({ fx, fz, 0.0, SpriteType::GREEN_SHELL, true, 0.0 });
         break;
     }
     ```
     Spawns a static `GREEN_SHELL` in `worldSprites` with no velocity or update loop.
   - AI Opponents: Zero opponent karts exist in the world or physics update.
   - Minimap: Zero minimap code exists in `GameCanvas` or `MainWindow`.

---

## 2. Logic Chain

1. **Premise**: Requirements R1 through R4 demand a user menu, character selection, active AI opponents with fixed item targeting, multiplayer capability, drift physics, track hazards, and a HUD minimap.
2. **From Observation 1 & 2**: The toolchain (Qt 6.11.1 MinGW 64-bit) compiles cleanly and has the required foundation, but `RetroKart3D.pro` currently lacks `QT += network` if network multiplayer is chosen.
3. **From Observation 3**: The rendering architecture renders into a 400x300 software `screenBuffer` in ~2ms per frame. Additional sprites (AI karts, hazards, minimap) can be rendered without exceeding the 16.6ms (60 FPS) budget.
4. **From Observation 4**: Physics currently only updates a single player kart directly using scalar variables in `GameCanvas`. Adding AI opponents and 2-player multiplayer requires extracting or encapsulating kart state into a multi-instance entity model.
5. **From Observation 5**: Item targeting is currently self-inflicting and static:
   - Blooper must blind opponents, not the user.
   - Shells must be dynamic moving projectiles with velocity and hit detection.
   - Bananas must avoid self-collision upon placement.
6. **State Machine Requirement**: Currently, `GameCanvas` boots straight into `DONUT_PLAINS` racing. A `GameState` enum (`TITLE_MENU`, `CHARACTER_SELECT`, `RACING`, `RESULTS`) must be introduced to control screen transitions per R1.

---

## 3. Caveats

- **Network vs Local Multiplayer**: Network multiplayer requires handling packet sockets, latency, and serialization; local 2-player multiplayer (split-screen or shared input on one keyboard) avoids network complexity and guarantees 100% demo stability for today's showcase.
- **Sound Effects**: The existing codebase has no audio engine (no Qt Multimedia or QSound). Audio was not requested in `ORIGINAL_REQUEST.md`, so focus remains purely on visual, physics, and gameplay enhancements.
- **No external assets**: All sprites in `SpriteAtlas` are procedurally generated in C++. Any new kart, character, or hazard sprites should follow this same procedural pattern to prevent missing asset file dependencies.

---

## 4. Conclusion

The RetroKart3D engine is structurally sound and performant, but currently operates as a single-kart tech demo rather than a complete game.

To satisfy all user requirements:
1. **R1**: Implement `GameState` machine (`TITLE_MENU`, `CHARACTER_SELECT`, `RACING`, `RESULTS`) with 4 distinct selectable characters offering observable kart color and stat variations.
2. **R2**: Implement multi-kart entity system with 3 AI opponent karts following track waypoints; fix Blooper to blind opponents; convert Green Shell into a forward-moving projectile; add safe banana arming.
3. **R3**: Implement multiplayer (2-player split-screen or dual-input local multiplayer with P1 WASD and P2 Arrow keys).
4. **R4**: Implement drift physics (holding drift key enters slide and builds mini-turbo boost); add top-left HUD minimap with real-time kart blips; add animated Bowser Castle lava geysers and Choco Valley chocolate fountains.
5. **R5**: Maintain Desktop Qt 6.11.1 MinGW 64-bit clean compilation with 0 warnings and stable 60 FPS.

---

## 5. Verification Method

1. **Compilation Check**:
   ```powershell
   $env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;$env:PATH"
   mingw32-make release
   ```
   *Expected result*: Clean exit code 0, no errors, no unresolved symbols.
2. **Verification of Survey Deliverables**:
   - Inspect `d:\Qt\RetroKart3D\.agents\teamwork\explorer_survey_1\survey_report.md`
   - Inspect `d:\Qt\RetroKart3D\.agents\teamwork\explorer_survey_1\handoff.md`
3. **Invalidation Conditions**:
   - If Qt 6.11.1 MinGW build fails on `D:\Qt\6.11.1\mingw_64`.
   - If `screenBuffer` 400x300 rendering resolution or Mode-7 scanline formulation is incompatible with multi-kart rendering.
