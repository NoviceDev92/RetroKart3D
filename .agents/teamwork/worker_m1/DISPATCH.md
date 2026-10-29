## 2026-10-29T03:59:11Z
[Message] timestamp=2026-10-29T03:59:11Z sender=842ff862-512e-4d57-af04-61c29631cec0 priority=MESSAGE_PRIORITY_HIGH content=You are worker_m1, the Implementation Worker for Milestone 1: Game State Flow & Character Selection.
Your working directory is: d:\Qt\RetroKart3D\.agents\teamwork\worker_m1\
Workspace directory: d:\Qt\RetroKart3D

MANDATORY: Read the original user request first:
d:\Qt\RetroKart3D\.agents\teamwork\ORIGINAL_REQUEST.md

Also read:
d:\Qt\RetroKart3D\.agents\teamwork\PROJECT.md
d:\Qt\RetroKart3D\.agents\teamwork\explorer_m1_1\analysis.md
d:\Qt\RetroKart3D\.agents\teamwork\explorer_m1_2\analysis.md
d:\Qt\RetroKart3D\.agents\teamwork\explorer_m1_3\analysis.md

MANDATORY INTEGRITY WARNING:
DO NOT CHEAT. All implementations must be genuine. DO NOT hardcode test results, create dummy/facade implementations, or circumvent the intended task. A teamwork_auditor will independently verify your work. Integrity violations WILL be detected and your work WILL be rejected.

File Ownership:
You have exclusive write access to:
- game_canvas.h
- game_canvas.cpp
- sprite_atlas.h
- sprite_atlas.cpp
- mainwindow.h
- mainwindow.cpp

Implementation Objectives for Milestone 1:
1. Game State & Mode Machine:
   - Define `enum class GameState { TITLE_MENU, CHARACTER_SELECT, RACING, RACE_FINISHED };`
   - Define `enum class GameMode { PLAY_VS_AI, MULTIPLAYER, PRACTICE };`
   - Set initial state on application boot to `GameState::TITLE_MENU`.
2. Title & Main Menu:
   - Render a retro arcade Title and Main Menu onto `screenBuffer` with options:
     1. "PLAY VS AI"
     2. "MULTIPLAYER (2P)"
     3. "PRACTICE"
   - Implement navigation: Up/Down and W/S to move cursor, Enter/Space to select, visual highlight indicator.
3. Character Selection Screen:
   - Add `CharacterDef` struct with id, name, archetype, primaryColor, secondaryColor, maxSpeed, accel, turnRate, driftFactor, weight, speedPips, accelPips, handlingPips, weightPips.
   - Implement 4 distinct characters: Mario (Balanced: 160 km/h, 110 accel, 2.8 turn), Bowser (Heavy: 175 km/h, 85 accel, 2.2 turn), Peach (Agility: 152 km/h, 125 accel, 3.4 turn), Toad (Acceleration: 148 km/h, 140 accel, 3.1 turn).
   - Render 4 character cards with stat bars and descriptions onto `screenBuffer`.
   - Left/Right and A/D to cycle characters; Enter/Space to confirm; Esc to go back to Title Menu.
   - If Multiplayer selected, allow selecting P1 then P2 character.
4. Visual Appearance & Cockpit Integration:
   - Update `SpriteAtlas` if needed to support character kart color variations.
   - Update `renderCockpit()` in `game_canvas.cpp` to use the chosen character's `primaryColor` for the hood and appropriate accent colors, so the player visually sees their chosen kart.
5. Lifecycle & Input Isolation:
   - Implement `startRace()` to cleanly initialize race state, position player kart on the start line, reset timers, celebration flags, items, and apply character stats (`maxSpeed`, `accel`, `turnRate`, etc.).
   - Branch `updateGameLoop()` by state so Mode-7 rendering is only executed during `RACING` / `RACE_FINISHED`, and menu rendering is executed during menu states.
   - Clear `pressedKeys` when changing states to prevent key-bleed.
6. Build & Test:
   - Verify compilation using Desktop Qt 6.11.1 MinGW 64-bit:
     `$env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH; mingw32-make -f Makefile.Release`
   - Ensure 0 compiler errors and 0 warnings.
   - Document verification commands and output in your handoff report.
7. Write your changes summary and handoff.md in your working directory, and notify orchestrator via send_message. Keep progress.md updated.
