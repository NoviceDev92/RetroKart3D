# Milestone 2 Review & Adversarial Challenge Handoff Report

**Reviewer**: `reviewer_m2_1`  
**Roles**: reviewer, critic  
**Target**: Milestone 2 — AI Opponents & Item Targeting Fixes (F3, F4, F5, F6, F7, F8)  
**Date**: 2026-10-29  
**Verdict**: **APPROVE**  

---

## 1. Observation

1. **Source Code Audited**:
   - `d:\Qt\RetroKart3D\game_canvas.h`:
     - Multi-kart entity contract `KartEntity` (lines 65-107) with `isAI`, `isHuman`, `currentWaypoint`, `currentWaypointIndex`, `lookaheadDist`, `totalRaceDistance`, `spinTimer`, `inkTimer`, `bananaGraceTimer`.
     - Projectile contract `Projectile` / `ShellProjectile` (lines 109-120) with velocity `vx, vz`, `bouncesLeft`, `lifeTime`, `ownerId`.
     - Waypoint collection storage `std::vector<QPointF> trackWaypoints[3]` (lines 288-295) and `currentTrackWaypoints`.
     - Sprite atlas mappings and function signatures `updateAIOpponents(double dt)`, `getKartSpriteType(int, bool)`.
   - `d:\Qt\RetroKart3D\game_canvas.cpp`:
     - Dynamic 3-rival AI fleet initialization in `startRace()` (lines 226-267) staggering rival racers across the starting grid.
     - Closed-loop tarmac waypoint definitions in `extractDonutPlainsWaypoints()` (16 waypoints, lines 497-518), `extractChocoValleyWaypoints()` (20 waypoints, lines 520-545), and `extractBowserCastleWaypoints()` (24 waypoints, lines 547-576).
     - Full AI navigation simulation in `updateAIOpponents(double dt)` (lines 1045-1233): waypoint proximity advancement ($d < 35.0$), pure pursuit lookahead steering toward waypoint $(curr + 2) \% N$, angular error normalization within $[-\pi, \pi]$, cornering braking when $|\Delta\theta| > 0.65$, dynamic rubber-banding ($\pm 10\%$ to $\pm 15\%$), and mask-based terrain speed penalties.
     - Dynamic Green Shell projectile physics in `updateItemSystem(double dt)` (lines 1258-1330) with forward launch speed $\max(180, v_{\text{kart}} + 75)$, boundary bounces with velocity reversal, 3-bounce shatter limit, and opponent collision detection ($R \le 16.0$) triggering 1.2s spinout.
     - Ink Blooper fix in `useHeldItem()` (lines 1418-1433) setting player `inkTimer = 0.0` and `player1.isInked = false` (player windshield 100% clean) while applying ink debuffs and 35% speed penalties to all active AI opponents and Player 2.
     - Banana placement in `useHeldItem()` (lines 1390-1397) dropping peel 16 units behind with `ownerId = 0` and `immunityTimer = 1.2`. Player collision is bypassed during the grace period (lines 1024-1026) while trailing AI opponents immediately spin out on contact (lines 1099-1110).
     - 3D Billboard projection in `renderSprites()` (lines 1599-1658) mapping AI opponents and ballistic shells into camera space, sorting back-to-front by depth (Painter's algorithm), drawing ground contact shadows, and texture-mapping billboard sprites with alpha blending.
   - `d:\Qt\RetroKart3D\sprite_atlas.h` & `sprite_atlas.cpp`:
     - Added procedural billboard sprite generation for `KART_MARIO`, `KART_BOWSER`, `KART_PEACH`, `KART_TOAD` and their multiplayer alt palettes (lines 515-765 in `sprite_atlas.cpp`).

2. **Integrity & Anti-Cheat Audit**:
   - Zero hardcoded test names, fake outputs, or dummy facades detected in source code.
   - No mock bypasses in `game_canvas.cpp`.
   - Physics, AI pure-pursuit steering, projectile ballistic bounces, and Mode-7 billboard projections are genuine mathematical implementations.

3. **Compilation & Test Suite Execution**:
   - Clean Release Build:
     ```powershell
     $env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH; mingw32-make -f Makefile.Release
     ```
     Result: Clean compilation with 0 warnings, 0 errors, linking `release\RetroKart3D.exe`.
   - E2E Automated Test Runner (All 4 Tiers):
     ```powershell
     $env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH; & "d:\Qt\RetroKart3D\tests\build\release\test_runner.exe"
     ```
     Result:
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
     ```

---

## 2. Logic Chain

1. **Requirement R2 Fulfillment**:
   - **F3 & F5 (AI Opponents)**: `startRace()` instantiates 3 active AI opponents with individual character stats. `updateAIOpponents()` executes pure pursuit lookahead steering, proximity waypoint cycling, sharp-corner braking, and dynamic rubber-banding.
   - **F4 (Persistent Waypoints)**: Closed-loop waypoints for Donut Plains (16), Choco Valley (20), and Bowser Castle (24) are stored in persistent memory and properly synchronized when track levels change.
   - **F6 (Green Shell)**: Launches dynamically forward along the kart's heading with ballistic velocity, bounces up to 3 times off course borders, and triggers spinout and speed penalties upon striking opponents.
   - **F7 (Ink Blooper Fix)**: Zero self-inflicted ink on player (`inkTimer = 0.0`), leaving player's windshield completely clean, while successfully debuffing all AI rivals (and P2 in multiplayer) with speed reduction and steering wander.
   - **F8 (Banana Safety)**: Spawns behind with 1.2s owner immunity, preventing self-spinout during tight cornering or reversing, while inflicting spinouts on colliding opponents.
   - **3D Billboard Rendering**: Camera transformation, near-plane culling ($camZ > 5.0$), Painter's depth sorting, contact shadows, and clipping are properly integrated into the scanline Mode-7 pipeline.

2. **Adversarial Stress Testing & Boundary Analysis**:
   - *Zero-distance / Co-located coordinates*: Division by zero in pure-pursuit steering or Mode-7 sprite projection is guarded by `camZ > 5.0` and `effectiveMaxSpeed > 0.0` checks.
   - *Frame hitch / Lag spikes*: Frame delta time is clamped to $\le 0.05$s in both `updateGameLoop()` and `updateAIOpponents()`, preventing numerical instability or collision tunneling.
   - *Coordinate overflow*: Modulo boundary wrapping (`while (ai.x < 0) ...`) safely cycles world coordinates without overflow.
   - *Angular accumulation*: Steer and heading angles are wrapped to $[0, 2\pi)$ and $[-\pi, \pi]$ respectively.
   - *Projectile cleanup*: Projectiles that exhaust their lifetime or bounce limit are cleanly erased via `std::remove_if`, preventing memory leaks.

---

## 3. Caveats

- Milestone 3 track hazards (lava geysers on Bowser Castle, chocolate fountains/candies on Choco Valley, minimap HUD) and Milestone 4 split-screen multiplayer viewports are slated for subsequent milestones. All interface contracts and stubbed data structures have been preserved with zero regressions.
- No caveats regarding Milestone 2 scope.

---

## 4. Conclusion

The Milestone 2 implementation by `worker_m2` is correct, robust, performant, and conforms strictly to all requirements of R2 and `PROJECT.md`. There are zero integrity violations, zero compiler warnings, and 100% pass rate across the comprehensive 182-test E2E suite.

**Verdict**: **APPROVE**

---

## 5. Verification Method

1. **Build Check**:
   ```powershell
   $env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH
   mingw32-make -f Makefile.Release
   ```
   *Expected*: Exits with code 0, 0 compiler warnings/errors, generates `release\RetroKart3D.exe`.

2. **Automated E2E Test Suite**:
   ```powershell
   $env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH
   & "d:\Qt\RetroKart3D\tests\build\release\test_runner.exe"
   ```
   *Expected*: 182 / 182 tests pass, 1399 assertions pass, 0 failures.
