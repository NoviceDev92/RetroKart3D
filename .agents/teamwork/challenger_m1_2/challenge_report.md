# Milestone 1: Game State Flow & Character Selection — Empirical Challenge Report

**Agent**: `challenger_m1_2` (Empirical Challenger)  
**Date**: 2026-10-29  
**Target Environment**: Desktop Qt 6.11.1 MinGW 64-bit on Windows  
**Workspace**: `d:\Qt\RetroKart3D`  
**Verdict**: **APPROVE**

---

## 1. Executive Summary

Milestone 1 introduces the title menu, character selection screen, 4 distinct characters (Mario, Bowser, Peach, Toad) with unique stats and procedural billboard sprites, cockpit color matching, and multiplayer 2P selection flow.

An independent empirical challenge was conducted by compiling the application under Desktop Qt 6.11.1 MinGW 64-bit and executing a dedicated, live-object test harness (`tests/test_m1_empirical_challenger.cpp`) alongside the existing 182-test E2E test suite.

All empirical tests passed cleanly:
- 182 / 182 E2E test cases passed (1,399 / 1,399 assertions).
- 155 / 155 dedicated empirical stress assertions passed (0 failures).

**Overall Risk Assessment**: **LOW**

---

## 2. Empirical Verification Findings

### Challenge 1: `updateGameLoop()` CPU Utilization During Menu States
- **Hypothesis**: Menu states might consume excessive CPU time, fail to hit 60 FPS, or fail to bypass heavy Mode-7 perspective projection.
- **Empirical Test**: 500 consecutive frame ticks of `updateGameLoop()` measured via high-resolution timers for `TITLE_MENU`, `CHARACTER_SELECT`, and active `RACING`.
- **Results**:
  - `TITLE_MENU`: 457.24 ms total for 500 frames -> **0.914 ms/frame** (914.5 µs/frame).
    - 60 FPS frame budget consumption: **5.49%** of available 16.67 ms frame time.
  - `CHARACTER_SELECT`: 320.44 ms total for 500 frames -> **0.641 ms/frame** (640.9 µs/frame).
    - 60 FPS frame budget consumption: **3.85%** of available 16.67 ms frame time.
  - Both menu states leave **> 94%** of the 60 FPS CPU frame budget completely idle.
- **Conclusion**: Confirmed. `updateGameLoop()` CPU utilization during menu states is negligible.

### Challenge 2: SpriteAtlas Memory Safety & Procedural Generation
- **Hypothesis**: Programmatic pixel manipulation in `SpriteAtlas` might perform out-of-bounds writes, leak memory, or crash when queried with unmapped enum values.
- **Empirical Test**:
  - Queried all 20 `SpriteType` catalog enum values (`QUESTION_BOX`, `MUSHROOM`, `ROCKET`, `BANANA`, `GREEN_SHELL`, `INK_BLOOPER`, `GREEN_PIPE`, `THWOMP`, `OIL_SLICK`, `COIN`, `BANANA_DROPPED`, `SHELL_PROJECTILE`, `KART_MARIO`, `KART_BOWSER`, `KART_PEACH`, `KART_TOAD`, and all 4 `_ALT` palettes).
  - Validated image dimensions (`32x32`), format (`ARGB32`), non-null buffer, and non-zero alpha raster artwork (> 50 drawn pixels).
  - Queried out-of-bounds enum values (`-999` and `999999`).
  - Executed 500,000 rapid atlas lookups in a tight loop.
- **Results**:
  - All 20 sprite types are valid, non-null, exactly 32x32 ARGB32.
  - Out-of-bounds enum values cleanly return the 32x32 fallback image without exceptions, null pointer dereferences, or buffer overflows.
  - 500,000 queries completed in 2.70 ms (185 million lookups/sec) with zero memory growth.
  - Primary vs ALT palettes confirmed distinct across all 4 characters (e.g. Fire Mario white body, Dry Bowser charcoal shell, Daisy orange chassis, Yellow Toad yellow cap).
- **Conclusion**: Confirmed robust and memory-safe.

### Challenge 3: Adversarial Input Sequences During Character Selection
- **Hypothesis**: Rapid key alternating, reentrancy, or invalid key spam could desynchronize cursors, set character indices out of bounds (`< 0` or `>= 4`), or corrupt state machine transitions.
- **Empirical Test**:
  - Injected 10,000 alternating `Left`/`Right` and `A`/`D` keypresses into `GameCanvas`.
  - Injected 5,001 unidirectional `Right` keypresses to stress modulo boundary wrapping.
  - Flooded canvas with 27 non-gameplay / hostile Qt keycodes (`Tab`, `Backtab`, `Ctrl`, `Alt`, `F1-F12`, `Insert`, `Delete`, `QuoteLeft`, etc.).
  - Executed 1,000 rapid state flips between `TITLE_MENU` and `CHARACTER_SELECT` (`Return` -> `Escape` -> `Return` -> `Escape`).
  - Directly triggered track change keys (`1`, `2`, `3`) during character selection.
- **Results**:
  - `selectedCharP1` remained strictly within `[0, 3]` across all 10,000 rapid key events.
  - Unidirectional cycling modulo wrapping was exact (`5001 % 4 == 1`).
  - All hostile keycodes were ignored cleanly with zero side-effects.
  - State transitions settled with zero key bleed, zero stuck keys, and zero memory growth.
- **Conclusion**: Confirmed resilient against adversarial input sequences.

### Challenge 4: Multiplayer 2P Sequential Selection Edge Cases
- **Hypothesis**: Duplicate character selection by P1 and P2 could cause visual ambiguity, incorrect palette assignment, or navigation lockup.
- **Empirical Test**:
  - Tested 2P duplicate pick for all 4 characters: P1 Mario + P2 Mario; P1 Bowser + P2 Bowser; P1 Peach + P2 Peach; P1 Toad + P2 Toad.
  - Verified P1 receives primary color and P2 receives alternate color.
  - Verified disjoint picks (P1 Mario + P2 Peach) receive respective primary colors.
  - Tested back-navigation: verified pressing `Escape` in Phase 1 (P2 selecting) steps back to Phase 0 (P1 selecting) without aborting to Title Menu, while a second `Escape` in Phase 0 returns to Title Menu.
- **Results**:
  - When P1 and P2 select identical characters, P2 automatically receives the alternate palette (`p2Char.altColor` and `_ALT` sprite), preventing visual confusion.
  - Colors are distinct: Mario Red (`220, 20, 20`) vs Fire Mario White (`240, 240, 255`); Bowser Green (`35, 115, 45`) vs Dry Bowser Charcoal (`60, 60, 65`); Peach Pink (`245, 130, 175`) vs Daisy Orange (`255, 160, 30`); Toad Blue (`25, 95, 220`) vs Yellow Toad (`245, 210, 30`).
  - Disjoint picks retain primary colors.
  - Phase 1 `Escape` correctly reverts to Phase 0.
- **Conclusion**: Confirmed correct.

### Challenge 5: Character Stat Calibration & Hierarchy
- **Hypothesis**: Character attributes could have improper physics constants or violate design requirements.
- **Empirical Test**: Queried `getCharacters()` and asserted all statistical values and comparative hierarchies.
- **Results**:
  - Mario (Balanced): 160.0 km/h, 110.0 accel, 2.80 turnRate, 1.00 weight (3/3/3/3 pips).
  - Bowser (Heavyweight): 175.0 km/h, 85.0 accel, 2.20 turnRate, 1.60 weight (5/1/1/5 pips).
  - Peach (Agility): 152.0 km/h, 125.0 accel, 3.40 turnRate, 0.75 weight (2/4/5/2 pips).
  - Toad (Acceleration): 148.0 km/h, 140.0 accel, 3.10 turnRate, 0.65 weight (2/5/4/1 pips).
  - All 4 hierarchies verified:
    - Top Speed: Bowser > Mario > Peach > Toad.
    - Acceleration: Toad > Peach > Mario > Bowser.
    - Handling: Peach > Toad > Mario > Bowser.
    - Weight: Bowser > Mario > Peach > Toad.
- **Conclusion**: Confirmed calibrated to specification.

---

## 3. Build & Test Verification Commands

1. **Compilation Verification**:
   ```powershell
   $env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH
   mingw32-make -f Makefile.Release clean
   mingw32-make -f Makefile.Release
   ```
   *Result*: Exit code 0, 0 compiler warnings, 0 linker errors, binary `release\RetroKart3D.exe` generated.

2. **Full E2E Test Suite**:
   ```powershell
   cd d:\Qt\RetroKart3D\tests
   .\run_tests.bat
   ```
   *Result*: 182 / 182 test cases passed (100%), 1399 / 1399 assertions passed.

3. **Empirical Challenger Test Suite**:
   ```powershell
   cd d:\Qt\RetroKart3D\tests
   .\challenger_m1_runner.exe
   ```
   *Result*: 155 / 155 empirical stress assertions passed, 0 defects detected.

---

## 4. Final Verdict

**APPROVE**  
Milestone 1 is empirically sound, memory-safe, performant, and resilient against adversarial inputs. Downstream milestones (Milestone 2: AI Fleet & Items) may proceed immediately.
