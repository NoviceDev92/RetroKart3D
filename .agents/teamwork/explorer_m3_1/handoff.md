# Milestone 3 Handoff Report: Arcade Driving Physics, Drift & Mini-Turbo

**Author**: `explorer_m3_1`  
**Milestone**: Milestone 3 (Showcase Deliverable: Physics, Drift, Hazards, Minimap & AI Polish)  
**Date**: 2026-10-29  
**Type**: Hard Handoff (Investigation & Architecture Complete)  

---

## 1. Observation

1. **Character Stat Binding Desync in `game_canvas.cpp`**:
   At `game_canvas.cpp:170-176`:
   ```cpp
   player1.id = 0;
   player1.isHuman = true;
   player1.characterId = p1.id;
   player1.name = p1.name;
   player1.kartColor = p1.primaryColor;
   player1.maxSpeed = p1.maxSpeed;
   player1.accel = p1.accel;
   player1.turnRate = p1.turnRate;
   player1.driftFactor = p1.driftFactor;
   player1.weight = p1.weight;
   ```
   However, `GameCanvas` member variables `this->maxSpeed`, `this->accel`, `this->turnRate` were not updated here, retaining static default values ($160.0, 110.0, 2.4$) initialized at `game_canvas.h:234-238`. Meanwhile, `updatePhysics()` at `game_canvas.cpp:892, 955` consumes `maxSpeed`, `accel`, and `this->turnRate`.

2. **Braking Deceleration Discrepancy**:
   `game_canvas.h:79` and `game_canvas.h:236` define `brakeDecel = 160.0`. In contrast, `test_harness.h:112` and `test_tier1_features.cpp:614` require high-grip braking at $240.0\text{ u/s}^2$ (`player2.brakeDecel = 240.0;` and `aiKart.brakeDecel = 240.0;` in `game_canvas.cpp:209, 241`).

3. **Drift Activation & Handling Absence in `updatePhysics()`**:
   In `game_canvas.cpp:925-965`:
   ```cpp
   bool forward = pressedKeys.count(Qt::Key_W) || pressedKeys.count(Qt::Key_Up);
   bool backward = pressedKeys.count(Qt::Key_S) || pressedKeys.count(Qt::Key_Down);
   bool left = pressedKeys.count(Qt::Key_A) || pressedKeys.count(Qt::Key_Left);
   bool right = pressedKeys.count(Qt::Key_D) || pressedKeys.count(Qt::Key_Right);
   ```
   There is no evaluation of `Qt::Key_Shift` or `Qt::Key_Space` for drift state. Neither `isDrifting`, `driftTimer`, `miniTurboTimer`, nor lateral slip are simulated.

4. **Reference Implementation & E2E Test Suite Status**:
   Running the test suite via command:
   ```powershell
   $env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH
   & "d:\Qt\RetroKart3D\tests\build\release\test_runner.exe"
   ```
   Yielded:
   ```
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
   ```
   Tests `F9.1` through `F9.5` and boundaries `F9-B1` through `F9-B5` define the exact contracts:
   - `F9.1`: Speed $> 80.0\text{ u/s}$ after $1.0\text{s}$ throttle.
   - `F9.2`: High-grip braking with $-30\%$ reverse clamping.
   - `F9.3`: Speed $> 40.0\text{ u/s}$ threshold for drift activation.
   - `F9.4`: Drift cornering multiplier $> \text{normal turnRate}$.
   - `F9.5` & `F9-B3`: Drift release $\ge 1.0\text{s}$ awards $1.2\text{s}$ mini-turbo (`ASSERT_NEAR(kart.miniTurboTimer, 1.2, 0.06);`).

5. **Cockpit and Mode-7 Rendering Hooks**:
   `game_canvas.cpp:1529-1530` calculates Mode-7 ground projection using `cameraHeight` ($14.0$). `game_canvas.cpp:1768` places the cockpit hood at `hoodTop = BUFFER_HEIGHT - 35`.

---

## 2. Logic Chain

1. **From Observation 1**: Because `GameCanvas::startRace()` fails to update `this->maxSpeed`, `this->accel`, and `this->turnRate` from the player's selected `CharacterDef` (`p1`), player character selection has no observable physics effect during race execution. **Therefore**, assigning `this->maxSpeed = p1.maxSpeed; this->accel = p1.accel; this->turnRate = p1.turnRate; this->driftFactor = p1.driftFactor;` inside `startRace()` is required to enable character stat scaling.

2. **From Observation 2**: Because `brakeDecel` is currently $160.0$ in `GameCanvas`, braking feels sluggish and fails to match the high-grip contract ($240.0\text{ u/s}^2$). **Therefore**, setting `brakeDecel = 240.0` will provide snappy stopping power and immediate transition into reverse clamped at $-0.30 \times v_{\max}$.

3. **From Observations 3 and 4**: Because `updatePhysics()` does not check `Qt::Key_Shift` or `Qt::Key_Space`, drifting never activates. By introducing:
   - `bool driftKey = pressedKeys.count(Qt::Key_Shift) > 0 || pressedKeys.count(Qt::Key_Space) > 0;`
   - Activation condition: `driftKey && steerDirection != 0 && kartSpeed > 40.0`
   The kart enters power slide, accumulates `driftTimer`, and tightens steering by `driftFactor`.

4. **From Observation 4**: Because the test runner verifies `miniTurboTimer == 1.2` when `driftTimer >= 1.0s` is released (`test_tier1_features.cpp:671`), setting `miniTurboTimer = 1.2` upon release gives a $+35\%$ speed multiplier ($1.35 \times v_{\max}$) and an immediate forward surge of $+25.0\text{ u/s}$, fulfilling both the test suite and user showcase visual requirements.

5. **From Observation 5**: Because `cameraHeight` governs Mode-7 row scanning and `hoodTop` governs the cockpit hood, introducing a $0.22\text{s}$ parabolic hop offset ($h_{\text{hop}} = 3.5 \times \sin(\pi \tau)$) directly modulates `cameraHeight + h_{\text{hop}} * 0.6` and `hoodTop - h_{\text{hop}} * 2.2`, delivering an arcade kart jump and suspension bounce.

6. **From Observations 3 and 5**: Integrating lateral slip angle $\beta$ into position calculation ($x += v \cos(\theta - \beta)\Delta t, z += v \sin(\theta - \beta)\Delta t$) causes the chassis to angle sharply into the apex while momentum drifts laterally outward. Rendering dual-stage spark particles at the outer rear tire ($x \approx 75$ or $325$) provides clear visual feedback (Blue at $\ge 0.5\text{s}$, Orange at $\ge 1.0\text{s}$).

---

## 3. Caveats

1. **Hazards & Minimap Scope Separation**: This investigation focused exclusively on arcade physics, drift kinematics, and mini-turbo. Bowser lava geysers, chocolate fountains, and the top-left HUD minimap are under the purview of peer explorers `explorer_m3_2` and `explorer_m3_3`.
2. **Audio Feedback**: The project utilizes software rasterization and Qt Core/Widgets without `QSoundEffect` or Qt Multimedia linked in `RetroKart3D.pro`. Mini-turbo audio feedback is handled visually via boost flame bursts, HUD surface text notifications (`"Road (⚡ MINI-TURBO! ⚡)"`), and screen flash intensity.
3. **No Caveats** regarding physics or test suite compliance.

---

## 4. Conclusion

The arcade driving physics, dedicated drift hop, lateral slip power slide, and mini-turbo boost system are fully analyzed, mathematically modeled, and ready for drop-in implementation by `worker_m3`.

Key deliverables produced in `d:\Qt\RetroKart3D\.agents\teamwork\explorer_m3_1\analysis.md`:
- Non-linear throttle torque curve equations ($a_{\text{eff}} = a_{\text{base}}(1.0 - 0.4(v/v_{\max}))$).
- $240.0\text{ u/s}^2$ high-grip braking and $-30\%$ reverse speed clamping.
- $0.22\text{s}$ parabolic suspension hop ($H_{\max} = 3.5\text{ u}$) with camera height and cockpit hood modulation.
- Lateral slip angle $\beta = \text{steerDir} \times 0.28 \times \text{driftFactor}$ with Peach ($1.25$) tightest and Bowser ($0.85$) widest.
- Dual-stage drift charging sparks (Blue at $\ge 0.5\text{s}$, Orange at $\ge 1.0\text{s}$) emitting from rear wheel bounds.
- Release mini-turbo boost ($1.2\text{s}$ duration, $1.35\times$ top speed, $+25.0\text{ u/s}$ impulse, boost flames).
- Complete, exact code replacements for `game_canvas.h` and `game_canvas.cpp`.

---

## 5. Verification Method

1. **Build and Test Verification**:
   Execute the full automated test suite to confirm all 182 test cases pass without regressions:
   ```powershell
   $env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH
   & "d:\Qt\RetroKart3D\tests\build\release\test_runner.exe" --tier=1
   & "d:\Qt\RetroKart3D\tests\build\release\test_runner.exe" --tier=2
   ```

2. **Interactive Showcase Verification**:
   Build and launch the main application executable:
   ```powershell
   $env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH
   & "d:\Qt\RetroKart3D\release\RetroKart3D.exe"
   ```
   Verify during active race:
   - Accelerating with $W$ provides smooth launch torque.
   - Pressing $S$ stops the kart rapidly and reverses at clamped $-30\%$ top speed.
   - Pressing and holding `Shift` while turning at speed $> 40.0$ executes an initial hop and enters power slide.
   - Holding drift for $\ge 0.5\text{s}$ produces electric blue sparks; holding for $\ge 1.0\text{s}$ transitions to fiery orange sparks.
   - Releasing `Shift` surges the kart forward with a mini-turbo speed boost, flame visuals, and HUD status update.
   - Selecting Peach produces noticeably sharper turns than selecting Bowser.

3. **Invalidation Conditions**:
   - `test_runner.exe` reporting any assertion failures in F9.1 - F9.5 or F9-B1 - F9-B5.
   - Kart able to drift at speeds $\le 40.0\text{ u/s}$.
   - Mini-turbo awarded on drift release when `driftTimer < 1.0s`.
