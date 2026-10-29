# RetroKart 3D: Gameplay & AI Technical Investigation Report

**Author:** `explorer_survey_2` (Gameplay & AI Explorer)  
**Date:** 2026-10-29  
**Target Project:** RetroKart 3D (`d:\Qt\RetroKart3D`)  
**Scope:** AI Opponents, Item System & Targeting Bugs, Track Hazards, HUD & Minimap, Multiplayer Feasibility

---

## 1. Executive Summary

RetroKart 3D delivers an authentic software-rasterized Mode 7 racing engine written in modern C++ and Qt 6. However, an in-depth codebase audit reveals that **gameplay mechanics are largely single-player test scaffolds**:
1. **AI Opponents**: There are currently **zero** AI opponents instantiated, updated, or rendered in the game. All kart variables (`kartX`, `kartZ`, `kartAngle`, `kartSpeed`) exist as scalar members of `GameCanvas` representing only a single player. Furthermore, track centerlines and waypoints are discarded immediately after track bitmap generation.
2. **Item System Bugs**: Offensive items either self-inflict debuffs onto the player or sit completely still in the world. Specifically:
   - `INK_BLOOPER` hardcodes `inkTimer = 4.5` on the player kart (`game_canvas.cpp:603`), blinding the user who pressed Space.
   - `GREEN_SHELL` spawns as a static billboard sprite (`game_canvas.cpp:598`) with zero velocity and is completely omitted from the collision detection loop (`game_canvas.cpp:468–504`).
   - `BANANA` drops behind the player with no immunity window and no owner tracking; with no opponents on track, the player is the only entity that can hit it.
3. **Track Hazards**: Tracks use a dual-map architecture (`trackVisualMap` + `trackMaskMap`), but the required **lava geysers** (Bowser Castle) and **chocolate fountains/candies** (Choco Valley) do not exist. Obstacles like Thwomp are static props without height-based collision or vertical animation.
4. **HUD & Minimap**: While an item roulette box and victory banner exist, there is **zero minimap implementation** anywhere in the codebase.
5. **Multiplayer Feasibility**: Local split-screen multiplayer (P1 WASD/Space, P2 Arrows/Enter) is overwhelmingly superior to LAN sockets for today's showcase deliverable. Because Mode 7 rasterizes scanlines independently, rendering two horizontal viewports (each 400×150) takes the exact same number of scanline calculations (60,000 pixels total) as a single 400×300 frame, guaranteeing 60 FPS performance with zero network latency or socket configuration risk.

---

## 2. AI Opponents: Current State & Implementation Blueprint

### 2.1 Current State Analysis
- **Kart Modeling**: `game_canvas.h` lines 83–94 define only scalar fields:
  ```cpp
  double kartX = 470.0;
  double kartZ = 200.0;
  double kartAngle = 0.0;
  double kartSpeed = 0.0;
  double maxSpeed = 160.0;
  double accel = 110.0;
  double brakeDecel = 160.0;
  double friction = 45.0;
  double turnRate = 2.4;
  ```
  There is no collection, vector, or abstraction for opponent karts.
- **Population**: In `game_canvas.cpp:setTrackLevel()`, tracks are generated, sprites are cleared and populated, but **no opponent karts are created**. The race has exactly 1 participant (the player).
- **Waypoint & Pathfinding Absence**: In `generateDonutPlains()` (lines 133–137) and `generateChocoValley()` (lines 203–209), the track path is constructed using a local `QPainterPath path`. In `generateBowserCastle()` (lines 256–262), a local `QPolygon castlePoly` is used. **None of these geometric paths are retained in memory.** No waypoint arrays, centerlines, or graph nodes exist after track generation completes.

### 2.2 Waypoint Definition Architecture
To enable AI navigation, each level must maintain an ordered vector of 2D waypoints:
```cpp
struct TrackWaypoint {
    double x;
    double z;
    double speedLimit; // Target speed on approach
};
```
During track initialization:
- **Level 1 (Donut Plains)**: 16 discrete waypoints along the 1024×1024 oval centerline:
  - Starting straight: `(512, 200) -> (680, 200) -> (820, 260)`
  - East curve: `(860, 400) -> (860, 600) -> (820, 740)`
  - Back straight: `(680, 800) -> (512, 800) -> (340, 800)`
  - West curve: `(200, 740) -> (160, 600) -> (160, 400) -> (200, 260) -> (340, 200) -> (512, 200)`
- **Level 2 (Choco Valley)**: 20 waypoints tracing the S-curves:
  - `(512, 200) -> (700, 210) -> (840, 300) -> (780, 460) -> (660, 520) -> (580, 650) -> (520, 810) -> (350, 820) -> (220, 650) -> (220, 450) -> (160, 300) -> (320, 210) -> (512, 200)`
- **Level 3 (Bowser Castle)**: 16 waypoints rounding the rectangular fortress corners:
  - `(512, 250) -> (700, 250) -> (770, 280) -> (770, 500) -> (730, 545) -> (560, 545) -> (550, 620) -> (550, 750) -> (500, 775) -> (300, 775) -> (255, 740) -> (255, 300) -> (290, 255) -> (512, 250)`

### 2.3 AI Opponent Kinematics & Steering Logic
```cpp
struct OpponentKart {
    int id;
    QString name;
    QColor color;
    double x, z;
    double angle;
    double speed;
    double maxSpeed;
    double accel;
    double turnRate;
    int currentWaypoint;
    int lap;
    bool isSpinning;
    double spinTimer;
    double inkTimer;
    SpriteType spriteType; // Procedural kart billboard
};
```
In `updateAI(double dt)`:
1. **Targeting**: Query `targetWp = waypoints[kart.currentWaypoint]`.
2. **Distance & Advancement**:
   $$\Delta x = \text{targetWp.x} - \text{kart.x}, \quad \Delta z = \text{targetWp.z} - \text{kart.z}$$
   $$\text{dist} = \sqrt{\Delta x^2 + \Delta z^2}$$
   If $\text{dist} < 45.0$, advance `currentWaypoint = (currentWaypoint + 1) % waypoints.size()`.
3. **Steering Angle Calculation**:
   $$\theta_{\text{desired}} = \operatorname{atan2}(\Delta z, \Delta x)$$
   $$\Delta\theta = \theta_{\text{desired}} - \text{kart.angle}$$
   Normalize $\Delta\theta \in [-\pi, \pi]$:
   ```cpp
   while (diff > M_PI) diff -= 2.0 * M_PI;
   while (diff < -M_PI) diff += 2.0 * M_PI;
   ```
4. **Steering Response**:
   $$\text{kart.angle} \mathrel{+}= \operatorname{clamp}(\Delta\theta \times 3.8, -\text{kart.turnRate}, \text{kart.turnRate}) \times dt$$
5. **Throttle & Rubber-Banding**:
   - Base throttle: accelerate towards `kart.maxSpeed` (e.g. 130–150 km/h).
   - Cornering adjustment: If $|\Delta\theta| > 0.4$ radians, reduce speed to $0.75 \times \text{maxSpeed}$ to prevent understeer.
   - Dynamic Catchup (Rubber-Banding): If an AI kart is $> 300$ units behind the player, grant a $+15\%$ speed boost; if $> 200$ units ahead, reduce top speed by $10\%$.
6. **Spinout Recovery**: If `kart.isSpinning`, rotate $\Delta\theta = 15.0 \times dt$, decrease speed rapidly, decrement `spinTimer`.

### 2.4 Rendering Opponents in Mode 7
Opponent karts are rendered directly through the existing billboard pipeline in `renderSprites()`:
1. Each AI kart is converted to a `WorldSprite` or projected directly into `renderList`:
   ```cpp
   double dx = ai.x - playerKartX;
   double dz = ai.z - playerKartZ;
   double camX = -dx * sinA + dz * cosA;
   double camZ =  dx * cosA + dz * sinA;
   ```
2. Near-plane clipped ($camZ > cameraNearPlane$).
3. Scaled: $\text{drawW} = \text{drawH} = \text{static\_cast<int>}(22.0 \times \frac{focalLength}{camZ})$.
4. Depth-sorted with existing item boxes, obstacles, and powerups via the Painter's Algorithm.
5. In `SpriteAtlas`, add procedural 32×32 kart sprites tinted per character (Mario Red, Luigi Green, Peach Pink, Bowser Yellow/Green).

---

## 3. Item System: Root Cause Analysis & Fix Specifications

### 3.1 Inventory of Items
The current system defines:
- `MUSHROOM`: Player speed $\times 1.4$ for 3.0s (`game_canvas.cpp:575`).
- `ROCKET`: Player speed $\times 1.65$ for 4.5s (`game_canvas.cpp:580`).
- `BANANA`: Drops behind kart (`game_canvas.cpp:585`).
- `GREEN_SHELL`: Spawns ahead of kart (`game_canvas.cpp:594`).
- `INK_BLOOPER`: Obscures vision (`game_canvas.cpp:602`).

### 3.2 Detailed Root Cause Analysis of Item Targeting Bugs

#### Bug 1: INK_BLOOPER Blinds the Player
- **Exact Line**: `game_canvas.cpp:602–604`
  ```cpp
  case PlayerItem::INK_BLOOPER:
      inkTimer = 4.5;
      break;
  ```
- **Mechanism**: `inkTimer` is a member of `GameCanvas` governing the player's screen overlay. Lines 324 and 988–1014 (`renderInkOverlay()`) render 10 black alpha-blended ink splatters covering the player's viewport.
- **Root Cause**: The developer implemented the visual effect on the local canvas without any check or target distinction. In Mario Kart, Blooper obscures **rivals'** screens, never the user's.
- **Correct Behavior**:
  - When Player 1 uses `INK_BLOOPER`, player 1 receives **no** ink on screen.
  - All active AI opponents receive `ai.inkTimer = 4.5`, reducing their speed by $30\%$ and causing slight randomized steering swerves.
  - In 2-Player Split-Screen, Player 2's viewport receives the ink splatter overlay, while Player 1 remains clear!

#### Bug 2: GREEN_SHELL is a Motionless Prop with No Collision
- **Exact Lines**: `game_canvas.cpp:593–600`
  ```cpp
  case PlayerItem::GREEN_SHELL: {
      // Fire green shell forward
      double fireDist = 28.0;
      double fx = kartX + std::cos(kartAngle) * fireDist;
      double fz = kartZ + std::sin(kartAngle) * fireDist;
      worldSprites.push_back({ fx, fz, 0.0, SpriteType::GREEN_SHELL, true, 0.0 });
      break;
  }
  ```
- **Mechanism**: The shell is appended to `worldSprites` as a static billboard. It has no velocity vector, no angle, and no lifetime.
- **Omission in Collision Loop**: Lines 488–504 check collisions against:
  - `QUESTION_BOX`
  - `COIN`
  - `BANANA` / `BANANA_DROPPED` / `OIL_SLICK`
  - `GREEN_PIPE` / `THWOMP`
  - `MUSHROOM`
  `GREEN_SHELL` is **not included in any collision branch**! A kart driving into it passes right through.
- **Correct Behavior**:
  Green shells must be managed in a dedicated active projectile container:
  ```cpp
  struct ShellProjectile {
      double x, z;
      double vx, vz;
      int ownerId;        // 0 = Player 1, 1 = P2, 100+ = AI
      double lifetime;    // 6.0 seconds max
      int bounceCount;    // Bounces off walls up to 3 times
      bool active;
  };
  ```
  In `updateProjectiles(dt)`:
  - Move shell forward: `x += vx * dt; z += vz * dt;`
  - Check collision against all karts (Player and AI):
    If distance $< 16.0$ units and `(ownerId != targetId || lifetime < 5.5)`:
    - Target spins out (`isSpinning = true`, `spinTimer = 1.0`, `speed *= 0.35`).
    - Shell deactivates.

#### Bug 3: Self-Inflicted BANANA Drops
- **Exact Lines**: `game_canvas.cpp:584–591`
  ```cpp
  case PlayerItem::BANANA: {
      double dropDist = 24.0;
      double bx = kartX - std::cos(kartAngle) * dropDist;
      double bz = kartZ - std::sin(kartAngle) * dropDist;
      worldSprites.push_back({ bx, bz, 0.0, SpriteType::BANANA_DROPPED, true, 0.0 });
      break;
  }
  ```
- **Mechanism**: `WorldSprite` lacks owner tracking. If the kart reverses, turns sharply, or spins, it collides with its own banana immediately. Furthermore, because no opponents exist, the player is guaranteed to be the only victim.
- **Fix**:
  1. Add `ownerId` and `immunityTimer = 1.5s` to dropped traps. The owner kart cannot trigger the banana during the first 1.5 seconds.
  2. Populate AI opponents who navigate the track and trigger the trap when driving over it.

---

## 4. Track Hazards: Bowser Castle & Choco Valley

### 4.1 Track Surface & Representation Pipeline
Tracks use two 1024×1024 `QImage` buffers:
1. `trackVisualMap`: Rendered to the screen via Mode 7 pixel sampling (`game_canvas.cpp:708–714`).
2. `trackMaskMap`: Queried for physics (`game_canvas.cpp:354–389`):
   - White `(255, 255, 255)`: Asphalt/Road (full speed).
   - Green `(0, 255, 0)`: Off-road/Grass/Sand (speed cut to $45\%$, drag $\times 2.2$).
   - Magenta `(255, 0, 255)`: Lava hazard (recoil, spin, speed cut to $35\%$).
   - Yellow `(255, 255, 0)`: Checkpoint/Finish line.

### 4.2 Missing Bowser Castle Hazards: Spewing Lava Geysers
- **Current State**: Lava is merely a flat, static magenta background color on the mask. Touching it triggers a recoil, but there are **no lava geysers, no visual eruptions, and no interactive hazards**.
- **Specification for Lava Geysers**:
  ```cpp
  struct LavaGeyser {
      double x, z;
      double cycleTimer;    // Period (e.g. 4.0s cycle)
      double eruptionDuration; // Erupts for 1.2s
      bool isErupting;
      double flameHeight;
  };
  ```
  - **Visuals**: Position 4–6 geysers in the lava moat directly bordering the stone raceway:
    - (210, 250), (820, 250), (820, 580), (520, 750), (210, 750).
    - When `cycleTimer < eruptionDuration`: Render animated erupting flame column billboards (height pulsing from $0$ to $35$ units).
  - **Interaction**: If a kart drives within 20 units of an active geyser:
    - Triggers lava spinout, screen flash (`flashIntensity = 0.7`), and speed penalty.

### 4.3 Missing Choco Valley Hazards: Chocolate Fountains & Rolling Candies
- **Current State**: Choco Valley currently contains 2 static `OIL_SLICK` billboards (`game_canvas.cpp:239–240`) and 1 static dropped banana. There are **zero chocolate fountains and zero candies**.
- **Specification for Chocolate Fountains & Candies**:
  1. **Chocolate Fountains**:
     - 2 bubbling chocolate geysers placed at the apex of the hairpin corners: `(840, 360)` and `(240, 520)`.
     - Procedural billboard sprite with brown/cream bubbling droplets animated using `std::sin(globalTime * 8.0)`.
  2. **Rolling Candies / Chocolate Boulders**:
     - 2–3 circular candy hazard sprites that oscillate or roll across the track width:
       `x = baseCenterX + std::sin(globalTime * 1.5) * 40.0;`
     - Hitting a candy boulder causes a spinout and $-20$ speed reduction.

---

## 5. HUD & Minimap Radar System

### 5.1 Current HUD Structure
1. **In-Canvas Overlays** (`game_canvas.cpp`):
   - Cockpit hood and steering wheel (`renderCockpit()`, lines 859–905).
   - Item roulette / held item box at top center (`renderItemHUD()`, lines 907–986).
   - Ink overlay (`renderInkOverlay()`, lines 988–1014).
   - Celebration/lap banner (`renderCelebration()`, lines 1101–1201).
2. **Sidebar Telemetry** (`mainwindow.cpp` / `mainwindow.ui`):
   - Speedometer, Lap counter, World coordinates $(X, Z)$, Heading angle, and Surface material string.
3. **Minimap Status**: **Completely missing.** There is no minimap on the HUD or anywhere else.

### 5.2 Functional Minimap Blueprint (Top-Left HUD)
Requirement R4 mandates a **"functional minimap in the top-left corner"** showing track outline and real-time kart positions.

- **Placement & Dimensions**:
  - Position: `x = 8, y = 8` in `screenBuffer`.
  - Dimensions: `72 × 72` pixels.
  - Border: Semi-transparent dark rounded container `QColor(15, 18, 30, 210)` with 1px gold border `QColor(255, 215, 0, 180)`.
- **Pre-computed Radar Track Thumbnail**:
  - Rather than downsampling the full 1024×1024 map every frame, cache a clean 64×64 thumbnail `trackMinimapIcon` when the track is loaded.
  - Draw the track centerline/ribbon in contrasting gray/tan/stone with a subtle dark outline.
- **Real-Time Kart Markers**:
  - Coordinate Mapping:
    $$\text{mapX} = 8 + 4 + \text{static\_cast<int>}\left(\frac{\text{kart.x}}{1024.0} \times 64.0\right)$$
    $$\text{mapY} = 8 + 4 + \text{static\_cast<int>}\left(\frac{\text{kart.z}}{1024.0} \times 64.0\right)$$
  - **Player 1**: 4×4 bright golden/yellow square with a 1px white border and a short heading pointer line.
  - **Opponents / Player 2**: 3×3 color-coded circular dots (Red, Green, Cyan, Purple).
  - **Start/Finish Line**: Small 2px checkered mark at (512, 200).

```
+--------------------------+
| MINIMAP (72x72)          |
|  .--------------------.  |
|  |     [===Finish===] |  |
|  |   /              \ |  |
|  |  |    ▲ Player   | |  |
|  |  |    ● AI Rival | |  |
|  |   \              / |  |
|  |     '----------'   |  |
|  '--------------------'  |
+--------------------------+
```

---

## 6. Multiplayer Feasibility & Recommended Approach

### 6.1 Evaluation: Local Split-Screen vs. LAN Sockets

| Dimension | Option A: Local Split-Screen | Option B: LAN Sockets (TCP/UDP) |
| :--- | :--- | :--- |
| **Showcase Reliability** | **100% Deterministic** — Runs on one machine, immune to network drops, OS firewall popups, IP binding errors, or port conflicts. | **High Risk** — Requires two machines or two running instances, loopback socket binding, firewall permissions, and packet serialization. |
| **Visual Presentation** | **Spectacular** — Audience immediately sees both players racing head-to-head on the same screen, identical to classic *Super Mario Kart* (SNES). | **Divided** — Audience can only watch one screen at a time; requires switching between windows or laptops. |
| **Rendering Performance** | **Identical to Single-Player** — Rendering two viewports of 400×150 equals 60,000 pixels total, identical to 400×300. CPU cost is identical. | Extra overhead for socket polling, serialization/deserialization, and state interpolation. |
| **Input Handling** | Simple: P1 uses WASD + Space; P2 uses Arrow Keys + Enter. Handled in `pressedKeys`. | Requires client-side prediction or reconciliation to prevent jitter. |
| **Item Interaction** | Direct: When P1 fires a shell or uses Blooper, P2's viewport immediately reflects the impact. | Requires synchronization of item states and projectile positions over the network. |

### 6.2 Recommended Strategy: Local Split-Screen Multiplayer
Local split-screen is **decisively the best choice** for today's showcase deliverable. It eliminates all networking failure modes while maximizing the showcase impact.

#### Implementation Architecture:
1. **Game Modes**:
   ```cpp
   enum class GameMode {
       PRACTICE,       // 1 Player, no AI, free driving
       PLAY_VS_AI,     // 1 Player vs 3 AI opponents
       MULTIPLAYER     // 2 Players split-screen (P1 vs P2 + optional AI)
   };
   ```
2. **Dual Viewport Scanline Rasterization**:
   - `screenBuffer` remains 400×300.
   - **Top Viewport (Player 1)**: Rows `0 .. 148`.
     - Camera positioned at `(player1.x, player1.z, player1.angle)`.
     - Scanlines `75 .. 148` render Player 1's ground.
     - Sprites, Player 2's kart, and AI karts rendered from P1's perspective.
     - Cockpit hood and HUD rendered in top half.
   - **Divider Line**: Row 149 (1px gold border).
   - **Bottom Viewport (Player 2)**: Rows `150 .. 299`.
     - Camera positioned at `(player2.x, player2.z, player2.angle)`.
     - Scanlines `225 .. 299` render Player 2's ground.
     - Sprites, Player 1's kart, and AI karts rendered from P2's perspective.
     - Cockpit hood and HUD rendered in bottom half.
3. **Controls Multiplexing**:
   - **Player 1**: `Key_W` (Acc), `Key_S` (Brake), `Key_A` (Left), `Key_D` (Right), `Key_Space` (Item), `Key_Shift` (Drift).
   - **Player 2**: `Key_Up` (Acc), `Key_Down` (Brake), `Key_Left` (Left), `Key_Right` (Right), `Key_Return` (Item), `Key_Slash` / `Key_Control` (Drift).

---

## 7. Menu Flow & Character Selection Architecture

### 7.1 Flow Hierarchy
```
+-------------------------------------------------------+
| 1. MAIN TITLE MENU                                   |
|    [ 🏁 Play vs AI ]   [ 👥 2P Split-Screen ]  [ ⏱ Practice ] |
+-------------------------------------------------------+
                           |
                           v
+-------------------------------------------------------+
| 2. CHARACTER SELECTION SCREEN                         |
|    Select Character / Kart Stats:                    |
|    • Mario (Balanced)   • Peach (High Accel)          |
|    • Toad (High Speed)  • Bowser (Heavyweight/Grip)   |
+-------------------------------------------------------+
                           |
                           v
+-------------------------------------------------------+
| 3. TRACK SELECTION (Donut Plains / Choco / Bowser)    |
+-------------------------------------------------------+
                           |
                           v
+-------------------------------------------------------+
| 4. ACTIVE 3D RACE (GameCanvas)                        |
+-------------------------------------------------------+
```

### 7.2 Distinct Character Profiles & Stats
```cpp
struct CharacterDef {
    QString name;
    QString kartName;
    QColor kartColor;
    double maxSpeed;    // Base: 160.0 km/h (150.0 to 175.0)
    double accel;       // Base: 110.0 (90.0 to 135.0)
    double turnRate;    // Base: 2.4 (2.0 to 2.9)
    double weight;      // Collision mass
};
```
- **Mario (Red Fire)**: Balanced (`maxSpeed = 160, accel = 110, turnRate = 2.4`)
- **Peach (Pink Heart)**: High Acceleration & Sharp Handling (`maxSpeed = 152, accel = 135, turnRate = 2.8`)
- **Toad (Blue Star)**: Lightweight Speedster (`maxSpeed = 170, accel = 100, turnRate = 2.6`)
- **Bowser (Koopa Crusher)**: Heavyweight Brute (`maxSpeed = 175, accel = 85, turnRate = 2.0`, immune to minor bumps)

---

## 8. Summary of Actionable Tasks for Implementers

1. **AI System**:
   - Store waypoint loops for all 3 tracks.
   - Implement `OpponentKart` struct, update loop (`updateAI`), and billboard projection.
   - Populate 1–3 AI karts depending on track and mode.
2. **Item System**:
   - Fix `INK_BLOOPER` so it targets opponents and Player 2 instead of the user.
   - Implement `ShellProjectile` with forward velocity, bounce physics, and kart collision.
   - Add owner tracking and immunity timers to dropped bananas.
3. **Track Hazards**:
   - Add animated lava geysers to Bowser's Castle.
   - Add chocolate fountains and rolling candy obstacles to Choco Valley.
4. **HUD & Minimap**:
   - Implement top-left 72×72 minimap radar with track outline thumbnail and real-time kart position markers.
5. **Menu & Multiplayer**:
   - Add Title / Main Menu flow and Character Selection.
   - Add 2-Player local split-screen mode with independent viewports and controls.
