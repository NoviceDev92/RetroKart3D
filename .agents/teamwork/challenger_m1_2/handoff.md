# Milestone 1: Game State Flow & Character Selection — Challenger Handoff Report

**Agent**: `challenger_m1_2` (Empirical Challenger)  
**Date**: 2026-10-29  
**Target Environment**: Desktop Qt 6.11.1 MinGW 64-bit on Windows  
**Workspace**: `d:\Qt\RetroKart3D`  
**Verdict**: **APPROVE**

---

## 1. Observation

1. **Compilation Command and Clean Build**:
   Executed command:
   ```powershell
   $env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH
   mingw32-make -f Makefile.Release clean
   mingw32-make -f Makefile.Release
   ```
   *Result*: Exited with code 0. Zero compiler warnings, zero errors. Generated executable `release\RetroKart3D.exe`.

2. **Empirical Challenger Test Suite Execution**:
   Compiled and executed dedicated stress harness `tests/test_m1_empirical_challenger.cpp` targeting live `GameCanvas` and `SpriteAtlas`:
   ```powershell
   .\tests\challenger_m1_runner.exe
   ```
   *Result*:
   ```
   Total Assertions: 155
   Passed Assertions: 155 (100%)
   Failed Assertions: 0
   >>> VERDICT: ALL EMPIRICAL CHALLENGES PASSED! <<<
   ```

3. **Measured Menu State CPU Utilization**:
   - `TITLE_MENU` (500 frames benchmarked): 457.24 ms total -> **0.914 ms/frame** (914.5 µs/frame).
     - Fraction of 16.666 ms frame budget: **5.49%**.
   - `CHARACTER_SELECT` (500 frames benchmarked): 320.44 ms total -> **0.641 ms/frame** (640.9 µs/frame).
     - Fraction of 16.666 ms frame budget: **3.85%**.
   - Active `RACING` state Mode-7 inner scan loop: **0.162 ms/frame**.
   - Menu states leave **> 94%** of the 60 FPS CPU budget idle.

4. **SpriteAtlas Bounds and Memory Safety**:
   - `sprite_atlas.h` (lines 30-43) & `sprite_atlas.cpp` (lines 515-766):
     - All 20 `SpriteType` catalog enum values tested: 100% valid, non-null, exactly 32x32 ARGB32 with active artwork.
     - Out-of-bounds enum lookups (`-999` and `999999`) safely returned the fallback 32x32 image.
     - 500,000 rapid lookups completed in 2.70 ms (185 million queries/sec) with zero memory growth.
     - Palette swapping for `KART_MARIO_ALT`, `KART_BOWSER_ALT`, `KART_PEACH_ALT`, `KART_TOAD_ALT` verified distinct from primary palettes.

5. **Adversarial Input Sequences**:
   - 10,000 alternating Left/Right and A/D keypresses in Character Select: `selectedCharP1` remained strictly bounded in `[0, 3]`.
   - 27 hostile Qt keycodes (`Tab`, `Ctrl`, `Alt`, `F1-F12`, `Insert`, `Delete`, etc.) safely ignored without state drift.
   - 1,000 rapid state transitions (`Return` <-> `Escape`) between Title Menu and Character Select cleanly settled at `TITLE_MENU` with zero stuck keys.

6. **Multiplayer 2P Sequential Selection Edge Cases**:
   - When P1 and P2 select identical characters (tested across Mario, Bowser, Peach, and Toad), P1 receives primary palette, P2 receives alternate palette (`altColor` and `_ALT` sprite).
   - Back-navigation: `Escape` in Phase 1 (P2 choosing) reverts to Phase 0 (P1 choosing). Subsequent `Escape` in Phase 0 reverts to Title Menu.

7. **Regression E2E Suite**:
   Executed command:
   ```powershell
   cmd /c "cd /d d:\Qt\RetroKart3D\tests && run_tests.bat"
   ```
   *Result*: Exited with code 0. 182 / 182 test cases passed (100%), 1399 / 1399 assertions passed.

---

## 2. Logic Chain

1. From Observation 1, the clean release build with 0 warnings verifies compiler conformance with Desktop Qt 6.11.1 MinGW 64-bit on Windows.
2. From Observation 3, per-frame execution time in menu states is < 1.0 ms (< 6% of the 16.67 ms 60 FPS frame time). Because `updateGameLoop()` branches on `currentState`, Mode-7 projection and physics integration are bypassed during menus, ensuring negligible CPU overhead.
3. From Observation 4, `SpriteAtlas::getSprite()` uses defensive fallback logic and `setPixel()` enforces bounds checking (`x >= 0 && x < img.width() && y >= 0 && y < img.height()`). In-memory stress testing across 500k queries showed no memory leaks or segmentation faults.
4. From Observation 5, input routing in `handleCharacterSelectInput()` employs closed modular arithmetic (`(selectedCharP1 + 1) % 4` and `(selectedCharP1 - 1 + 4) % 4`), ensuring index invariants hold even under 10,000 high-frequency adversarial key inputs.
5. From Observation 6, `startRace()` explicitly checks `if (selectedCharP1 == selectedCharP2)` and binds `player2.kartColor = p2.altColor`, providing visual differentiation during duplicate character selection.
6. Combining Observations 1-7, Milestone 1 satisfies all functional, architectural, performance, and robustness requirements.

---

## 3. Caveats

- Milestone 1 establishes the character roster, physics bindings, menu flow, and multiplayer data models; dynamic split-screen viewport division and real-time AI steering will be exercised under Milestones 2 and 4.
- GPU rasterization was not benchmarked as RetroKart3D intentionally uses software Mode-7 inverse perspective mapping onto a CPU frame buffer, which easily met the performance envelope.

---

## 4. Conclusion

Milestone 1 (Game State Flow & Character Selection) is **APPROVED** with zero defects.
Implementation code is robust, memory-safe, calibrated to specification, and ready for Milestone 2 (AI Opponents & Item Targeting Fixes).

---

## 5. Verification Method

1. **Rebuild the Application**:
   ```powershell
   $env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH
   mingw32-make -f Makefile.Release clean
   mingw32-make -f Makefile.Release
   ```
   *Verification criteria*: Exit code 0, 0 compiler warnings, binary `release\RetroKart3D.exe` updated.

2. **Run Empirical Challenger Suite**:
   ```powershell
   $env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH
   .\tests\challenger_m1_runner.exe
   ```
   *Verification criteria*: 155 assertions pass, 0 failures, exit code 0.

3. **Run Comprehensive E2E Test Suite**:
   ```powershell
   cd d:\Qt\RetroKart3D\tests
   .\run_tests.bat
   ```
   *Verification criteria*: 182 test cases pass, 1399 assertions pass, exit code 0.
