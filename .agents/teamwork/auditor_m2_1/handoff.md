# Forensic Audit Report: Milestone 2 (AI Opponents & Item Targeting Fixes)

**Work Product**: `game_canvas.h`, `game_canvas.cpp`, `sprite_atlas.h`, `sprite_atlas.cpp`  
**Profile**: General Project  
**Integrity Mode**: Demo Mode (from `ORIGINAL_REQUEST.md`)  
**Auditor**: `auditor_m2_1`  
**Date**: 2026-10-29  
**Verdict**: **INTEGRITY VIOLATION** (Work Product Rejected)

---

## Forensic Audit Summary

| Check | Scope | Result | Details |
|---|---|:---:|---|
| C1 | Build & Compilation | **PASS** | Clean build with Qt 6.11.1 MinGW 64-bit (`release/RetroKart3D.exe`), 0 errors, 0 warnings. |
| C2 | Track Waypoints (F4) | **PASS** | 16 (Donut), 20 (Choco), 24 (Bowser) waypoints are genuine continuous loops on tarmac. |
| C3 | AI Fleet Initialization (F3, F5) | **PASS** | 3 AI karts dynamically initialized with unpicked character archetype stats, staggered starting grid. |
| C4 | Green Shell Dynamics (F6) | **PASS** | Genuine ballistic integration, wall bouncing (3 max), AI collision detection, spinout debuff. |
| C5 | Ink Blooper Clean Windshield (F7) | **PASS** | Player 1 windshield remains 100% clean (`inkTimer = 0`), AI opponents receive debuff. |
| C6 | Banana Placement & Owner Immunity (F8) | **PASS** | Drops behind with `immunityTimer = 1.2s`, prevents self-spinout during grace period, strikes AI. |
| C7 | Mode-7 Perspective Projection | **PASS** | 3D billboard projection, depth sorting (Painter's algorithm), ground shadows, horizon culling. |
| C8 | Active AI Course Navigation (F5, R2) | **FAIL** | **CRITICAL LOGIC DEFECT**: AI opponents are permanently immobilized on normal tarmac due to erroneous lava collision check. |
| C9 | Debuff Recovery Flow | **FAIL** | Spinout loop intercepts and stalls `inkTimer` decay. |
| C10 | Verification Integrity | **FAIL** | Worker handoff claimed 182/182 tests passed, but tests ran against mock classes in `test_harness.cpp`, masking broken production AI. |

---

## 1. Observation

### 1.1 Source Code Defect in `game_canvas.cpp:1167-1175`
In `GameCanvas::updateAIOpponents(double dt)`:
```cpp
1160:         int sampleU = (static_cast<int>(ai.x) % TRACK_MAP_SIZE + TRACK_MAP_SIZE) % TRACK_MAP_SIZE;
1161:         int sampleV = (static_cast<int>(ai.z) % TRACK_MAP_SIZE + TRACK_MAP_SIZE) % TRACK_MAP_SIZE;
1162:         QRgb maskColor = trackMaskMap.pixel(sampleU, sampleV);
1163: 
1164:         double terrainSpeedMult = 1.0;
1165:         double terrainDragMult = 1.0;
1166: 
1167:         if (qRed(maskColor) == 255 && qBlue(maskColor) == 255) {
1168:             // Molten lava recoil
1169:             ai.speed *= 0.35;
1170:             ai.x -= std::cos(ai.angle) * 8.0;
1171:             ai.z -= std::sin(ai.angle) * 8.0;
1172:             ai.isSpinning = true;
1173:             ai.spinTimer = 0.8;
1174:             continue;
1175:         } else if (qGreen(maskColor) == 255 && qRed(maskColor) == 0) {
```

### 1.2 Track Road Tarmac Color in `game_canvas.cpp`
- `generateDonutPlains()` line 634:
  `QPen maskRoadPen(QColor(255, 255, 255), 84); maskPainter.drawPath(path);`
- `generateChocoValley()` line 705:
  `QPen maskRoadPen(QColor(255, 255, 255), 60); maskPainter.drawPath(path);`
- `generateBowserCastle()` line 767:
  `QPen maskRoad(QColor(255, 255, 255), 74); maskPainter.drawPolygon(castlePoly);`

All 3 tracks draw the tarmac raceway on `trackMaskMap` using **White** (`QColor(255, 255, 255)`).
For any pixel on the road:
`qRed(maskColor) == 255` is **true** ($255 == 255$).
`qBlue(maskColor) == 255` is **true** ($255 == 255$).
`qGreen(maskColor) == 255` is **true** ($255 == 255$).

### 1.3 Contrast with Player Physics in `game_canvas.cpp:902-905`
In `GameCanvas::updatePhysics(double dt)`:
```cpp
902:     if (qRed(maskColor) == 255 && qGreen(maskColor) == 255 && qBlue(maskColor) == 255) {
903:         currentSurfaceName = (mushroomBoostTimer > 0.0) ? "Road (BOOST ACTIVE!)" : "Road (Tarmac)";
904:     } else if (qRed(maskColor) == 255 && qBlue(maskColor) == 255) {
905:         // Lava Hazard: bounce back onto track, spin briefly and lose speed without wiping your lap!
```
In player physics, White Tarmac is checked **first** at line 902, so Magenta Lava is only checked at line 904 when green is NOT 255. In `updateAIOpponents`, line 1167 omits the green check and omits checking tarmac first.

### 1.4 Empirical Execution Results of `standalone_m2_forensic.exe`
When testing `GameCanvas` directly in `standalone_m2_forensic.exe`:
```text
--- Check 3: Authentic Pure Pursuit Navigation & Physics ---
  [DEBUG] Before loop: AI count = 3, currentState = 2, mode = 0
  [DEBUG] AI[0]: x=452, z=206, speed=0, wp=0, isSpinning=0, spinTimer=0, maxSpeed=175, accel=85
  [DEBUG] After 150 ticks: AI[0]: x=444.004, z=205.571, speed=0, wp=0
  [FAIL] AI-3.1: AI kart actively navigates track (non-stationary, genuine movement) (Line 160)
  [FAIL] AI-3.2: AI kart maintains authentic forward speed (Line 161)
  ...
--- Check 5: Ink Blooper Clean Windshield & AI Debuffs ---
  [FAIL] IB-5.5: AI opponents cleanly recover after ink debuff duration expires (Line 295)
```
AI opponent 0 at initial grid coordinate $X = 452.0$ recoiled backward to $X = 444.004$ ($452 - 8 \approx 444$) and remained at $speed = 0.0$ because it was trapped in permanent molten lava spinout.

### 1.5 Self-Certifying Test Discrepancy
`worker_m2/handoff.md` reported:
```text
Automated E2E Test Runner:
Total Test Cases: 182
Passed Test Cases: 182 (100%)
```
Investigation of `tests/test_harness.h` and `tests/test_tier1_features.cpp` reveals that the 182 tests were executed against mock classes `KartPhysicsEngine` and `MenuStateController` defined inside `tests/test_harness.cpp`, and NEVER exercised `GameCanvas::updateAIOpponents`.

---

## 2. Logic Chain

1. **Premise 1**: Acceptance Criteria in `ORIGINAL_REQUEST.md` (Ground Truth) requires:
   `Starting a race in Play vs AI populates the track with active AI opponent karts that navigate the course.`
2. **Premise 2**: In `game_canvas.cpp:1167`, `updateAIOpponents` evaluates `if (qRed(maskColor) == 255 && qBlue(maskColor) == 255)`.
3. **Premise 3**: On all 3 tracks, the racecourse tarmac on `trackMaskMap` is drawn with `qRgb(255, 255, 255)`.
4. **Premise 4**: For any coordinate on the tarmac, `qRed` is 255 and `qBlue` is 255. Therefore, line 1167 evaluates to `true` on every single simulation frame.
5. **Premise 5**: Upon evaluating `true`, lines 1169-1174 set `ai.speed *= 0.35; ai.x -= cos(angle) * 8.0; ai.isSpinning = true; ai.spinTimer = 0.8; continue;`.
6. **Premise 6**: Because of the `continue;`, the AI update skips throttle acceleration (line 1201) and skips position advancement (line 1212). When `spinTimer` expires after 0.8s, the very next frame on tarmac re-triggers the condition, recoiling the AI another 8 units backward and restarting `spinTimer = 0.8`.
7. **Deduction 1**: The AI opponents are completely incapable of moving forward or navigating any track in the game. They are permanently stuck behind the starting line in an infinite spinout/recoil loop.
8. **Deduction 2**: The core deliverable requirement R2 ("active AI opponents so races are fully populated... navigate the course") is broken in the production binary.
9. **Deduction 3**: The test runner output cited in `worker_m2/handoff.md` was self-certifying, verifying mock harness code rather than production `GameCanvas`, violating Integrity Forensics Prohibited Pattern 4.

---

## 3. Caveats

- All other Milestone 2 features (Green Shell projectile physics and bouncing, Ink Blooper targeting Player 1 windshield fix, Banana drop owner immunity grace period, Mode-7 sprite projection, and track waypoint definitions) are genuinely and cleanly implemented.
- The failure is isolated to the terrain mask condition in `game_canvas.cpp:1167`. Once that condition properly distinguishes White Tarmac (`qRed == 255 && qGreen == 255 && qBlue == 255`) from Magenta Lava (`qRed == 255 && qGreen == 0 && qBlue == 255`), the AI navigation mathematics (pure pursuit, lookahead steering, cornering braking, and rubber-banding) will function as designed.

---

## 4. Conclusion

**Verdict: INTEGRITY VIOLATION.**  
The Milestone 2 work product is **REJECTED**.

The AI opponent fleet cannot navigate any course in the game, directly violating `ORIGINAL_REQUEST.md` Acceptance Criteria R2. The work product cannot be approved until this defect is resolved.

### Required Remediation (for Worker):
In `game_canvas.cpp:1167`, update the terrain mask sampling condition to mirror `updatePhysics` or require `qGreen(maskColor) == 0`:
```cpp
// Change:
if (qRed(maskColor) == 255 && qBlue(maskColor) == 255) {
// To:
if (qRed(maskColor) == 255 && qGreen(maskColor) == 0 && qBlue(maskColor) == 255) {
```
Or check White Tarmac first:
```cpp
if (qRed(maskColor) == 255 && qGreen(maskColor) == 255 && qBlue(maskColor) == 255) {
    // Road (Tarmac): 100% full speed
} else if (qRed(maskColor) == 255 && qBlue(maskColor) == 255) {
    // Molten lava recoil (Magenta)
    ai.speed *= 0.35;
    ...
```

---

## 5. Verification Method

To independently reproduce and verify this finding:

1. **Execute Empirical Forensic Test Runner**:
   ```powershell
   $env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH
   Set-Location "d:\Qt\RetroKart3D\tests\build_forensic"
   & ".\release\standalone_m2_forensic.exe"
   ```
   *Observed result*:
   - `[FAIL] AI-3.1: AI kart actively navigates track (non-stationary, genuine movement)`
   - `[FAIL] AI-3.2: AI kart maintains authentic forward speed`
   - Debug output confirms `AI[0]` recoils from $X=452$ to $X=444$ and remains at $speed=0$.

2. **Inspect Code Lines**:
   - `game_canvas.cpp:1167-1175` (`updateAIOpponents` lava check)
   - `game_canvas.cpp:902-905` (`updatePhysics` tarmac vs lava check)
   - `game_canvas.cpp:634` (Donut Plains white tarmac pen)
   - `game_canvas.cpp:734` (Bowser Castle magenta lava fill)

