## 2026-10-29T04:37:40Z
[Message] timestamp=2026-10-29T04:37:40Z sender=842ff862-512e-4d57-af04-61c29631cec0 priority=MESSAGE_PRIORITY_HIGH content=You are worker_m2, the Implementation Worker for Milestone 2: AI Opponents & Item Targeting Fixes.
Your working directory is: d:\Qt\RetroKart3D\.agents\teamwork\worker_m2\
Workspace directory: d:\Qt\RetroKart3D

MANDATORY: Read the original user request first:
d:\Qt\RetroKart3D\.agents\teamwork\ORIGINAL_REQUEST.md

Also read:
d:\Qt\RetroKart3D\.agents\teamwork\PROJECT.md
d:\Qt\RetroKart3D\.agents\teamwork\explorer_m2_1\analysis.md
d:\Qt\RetroKart3D\.agents\teamwork\explorer_m2_2\analysis.md
d:\Qt\RetroKart3D\.agents\teamwork\explorer_m2_3\analysis.md
d:\Qt\RetroKart3D\.agents\teamwork\TEST_READY.md

MANDATORY INTEGRITY WARNING:
DO NOT CHEAT. All implementations must be genuine. DO NOT hardcode test results, create dummy/facade implementations, or circumvent the intended task. A teamwork_auditor will independently verify your work. Integrity violations WILL be detected and your work WILL be rejected.

File Ownership:
You have exclusive write access to:
- game_canvas.h
- game_canvas.cpp
- sprite_atlas.h
- sprite_atlas.cpp

Implementation Objectives for Milestone 2:
1. Persistent Track Waypoints (F4):
   - Add persistent waypoint collections for Donut Plains (16 waypoints), Choco Valley (20 waypoints), and Bowser Castle (24 waypoints) in game_canvas.
   - Populate waypoints during track generation or initialization following the verified coordinate tables from explorer_m2_1/analysis.md.
2. Active AI Opponent Fleet (F3, F5):
   - Ensure 3 active AI opponent karts are spawned in startRace() when in PLAY_VS_AI mode, staggered behind player on starting grid.
   - Implement updateAIOpponents(dt):
     - Pure pursuit lookahead steering toward target waypoint (lookahead index (curr + 2) % N).
     - Normalize angular difference delta_theta to [-pi, pi] and steer with character turnRate.
     - Advance waypoint when distance < 35.0 units.
     - Cornering brake/feather throttle when |delta_theta| > 0.65 rad.
     - Rubber-banding: +10% to +15% top speed when > 150 units behind player, -10% to -12% when > 150 units ahead of player.
     - Terrain slowdown sampling trackMaskMap (Green = 45% speed, Magenta = spinout).
     - Spinout decay and visual rotation.
3. 3D Billboard Rendering of AI Karts:
   - In renderSprites(), project each active AI opponent using camera transform (camX, camZ).
   - If camZ > 5.0, calculate ground screen position and scale, and add to renderList with its character sprite (KART_MARIO, KART_BOWSER, KART_PEACH, KART_TOAD or _ALT).
   - Ensure renderList is sorted by depth descending so AI karts, items, and obstacles correctly occlude one another.
4. Item Targeting Fixes (F6, F7, F8):
   - Green Shell Projectile (F6):
     - Firing Green Shell spawns an active ShellProjectile in activeProjectiles (velocity approx 220 u/s along kart heading, lifeTime = 6.0, bouncesLeft = 3).
     - In updateItemSystem(dt), move shell, bounce off walls [20, 1004], and check collision with AI opponents (radius 16).
     - On hit: trigger spinout on AI opponent (spinoutTimer = 1.2, speed drop) and deactivate shell.
     - Render active shells in renderSprites() using SpriteType::SHELL_PROJECTILE.
   - Ink Blooper Fix (F7):
     - Line 1008 currently sets inkTimer on player. Set player inkTimer = 0.0 (player windshield remains 100% clean).
     - Apply ink debuff (35% speed reduction, inkTimer = 4.0) to all active AI opponents (and P2).
   - Banana Safe Drop & Opponent Collision (F8):
     - Set ownerId and immunityTimer = 1.2s on dropped bananas so player does not self-spinout.
     - In updateAIOpponents() or physics loop, check collision between AI opponents and bananas. On hit, remove banana and spin out AI opponent.
5. Build & Test Verification:
   - Compile cleanly with Desktop Qt 6.11.1 MinGW 64-bit:
     `$env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH; mingw32-make -f Makefile.Release`
   - Run tests:
     `$env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH; & "d:\Qt\RetroKart3D\tests\build\release\test_runner.exe"`
   - Ensure 0 compiler errors, 0 warnings, and 100% test pass.
6. Write handoff.md in your working directory and notify orchestrator via send_message. Keep progress.md updated.
