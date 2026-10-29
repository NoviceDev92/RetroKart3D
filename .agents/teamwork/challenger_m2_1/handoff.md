# Milestone 2 Empirical Challenger Handoff Report

**Author**: `challenger_m2_1` (Empirical Challenger)  
**Date**: 2026-10-29  
**Target Milestone**: Milestone 2 (AI Opponents & Item Targeting Fixes)  
**Empirical Verdict**: **REQUEST_CHANGES**

---

## 1. Observation

1. **Standalone Test Execution & Output**:
   Command:
   ```powershell
   $env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH
   Set-Location "d:\Qt\RetroKart3D\tests\build_stress_m2"
   mingw32-make -f Makefile.Release
   & "release\standalone_m2_stress.exe"
   ```
   Verbatim Output:
   ```text
   ===============================================================
    RETROKART 3D: EMPIRICAL CHALLENGER STRESS HARNESS (MILESTONE 2)
    Verification of AI Waypoints, Shell Physics, Blooper & Banana
   ===============================================================

   ===============================================================
    SUITE 1: Long-Distance AI Waypoint Navigation Across 3 Tracks
   ===============================================================

     --- Stress-Testing Donut Plains ---
     [PASS] Donut Plains: Persistent waypoints count matches expected (16)
     [PASS] Donut Plains: 3 active AI rivals spawned on starting grid
     [PASS] Donut Plains: AI coordinates strictly finite across all ticks (no NaN / Inf)
       [Donut Plains Results] Ticks Run=5000 | AI0 Advances=0 | AI1 Advances=0 | AI2 Advances=0
       [Root Cause Diagnostic] AI0 stuck at (435.366, 203.161) spd=0 isSpinning=1 spinTimer=0.02 wp=0 maskColor=RGB(255,255,255)
     [FAIL] Donut Plains: All 3 AI karts completed 100+ waypoint advances without stalling -> AI stalled: AI0=0, AI1=0, AI2=0 advances

     --- Stress-Testing Choco Valley ---
     [PASS] Choco Valley: Persistent waypoints count matches expected (20)
     [PASS] Choco Valley: 3 active AI rivals spawned on starting grid
     [PASS] Choco Valley: AI coordinates strictly finite across all ticks (no NaN / Inf)
       [Choco Valley Results] Ticks Run=5000 | AI0 Advances=0 | AI1 Advances=0 | AI2 Advances=0
       [Root Cause Diagnostic] AI0 stuck at (435.377, 203.096) spd=0 isSpinning=1 spinTimer=0.02 wp=0 maskColor=RGB(255,255,255)
     [FAIL] Choco Valley: All 3 AI karts completed 100+ waypoint advances without stalling -> AI stalled: AI0=0, AI1=0, AI2=0 advances

     --- Stress-Testing Bowser Castle ---
     [PASS] Bowser Castle: Persistent waypoints count matches expected (24)
     [PASS] Bowser Castle: 3 active AI rivals spawned on starting grid
     [PASS] Bowser Castle: AI coordinates strictly finite across all ticks (no NaN / Inf)
       [Bowser Castle Results] Ticks Run=6000 | AI0 Advances=0 | AI1 Advances=0 | AI2 Advances=0
       [Root Cause Diagnostic] AI0 stuck at (427.806, 256.408) spd=0 isSpinning=1 spinTimer=0.02 wp=0 maskColor=RGB(255,255,255)
     [FAIL] Bowser Castle: All 3 AI karts completed 100+ waypoint advances without stalling -> AI stalled: AI0=0, AI1=0, AI2=0 advances

   ===============================================================
    SUITE 2: Shell Projectile Physics & Collision Stress Test
   ===============================================================
     [PASS] T2.1: Green Shell fires forward at calibrated speed across 8 radial angles
     [PASS] T2.2: Left boundary reflection inverts vx to positive and decrements bouncesLeft
     [PASS] T2.3: Right boundary reflection inverts vx to negative
     [PASS] T2.4: Top boundary reflection inverts vz to positive
     [PASS] T2.5: Bottom boundary reflection inverts vz to negative
     [PASS] T2.6: Shell shatters and deactivates after exhausting 3 bounces
     [PASS] T2.7: Shell deactivates upon reaching lifeTime <= 0
     [PASS] T2.8: Shell consumed upon colliding with AI opponent
     [PASS] T2.9: AI opponent enters 1.2s spinout on shell collision
     [PASS] T2.10: AI opponent speed reduced to 35% on shell collision

   ===============================================================
    SUITE 3: Ink Blooper Effect & Screen Overlay Isolation
   ===============================================================
     [PASS] T3.1: Player 1 inkTimer strictly 0.0s after using Blooper
     [PASS] T3.2: Player 1 entity isInked=false and inkTimer=0.0s
     [PASS] T3.3: Exactly 0 ink splatter pixels rendered on Player 1 screenBuffer (100% clean windshield)
     [PASS] T3.4: All active AI opponents receive isInked=true and inkTimer=4.0s
     [PASS] T3.5: Player 1 windshield remains clean in Multiplayer
     [PASS] T3.6: Player 2 targeted by Blooper (isInked=true, inkTimer=4.0s, speed cut by 35%)

   ===============================================================
    SUITE 4: Banana Drop & Owner Immunity Window Stress Test
   ===============================================================
     [PASS] T4.1: Deploying Banana spawns new WorldSprite in worldSprites
     [PASS] T4.2: Banana dropped exactly 16 units behind kart heading
     [PASS] T4.3: Banana initialized with ownerId=0, immunityTimer=1.2s, active=true
     [PASS] T4.4: Owner braking/reversing onto banana within 1.2s DOES NOT trigger self-spinout
     [PASS] T4.5: Owner kart spins out and consumes banana when contacting AFTER 1.2s immunity expires
     [PASS] T4.6: Banana consumed upon collision with trailing AI opponent
     [PASS] T4.7: Trailing AI opponent enters 1.2s spinout upon running over banana
     [PASS] T4.8: Trailing AI speed reduced to 35% on banana impact

   ===============================================================
                    CHALLENGE SUITE SUMMARY                       
   ===============================================================
    Total Challenges Executed: 36
    Passed Challenges:        33 (91.7%)
    Failed Challenges:        3 (8.3%)
   ===============================================================
   ```

2. **Codebase Inspection**:
   - In `game_canvas.cpp:1167-1175`:
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
   - In `game_canvas.cpp:634`, `710`, `767`:
     Tracks draw asphalt tarmac mask using `QColor(255, 255, 255)`.
   - In `game_canvas.cpp:902-905` (Player 1 terrain check):
     ```cpp
     if (qRed(maskColor) == 255 && qGreen(maskColor) == 255 && qBlue(maskColor) == 255) {
         currentSurfaceName = (mushroomBoostTimer > 0.0) ? "Road (BOOST ACTIVE!)" : "Road (Tarmac)";
     } else if (qRed(maskColor) == 255 && qBlue(maskColor) == 255) {
         // Lava Hazard...
     ```

---

## 2. Logic Chain

1. **Road Color Invariant**:
   From Observation 2, procedural track generation on all courses paints the drivable asphalt raceway in `trackMaskMap` using `QColor(255, 255, 255)`. White pixels have channel values `qRed == 255`, `qGreen == 255`, and `qBlue == 255`.
2. **Hazard Check Flaw**:
   From Observation 2, Molten Lava hazard on Bowser Castle is painted in Magenta `QColor(255, 0, 255)` (`qRed == 255`, `qGreen == 0`, `qBlue == 255`). In `updateAIOpponents` (line 1167), the hazard check tests `if (qRed(maskColor) == 255 && qBlue(maskColor) == 255)`.
3. **White Matches Hazard Condition**:
   Because White (`255, 255, 255`) satisfies `qRed == 255 && qBlue == 255`, any AI kart standing or driving on the asphalt tarmac raceway triggers the Molten Lava recoil branch.
4. **Infinite Spinout Loop**:
   When triggered, line 1172-1174 applies `ai.isSpinning = true; ai.spinTimer = 0.8; ai.speed *= 0.35; continue;`. While spinning, waypoint pursuit and steering are completely skipped (`continue;`). When `spinTimer` expires after 0.8s, the kart re-samples the tarmac road at its current position, immediately re-triggering the lava recoil.
5. **Empirical Failure Reproduction**:
   Supported by Observation 1: across 5,000 to 6,000 physics ticks on Donut Plains, Choco Valley, and Bowser Castle, AI opponents remain trapped at `(435.366, 203.161)` on `RGB(255, 255, 255)` with `spd=0` and `isSpinning=1`, completing exactly `0` waypoint advances.

---

## 3. Caveats

- Green Shell forward ballistics, 4-wall boundary reflections, lifetime expiration, and AI collision spinout work flawlessly (Suite 2: 10/10 passed).
- Ink Blooper leaves player windshield 100% clean with 0 screen pixel changes and targets all opponents (Suite 3: 6/6 passed).
- Banana drop placement and owner immunity grace period (preventing self-spinout even under immediate brake/reverse within 1.2s) work as specified (Suite 4: 8/8 passed).
- The defect is strictly localized to `game_canvas.cpp:1167` in `updateAIOpponents`.

---

## 4. Conclusion

**Verdict**: **REQUEST_CHANGES**.

Milestone 2 cannot be approved because AI opponents cannot navigate any track and remain indefinitely stuck in a perpetual spinout loop on the tarmac starting grid.

**Actionable Fix for Worker**:
In `game_canvas.cpp:1167`, update the lava condition to require `qGreen(maskColor) == 0`:
```cpp
if (qRed(maskColor) == 255 && qGreen(maskColor) == 0 && qBlue(maskColor) == 255) {
    // Molten lava recoil (Magenta only)
    ai.speed *= 0.35;
    ai.x -= std::cos(ai.angle) * 8.0;
    ai.z -= std::sin(ai.angle) * 8.0;
    ai.isSpinning = true;
    ai.spinTimer = 0.8;
    continue;
} else if (qGreen(maskColor) == 255 && qRed(maskColor) == 0) {
```
Also, in `updateAIOpponents`, move `ai.inkTimer -= dt;` before `if (ai.spinTimer > 0.0 || ai.isSpinning)` so that ink debuffs decay continuously even during a spinout.

---

## 5. Verification Method

To independently reproduce the bug and verify the fix:

```powershell
$env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH
Set-Location "d:\Qt\RetroKart3D\tests\build_stress_m2"
mingw32-make -f Makefile.Release
& "release\standalone_m2_stress.exe"
```

**Invalidation / Success Condition**:
All 36 tests pass (100% pass rate), with:
- Donut Plains AI advancing >= 100 waypoints.
- Choco Valley AI advancing >= 100 waypoints.
- Bowser Castle AI advancing >= 100 waypoints.
