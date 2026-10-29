# Empirical Challenge Report: Milestone 1 (Game State Flow & Character Selection)

**Challenger**: `challenger_m1_1`  
**Target Files**: `game_canvas.h`, `game_canvas.cpp`, `sprite_atlas.h`, `sprite_atlas.cpp`  
**Test Harness**: `tests/standalone_m1_stress.cpp`, `tests/run_stress_tests.bat`  
**Date**: 2026-10-29  
**Verdict**: **APPROVE**  

---

## 1. Challenge Summary

**Overall risk assessment**: **LOW**

The Milestone 1 implementation by `worker_m1` was subjected to rigorous empirical stress testing, comprising 53 distinct adversarial test cases executed against live instances of `GameCanvas` and `SpriteAtlas`. All 53 challenges passed cleanly (100% pass rate). In addition, the full 182-test project acceptance suite passed with 1399 assertions, and the MinGW 64-bit release build completed with 0 errors and 0 warnings.

---

## 2. Adversarial Challenges & Hypotheses

### Challenge 1: Rapid Menu State Transitions & Key Fuzzing
- **Assumption Challenged**: Rapid alternation between `TITLE_MENU`, `CHARACTER_SELECT`, and `RACING`, or chaotic user key presses, might leave the game in an illegal state, leak timer callbacks, or trigger unhandled null pointer dereferences.
- **Empirical Attack Scenario**:
  1. Executed 2,000 continuous full-cycle transitions (`TITLE_MENU` $\to$ `CHARACTER_SELECT` $\to$ `RACING` $\to$ `TITLE_MENU`) in a tight event loop.
  2. Injected a 5,000-event fuzzing storm of pseudo-random key presses, releases, and clicks across all supported keys interleaved with `updateGameLoop()` render/physics ticks.
  3. Tested all 16 pairwise state changes in the transition matrix via `setGameState()`.
- **Observed Behavior**:
  - All 2,000 cycles completed without a single state corruption or memory leak (`T1.5`: PASS).
  - All 5,000 fuzzing events were absorbed safely with state invariants strictly confined to `{TITLE_MENU, CHARACTER_SELECT, RACING, RACE_FINISHED}` (`T1.6`: PASS).
  - All 16 state matrix transitions verified (`T1.7`: PASS).
- **Risk Assessment**: **RESOLVED / ROBUST**

---

### Challenge 2: Character Index Boundaries & Modulo Wrap-Around
- **Assumption Challenged**: Passing extreme negative or overflowing integer indices to `setSelectedCharIndexP1/P2` or spamming navigation arrows could lead to out-of-bounds indexing in `characters[selectedCharP1]` during rendering or race start.
- **Empirical Attack Scenario**:
  1. Injected boundary setter values across extreme integers: `INT_MIN`, `-1000000`, `-5`, `-4`, `-3`, `-2`, `-1`, `0`, `1`, `2`, `3`, `4`, `5`, `6`, `7`, `8`, `1000000`, `INT_MAX`.
  2. Verified exact mathematical modulo wrap-around: `-1` $\to$ 3 (Toad), `-4` $\to$ 0 (Mario), `4` $\to$ 0 (Mario), `7` $\to$ 3 (Toad).
  3. Hammered Left/A navigation arrow 10,000 consecutive times in `CHARACTER_SELECT`.
  4. Tested 2-Player Multiplayer character selection flow, including P2 auto-offsetting cursor, Backspace reverting from Phase 1 (P2 select) to Phase 0 (P1 select) without exiting to Title, and Backspace in Phase 0 cleanly returning to `TITLE_MENU`.
- **Observed Behavior**:
  - `(index % 4 + 4) % 4` sanitization ensures `selectedCharP1` and `selectedCharP2` remain strictly in `[0, 3]` across all integer inputs (`T2.1`: PASS).
  - Modulo wrap-around correctly navigates between all four canonical racers (`T2.2-T2.5`: PASS).
  - 10,000 arrow pulses stayed strictly within bounds (`T2.10`: PASS).
  - Multiplayer 2P flow transitions between Phase 0 and Phase 1 seamlessly, with back actions behaving identically to standard arcade console conventions (`T2.11-T2.14`: PASS).
- **Risk Assessment**: **RESOLVED / ROBUST**

---

### Challenge 3: Key-Bleed Prevention Across State Boundaries
- **Assumption Challenged**: Keys held down while navigating or confirming menu items (e.g. holding `W` or `Up` for throttle, `A` or `Left` for steering, `Space` for confirm) might persist into `pressedKeys` and immediately execute race actions on frame 0 of active gameplay.
- **Empirical Attack Scenario**:
  1. Held `Qt::Key_W` down in `TITLE_MENU`, transitioned to `CHARACTER_SELECT`, and confirmed into `RACING` without releasing `W`. Ran 10 physics ticks and measured `kartSpeed`.
  2. Held `Qt::Key_A` down in `CHARACTER_SELECT`, confirmed into `RACING` without releasing `A`. Ran 10 physics ticks and measured `kartAngle`.
  3. Accelerated kart to $> 50$ km/h, pressed `Escape` to return to `TITLE_MENU`, and re-entered `RACING`.
  4. Accelerated kart and simulated window focus loss (`focusOutEvent`).
- **Observed Behavior**:
  - Kart speed on frame 0 through tick 10 was strictly `0.0 km/h` (`T3.3`: PASS). No throttle key bleed occurred because `startRace()` and `setGameState()` call `pressedKeys.clear()`, and menu handlers do not insert keys into `pressedKeys`.
  - Kart heading angle remained strictly `0.0 deg` (`T3.5`: PASS). No steering bleed occurred.
  - Race restart cleanly flushed controls and reset velocity (`T3.8`: PASS).
  - Window focus loss triggered `focusOutEvent()`, clearing all keys and allowing rolling friction to naturally decelerate the vehicle (`T3.10`: PASS).
- **Risk Assessment**: **RESOLVED / ROBUST**

---

### Challenge 4: Character Stat Scaling & Dynamic Mode-7 Physics
- **Assumption Challenged**: The advertised character stat differences (Bowser top speed, Toad acceleration, Peach handling) might only exist as display text or metadata, without actually affecting the underlying Mode-7 driving simulation.
- **Empirical Attack Scenario**:
  1. Verified static mathematical inequalities:
     - Bowser `maxSpeed` ($175.0$) > Mario `maxSpeed` ($160.0$)
     - Toad `accel` ($140.0$) > Mario `accel` ($110.0$)
     - Peach `turnRate` ($3.40$) > Bowser `turnRate` ($2.20$)
     - Calibrated weight rankings: Bowser ($1.6$) > Mario ($1.0$) > Peach ($0.75$) > Toad ($0.65$)
  2. Measured dynamic in-game acceleration from standstill under full throttle for $\sim 0.5$ seconds:
     - Toad achieved: $\mathbf{86.66\text{ km/h}}$
     - Mario achieved: $\mathbf{71.88\text{ km/h}}$
     - Bowser achieved: $\mathbf{54.53\text{ km/h}}$
     - Strict empirical order: $\text{Toad} > \text{Mario} > \text{Bowser}$.
  3. Measured dynamic saturated top speed along the tarmac straightaway:
     - Bowser reached: $\mathbf{175.0\text{ km/h}}$
     - Mario reached: $\mathbf{160.0\text{ km/h}}$
     - Strict empirical order: $\text{Bowser} > \text{Mario}$.
     - Confirmed off-road physics: when driving past $X \approx 660$ into off-road grass, speed is properly clamped to $0.45 \times \text{maxSpeed}$ ($78.75$ for Bowser, $72.0$ for Mario).
  4. Measured dynamic cornering agility (heading deflection after 20 steering ticks at 60 km/h):
     - Peach achieved: $\mathbf{69.55^{\circ}}$ deflection
     - Bowser achieved: $\mathbf{35.56^{\circ}}$ deflection
     - Strict empirical order: $\text{Peach} > \text{Bowser}$.
- **Observed Behavior**:
  - Dynamic gameplay physics exactly reflects the calibrated character stats across all performance vectors (`T4.1-T4.13`: PASS).
- **Risk Assessment**: **RESOLVED / ROBUST**

---

### Challenge 5: Procedural Sprite Atlas Assets & Palette Variations
- **Assumption Challenged**: Billboard sprites for Mario, Bowser, Peach, and Toad might fail procedural generation, have invalid dimensions, or lack distinct alternate palettes for duplicate multiplayer selections.
- **Empirical Attack Scenario**:
  1. Inspected `SpriteAtlas::instance().getSprite(...)` for all 4 primary sprites (`KART_MARIO`, `KART_BOWSER`, `KART_PEACH`, `KART_TOAD`) and all 4 alternate palettes (`KART_MARIO_ALT`, `KART_BOWSER_ALT`, `KART_PEACH_ALT`, `KART_TOAD_ALT`).
  2. Verified pixel dimensions ($32 \times 32$) and non-transparent pixel volume ($> 100$ non-alpha pixels per sprite).
  3. Computed per-pixel color delta between each primary sprite and its alternate palette.
- **Observed Behavior**:
  - All 8 sprites are valid $32 \times 32$ images (`T5.1, T5.3, T5.5, T5.7`: PASS).
  - Alternate palettes have significant, visible pixel deltas:
    - Mario vs Fire Mario: 42 px diff (`T5.2`: PASS)
    - Bowser vs Dry Bowser: 122 px diff (`T5.4`: PASS)
    - Peach vs Daisy variant: 94 px diff (`T5.6`: PASS)
    - Toad vs Yellow Toad: 77 px diff (`T5.8`: PASS)
- **Risk Assessment**: **RESOLVED / ROBUST**

---

## 3. Stress Test Results Summary

| Suite # | Test Suite Description | Challenges | Passed | Failed | Status |
|---|---|---|---|---|---|
| Suite 1 | Rapid Menu State Transitions Stress Test | 7 | 7 | 0 | **PASS** |
| Suite 2 | Character Index Boundaries & Wrap-Around Stress Test | 15 | 15 | 0 | **PASS** |
| Suite 3 | Key-Bleed Prevention Stress Test | 10 | 10 | 0 | **PASS** |
| Suite 4 | Character Stat Scaling & Dynamics Verification | 13 | 13 | 0 | **PASS** |
| Suite 5 | Sprite Atlas Procedural Assets Verification | 8 | 8 | 0 | **PASS** |
| **TOTAL** | **Milestone 1 Empirical Challenger Harness** | **53** | **53** | **0** | **100% PASS** |

In addition:
- Full Project Acceptance Runner (`tests/test_runner.exe`): 182 / 182 test cases passed (1399 / 1399 assertions).
- MinGW 64-bit Compilation (`Makefile.Release`): Clean exit code 0, 0 compiler errors, 0 warnings.

---

## 4. Unchallenged Areas
- Milestone 2 AI waypoint pursuit & dynamic rubber-banding: out of scope for M1 (pre-populated structs verified).
- Milestone 4 split-screen dual Mode-7 viewport rendering: out of scope for M1 (P1/P2 roster selection verified).

---

## 5. Final Empirical Verdict

**VERDICT: APPROVE**

Milestone 1 satisfies all functional, architectural, stability, and performance criteria with zero observed regressions or vulnerabilities.

