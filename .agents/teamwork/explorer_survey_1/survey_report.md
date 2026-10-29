# RetroKart3D — Comprehensive Codebase & Architecture Survey Report

**Prepared by**: `explorer_survey_1` (Codebase & Architecture Explorer)  
**Date**: 2026-10-29  
**Target Environment**: Windows 11 / Desktop Qt 6.11.1 MinGW 64-bit (`D:\Qt\6.11.1\mingw_64`)  
**Workspace**: `d:\Qt\RetroKart3D`

---

## Executive Summary

RetroKart3D is a software-rasterized first-person pseudo-3D kart racing game written in C++17 using Qt 6 Widgets. It executes entirely without hardware acceleration (no OpenGL/Vulkan/DirectX shaders), using Mode-7 scanline inverse perspective mapping, procedural 32x32 pixel sprites, and scanline rendering into a 400x300 RGB32 buffer (`screenBuffer`) which is blitted to the widget canvas via Qt's `QPainter`.

While the foundational Mode-7 ground projection and celebration animation systems are working and compile cleanly with Qt 6.11.1 MinGW 64-bit with zero warnings, the codebase currently has major architectural gaps relative to the project showcase requirements:
1. **No Game State Machine**: No Title screen, Main Menu, or Character Selection screen exists. The application launches straight into an active race on Donut Plains.
2. **No AI Opponents or Multi-Entity Support**: The player is the sole kart in the world. The player kart's physics variables are hardcoded directly into `GameCanvas`. There are no opponent karts, no AI waypoint paths, and no kart rendering billboard sprites.
3. **Broken / Inverted Item Targeting**: 
   - Ink Blooper blinds the player who deployed it (`inkTimer = 4.5` on the player).
   - Green Shell spawns as a static world sprite with zero forward velocity or collision targeting.
   - Bananas drop immediately behind the player and can spin out the player upon backing up or re-crossing.
4. **No Drift Mechanics or Character Stats**: Physics uses an instantaneous heading rotation without lateral drift, counter-steering, or mini-turbo sparks. Character stats (speed, accel, handling) are hardcoded single values.
5. **Missing Minimap & Interactive Track Hazards**: No HUD minimap exists. Bowser Castle and Choco Valley lack interactive hazards like spewing lava geysers or chocolate fountains.
6. **No Multiplayer Pipeline**: Input is bound to a single player (WASD and arrow keys both control the same kart). No split-screen or network multiplayer exists.

---

## 1. Project Configuration & Build Environment

### 1.1 Qt Project File (`RetroKart3D.pro`)
```qmake
QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += c++17
SOURCES += main.cpp mainwindow.cpp game_canvas.cpp sprite_atlas.cpp
HEADERS += mainwindow.h game_canvas.h sprite_atlas.h
FORMS   += mainwindow.ui
```
- **Qt Modules**: Only `core`, `gui`, and `widgets` are included.
- **Multilplayer Note**: If network multiplayer (LAN/sockets via `QTcpSocket`/`QUdpSocket`) is implemented, `QT += network` must be added to `RetroKart3D.pro`. If local multiplayer (split-screen or shared-screen) is implemented, no additional Qt modules are needed.

### 1.2 Toolchain Verification
- **Qt Version**: Qt 6.11.1 (`D:\Qt\6.11.1\mingw_64`)
- **qmake**: `D:\Qt\6.11.1\mingw_64\bin\qmake.exe` (QMake version 3.1)
- **Compiler**: MinGW-w64 GCC 13.1.0 (`D:\Qt\Tools\mingw1310_64\bin\g++.exe`)
- **Make Tool**: GNU Make 4.2.1 (`D:\Qt\Tools\mingw1310_64\bin\mingw32-make.exe`)
- **Compilation Check**:
  - The project compiles cleanly under `g++ -O2 -std=gnu++1z -Wall -Wextra` with **0 errors and 0 warnings**.
  - The release binary `release/RetroKart3D.exe` (110 KB) runs stably without external asset dependencies because all sprites and tracks are generated procedurally in code.

---

## 2. Current Architecture & Class Hierarchy

```
+-------------------------------------------------------------+
|                          main.cpp                           |
|                  Instantiates QApplication                  |
|                   Instantiates MainWindow                   |
+------------------------------+------------------------------+
                               |
                               v
+-------------------------------------------------------------+
|                         MainWindow                          |
|  - Manages centralWidget with QHBoxLayout                   |
|  - Holds controlPanel (Right Sidebar, 260px width)          |
|    - Track buttons (btnLevel1, btnLevel2, btnLevel3)        |
|    - Telemetry display (lblSpeed, lblLap, lblPos, etc.)     |
|    - Action buttons (btnReset, btnCelebration)              |
|  - Embeds GameCanvas inside ui->canvasContainer             |
+------------------------------+------------------------------+
                               |
                               v
+-------------------------------------------------------------+
|                         GameCanvas                          |
|  (God Object - 1202 LOC)                                    |
|  - Loop & Timing: QTimer (16ms / ~60 FPS), QElapsedTimer    |
|  - State: TrackLevel, PlayerItem, Checkpoints, Laps         |
|  - Player Kart Physics: speed, heading, position, accel     |
|  - World Sprites: std::vector<WorldSprite>                  |
|  - Track Procedural Maps: trackVisualMap, trackMaskMap      |
|  - Software Rasterizer:                                     |
|      * renderSky (Parallax mountain gradient)               |
|      * renderGroundMode7 (Scanline inverse mapping)         |
|      * renderSprites (Camera transform, depth sort)         |
|      * renderCockpit (First-person hood & steering wheel)   |
|      * renderItemHUD (Roulette / held item display)         |
|      * renderInkOverlay (Windshield splatter)               |
|      * renderCelebration (Confetti, ribbon, victory banner) |
|  - Input: keyPressEvent / keyReleaseEvent (pressedKeys)     |
+------------------------------+------------------------------+
                               |
                               v
+-------------------------------------------------------------+
|                        SpriteAtlas                          |
|  (Singleton - Procedural 32x32 ARGB Pixel Sprites)          |
|  - Pickups: QUESTION_BOX, MUSHROOM, ROCKET, BANANA,         |
|             GREEN_SHELL, INK_BLOOPER, COIN                  |
|  - Obstacles: GREEN_PIPE, THWOMP, OIL_SLICK, BANANA_DROPPED |
|  - Projectiles: SHELL_PROJECTILE                            |
+-------------------------------------------------------------+
```

### Separation of Concerns Violations:
1. **God-Class in `GameCanvas`**: `GameCanvas` handles rendering, physics, procedural map generation, item roulette, particle physics, celebrations, input, and checkpoint detection all in one 1202-line file.
2. **Missing Entity Model**: There is no `Kart` or `Player` class. The player kart's physics variables (`kartX`, `kartZ`, `kartAngle`, `kartSpeed`, `maxSpeed`, `accel`, `friction`, `turnRate`, `isSpinning`, `mushroomBoostTimer`, etc.) exist as flat members of `GameCanvas`.
3. **No Opponent Representation**: There is no data structure for AI opponents or second players.

---

## 3. Game Loop, Timing & Frame Rate Structure

- **Timer Configuration** (`game_canvas.cpp:27-29`):
  ```cpp
  connect(&gameTimer, &QTimer::timeout, this, &GameCanvas::updateGameLoop);
  gameTimer.start(16); // ~60 FPS
  frameTimer.start();
  ```
- **Loop Execution** (`game_canvas.cpp:310-330`):
  ```cpp
  void GameCanvas::updateGameLoop()
  {
      double dt = frameTimer.restart() / 1000.0;
      if (dt > 0.05) dt = 0.05; // clamp delta time for stability

      updatePhysics(dt);
      updateItemSystem(dt);
      updateCelebration(dt);

      renderSky();
      renderGroundMode7();
      renderSprites();
      renderCockpit();
      renderItemHUD();
      renderInkOverlay();
      renderCelebration();

      emit statsUpdated(kartSpeed, currentLap, kartX, kartZ, kartAngle * (180.0 / M_PI), currentSurfaceName);

      this->update(); // Request Qt paintEvent
  }
  ```
- **Presentation** (`game_canvas.cpp:1016-1022`):
  ```cpp
  void GameCanvas::paintEvent(QPaintEvent *)
  {
      QPainter painter(this);
      painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
      painter.drawImage(rect(), screenBuffer);
  }
  ```
- **Performance Characteristics**:
  - `screenBuffer` resolution is fixed at 400x300 (`BUFFER_WIDTH` x `BUFFER_HEIGHT`).
  - Total pixels rendered per frame: 120,000.
  - Mode 7 inverse mapping only processes the lower half of the screen (`y = 150` to `300` = 60,000 pixels).
  - Software rendering at 400x300 runs in ~1.5 to 2.2 ms per frame on modern CPU threads, comfortably achieving 60 FPS well within the 16.6ms budget.

---

## 4. Rendering Pipeline Analysis

### 4.1 Mode 7 Ground Projection (`renderGroundMode7`, lines 687-717)
- Projects a 2D 1024x1024 track texture (`trackVisualMap`) onto a 3D ground plane.
- For each scanline `y` from horizon (150) to bottom (300):
  - Calculates ground distance: $z_{ground} = \frac{h_{cam} \cdot f}{y - y_{horizon}}$
  - Horizontal step scale: $s_{ground} = \frac{h_{cam}}{y - y_{horizon}}$
  - Transforms pixel offset $(dx \cdot s_{ground}, z_{ground})$ by camera heading $(-\sin\theta, \cos\theta)$ and translates by player position $(x, z)$.
  - Samples pixel with modulo wrapping: $(u \pmod{1024}, v \pmod{1024})$.

### 4.2 Billboard Sprite Projection (`renderSprites`, lines 719-809)
- Transforms world coordinates $(x, z, y)$ to camera space:
  - $camX = -(x - kartX)\sin\theta + (z - kartZ)\cos\theta$
  - $camZ = (x - kartX)\cos\theta + (z - kartZ)\sin\theta$
- Frustum culling: $2.0 < camZ \le 450.0$.
- Screen projection:
  - $sx = \frac{W}{2} + \frac{f \cdot camX}{camZ}$
  - $sy = Y_{horizon} + \frac{f \cdot (h_{cam} - y_{eff})}{camZ}$
  - $size = \frac{baseSize \cdot f}{camZ}$
- Depth sorting using std::sort (Painter's algorithm: farthest to nearest).
- Alpha-blended software blitting via `drawTexturedBillboard` (lines 811-857).

### 4.3 Missing Rendering Elements
1. **Opponent Karts**: No karts are rendered in the 3D world. Currently, the only kart visual is the player's 2D first-person hood in `renderCockpit()`.
2. **Minimap**: No minimap is rendered.
3. **Interactive Track Hazards**: Bowser's Castle lava and Choco Valley puddles are static ground pixels or simple pipes/thwomps. Spewing lava geysers and chocolate fountains with animated particle effects are absent.

---

## 5. Input Handling & Controls

- **Key Listener**: `pressedKeys` set in `GameCanvas` captures `keyPressEvent` and `keyReleaseEvent`.
- **Active Controls**:
  - `W` / `Up Arrow`: Accelerate (`kartSpeed += accel * dt`)
  - `S` / `Down Arrow`: Brake / Reverse (`kartSpeed -= brakeDecel * dt`)
  - `A` / `Left Arrow`: Steer Left (`kartAngle += steerDirection * 3.4 * steerSpeedFactor * dt`)
  - `D` / `Right Arrow`: Steer Right
  - `Space`: Use held item (`useHeldItem()`)
  - `R`: Reset kart to start
  - `F`: Trigger celebration animation
  - `1`, `2`, `3`: Select track level
- **Deficiencies**:
  - No drift key or drift mechanic.
  - Controls are shared between WASD and Arrow Keys for a single kart, preventing 2-player local play.
  - Key focus issues: If a button on the UI sidebar is clicked, `GameCanvas` loses focus unless `gameCanvas->setFocus()` is explicitly called.

---

## 6. Physics Implementation Details

### 6.1 Acceleration & Braking
```cpp
// Forward throttle
kartSpeed += currentAccel * dt;
if (kartSpeed > currentMaxSpeed) kartSpeed = currentMaxSpeed;

// Braking / Reverse
kartSpeed -= brakeDecel * dt;
if (kartSpeed < -currentMaxSpeed * 0.3) kartSpeed = -currentMaxSpeed * 0.3;

// Friction
if (kartSpeed > 0) kartSpeed = std::max(0.0, kartSpeed - currentDrag * dt);
```
- Base stats: `maxSpeed = 160.0`, `accel = 110.0`, `brakeDecel = 160.0`, `friction = 45.0`.
- Mushroom/Rocket boost: `currentMaxSpeed = maxSpeed * 1.55`, `currentAccel = accel * 2.2`.

### 6.2 Surface Friction & Off-Road Penalties
Evaluated via `trackMaskMap` RGB values:
- **Tarmac / Road** (`RGB(255, 255, 255)`): 100% speed, normal friction.
- **Grass / Off-Road** (`RGB(0, 255, 0)`): Speed capped at 45% (`maxSpeed * 0.45`), drag multiplied by 2.2 (`currentDrag *= 2.2`).
- **Stone Curb** (`RGB(0, 255, 0)` on Bowser Castle): Speed capped at 70%, drag 2.2x.
- **Lava Hazard** (`RGB(255, 0, 255)`): Recoils kart backward 8 units, cuts speed to 35%, triggers spinout for 0.45s.

### 6.3 Steering & Handling
- Angular velocity is dynamically scaled with speed:
  `steerSpeedFactor = std::clamp(std::abs(kartSpeed) / 50.0, 0.70, 1.30);`
  `kartAngle += steerDirection * 3.4 * steerSpeedFactor * dt;`
- Position integration:
  `kartX += kartSpeed * cos(kartAngle) * dt;`
  `kartZ += kartSpeed * sin(kartAngle) * dt;`
- **Missing Drift Physics**:
  - Drift button (`Shift` or `Space` / `V`).
  - Lateral slip velocity vector (drifting outward while heading points inward).
  - Mini-turbo accumulation timer and spark particles.

---

## 7. State Management Analysis

### Current Status: None
There is no game state enum or state machine in the current codebase.
```cpp
// Current state variables in GameCanvas:
bool isCelebrationActive = false;
bool isRaceFinished = false;
bool isRouletteActive = false;
bool isSpinning = false;
```
The application boots directly into race mode.

### Required State Flow (per R1):
```
                  +---------------------------+
                  |        TITLE / MENU       |
                  |  - Play vs AI             |
                  |  - Multiplayer            |
                  |  - Practice Mode          |
                  +-------------+-------------+
                                |
                                v
                  +---------------------------+
                  |     CHARACTER SELECT      |
                  |  - 4 Distinct Characters  |
                  |  - Unique Kart & Stats    |
                  +-------------+-------------+
                                |
                                v
                  +---------------------------+
                  |         TRACK RACE        |
                  |  - Race 3 Laps            |
                  |  - AI Opponents Active    |
                  |  - HUD with Minimap       |
                  +-------------+-------------+
                                |
                                v
                  +---------------------------+
                  |       RACE RESULTS        |
                  |  - Placements (1st - 4th) |
                  |  - Lap Times & Replay     |
                  +---------------------------+
```

---

## 8. Item System & AI Opponents Breakdown (R2)

### Current Inverted / Broken Item Behavior
1. **Ink Blooper** (`game_canvas.cpp:602-604`):
   ```cpp
   case PlayerItem::INK_BLOOPER:
       inkTimer = 4.5; // BUG: Blinds the player who activated the item!
       break;
   ```
   **Fix**: When the player activates Ink Blooper, `inkTimer` must NOT be set on the player; instead, all opponent AI karts (or peer players) must be affected (causing them to steer erratically and slow down).
2. **Green Shell** (`game_canvas.cpp:593-600`):
   ```cpp
   case PlayerItem::GREEN_SHELL: {
       double fireDist = 28.0;
       double fx = kartX + std::cos(kartAngle) * fireDist;
       double fz = kartZ + std::sin(kartAngle) * fireDist;
       worldSprites.push_back({ fx, fz, 0.0, SpriteType::GREEN_SHELL, true, 0.0 });
       break;
   }
   ```
   **Bug**: Green shell is spawned as a static pickup/obstacle in `worldSprites`! It has no velocity vector, doesn't travel forward, and cannot hit opponents ahead.
   **Fix**: Introduce dynamic `Projectile` struct with `(x, z, vx, vz, lifeTime, ownerId)` that moves rapidly forward along firing heading, checks collisions with karts, and spins them out.
3. **Banana Peel** (`game_canvas.cpp:584-591`):
   - Currently placed 24 units behind player. If player reverses or turns around, player hits their own banana immediately.
   - Bananas need an activation arming delay or owner immunity flag.

### Missing AI Opponents
- Track curves need defined waypoints or parametric centerlines:
  - Donut Plains: Oval cubic Bezier path `(512, 200) -> (850, 500) -> (512, 800) -> (180, 500)`.
  - Choco Valley: S-curve cubic Bezier path.
  - Bowser Castle: Polygon vertex path.
- AI Kart entity needs:
  - Position `(x, z)`, heading $\theta$, speed, current waypoint index.
  - Steering toward target waypoint with slight noise / overtaking offsets.
  - Item reaction (spinning out when hitting bananas or shells).
  - Billboard sprite generation in `SpriteAtlas` with 8 or 4 directional views or distinct kart colorings.

---

## 9. Multiplayer Feasibility Assessment (R3)

Two viable architectures exist for multiplayer:
1. **Local Shared/Split-Screen Multiplayer** (Recommended for immediate showcase demo):
   - Player 1: WASD + Space (Item) + V (Drift)
   - Player 2: Arrow Keys + Numpad 0 / Enter (Item) + Right Ctrl (Drift)
   - Rendering: Split `screenBuffer` horizontally or vertically (e.g., top half P1, bottom half P2, each 400x150, or side-by-side 200x300), OR shared single screen with close camera tracking.
   - Advantages: 100% reliable, zero network latency, no firewall/port binding issues, runs immediately in a single window.
2. **Network Multiplayer (LAN / Sockets)**:
   - Uses `QTcpServer` / `QTcpSocket` or `QUdpSocket`.
   - Requires adding `QT += network` to `RetroKart3D.pro`.
   - State synchronization: Host sends tick packets with kart positions, clients send input packets.
   - Consideration: Takes more architectural complexity to handle connection handshakes and packet serialization.

Local multiplayer with separate controls or local LAN socket play can be chosen based on team coordination.

---

## 10. Architectural Recommendations for Implementation Team

1. **Refactor Entity Representation**:
   - Create a clean `Kart` struct/class:
     ```cpp
     struct Kart {
         int id;
         bool isAI;
         bool isLocalPlayer;
         int characterIndex;
         double x, z, y;
         double angle;
         double speed;
         double maxSpeed, accel, handling, weight;
         PlayerItem heldItem;
         bool isSpinning;
         double spinTimer;
         double boostTimer;
         int currentLap;
         int currentWaypoint;
         // Drift state
         bool isDrifting;
         int driftDirection; // -1 left, +1 right
         double driftCharge;
     };
     ```
2. **Implement Game State Machine**:
   - `enum class GameState { TITLE_MENU, CHARACTER_SELECT, RACING, RESULTS };`
   - Render title menu and character select screens directly on the canvas or via clean overlay widgets.
3. **Add 3D Kart Billboard Sprites to `SpriteAtlas`**:
   - Generate distinct colored kart sprites (e.g. Red, Green, Blue, Yellow/Orange).
4. **Implement Minimap in HUD**:
   - Draw 80x80 minimap in top-left corner (`BUFFER_WIDTH` top-left).
   - Render scaled track path + blit small colored dots for player and AI opponents.
5. **Add Animated Track Hazards**:
   - Lava Geyser on Bowser Castle: timed eruption animation with fireball particles and spinout collision.
   - Chocolate Fountain on Choco Valley: bubbling chocolate fountain sprite with sticky mud puddles.

---

## 11. Acceptance Criteria Traceability Matrix

| Requirement | Codebase Status | Key Action Required |
| :--- | :--- | :--- |
| **R1. Menu & Character Select** | Not implemented | Implement `GameState` machine, Menu UI (Play vs AI, Multiplayer, Practice), Character Select (4 characters with distinct stats & colors). |
| **R2. AI Opponents & Item Targeting** | Opponents missing; items inverted/static | Add AI waypoint following; fix Blooper to target opponents; make Green Shell dynamic forward projectile; arm bananas safely. |
| **R3. Multiplayer Support** | Not implemented | Add 2-player mode (local split/dual controls or network sockets). |
| **R4. Physics & Track Hazards** | Physics working; drift missing; hazards static; minimap missing | Add responsive WASD drift & mini-turbo; add animated lava geysers & choco fountains; add top-left HUD minimap. |
| **R5. Build Stability & 60 FPS** | Verified working (0 warnings, 60 FPS) | Maintain software buffer efficiency; ensure no memory leaks or race conditions across frames. |
