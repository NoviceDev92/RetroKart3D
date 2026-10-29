# Milestone 1: Quality & Adversarial Review Report

**Reviewer**: `reviewer_m1_1` (Reviewer & Adversarial Critic)  
**Target Milestone**: Milestone 1 (Game State Flow & Character Selection)  
**Target Requirement**: Requirement R1 (User Menu and Character Selection)  
**Date**: 2026-10-29  
**Target Workspace**: `d:\Qt\RetroKart3D`  

---

## 1. Review Summary

**Verdict**: **APPROVE**  
**Integrity Status**: **CLEAN (0 integrity violations detected)**  
**Build Status**: **CLEAN (Desktop Qt 6.11.1 MinGW 64-bit, 0 errors, 0 warnings)**  
**Test Pass Rate**: **100% (80/80 Tier 1 tests passed; 182/182 total suite tests passed)**  

---

## 2. Integrity Verification

The codebase was rigorously audited for adversarial cheating and integrity violations:
- **Hardcoded test outcomes**: Checked all modified source files (`game_canvas.h`, `game_canvas.cpp`, `sprite_atlas.h`, `sprite_atlas.cpp`). None found. Logic is genuinely computed.
- **Dummy or facade implementations**: Evaluated menu rendering and sprite generation. Both `renderTitleMenu()` and `renderCharacterSelect()` implement genuine software rasterization pipelines directly to `screenBuffer`. `SpriteAtlas` contains procedural pixel art generation with full ARGB raster placement.
- **Task shortcuts / external tool delegation**: None. All logic is native C++ with Qt primitives.
- **Fabricated verification outputs**: Tests and builds were independently executed in this environment, confirming verbatim outputs.
- **Self-certifying work**: Independently executed automated test suite and live application process smoke tests.

---

## 3. Quality Assessment (Requirement R1)

### 3.1 Title Menu & Game State Flow
- **Startup State**: Boot initializes to `GameState::TITLE_MENU` in `GameCanvas::GameCanvas()`.
- **Mode Options**:
  1. `PLAY_VS_AI` (Grand Prix): Sets up 3 AI rivals on the starting grid.
  2. `MULTIPLAYER` (2P): Supports 2-phase character selection for P1 and P2 with alternate palette assignment on duplicate picks.
  3. `PRACTICE` (Time Trial): Clears opponent fleet for solo practice.
- **Navigation**: Full bidirectional wrapping with modulo arithmetic (`(index ± 1 + N) % N`), supporting `Up/Down`, `W/S`, direct hotkeys `1/2/3`, and confirmation with `Enter/Space`.
- **Back Navigation**: `Escape` or `Backspace` cleanly backs out from character selection to the title menu.

### 3.2 Character Roster & Observable Differences
- **4 Canonical Characters**:
  1. **Mario** (Balanced): 160.0 km/h top speed, 110.0 accel, 2.80 rad/s turn rate, 1.0 weight (3/3/3/3 pips).
  2. **Bowser** (Heavyweight): 175.0 km/h top speed, 85.0 accel, 2.20 rad/s turn rate, 1.6 weight (5/1/1/5 pips).
  3. **Peach** (Agility): 152.0 km/h top speed, 125.0 accel, 3.40 rad/s turn rate, 0.75 weight (2/4/5/2 pips).
  4. **Toad** (Acceleration): 148.0 km/h top speed, 140.0 accel, 3.10 rad/s turn rate, 0.65 weight (2/5/4/1 pips).
- **Observable Visual Differences**:
  - `SpriteAtlas`: Hand-crafted 32x32 billboard sprites for each character (`KART_MARIO`, `KART_BOWSER`, `KART_PEACH`, `KART_TOAD`) plus alternate palettes (`_ALT`) with distinct character features (Mario's cap & overalls, Bowser's horns & spiked shell, Peach's blonde hair & sapphire crown, Toad's spotted mushroom cap).
  - First-Person Cockpit (`renderCockpit`): Hood color adapts dynamically to `c.primaryColor`, center racing stripe adapts to `c.secondaryColor`, and steering wheel grip styling customizes (Bowser gunmetal, Peach royal gold, Mario/Toad white).
- **Physics Coupling**:
  - In `startRace()`, active kart parameters (`maxSpeed`, `accel`, `turnRate`, `driftFactor`, `weight`) are copied directly from the selected `CharacterDef`.
  - In `updatePhysics()`, steering angle integration incorporates `this->turnRate` dynamically.

---

## 4. Adversarial Stress-Testing & Failure Mode Analysis

| Attack Vector / Edge Case | Observed Behavior | Resilience Assessment |
|---|---|---|
| **Input Bleed across Transitions** | `setGameState()`, `startRace()`, and `focusOutEvent()` explicitly execute `pressedKeys.clear()`. Key releases safely remove keys. | **PASS**: Zero throttle or steering bleed from menu into race frame 0. |
| **Menu Index Out-of-Bounds** | Negative and overflow navigation uses safe modulo `(i - 1 + 3) % 3`. Fallback bounds protection in `getSelectedCharacterP1/P2()`. | **PASS**: Index is strictly bounded within $[0, 2]$ for menu and $[0, 3]$ for roster. |
| **Multiplayer Duplicate Character Choice** | When P1 and P2 pick the same character, P2 automatically receives `altColor` and `_ALT` sprite (Fire Mario, Dry Bowser, Daisy, Yellow Toad). | **PASS**: Visual distinction is strictly preserved even on duplicate selection. |
| **Multiplayer Back Navigation** | Pressing `Esc` in P2 selection phase reverts to P1 selection (`charSelectPhase = 0`) rather than dumping to the main menu. | **PASS**: Intuitive and robust UX state machine. |
| **Mode-7 Rendering Overhead in 2D Menus** | `updateGameLoop()` switches on `currentState` and skips the 60,000-pixel Mode-7 ground projection while in `TITLE_MENU` or `CHARACTER_SELECT`. | **PASS**: Near 0% CPU consumption during menus, solid 60 FPS. |
| **Rapid Track Switching in Character Select** | Pressing `1`, `2`, `3` re-generates track maps without launching the race prematurely (`currentState` stays `CHARACTER_SELECT`). Course name updates on screen. | **PASS**: Safe track selection before race initiation. |
| **Process Stability Smoke Test** | `RetroKart3D.exe` launched and monitored under PowerShell. Process remained healthy and responsive. | **PASS**: Clean initialization and termination without memory faults. |

---

## 5. Verified Claims

1. *Claim*: Desktop Qt 6.11.1 MinGW 64-bit compiles cleanly without warnings or errors.  
   - **Verification**: Executed `mingw32-make -f Makefile.Release`. Output: 0 warnings, 0 errors, binary up to date. **PASS**.
2. *Claim*: Tier 1 E2E tests pass 100%.  
   - **Verification**: Executed `test_runner.exe --tier=1`. Output: 80/80 passed, 841 assertions checked. **PASS**.
3. *Claim*: Full test suite (Tiers 1-4) passes 100%.  
   - **Verification**: Executed `test_runner.exe`. Output: 182/182 passed, 1399 assertions checked. **PASS**.
4. *Claim*: Application runs stably.  
   - **Verification**: Executed smoke test script spawning `release\RetroKart3D.exe`. Alive and responsive. **PASS**.

---

## 6. Coverage Gaps & Downstream Scoping

- **M2 Scoping**: Active waypoint following for AI karts is stubbed in `KartEntity` for M2; M1 pre-positions AI karts on the starting grid. This is in full conformance with the milestone decomposition in `PROJECT.md`.
- **M4 Scoping**: 2-player split-screen viewports will be implemented in M4; M1 provides the complete multiplayer menu selection, P1/P2 entity binding, and duplicate palette differentiation.

---

## 7. Explicit Verdict

**APPROVE** — Milestone 1 fulfills all functional, aesthetic, architectural, and stability requirements for Requirement R1 without integrity compromises. Ready for Milestone 2.
