## 2026-10-29T04:26:21Z
You are explorer_m2_2, an Explorer for Milestone 2: AI Opponents & Item Targeting Fixes (AI Fleet & Rendering).
Your working directory is: d:\Qt\RetroKart3D\.agents\teamwork\explorer_m2_2\
Workspace directory: d:\Qt\RetroKart3D

MANDATORY: Read the original user request first:
d:\Qt\RetroKart3D\.agents\teamwork\ORIGINAL_REQUEST.md

Also read:
d:\Qt\RetroKart3D\.agents\teamwork\PROJECT.md
d:\Qt\RetroKart3D\.agents\teamwork\explorer_survey_2\survey_report.md
d:\Qt\RetroKart3D\.agents\teamwork\TEST_READY.md

Your task is to explore and produce a detailed implementation strategy for:
1. AI Opponent Fleet Simulation:
   - In `PLAY_VS_AI` mode, ensure 3 active AI opponents are spawned at race start on staggered grid positions behind the player ($X_i = 470 - i \cdot 18$).
   - Implement `updateAIOpponents(dt)`: integrate velocity along AI heading, clamp to character `maxSpeed`, handle terrain slowdown if off-road, handle spinout timers if hit by an item/hazard.
   - Rubber-banding / speed regulation: subtle speed scaling ($\pm 10-15\%$) based on distance to player so the race stays closely contested and exciting.
2. AI Opponent 3D Billboard Rendering:
   - How `renderSprites()` in `game_canvas.cpp` projects and renders the 3 AI karts using their respective character billboard sprites (`KART_MARIO`, `KART_BOWSER`, `KART_PEACH`, `KART_TOAD` from `SpriteAtlas`).
   - Perspective depth calculation, billboard scaling, horizon clipping, and sorting by camera distance.
3. Deliver exact code recommendations and integration steps.
Write your findings to `d:\Qt\RetroKart3D\.agents\teamwork\explorer_m2_2\analysis.md` and complete `handoff.md`.
Remember: DO NOT modify any source code files. You are an Explorer. Keep progress.md updated.
