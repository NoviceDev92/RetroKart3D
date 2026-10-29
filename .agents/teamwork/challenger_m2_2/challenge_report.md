# Empirical Challenge Report: Milestone 2 (AI Opponents & Item Targeting Fixes)

**Challenger**: `challenger_m2_2`  
**Date**: 2026-10-29  
**Target Milestone**: Milestone 2 (F3, F4, F5, F6, F7, F8)  
**Verdict**: **REQUEST_CHANGES**

---

## Challenge Summary

**Overall risk assessment**: **CRITICAL**

While Milestone 2 introduces successful implementations of 3D billboard projection, Painter's algorithm depth sorting, forward-firing bouncing Green Shells, clean-windshield Ink Bloopers, and 1.2s Banana owner immunity, **the core AI opponent navigation is fatally broken in runtime gameplay**:
A critical defect in `game_canvas.cpp:1167` causes all AI opponent karts to mistake standard White tarmac raceways (`RGB(255, 255, 255)`) for Molten Lava. Consequently, every AI kart that touches the road suffers immediate 0.8s spinout, 8-unit backward recoil, and 65% speed reduction on every physics step, completely paralyzing the entire AI fleet across all three courses (Donut Plains, Choco Valley, Bowser Castle).

---

## Confirmed Challenges

### [Critical] Challenge 1: AI Terrain Collision Mask Check Flags White Road as Molten Lava (BUG-M2-01)

- **Assumption challenged**: That `updateAIOpponents` correctly samples `trackMaskMap` to differentiate between normal road, off-road grass/sand, and molten lava hazards.
- **Attack scenario**:
  Place an AI kart on the centerline of the raceway on Donut Plains, Choco Valley, or Bowser Castle (e.g., at coordinates $X = 550.0, Z = 200.0$ on Donut Plains). Execute 1 physics tick of `updateAIOpponents(0.016)`.
- **Root Cause Analysis**:
  In `game_canvas.cpp:1167`:
  ```cpp
  if (qRed(maskColor) == 255 && qBlue(maskColor) == 255) {
      // Molten lava recoil
      ai.speed *= 0.35;
      ai.x -= std::cos(ai.angle) * 8.0;
      ai.z -= std::sin(ai.angle) * 8.0;
      ai.isSpinning = true;
      ai.spinTimer = 0.8;
      continue;
  }
  ```
  On all 3 tracks, the asphalt road is rasterized onto `trackMaskMap` using `QColor(255, 255, 255)` (White).
  For White: `qRed(maskColor) == 255` is **true** and `qBlue(maskColor) == 255` is **true**.
  The code fails to check whether `qGreen(maskColor) != 255` (or check for road first, as Player 1 does at line 902).
- **Empirical Observation**:
  - Sampled mask color at $(550, 200)$: `RGB(255, 255, 255)` (White Road).
  - Prior to tick: `speed = 100.0, isSpinning = false, spinTimer = 0.0`.
  - After 1 tick: `speed = 35.0, isSpinning = true, spinTimer = 0.8, x = 542.0` (pushed back by 8 units).
  - In a 1,000-tick active race simulation, all AI karts advanced **0 waypoints**, trapped in endless spinout recoil.
- **Blast radius**:
  Total failure of Requirement R2 / Acceptance Criteria Gameplay & AI: "Starting a race in Play vs AI populates the track with active AI opponent karts that navigate the course." AI karts are completely paralyzed on the starting straight.
- **Mitigation**:
  Check for White Road first, or require `qGreen(maskColor) == 0` when detecting molten lava:
  ```cpp
  if (qRed(maskColor) == 255 && qGreen(maskColor) == 255 && qBlue(maskColor) == 255) {
      // White Road: normal grip, 100% max speed
      terrainSpeedMult = 1.0;
      terrainDragMult = 1.0;
  } else if (qRed(maskColor) == 255 && qBlue(maskColor) == 255 && qGreen(maskColor) == 0) {
      // Magenta Molten Lava Recoil
      ai.speed *= 0.35;
      ai.x -= std::cos(ai.angle) * 8.0;
      ai.z -= std::sin(ai.angle) * 8.0;
      ai.isSpinning = true;
      ai.spinTimer = 0.8;
      continue;
  }
  ```

---

### [Medium] Challenge 2: AI Spinout Early `continue;` Freezes `inkTimer` Debuff Countdown (BUG-M2-02)

- **Assumption challenged**: That item debuff timers decay monotonically and expire cleanly for AI opponents regardless of other status effects.
- **Attack scenario**:
  An AI opponent is hit by Ink Blooper (`isInked = true, inkTimer = 4.0`). While inked, the AI kart encounters an obstacle (shell or banana) and enters spinout (`isSpinning = true, spinTimer = 1.2`). Simulate 0.5s of physics updates.
- **Root Cause Analysis**:
  In `game_canvas.cpp:1064-1081`:
  ```cpp
  // 1. Spinout debuff decay and visual spin
  if (ai.spinTimer > 0.0 || ai.isSpinning) {
      ai.spinTimer -= dt;
      ...
      continue; // <--- Skips Section 2 below!
  }

  // 2. Ink debuff decay
  if (ai.inkTimer > 0.0) {
      ai.inkTimer -= dt;
      ...
  }
  ```
  Because `continue;` is invoked inside the spinout branch, Section 2 (`Ink debuff decay`) is never reached while `spinTimer > 0.0`.
- **Empirical Observation**:
  After 0.5s of simulation (10 ticks at $dt = 0.05$), `spinTimer` decayed from 1.2 to 0.7, but `inkTimer` remained frozen at `4.000000`. Expected `3.5s`.
- **Blast radius**:
  AI karts remain inked longer than the design specification (4.0s). If stuck in repeated collisions (or Bug 1 lava recoil), the ink debuff never expires.
- **Mitigation**:
  Process Section 2 (`Ink debuff decay`) before the spinout check or remove `continue;` and use conditional branching for throttle/steering.

---

### [Low] Challenge 3: Finish Line Detection Slice Risk of Tunneling (BUG-M2-03)

- **Assumption challenged**: That `ai.x >= 508.0 && ai.x <= 516.0` reliably registers lap completion at high speed.
- **Attack scenario**:
  Boosted AI kart moving at $v = 184$ units/s with frame hitch $dt = 0.05$s moves $\Delta x = 184 \times 0.05 = 9.2$ units in a single tick.
- **Root Cause Analysis**:
  The detection window $[508.0, 516.0]$ is only 8.0 units wide. If an AI kart is at $X = 507.5$ in frame $N$, it advances to $X = 516.7$ in frame $N+1$, completely bypassing the detection interval and missing the lap increment.
- **Mitigation**:
  Track previous position $X_{\text{prev}}$ and test whether the segment $[X_{\text{prev}}, X_{\text{curr}}]$ crosses the finish line $X = 512.0$.

---

## Empirical Stress Test Results

Executed via standalone test suite `tests/build_challenger_m2_2/release/test_m2_empirical_challenger.exe`:

| ID | Test Scenario | Expected Behavior | Actual Behavior | Result |
|---|---|---|---|:---:|
| T1.1 | Forward camera space projection | $camX = 0.0, camZ = 60.0$ | $camX = 0.0, camZ = 60.0$ | **PASS** |
| T1.2 | Frustum near plane ($\le 5$) & far plane ($> 480$) clipping | Billboards culled safely without crash | Culled safely, no crash | **PASS** |
| T1.3 | Painter's Algorithm depth-sorting | Bowser ($Z=45$) occludes Peach ($Z=150$) | Bowser pixels rendered on top | **PASS** |
| T1.4 | 60 FPS rendering performance benchmark | Frame time $< 16.6$ ms | $0.224$ - $0.600$ ms/frame ($> 1600$ FPS) | **PASS** |
| T1.5 | Buffer overrun stress (coords $\pm 10^7$) | Bounds clamped, zero buffer overrun | Clamped safely, no overrun | **PASS** |
| T2.1 | Green Shell launch spawning | Dynamically added to `activeProjectiles` | Added to list | **PASS** |
| T2.2 | Green Shell velocity calibration | Launch speed $= \max(180, v + 75)$ | Speed $= 180.0 \pm 0.1$ | **PASS** |
| T2.3 | Green Shell wall bounce reflection | $v_x$ inverted, `bouncesLeft` decremented | $v_x < 0$, bounces left $= 2$ | **PASS** |
| T2.4 | Green Shell 3-bounce shattering | Pruned from `activeProjectiles` | Pruned cleanly | **PASS** |
| T2.5 | Green Shell target collision | Consumed on impact with AI | Consumed immediately | **PASS** |
| T2.6 | Green Shell spinout effect on AI | Deals 1.2s spinout and cuts speed by 65% | $isSpinning = true, spinTimer > 0.5$ | **PASS** |
| T2.7 | Banana placement geometry | Spawns 16 units behind kart | Spawns at $X - 16\cos(\theta)$ | **PASS** |
| T2.8 | Banana owner immunity initialization | `ownerId = 0`, `immunityTimer = 1.2s` | Initialized correctly | **PASS** |
| T2.9 | Banana owner reversing within 1.2s grace | No self-spinout during grace period | $isSpinning = false$ | **PASS** |
| T2.10 | Trailing AI rival hits banana | AI spins out immediately | $isSpinning = true$ | **PASS** |
| T2.11 | Banana consumed on collision | Banana peel deactivated | `active = false` | **PASS** |
| T2.12 | Ink Blooper player windshield isolation | Player windshield 100% clean | `inkTimer = 0.0, isInked = false` | **PASS** |
| T2.13 | Ink Blooper opponent targeting | Blinds all active AI opponents | All AI receive 4.0s ink debuff | **PASS** |
| T3.1 | AI on White Tarmac (`RGB 255,255,255`) | Drives normally at full speed | **Triggers molten lava recoil (0.8s spinout, -8 unit recoil)** | **FAIL** |
| T3.2 | AI inkTimer decay while spinning | Timer decays from 4.0 to 3.5s | **Timer frozen at 4.0s due to line 1080 `continue;`** | **FAIL** |
| T3.3 | AI grid starting positions on Donut Plains | Karts spawn on road | Spawned on road mask ($X \in [416, 452]$) | **PASS** |
| T3.4 | Active 1000-tick AI waypoint navigation | AI advances along course | **AI advances 0 waypoints (paralyzed on tarmac)** | **FAIL** |

---

## Unchallenged Areas

- **Milestone 3 Track Hazards (Lava geysers, chocolate fountains)**: Out of Milestone 2 scope; planned for Milestone 3.
- **Milestone 4 Split-Screen Multi-Viewport**: Out of Milestone 2 scope; planned for Milestone 4.

---

## Recommendation

**REQUEST_CHANGES**: Worker must fix `game_canvas.cpp:1167` so that White tarmac (`RGB(255, 255, 255)`) is recognized as road rather than molten lava, and fix `game_canvas.cpp:1080` so that `inkTimer` continues counting down while karts are spinning.
