# Milestone 1: Reviewer Handoff Report

**Agent**: `reviewer_m1_2` (Reviewer & Adversarial Critic)  
**Date**: 2026-10-29  
**Milestone**: Milestone 1 (Game State Flow & Character Selection)  
**Target Files**: `game_canvas.h`, `game_canvas.cpp`, `sprite_atlas.h`, `sprite_atlas.cpp`  
**Verdict**: **APPROVE**  

---

## 1. Observation

1. **Build & Test Toolchain Execution**:
   - Build Command:
     ```powershell
     $env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH; mingw32-make -f Makefile.Release
     ```
     Result: Exit Code 0, 0 compiler errors, 0 warnings. Binary `release\RetroKart3D.exe` compiled cleanly.
   - Process Smoke Test:
     Launched `release\RetroKart3D.exe` via PowerShell: PID 30696 spawned, ran cleanly with GUI event loop, and closed cleanly upon termination.
   - E2E Test Suite Command:
     ```powershell
     $env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH; & "d:\Qt\RetroKart3D\tests\build\release\test_runner.exe"
     ```
     Result: Exit Code 0. 182 test cases executed, 182 passed (100%), 0 failed, 1,399 assertions verified.

2. **Game State Machine & Lifecycle**:
   - `game_canvas.h` lines 15-26: `enum class GameState { TITLE_MENU, CHARACTER_SELECT, RACING, RACE_FINISHED };` and `enum class GameMode { PLAY_VS_AI, MULTIPLAYER, PRACTICE };`.
   - `game_canvas.cpp` lines 27: `currentState = GameState::TITLE_MENU;` boots directly into title menu on startup.
   - `game_canvas.cpp` lines 346-379: `handleTitleMenuInput()` and `confirmTitleMenuSelection()` route menu options 0 (`PLAY_VS_AI`), 1 (`MULTIPLAYER`), and 2 (`PRACTICE`) to `CHARACTER_SELECT`.
   - `game_canvas.cpp` lines 381-441: `handleCharacterSelectInput()` handles P1/P2 selection phases, track level keys (1, 2, 3), and back action (`Escape`/`Backspace`) returning to `TITLE_MENU`.
   - `game_canvas.cpp` lines 445-453: `handleRacingInput()` handles `RACE_FINISHED` state, allowing restart with `R` or return to `TITLE_MENU` with `Enter`/`Space`/`Escape`.

3. **Character Roster & Physics Binding**:
   - `game_canvas.cpp` lines 36-93: `initCharacters()` defines 4 distinct racers:
     - Mario (Balanced): 160 km/h, 110 m/s², 2.8 rad/s, 1.0 drift, 1.0 weight
     - Bowser (Heavy): 175 km/h, 85 m/s², 2.2 rad/s, 0.85 drift, 1.6 weight
     - Peach (Agility): 152 km/h, 125 m/s², 3.4 rad/s, 1.25 drift, 0.75 weight
     - Toad (Acceleration): 148 km/h, 140 m/s², 3.1 rad/s, 1.15 drift, 0.65 weight
   - `game_canvas.cpp` lines 159-174: `startRace()` synchronizes `maxSpeed`, `accel`, `turnRate`, and `KartEntity` fields with selected character definition.
   - `game_canvas.cpp` lines 804, 826: `kartSpeed += currentAccel * dt;` clamped to `currentMaxSpeed`, and `kartAngle += steerDirection * this->turnRate * steerSpeedFactor * dt;` directly binding character stats into vehicle simulation.

4. **Visual Personalization & Billboard Sprites**:
   - `game_canvas.cpp` lines 1286-1323: `renderCockpit()` binds `c.primaryColor` to the kart hood, `c.secondaryColor` to the central racing stripe, and sets steering wheel grip styling (Bowser gunmetal `qRgb(80,85,90)`, Peach gold `qRgb(255,215,50)`, Mario/Toad white).
   - `sprite_atlas.h` lines 32-43 and `sprite_atlas.cpp` lines 516-764: Procedural billboard sprites generated for `KART_MARIO`, `KART_BOWSER`, `KART_PEACH`, `KART_TOAD` and their 4 alternate palettes (`_ALT`), displayed in character preview boxes with idle bobbing.

5. **Key-Bleed Prevention & Input Isolation**:
   - `game_canvas.cpp` lines 321-339: `keyPressEvent()` delegates to state-isolated handlers (`handleTitleMenuInput`, `handleCharacterSelectInput`, `handleRacingInput`). Only `handleRacingInput()` inserts keys into `pressedKeys`.
   - `game_canvas.cpp` lines 115, 145, 266, 317: `setGameState()`, `startRace()`, and `focusOutEvent()` explicitly execute `pressedKeys.clear()`.

---

## 2. Logic Chain

1. **State Flow Correctness**:
   - By setting `currentState = GameState::TITLE_MENU` at startup (Observation 2), the user is greeted by the Title Menu rather than booting into an active track, satisfying Requirement R1.
   - Menu confirmation transitions cleanly to `CHARACTER_SELECT` and sets `selectedGameMode` according to `menuIndex`.
   - Race completion properly transitions to `RACE_FINISHED` overlay, allowing players to either restart race (`R`) or return to Title Menu (`Enter`/`Escape`).

2. **Character Differentiation & Physics Integrity**:
   - Observations 3 and 4 confirm that the 4 canonical racers are defined with differentiated stats and palettes.
   - When a race begins, `startRace()` copies character stats directly to physics properties (`maxSpeed`, `accel`, `turnRate`), producing observable differences in acceleration, top speed, and turning radius during active Mode-7 racing.

3. **Input Isolation & Anti-Bleed Robustness**:
   - Observations 2 and 5 show that keys held during title navigation or character selection are never passed to the racing input buffer.
   - Any state change or focus loss event triggers `pressedKeys.clear()`, guaranteeing that karts never suffer stuck throttle or phantom steering upon race startup.

4. **Code Quality & Integrity Check**:
   - Zero hardcoded test hacks, fake bypass conditions, or facade stubs exist.
   - All sprites are generated genuinely via procedural pixel algorithms in `SpriteAtlas`.
   - 0 compiler warnings, 0 linker warnings under Desktop Qt 6.11.1 MinGW 64-bit.

---

## 3. Caveats

- **Milestone 2 AI Navigation**: Milestone 1 correctly provisions `aiOpponents` in `startRace()` with 3 competitor karts on the starting grid, but waypoint pathfinding logic will be implemented in Milestone 2.
- **Milestone 4 Split-Screen Pipeline**: Milestone 1 provides full 2P character selection flow, dual player state entities (`player1`, `player2`), and alternate palette handling, but viewport splitting will be implemented in Milestone 4.

---

## 4. Conclusion

Milestone 1 satisfies Requirement R1 and all associated acceptance criteria with zero integrity violations and 100% test pass rate. The implementation is clean, robust, and safe for downstream integration.

**Verdict**: **APPROVE**.

---

## 5. Verification Method

To independently verify this milestone:

1. **Build Verification**:
   ```powershell
   $env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH
   mingw32-make -f Makefile.Release
   ```
   *Expected Result*: Exit Code 0, binary `release\RetroKart3D.exe` updated.

2. **Automated E2E Tests**:
   ```powershell
   $env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH
   & "d:\Qt\RetroKart3D\tests\build\release\test_runner.exe"
   ```
   *Expected Result*: 182 / 182 test cases pass (100% pass rate).

3. **Interactive Menu & Visual Inspection**:
   - Launch `release\RetroKart3D.exe`.
   - Observe title menu with animated chevrons and mode choices.
   - Select "PLAY VS AI", pick "BOWSER" (Heavyweight), start race -> observe dark green hood with orange stripe and gunmetal wheel grip, slow acceleration, high top speed.
   - Press `Escape`, select "PRACTICE", pick "PEACH" (Agility), start race -> observe pink hood with white stripe and gold wheel grip, razor-sharp turning.
