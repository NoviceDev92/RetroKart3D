## 2026-10-29T04:29:01Z
You are explorer_m2_3, an Explorer for Milestone 2: AI Opponents and Item Targeting Fixes.
Your working directory is: d:\Qt\RetroKart3D\.agents\teamwork\explorer_m2_3\
Workspace directory: d:\Qt\RetroKart3D

MANDATORY: Read the original user request first:
d:\Qt\RetroKart3D\.agents\teamwork\ORIGINAL_REQUEST.md

Also read:
d:\Qt\RetroKart3D\.agents\teamwork\PROJECT.md
d:\Qt\RetroKart3D\.agents\teamwork\explorer_survey_2\survey_report.md
d:\Qt\RetroKart3D\.agents\teamwork\TEST_READY.md

Your task is to explore and produce a detailed implementation strategy for fixing item targeting so items affect opponents rather than the player:
1. Green Shell Projectile:
   - Replace static billboard spawning in game_canvas.cpp (lines 593-600) with dynamic ShellProjectile structures:
     x, z, vx, vz, lifeTime, ownerId, active.
   - In updateItemSystem(dt): integrate position along velocity vector (v approx 220.0), bounce off walls, check distance against AI opponents (radius approx 16.0).
   - On collision with an AI opponent: deactivate shell, trigger spinout debuff on opponent (ai.spinoutTimer = 1.2), spawn burst effect.
2. Ink Blooper Targeting Fix:
   - In game_canvas.cpp line 603, inkTimer = 4.5 is set on the player themselves, blinding the user.
   - Fix: Player windshield remains completely clean (inkTimer = 0.0). Instead, apply ink blindness and speed debuff (40% speed reduction and spin/ink timer) to all active AI opponents (or Player 2 in multiplayer).
3. Banana Safe Drop and Opponent Collision:
   - Fix: Add owner immunity timer / grace distance on dropped bananas so turning or braking immediately after dropping does not self-inflict a spinout on the player.
   - Detect collision between dropped bananas and AI opponents: when an AI opponent drives over a banana, remove the banana and trigger spinout on the AI opponent.
4. Deliver exact code recommendations, mathematical collision formulas, and interface contracts.
Write your findings to d:\Qt\RetroKart3D\.agents\teamwork\explorer_m2_3\analysis.md and complete handoff.md.
Remember: DO NOT modify any source code files. You are an Explorer. Keep progress.md updated.
