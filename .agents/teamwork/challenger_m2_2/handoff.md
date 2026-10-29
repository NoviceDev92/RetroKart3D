# Milestone 2 Handoff Report: Empirical Challenger Verdict

**Author**: `challenger_m2_2` (Empirical Challenger)  
**Date**: 2026-10-29  
**Target Milestone**: Milestone 2 (F3, F4, F5, F6, F7, F8)  
**Verdict**: **REQUEST_CHANGES**  
**Status**: Complete (Empirically verified failure modes with reproducible test suite)

---

## 1. Observation

1. **Compilation and Existing E2E Runner**:
   - Clean compilation of main game executable:
     ```powershell
     $env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH; mingw32-make -f Makefile.Release
     ```
     Result: Exit code 0, clean build.
   - Project test runner execution:
     ```powershell
     $env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH; & "d:\Qt\RetroKart3D\tests\build\release\test_runner.exe"
     ```
     Result: 182 / 182 tests passed. Note: In `tests/test_tier1_features.cpp`, AI tests invoked mock `AISteeringEngine` rather than `GameCanvas::updateAIOpponents` against real `trackMaskMap` bitmaps.

2. **Empirical Challenger Test Suite Execution**:
   - Built and executed dedicated stress suite:
     ```powershell
     $env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH
     & "d:\Qt\RetroKart3D\tests\build_challenger_m2_2\release\test_m2_empirical_challenger.exe"
     ```
   - Verbatim Output:
     ```text
     ===============================================================
      RETROKART 3D: EMPIRICAL CHALLENGER TEST SUITE (MILESTONE 2)
      Independent Verification of 3D Billboards, Sorting & AI Dynamics
     ===============================================================

     ===============================================================
      SUITE 1: 3D Billboard Projection & Painter's Algorithm Sorting
     ===============================================================
       [PASS] T1.1: Camera space coordinate transformation (forward camZ=60.0, lateral camX=0.0)
       [PASS] T1.2: Frustum near plane (<=5.0) and far plane (>480.0) clipping executes without crash
       [PASS] T1.3: Painter's Algorithm depth-sorting: near kart (Bowser) renders on top of distant kart (Peach)
         [Benchmark Data] 500 Mode-7 + Billboard frames in 300 ms (0.600 ms/frame -> 1666.7 FPS)
       [PASS] T1.4: 3D billboard projection + Mode-7 software rasterizer achieves sustained 60 FPS (<16.6ms/frame)
       [PASS] T1.5: Extreme pathological coordinates (-1e7, +1e7, camZ ~ 0, behind cam) produce no buffer overruns

     ===============================================================
      SUITE 2: Adversarial Item Interactions & Projectile Physics
     ===============================================================
       [PASS] T2.1: Firing Green Shell spawns dynamic projectile in activeProjectiles
       [PASS] T2.2: Green Shell launch speed calibrated to max(180, kartSpeed + 75.0)
       [PASS] T2.3: Shell reflects vx negatively and decrements bouncesLeft from 3 to 2 on wall bounce
       [PASS] T2.4: Shell shatters and is pruned from activeProjectiles after 3 wall reflections
       [PASS] T2.5: Shell consumed upon hitting opponent kart
       [PASS] T2.6: Opponent kart enters 1.2s spinout on shell impact
       [PASS] T2.7: Deploying Banana spawns new WorldSprite 16 units behind kart
       [PASS] T2.8: Dropped banana initialized with ownerId=0 and immunityTimer=1.2s
       [PASS] T2.9: Owner kart is immune and does NOT spin out during 1.2s grace period
       [PASS] T2.10: Trailing AI rival spins out immediately upon hitting banana
       [PASS] T2.11: Banana peel is consumed (active=false) upon collision
       [PASS] T2.12: Player 1 windshield remains 100% clean when using Ink Blooper
       [PASS] T2.13: Ink Blooper targets ALL AI opponents (isInked=true, inkTimer=4.0, speed reduced by 35%)

     ===============================================================
      SUITE 3: Empirical Root Cause Challenges & AI Failure Modes
     ===============================================================
         [Road Mask Sample] Donut Plains at (550, 200): RGB(255, 255, 255)
         [AI State After Road Tick] isSpinning=1 spinTimer=0.8 speed=35.0 x=542.0 z=199.6
       [FAIL] T3.1: AI kart on tarmac (White road RGB 255,255,255) MUST NOT trigger molten lava recoil -> CRITICAL BUG CONFIRMED: Line 1167 checks `qRed==255 && qBlue==255` without checking green!=255, causing AI to treat tarmac as molten lava!
         [Ink While Spinout Data] After 0.5s: spinTimer=0.7, inkTimer=4.0 (expected 3.5s)
       [FAIL] T3.2: AI inkTimer debuff MUST decay while kart is spinning -> DEFECT CONFIRMED: Line 1080 `continue;` skips inkTimer countdown during spinout, freezing ink timer at 4.000000
         [Grid Positions on Donut Plains]:
           AI 0: x=452.0 z=206.0 -> Mask RGB(255,255,255) [ROAD]
           AI 1: x=434.0 z=194.0 -> Mask RGB(255,255,255) [ROAD]
           AI 2: x=416.0 z=206.0 -> Mask RGB(255,255,255) [ROAD]
       [PASS] T3.3: AI opponents MUST spawn on tarmac raceway, NOT in off-road grass
         [1000 Ticks Lap Progress]: AI 0 advanced 0 waypoints
       [FAIL] T3.4: AI karts MUST advance along waypoints under active gameplay loop -> STALLING CONFIRMED: AI advanced 0 waypoints in 1000 ticks due to road-as-lava bug!

     ===============================================================
                      CHALLENGE SUITE SUMMARY                       
     ===============================================================
      Total Challenges Executed: 22
      Passed Challenges:        19 (86%)
      Failed Challenges:        3
     ===============================================================

     >>> VERDICT: REQUEST_CHANGES <<<
     Empirically Confirmed Failure Modes:
       * T3.1: AI kart on tarmac (White road RGB 255,255,255) MUST NOT trigger molten lava recoil -> FAIL: CRITICAL BUG CONFIRMED: Line 1167 checks `qRed==255 && qBlue==255` without checking green!=255, causing AI to treat tarmac as molten lava!
       * T3.2: AI inkTimer debuff MUST decay while kart is spinning -> FAIL: DEFECT CONFIRMED: Line 1080 `continue;` skips inkTimer countdown during spinout, freezing ink timer at 4.000000
       * T3.4: AI karts MUST advance along waypoints under active gameplay loop -> FAIL: STALLING CONFIRMED: AI advanced 0 waypoints in 1000 ticks due to road-as-lava bug!
     ```

3. **Code Inspection**:
   - `game_canvas.cpp:1167-1175`:
     ```cpp
     if (qRed(maskColor) == 255 && qBlue(maskColor) == 255) {
         // Molten lava recoil
         ai.speed *= 0.35;
         ai.x -= std::cos(ai.angle) * 8.0;
         ai.z -= std::sin(ai.angle) * 8.0;
         ai.isSpinning = true;
         ai.spinTimer = 0.8;
         continue;
     } else if (qGreen(maskColor) == 255 && qRed(maskColor) == 0) {
     ```
   - `game_canvas.cpp:902-905` (Contrast with player's handling):
     ```cpp
     if (qRed(maskColor) == 255 && qGreen(maskColor) == 255 && qBlue(maskColor) == 255) {
         // Road: normal speed
     } else if (qRed(maskColor) == 255 && qBlue(maskColor) == 255) {
         // Lava: molten lava
     }
     ```
   - `game_canvas.cpp:1064-1081`:
     ```cpp
     if (ai.spinTimer > 0.0 || ai.isSpinning) {
         ai.spinTimer -= dt;
         ...
         continue; // Bypasses Section 2 (ink debuff decay) below!
     }
     ```

---

## 2. Logic Chain

1. **Terrain Collision Logic (BUG-M2-01)**:
   - Supported by Observation 2 and Observation 3.
   - On all three tracks (`generateDonutPlains`, `generateChocoValley`, `generateBowserCastle`), the tarmac raceway is drawn onto `trackMaskMap` using `QColor(255, 255, 255)` (White).
   - In `updateAIOpponents` (line 1167), molten lava recoil is conditioned on:
     `if (qRed(maskColor) == 255 && qBlue(maskColor) == 255)`.
   - For White tarmac: `qRed` is 255 and `qBlue` is 255.
   - Because `updateAIOpponents` does not check for road first (unlike line 902 for Player 1) and does not verify `qGreen == 0`, every AI kart that touches the tarmac immediately enters the molten lava recoil block.
   - This cuts speed by 65% (`ai.speed *= 0.35`), pushes the kart backward by 8 units, forces `spinTimer = 0.8`, and halts forward execution via `continue;`.
   - In 1,000 game simulation ticks, the AI karts made 0 waypoint advances, completely stalling.
   - Therefore, Requirement R2 ("active AI opponents that navigate the course") fails empirically in active gameplay.

2. **Debuff Timer Freezing During Spinout (BUG-M2-02)**:
   - Supported by Observation 2 and Observation 3.
   - In `updateAIOpponents:1080`, when an AI kart is spinning (`ai.spinTimer > 0.0 || ai.isSpinning`), the loop executes `continue;`.
   - Section 2 (`Ink debuff decay`) is located at line 1084, after the spinout block.
   - When an AI kart is in spinout, Section 2 is never executed, and `ai.inkTimer` fails to count down.
   - In empirical testing, after 0.5s of simulation while spinning, `inkTimer` remained frozen at 4.0s instead of decaying to 3.5s.
   - Combined with Bug 1, because AI karts are perpetually in spinout on tarmac, ink debuffs never expire.

3. **What Passed & Verified Robust**:
   - 3D Billboard projection transform: Correct camera space mapping ($camX = -dx \sin A + dz \cos A, camZ = dx \cos A + dz \sin A$).
   - Frustum clipping at $camZ \le 5.0$ and $camZ > 480.0$ prevents division by zero and near-plane distortion.
   - Painter's Algorithm depth-sorting: Descending sort by depth ensures near billboards and soft contact shadows correctly overwrite distant objects.
   - 60 FPS performance: Measured frame time of $0.224$ - $0.600$ ms/frame ($> 1600$ FPS) across 500 Mode-7 + billboard cycles.
   - Projectile physics: Forward Green Shell launch velocity calibrated to $\max(180, v + 75)$, reflects off walls at $[20, 1004]$, decrements bounces, shatters on 3rd bounce, strikes opponents with 1.2s spinout.
   - Banana placement: Drops 16 units behind with 1.2s owner immunity window preventing self-spinout during reversing/drifting, while trailing AI rivals spin out immediately.
   - Ink Blooper targeting: Leaves Player 1 windshield 100% clean, correctly blinding AI opponents.

---

## 3. Caveats

- Milestone 3 hazards (animated lava geysers on Bowser Castle, chocolate fountains/candies on Choco Valley, minimap HUD) and Milestone 4 split-screen multiplayer viewports were not evaluated, as they are designated for subsequent milestones per `PROJECT.md`.
- No caveats regarding Milestone 2 scope.

---

## 4. Conclusion

**Verdict: REQUEST_CHANGES**

Milestone 2 cannot be approved in its current state due to a blocker bug that prevents AI opponents from racing:
1. **CRITICAL**: In `game_canvas.cpp:1167`, the molten lava check treats White tarmac (`RGB 255, 255, 255`) as molten lava. All AI opponents endlessly recoil and stall on the track.
2. **MEDIUM**: In `game_canvas.cpp:1080`, early `continue;` during spinouts freezes `ai.inkTimer` countdown.

Once `game_canvas.cpp:1167` checks for road first (or checks `qGreen == 0` for lava) and `game_canvas.cpp:1080` lets `inkTimer` decay, Milestone 2 will achieve full empirical compliance.

---

## 5. Verification Method

To independently reproduce and verify these findings:

1. **Run Challenger Stress Suite**:
   ```powershell
   $env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH
   cd "d:\Qt\RetroKart3D\tests\build_challenger_m2_2"
   & "d:\Qt\RetroKart3D\tests\build_challenger_m2_2\release\test_m2_empirical_challenger.exe"
   ```
   *Expected result*: Demonstrates failure on T3.1 (road-as-lava), T3.2 (frozen ink timer), and T3.4 (AI stalling at 0 waypoints), while passing all 19 other projection, sorting, and item tests.

2. **Inspect Production Code**:
   - `game_canvas.cpp:1167` vs `game_canvas.cpp:902`
   - `game_canvas.cpp:1080`
