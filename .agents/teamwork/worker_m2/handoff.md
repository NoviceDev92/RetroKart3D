# Milestone 2 Handoff Report: AI Opponents & Item Targeting Fixes

**Author**: `worker_m2` (Implementation Worker)  
**Date**: 2026-10-29  
**Target Milestone**: Milestone 2 (F3, F4, F5, F6, F7, F8)  
**Status**: **COMPLETE (Hard Handoff — 100% Pass Rate, 0 Errors, 0 Warnings)**

---

## 1. Observation

1. **Initial Codebase Deficiencies**:
   - In `game_canvas.h`, `KartEntity` lacked `totalRaceDistance`, `isAI`, and `currentWaypointIndex`.
   - In `game_canvas.cpp:219-247`, `startRace()` spawned AI karts into `aiOpponents`, but `updateGameLoop()` had no `updateAIOpponents(dt)` call; spawned AI karts remained completely stationary on the starting grid.
   - In `game_canvas.cpp:1125-1175` (`renderSprites()`), only `worldSprites` were projected into `renderList`. `aiOpponents` were never transformed into camera space, never projected, and never rendered.
   - In `game_canvas.cpp:1008`, `useHeldItem()` for `INK_BLOOPER` executed `inkTimer = 4.5;`, which set the ink blinding effect on Player 1's own windshield instead of targeting opponents.
   - In `game_canvas.cpp:991`, `useHeldItem()` for `BANANA` dropped a banana peel 24 units behind without owner tracking or immunity, causing immediate self-spinout if the player reversed or turned tightly. AI opponents never checked distance to bananas.
   - In `game_canvas.cpp:1004`, `GREEN_SHELL` was spawned as a static `WorldSprite` with no velocity or collision logic.
   - Track paths in `generateDonutPlains()`, `generateChocoValley()`, and `generateBowserCastle()` used local `QPainterPath` / `QPolygon` variables that were discarded after track rendering, with no persistent waypoint arrays stored in `GameCanvas`.

2. **Executed Commands & Verbatim Outputs**:
   - Release Build:
     ```powershell
     $env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH; mingw32-make -f Makefile.Release
     ```
     Output:
     ```text
     g++ -Wl,-s -Wl,-subsystem,windows -mthreads -o release\RetroKart3D.exe release/main.o release/mainwindow.o release/game_canvas.o release/sprite_atlas.o release/moc_mainwindow.o release/moc_game_canvas.o D:\Qt\6.11.1\mingw_64\lib\libQt6Widgets.a D:\Qt\6.11.1\mingw_64\lib\libQt6Gui.a D:\Qt\6.11.1\mingw_64\lib\libQt6Core.a -lmingw32 D:\Qt\6.11.1\mingw_64\lib\libQt6EntryPoint.a -lshell32
     Command exited with code 0.
     ```
   - Automated E2E Test Runner:
     ```powershell
     $env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH; & "d:\Qt\RetroKart3D\tests\build\release\test_runner.exe"
     ```
     Output:
     ```text
     =======================================================
                RETROKART 3D E2E TEST SUMMARY               
     =======================================================
      Total Test Cases:    182
      Passed Test Cases:   182 (100%)
      Failed Test Cases:     0
     -------------------------------------------------------
      Total Assertions:   1399
      Passed Assertions:  1399
      Failed Assertions:     0
     =======================================================
      >>> ALL RETROKART 3D TEST SUITES PASSED CLEANLY! <<<
     =======================================================
     Command exited with code 0.
     ```

---

## 2. Logic Chain

1. **Persistent Track Waypoint Geometry (F4)**:
   - Supported by Observation 1: Procedural track generation uses Bézier curves and polygon perimeters. We added persistent storage `std::vector<QPointF> trackWaypoints[3]` and `currentTrackWaypoints` to `game_canvas.h`.
   - Populated waypoints during `initTrackWaypoints()` with exact tarmac coordinates:
     - Donut Plains: 16 waypoints circling the oval raceway from `(512.0, 200.0)` clockwise.
     - Choco Valley: 20 waypoints tracing the 4 cubic Bézier S-curves from `(512.0, 200.0)` clockwise.
     - Bowser Castle: 24 waypoints along the castle polygon raceway from `(512.0, 250.0)` clockwise.
   - Synchronized `currentTrackWaypoints` whenever `setTrackLevel()` or track generation is called.

2. **Active AI Opponent Fleet Simulation (F3, F5)**:
   - In `startRace()` when `selectedGameMode == GameMode::PLAY_VS_AI`: 3 AI opponent karts are dynamically instantiated from unpicked character roster entries, staggered along the starting grid (`X = 470 - 18 * slot`, `Z = baseZ +/- 6`), bound with their archetype physics stats (`maxSpeed`, `accel`, `turnRate`, `driftFactor`, `weight`, `primaryColor`), and initialized with `isAI = true`, `isHuman = false`.
   - In `updateGameLoop()`: Added `updateAIOpponents(dt)` call for active racing and race-finished states.
   - In `updateAIOpponents(double dt)`:
     - **Waypoint Advancement**: Distance to current target waypoint $d_C < 35.0$ advances waypoint index `(curr + 1) % N`.
     - **Pure Pursuit Lookahead Steering**: Lookahead waypoint index `(curr + 2) % N` calculates target angle $\operatorname{atan2}(\Delta z, \Delta x)$. Heading error $\Delta\theta$ is normalized to $[-\pi, \pi]$ and rate-limited by character `turnRate * dt`.
     - **Cornering Feathering & Braking**: When $|\Delta\theta| > 0.65$ rad, throttle is released and high-speed braking is engaged if $v > 80.0$.
     - **Terrain Mask Sampling**: Samples `trackMaskMap`. Molten lava (Magenta) triggers recoil and spinout; off-road grass/sand (Green) clamps top speed to 45% (70% on Bowser Castle shoulder) and increases drag.
     - **Dynamic Rubber-Banding**: Tracks $\Delta D = D_{\text{player}} - D_{\text{AI}}$. When AI is $> 150$ units behind, grants $+10\%$ to $+15\%$ top speed boost. When AI is $> 150$ units ahead, moderates speed by $-10\%$ to $-12\%$.
     - **Debuffs**: Spinout decay with visual rotation and rapid speed drop; ink timer decay with 35% speed penalty and subtle visual wander.
     - **Banana Collision**: AI checks distance to `worldSprites` bananas; on contact, banana is consumed and AI enters spinout.

3. **3D Billboard Rendering (Painter's Algorithm)**:
   - In `renderSprites()`: Projected each active AI opponent using camera transform `camX = -dx * sinA + dz * cosA; camZ = dx * cosA + dz * sinA;`.
   - Clipped at `camZ > 5.0` and `camZ <= 480.0`. Screen ground contact row computed via $groundY = \text{HORIZON\_Y} + (\text{focalLength} \times \text{cameraHeight}) / camZ$.
   - Assigned character billboard sprites (`KART_MARIO`, `KART_BOWSER`, `KART_PEACH`, `KART_TOAD` and their alts) using `getKartSpriteType(characterId)`.
   - Rendered projected active green shells (`SpriteType::SHELL_PROJECTILE`).
   - Sorted entire `renderList` descending by `depth` so distant objects are drawn first, followed by soft contact shadows and near billboards.

4. **Item Targeting Fixes (F6, F7, F8)**:
   - **Green Shell Dynamic Projectile (F6)**: Launching Green Shell spawns `Projectile` into `activeProjectiles` with forward velocity $v_{\text{launch}} = \max(180, v_{\text{kart}} + 75.0)$ along kart heading. Integrates velocity in `updateItemSystem()`, reflects off outer boundaries $[20.0, 1004.0]$ with up to 3 bounces, and detects collisions within radius $16.0$ against AI opponents (triggering 1.2s spinout and speed cut).
   - **Ink Blooper Fix (F7)**: Set player `inkTimer = 0.0` and `player1.isInked = false` (player windshield remains 100% clean). Simultaneously applies `isInked = true; inkTimer = 4.0; speed *= 0.65;` to all active AI rivals and Player 2 in split-screen mode.
   - **Banana Placement & Owner Immunity (F8)**: Added `ownerId` and `immunityTimer = 1.2` to `WorldSprite`. Dropping a banana peel spawns it 16 units behind with `ownerId = 0` and `immunityTimer = 1.2`. In `updatePhysics()`, collision is bypassed while owner immunity timer is active, preventing self-spinout. After 1.2s, trap becomes fully active for all racers.

---

## 3. Caveats

- Milestone 3 track hazard features (lava geysers on Bowser Castle, chocolate fountains/candies on Choco Valley, minimap HUD) and Milestone 4 split-screen multiplayer dual viewports are planned for subsequent milestones per `PROJECT.md` roadmap. All Milestone 2 contracts and data fields (`TrackHazard`, `HazardType`, `ViewportSplit`) have been preserved with zero breaking changes.
- No caveats regarding Milestone 2 scope.

---

## 4. Conclusion

All six Milestone 2 features (F3, F4, F5, F6, F7, F8) are fully implemented, verified, and integrated into `RetroKart3D`:
1. Persistent track waypoints for all 3 courses provide exact closed-loop navigation on tarmac.
2. Active AI opponent fleet of 3 rival karts populates `PLAY_VS_AI` mode with pure pursuit steering, cornering modulation, rubber-banding, and terrain sampling.
3. 3D Mode-7 billboard projection renders AI karts and ballistic shells with depth sorting and ground shadows.
4. Green Shells launch forward, bounce off boundaries, and strike opponents.
5. Ink Blooper leaves player windshield 100% clean and blinds opponents.
6. Bananas safely drop behind with 1.2s owner immunity and trigger spinouts on colliding opponents.

---

## 5. Verification Method

To independently verify the implementation:

1. **Clean Compilation**:
   ```powershell
   $env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH
   mingw32-make -f Makefile.Release
   ```
   *Expected result*: Clean build, 0 errors, 0 warnings, generating `release\RetroKart3D.exe`.

2. **Automated E2E Test Suite (All 4 Tiers)**:
   ```powershell
   $env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH
   & "d:\Qt\RetroKart3D\tests\build\release\test_runner.exe"
   ```
   *Expected result*: 182 / 182 tests pass (100% pass rate, 1399 assertions passed).

3. **Inspect Modified Files**:
   - `game_canvas.h`
   - `game_canvas.cpp`
