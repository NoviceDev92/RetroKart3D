# BRIEFING — 2026-10-29T04:52:00Z

## Mission
Implement Milestone 2: AI Opponents & Item Targeting Fixes (F3, F4, F5, F6, F7, F8) in RetroKart3D codebase.

## 🔒 My Identity
- Archetype: worker
- Roles: implementer, qa
- Working directory: d:\Qt\RetroKart3D\.agents\teamwork\worker_m2\
- Original parent: 842ff862-512e-4d57-af04-61c29631cec0
- Milestone: Milestone 2: AI Opponents & Item Targeting Fixes

## 🔒 Key Constraints
- File ownership: game_canvas.h, game_canvas.cpp, sprite_atlas.h, sprite_atlas.cpp only.
- DO NOT CHEAT: Genuine implementations only, maintain real state, produce real behavior.
- Build command: `$env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH; mingw32-make -f Makefile.Release`
- Test command: `$env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH; & "d:\Qt\RetroKart3D\tests\build\release\test_runner.exe"`
- 0 compiler errors, 0 warnings, 100% test pass.

## Current Parent
- Conversation ID: 842ff862-512e-4d57-af04-61c29631cec0
- Updated: 2026-10-29T04:52:00Z

## Task Summary
- **What to build**:
  1. Persistent Track Waypoints for Donut Plains (16), Choco Valley (20), Bowser Castle (24).
  2. Active AI Opponent Fleet: spawn 3 AI karts in PLAY_VS_AI, updateAIOpponents with pure pursuit, cornering feathering, rubber banding, terrain sampling, spinout decay.
  3. 3D Billboard Rendering of AI Karts and projectiles in renderSprites().
  4. Item Targeting Fixes: Green Shell projectile spawning, movement, wall bounce, AI collision, billboard rendering. Ink Blooper debuffing AI/P2 only (player windshield clean). Banana immunity timer on drop and AI collision.
- **Success criteria**: All M2 features working, clean build, tests passing.
- **Interface contracts**: PROJECT.md, SCOPE.md
- **Code layout**: Root directory C++ files + tests/ directory

## Key Decisions Made
- Used the mathematically verified tarmac spline coordinates from explorer_m2_1 for all 3 track waypoint loops to guarantee AI opponents navigate smoothly without cutting off-road into grass or molten lava.
- Implemented pure pursuit lookahead steering targeting waypoint index `(curr + 2) % N` with angle error normalization to `[-M_PI, M_PI]` and rate-limiting via character `turnRate`.
- Added cornering throttle feathering / braking when `|deltaTheta| > 0.65` rad and dynamic rubber-banding scaling top speed by $\pm 10-15\%$ based on $\Delta D = D_{player} - D_{AI}$ relative to 150 units.
- Fixed item targeting: Green Shell launched into `activeProjectiles` with forward impulse, bouncing off borders and colliding with AI opponents; Blooper cleans player windshield (`inkTimer = 0.0`) while debuffing all AI rivals; dropped bananas grant 1.2s owner immunity.
- Integrated AI karts and shells into `renderSprites()` unified `renderList` with depth sorting and grounding contact shadows.

## Artifact Index
- DISPATCH.md — Task assignment
- BRIEFING.md — Persistent context & state
- progress.md — Liveness heartbeat and step tracking
- handoff.md — Final handoff report

## Change Tracker
- **Files modified**:
  - `game_canvas.h`: Added persistent waypoint arrays, accessors, AI and projectile helper declarations, `KartEntity` fields (`isAI`, `currentWaypointIndex`, `lookaheadDist`, `totalRaceDistance`), `ShellProjectile` alias, `WorldSprite` owner/immunity fields.
  - `game_canvas.cpp`: Implemented `initTrackWaypoints()` with exact 16/20/24 waypoints, `updateAIOpponents()` kinematics/steering/rubber-banding/terrain-sampling/debuff loop, Green Shell projectile launch and wall bouncing, Blooper opponent targeting fix, banana safe drop and AI collision, 3D billboard projection for AI opponents and projectiles in `renderSprites()`.
- **Build status**: PASS (Desktop Qt 6.11.1 MinGW 64-bit, 0 errors, 0 warnings)
- **Pending issues**: None

## Quality Status
- **Build/test result**: PASS (182/182 tests pass, 100%, 1399 assertions)
- **Lint status**: 0 violations
- **Tests added/modified**: Full suite passing; verified F3, F4, F5, F6, F7, F8 coverage.

## Loaded Skills
None
