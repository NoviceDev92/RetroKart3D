## 2026-10-29T05:19:49Z
You are worker_m3, the Implementation Worker for Milestone 3 (Showcase Deliverable: Physics, Drift, Interactive Hazards, Minimap & AI Polish).
Your working directory is: d:\Qt\RetroKart3D\.agents\teamwork\worker_m3\
Workspace directory: d:\Qt\RetroKart3D

MANDATORY: Read the original user request first:
d:\Qt\RetroKart3D\.agents\teamwork\ORIGINAL_REQUEST.md

Also read:
d:\Qt\RetroKart3D\.agents\teamwork\PROJECT.md
d:\Qt\RetroKart3D\.agents\teamwork\explorer_m3_1\analysis.md
d:\Qt\RetroKart3D\.agents\teamwork\explorer_m3_2\analysis.md
d:\Qt\RetroKart3D\.agents\teamwork\explorer_m3_3\analysis.md
d:\Qt\RetroKart3D\.agents\teamwork\auditor_m2_1\handoff.md
d:\Qt\RetroKart3D\.agents\teamwork\TEST_READY.md

MANDATORY INTEGRITY WARNING:
DO NOT CHEAT. All implementations must be genuine. DO NOT hardcode test results, create dummy/facade implementations, or circumvent the intended task. A teamwork_auditor will independently verify your work. Integrity violations WILL be detected and your work WILL be rejected.

File Ownership:
You have exclusive write access to:
- game_canvas.h
- game_canvas.cpp
- sprite_atlas.h
- sprite_atlas.cpp

Implementation Objectives for Milestone 3:
1. Arcade Driving Physics, Drift & Mini-Turbo (F9):
   - In `startRace()`, synchronize character physics parameters:
     `this->maxSpeed = p1.maxSpeed; this->accel = p1.accel; this->turnRate = p1.turnRate; this->driftFactor = p1.driftFactor; this->weight = p1.weight;`
   - High-grip braking deceleration: set `this->brakeDecel = 240.0;`
   - Implement drift mechanics in `updatePhysics()`:
     - When steering and holding drift key (`Qt::Key_Shift` or `Qt::Key_Space`), trigger a 0.22s parabolic hop visual (bobbing camera height / cockpit hood) and enter drift state (`isDrifting = true`).
     - Compute lateral slip angle beta modifying velocity vector during slide, scaled by character `driftFactor`.
     - Mini-turbo charging: charge `driftTimer` while sliding. Generate dual-stage sparks (electric blue at >=0.5s, fiery orange at >=1.0s).
     - Upon releasing drift key, grant mini-turbo boost surge: 1.25x max speed, speed impulse surge for 0.8s, with boost flame visual.
2. Interactive Track Hazards & Procedural Sprites (F10, F11):
   - In `sprite_atlas.h/cpp`, add procedural 32x32 sprites:
     `LAVA_GEYSER_BUBBLING`, `LAVA_GEYSER_ERUPTING`, `CHOCOLATE_FOUNTAIN`, `CHOCOLATE_CANDY`.
   - Populate hazards in `generateBowserCastle()` (lava geysers in lava lakes) and `generateChocoValley()` (chocolate fountains in mud pools and rolling candies on curves).
   - Implement `updateTrackHazards(dt)`: cyclic animation state machine (dormant bubbling -> erupting plume -> cooling smoke). Height-aware proximity collision (hitting erupting geyser spins kart out; hitting candy/chocolate slows kart).
   - Project hazards in `renderSprites()` with 3D Mode-7 camera transform and depth sorting.
3. CRITICAL AI Terrain Mask & Debuff Fix:
   - In `game_canvas.cpp:1167` (`updateAIOpponents`):
     Fix condition so White tarmac (`qRed == 255 && qGreen == 255 && qBlue == 255`) is recognized as road. Molten lava recoil must only fire when `qRed == 255 && qGreen == 0 && qBlue == 255` (or check White Tarmac first as in line 902).
   - In `game_canvas.cpp:1080`: Ensure `inkTimer` decay is not skipped by `continue` during spinout.
   - This immediately frees all 3 AI opponents to race dynamically along track waypoints at full speed!
4. Top-Left HUD Minimap (F12):
   - 72x72 radar box at top-left (x = 8, y = 8) with dark translucent background (`QColor(0, 0, 0, 160)`) and gold/amber border.
   - Pre-render/cache track outline thumbnail in `minimapTrackCache` (72x72 ARGB32) during track initialization.
   - In `renderHUD()`: draw minimap radar box, render cached track outline, and draw real-time kart position blips:
     - Player 1: Pulsing white core dot (r=2.2) with glowing yellow halo (r=3.5).
     - AI Opponents: Chassis-colored dots (Mario Red, Bowser Orange, Peach Pink, Toad Blue) mapping world coordinates (x, z) in [0, 1024] to minimap pixels.
5. Build & Test Verification:
   - Compile cleanly with Desktop Qt 6.11.1 MinGW 64-bit:
     `$env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH; mingw32-make -f Makefile.Release`
   - Run test runner:
     `$env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH; & "d:\Qt\RetroKart3D\tests\build\release\test_runner.exe"`
   - Run forensic test runner:
     `$env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH; Set-Location "d:\Qt\RetroKart3D\tests\build_forensic"; & ".\release\standalone_m2_forensic.exe"`
   - Ensure 0 compiler errors, 0 warnings, and 100% test pass.
6. Write handoff.md in your working directory and notify orchestrator via send_message. Keep progress.md updated.
