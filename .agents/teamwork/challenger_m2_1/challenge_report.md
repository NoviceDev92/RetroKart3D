# Milestone 2 Empirical Challenge Report

**Date**: 2026-10-29  
**Reviewer**: `challenger_m2_1` (Empirical Challenger)  
**Target Milestone**: Milestone 2 (AI Opponents & Item Targeting Fixes)  
**Verdict**: **REQUEST_CHANGES** (33 Passed / 3 Failed across 36 stress challenges)

---

## Executive Summary

An independent, adversarial empirical stress test harness (`tests/standalone_m2_stress.cpp` built into `tests/build_stress_m2/release/standalone_m2_stress.exe`) was executed against the Milestone 2 deliverable in `game_canvas.h` and `game_canvas.cpp`.

While Green Shell projectile mechanics, Ink Blooper targeting isolation, and Banana owner immunity grace periods function cleanly and robustly, **a CRITICAL defect was discovered in AI terrain mask sampling (`game_canvas.cpp:1167`)**.

The mask check treats all White asphalt road pixels (`RGB(255, 255, 255)`) as Molten Lava (`RGB(255, 0, 255)`). Consequently, **all 3 AI opponents permanently recoil and spin out on the tarmac starting grid immediately upon race start, advancing ZERO waypoints across all 3 tracks**.

---

## Detailed Suite Results

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

---

## Root Cause Analysis: Bug #1 (Critical)

### 1. Defect Location
- **File**: `game_canvas.cpp`
- **Lines**: 1167-1175

```cpp
// game_canvas.cpp:1159-1175
// 7. Terrain slowdown sampling trackMaskMap
int sampleU = (static_cast<int>(ai.x) % TRACK_MAP_SIZE + TRACK_MAP_SIZE) % TRACK_MAP_SIZE;
int sampleV = (static_cast<int>(ai.z) % TRACK_MAP_SIZE + TRACK_MAP_SIZE) % TRACK_MAP_SIZE;
QRgb maskColor = trackMaskMap.pixel(sampleU, sampleV);

double terrainSpeedMult = 1.0;
double terrainDragMult = 1.0;

if (qRed(maskColor) == 255 && qBlue(maskColor) == 255) {
    // Molten lava recoil
    ai.speed *= 0.35;
    ai.x -= std::cos(ai.angle) * 8.0;
    ai.z -= std::sin(ai.angle) * 8.0;
    ai.isSpinning = true;
    ai.spinTimer = 0.8;
    continue;
} else if (qGreen(maskColor) == 255 && qRed(maskColor) == 0) {
    ...
```

### 2. Flawed Logic
- All tracks (`generateDonutPlains`, `generateChocoValley`, `generateBowserCastle`) draw the tarmac raceway into `trackMaskMap` using **White** (`QColor(255, 255, 255)`).
- White has: `qRed == 255`, `qGreen == 255`, and `qBlue == 255`.
- Molten Lava hazard on Bowser Castle is **Magenta** (`QColor(255, 0, 255)`), where `qRed == 255`, `qGreen == 0`, and `qBlue == 255`.
- The condition `if (qRed(maskColor) == 255 && qBlue(maskColor) == 255)` evaluates to **TRUE** for White road tarmac!
- In `updatePhysics()` (Player 1), lines 902-905 correctly check for road first:
  ```cpp
  if (qRed(maskColor) == 255 && qGreen(maskColor) == 255 && qBlue(maskColor) == 255) {
      currentSurfaceName = ... "Road (Tarmac)";
  } else if (qRed(maskColor) == 255 && qBlue(maskColor) == 255) {
      // Lava Hazard...
  }
  ```
  In `updateAIOpponents()`, line 1167 completely omits the White road check and omits checking `qGreen(maskColor) == 0`.

### 3. Blast Radius
- The moment an AI kart is on or touches the asphalt road (which is true at the starting grid on every single track), it immediately triggers Molten Lava recoil:
  - Recoil pushback: `ai.x -= cos(angle) * 8.0; ai.z -= sin(angle) * 8.0;`
  - Speed penalty: `ai.speed *= 0.35;`
  - Spinout debuff: `ai.isSpinning = true; ai.spinTimer = 0.8;`
  - `continue;`
- In `updateAIOpponents`, `if (ai.spinTimer > 0.0 || ai.isSpinning)` bypasses all waypoint advancement, steering, and acceleration.
- When `spinTimer` counts down to 0 after 0.8s, the kart re-samples the tarmac road and immediately re-triggers Molten Lava recoil.
- Result: **All AI opponents are permanently locked in an infinite spinout loop on the starting grid, never advancing past waypoint 0.**

---

## Recommended Remediation

In `game_canvas.cpp`:

1. **Fix Terrain Sampling in `updateAIOpponents` (Line 1167)**:
   Ensure Molten Lava only triggers when green channel is 0 (or specifically on Bowser Castle off-road):
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
       // Green off-road (Grass/Sand/Shoulder)
       terrainSpeedMult = (currentLevel == TrackLevel::BOWSER_CASTLE) ? 0.70 : 0.45;
       terrainDragMult = 2.2;
   }
   ```

2. **Debuff Timer Processing Order**:
   In `updateAIOpponents` (lines 1063-1093):
   Move the `inkTimer` decay check:
   ```cpp
   if (ai.inkTimer > 0.0) {
       ai.inkTimer -= dt;
       if (ai.inkTimer <= 0.0) {
           ai.inkTimer = 0.0;
           ai.isInked = false;
       } else {
           ai.isInked = true;
       }
   }
   ```
   **BEFORE** `if (ai.spinTimer > 0.0 || ai.isSpinning)` so that a spinning kart still counts down its ink debuff timer rather than freezing ink decay during spinouts.

---

## Invalidation & Verification Command

To independently reproduce this failure and verify subsequent fixes:

```powershell
$env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH
Set-Location "d:\Qt\RetroKart3D\tests\build_stress_m2"
mingw32-make -f Makefile.Release
& "release\standalone_m2_stress.exe"
```
