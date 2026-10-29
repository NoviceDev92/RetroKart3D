# Milestone 1: Game State Flow & Character Selection — Challenger Handoff Report

**Agent**: `challenger_m1_1` (Empirical Challenger)  
**Date**: 2026-10-29  
**Target Environment**: Desktop Qt 6.11.1 MinGW 64-bit on Windows  
**Workspace**: `d:\Qt\RetroKart3D`  
**Handoff Type**: Hard (Task Complete)  

---

## 1. Observation

1. **Source Code Implementation Inspected**:
   - `game_canvas.h` and `game_canvas.cpp`:
     - `enum class GameState` and `enum class GameMode` fully defined.
     - `struct CharacterDef` implemented with canonical attributes: name, archetype, primary/secondary/alt colors, maxSpeed, accel, turnRate, driftFactor, weight, and 5-pip indicators.
     - Character roster initialized with 4 distinct characters: Mario (160 km/h, 110 accel, 2.80 turn), Bowser (175 km/h, 85 accel, 2.20 turn), Peach (152 km/h, 125 accel, 3.40 turn), Toad (148 km/h, 140 accel, 3.10 turn).
     - State machine routes inputs via `handleTitleMenuInput()`, `handleCharacterSelectInput()`, and `handleRacingInput()`.
     - `setGameState()`, `startRace()`, and `focusOutEvent()` explicitly call `pressedKeys.clear()`.
     - Public index accessors `setSelectedCharIndexP1()` and `setSelectedCharIndexP2()` use mathematical wrap-around: `(index % 4 + 4) % 4`.
   - `sprite_atlas.h` and `sprite_atlas.cpp`:
     - Declared and implemented procedural $32 \times 32$ ARGB raster sprites for `KART_MARIO`, `KART_BOWSER`, `KART_PEACH`, `KART_TOAD` alongside alternate duplicate palettes (`_ALT`).

2. **Empirical Stress Test Harness Created**:
   - Created `tests/standalone_m1_stress.cpp`, `tests/standalone_m1_stress.pro`, and `tests/run_stress_tests.bat`.
   - Harness directly instantiates `TestableGameCanvas` (subclass of `GameCanvas`) and links against `game_canvas.cpp` and `sprite_atlas.cpp`.
   - Executed 53 adversarial stress test cases covering:
     - Rapid state transitions (2,000 cycles without corruption, 5,000 random key fuzzing storm, 16-pairwise transition matrix).
     - Boundary wrapping across extreme integers (`INT_MIN` to `INT_MAX`, 10,000 Left-arrow pulses).
     - Key-bleed elimination across `TITLE_MENU` $\to$ `CHARACTER_SELECT` $\to$ `RACING`.
     - Static and dynamic physics stat scaling (measured acceleration, straightaway peak speed, and steering angle deflection).
     - Sprite dimensions and palette differentiation.

3. **Empirical Test Results**:
   - `tests/run_stress_tests.bat`:
     - Total Challenges Executed: **53**
     - Passed Challenges: **53** (100%)
     - Failed Challenges: **0**
     - Console verdict: `>>> VERDICT: APPROVE <<<`
   - Full acceptance test runner (`tests/run_tests.bat`):
     - Total Test Cases: **182**
     - Passed Test Cases: **182** (100%)
     - Total Assertions: **1399**
     - Passed Assertions: **1399** (100%)
     - Failed Assertions: **0**

4. **Compilation Verification**:
   - Command:
     ```powershell
     $env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH
     mingw32-make -f Makefile.Release
     ```
   - Result: Exit code 0, 0 compiler warnings, 0 errors.

---

## 2. Logic Chain

1. **Menu Transition Stability**:
   - By subjecting `GameCanvas` to 2,000 full state transition cycles and 5,000 pseudo-random fuzzing key events (Observation 2 & 3), we empirically confirmed that the state machine does not leak resources, freeze, or enter invalid enum states under rapid or hostile input sequences.

2. **Character Boundary Safety**:
   - Applying `INT_MIN` through `INT_MAX` to `setSelectedCharIndexP1/P2` and hammering navigation inputs 10,000 times (Observation 2 & 3) verified that `(index % 4 + 4) % 4` prevents out-of-bounds indexing in `characters[...]` during both UI rasterization and race initialization.

3. **Key-Bleed Prevention**:
   - Injecting `KeyPress(W)` and `KeyPress(A)` during menu states and maintaining them into active racing mode while monitoring physics signals (Observation 2 & 3) proved that `kartSpeed` and `kartAngle` remain strictly `0.0` on frame 0. This confirms that `pressedKeys.clear()` in `startRace()` and `setGameState()` effectively prevents menu input bleed into vehicle kinematics.

4. **Stat Scaling & Physics Fidelity**:
   - Static analysis confirmed $175 > 160$ km/h top speed, $140 > 110$ accel, and $3.4 > 2.2$ turn rate.
   - Dynamic simulation confirmed:
     - After $\sim 0.5$s full throttle: Toad ($86.66$ km/h) > Mario ($71.88$ km/h) > Bowser ($54.53$ km/h).
     - Peak road speed: Bowser reached $175.0$ km/h, Mario reached $160.0$ km/h.
     - Cornering deflection after 20 ticks at 60 km/h: Peach ($69.55^{\circ}$) > Bowser ($35.56^{\circ}$).
   - This directly confirms that character stat differentiation is not merely cosmetic; it directly drives Mode-7 vehicle kinematics.

5. **Build and Asset Integrity**:
   - Clean compilation of both the release application binary and the standalone test executable confirms full toolchain compatibility under Qt 6.11.1 MinGW 64-bit on Windows.

---

## 3. Caveats

- Milestone 2 AI waypoint pursuit & dynamic rubber-banding: out of scope for M1 (pre-populated structs verified).
- Milestone 4 split-screen dual Mode-7 viewport rendering: out of scope for M1 (P1/P2 roster selection verified).
- No caveats regarding Milestone 1 deliverables.

---

## 4. Conclusion

**Verdict: APPROVE**

Milestone 1 (Game State Flow & Character Selection) has passed all empirical challenges without error or regression. The state machine, character roster, input isolation, Mode-7 physics stat integration, and procedural sprite generators meet all project specifications.

---

## 5. Verification Method

1. **Execute Standalone Empirical Stress Test Suite**:
   ```cmd
   .\tests\run_stress_tests.bat
   ```
   *Expected Result*: 53/53 tests PASS (100%), exit code 0, `>>> VERDICT: APPROVE <<<`.

2. **Execute Full Project E2E Acceptance Suite**:
   ```cmd
   .\tests\run_tests.bat
   ```
   *Expected Result*: 182/182 tests PASS (100%), 1399/1399 assertions pass, exit code 0.

3. **Verify Project Clean Build**:
   ```powershell
   $env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH
   mingw32-make -f Makefile.Release
   ```
   *Expected Result*: Exit code 0, 0 compiler warnings, 0 errors.

