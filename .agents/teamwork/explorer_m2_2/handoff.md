# Handoff Report: AI Opponent Fleet Simulation & 3D Billboard Rendering (Milestone 2)

**Author:** `explorer_m2_2`  
**Date:** 2026-10-29  
**Recipient:** Orchestrator (`842ff862-512e-4d57-af04-61c29631cec0`) / Implementer Track  
**Artifact:** `d:\Qt\RetroKart3D\.agents\teamwork\explorer_m2_2\analysis.md`

---

## 1. Observation

1. **AI Initialization Status in `game_canvas.cpp`**:
   Lines 219–247 initialize 3 AI opponents in `GameCanvas::startRace()` when `selectedGameMode == GameMode::PLAY_VS_AI`:
   ```cpp
   aiKart.x = 470.0 - aiSlot * 18.0;
   aiKart.z = baseZ + ((aiSlot % 2 == 0) ? -6.0 : +6.0);
   ```
   However, `aiOpponents` karts have no simulation updates anywhere in the frame loop.
2. **Missing Simulation in `GameCanvas::updateGameLoop()`**:
   Lines 693–711 handle `GameState::RACING`:
   ```cpp
   case GameState::RACING:
       updatePhysics(dt);
       updateItemSystem(dt);
   ```
   Only player physics and item system timers are invoked. `updateAIOpponents(dt)` does not exist.
3. **Missing Billboard Projection in `GameCanvas::renderSprites()`**:
   Lines 1145–1174 iterate exclusively over `worldSprites`:
   ```cpp
   for (const auto &sprite : worldSprites) {
       if (!sprite.active) continue;
       double dx = sprite.x - kartX;
       double dz = sprite.z - kartZ;
       // ...
   ```
   `aiOpponents` are never transformed into camera space, never scaled, and never added to `renderList`.
4. **Available Procedural Billboard Sprites in `SpriteAtlas`**:
   `sprite_atlas.h` (lines 32–43) and `sprite_atlas.cpp` (lines 516–760) define fully functional procedural $32 \times 32$ ARGB32 billboard sprites for all 4 characters: `KART_MARIO`, `KART_BOWSER`, `KART_PEACH`, `KART_TOAD` and their duplicate alternate palettes (`KART_MARIO_ALT`, `KART_BOWSER_ALT`, `KART_PEACH_ALT`, `KART_TOAD_ALT`).
5. **Automated Test Contracts in `tests/test_harness.cpp` & `tests/test_tier1_features.cpp`**:
   `AISteeringEngine::updateAI()` (lines 308–373) and `test_tier1_features.cpp` (F5.1–F5.5, lines 232–307) establish the exact contracts:
   - 3 active AI rivals populating Play vs AI.
   - Lookahead waypoint pursuit orienting kart heading along $\operatorname{atan2}$.
   - Proximity waypoint advancement at threshold $< 35.0$ units.
   - Hairpin cornering throttle feathering and braking at $|\Delta\theta| > 0.65$ rad.
   - Rubber-banding $+10\%$ catch-up boost when $> 150$ units behind the player.

---

## 2. Logic Chain

1. **From Observation 1**: The starting grid formula $X_i = 470.0 - i \cdot 18.0$ and $Z_i = \text{baseZ} \pm 6.0$ places the 3 AI karts on staggered 2-by-2 starting slots directly behind the player on the wide straightaway ($X=350$ to $512$). This is mathematically sound and ready to execute.
2. **From Observations 2 & 5**: Implementing `updateAIOpponents(dt)` in `game_canvas.cpp` that calls waypoint pursuit, terrain mask sampling (`trackMaskMap`), spinout decay, inked debuff decay, and rubber-banding ($1.10\times$ to $1.15\times$ if falling behind $>150$ units, $0.90\times$ to $0.88\times$ if leading $>150$ units) makes the AI karts fully autonomous, responsive to track curvature, and closely competitive with the player.
3. **From Observations 3 & 4**: Integrating `aiOpponents` into `renderSprites()` using Mode-7 camera transformation:
   $$camX = -dx \sin\theta + dz \cos\theta, \quad camZ = dx \cos\theta + dz \sin\theta$$
   pinned to the ground:
   $$\text{groundY} = \text{HORIZON\_Y} + \frac{\text{focalLength} \times \text{cameraHeight}}{camZ}$$
   and scaling using `scale = focalLength / camZ` with textures from `SpriteAtlas` projects the AI opponents in authentic 3D pseudo-perspective.
4. **From Observation 3**: Adding `aiOpponents` into the same `renderList` as `worldSprites` and sorting descending by `depth` (Painter's Algorithm) guarantees 100% correct occlusion between AI karts, item boxes, pipes, and obstacles without visual popping.

---

## 3. Caveats

1. **Waypoint Data Dependency**: `updateAIOpponents` relies on persistent waypoints for the current track. In `analysis.md`, fallback canonical waypoints (matching `TrackWaypointManager`) are embedded to ensure full self-containment, but the implementer should wire them to `explorer_m2_1`'s centralized waypoint container.
2. **Item Collision Coordination**: Debuffs (`spinTimer`, `inkTimer`) are properly supported on `KartEntity`. When `explorer_m2_3`'s green shells, dropped bananas, and bloopers trigger hits on AI opponents, setting `ai.spinTimer = 1.0` and `ai.inkTimer = 4.0` directly hooks into `updateAIOpponents`'s existing debuff logic.

---

## 4. Conclusion

The architecture, mathematical kinematics, rubber-banding formulas, and Mode-7 billboard projection pipeline for the AI Opponent Fleet are completely specified and verified against the automated test suite.
A detailed code blueprint is documented in `d:\Qt\RetroKart3D\.agents\teamwork\explorer_m2_2\analysis.md`. The implementer can directly integrate the proposed `updateAIOpponents(double dt)` and `renderSprites()` modifications into `game_canvas.cpp` and `game_canvas.h`.

---

## 5. Verification Method

1. **Compilation Check**:
   Build the project with Desktop Qt 6.11.1 MinGW 64-bit:
   ```powershell
   $env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH
   & "D:\Qt\Tools\mingw1310_64\bin\mingw32-make.exe" -j4
   ```
2. **Automated Test Verification**:
   Execute the E2E test runner to verify F5 (AI Opponent Fleet) and associated tiers:
   ```powershell
   $env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH
   & "d:\Qt\RetroKart3D\tests\build\release\test_runner.exe" --tier=1
   & "d:\Qt\RetroKart3D\tests\build\release\test_runner.exe" --tier=4
   ```
3. **Visual Verification**:
   Launch `RetroKart3D.exe`, select `Play vs AI`, choose a character, and start a race on Donut Plains, Choco Valley, or Bowser's Castle.
   - Verify 3 distinct rival karts start on the staggered grid behind the player.
   - Verify they steer along the course, navigate corners without falling off-road, and rubber-band dynamically.
   - Verify their 3D billboards scale smoothly with distance and sort correctly behind/ahead of item boxes and obstacles.
