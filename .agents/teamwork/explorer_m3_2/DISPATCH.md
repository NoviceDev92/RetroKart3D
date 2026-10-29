## 2026-10-29T05:10:15Z
You are explorer_m3_2, an Explorer for Milestone 3 (Interactive Track Hazards & Terrain Fix).
Your working directory is: d:\Qt\RetroKart3D\.agents\teamwork\explorer_m3_2\
Workspace directory: d:\Qt\RetroKart3D

MANDATORY: Read the original user request first:
d:\Qt\RetroKart3D\.agents\teamwork\ORIGINAL_REQUEST.md

Also read:
d:\Qt\RetroKart3D\.agents\teamwork\PROJECT.md
d:\Qt\RetroKart3D\.agents\teamwork\auditor_m2_1\handoff.md
d:\Qt\RetroKart3D\.agents\teamwork\TEST_READY.md

Your task is to explore and produce a detailed implementation strategy for:
1. Interactive Track Hazards:
   - Bowser Castle: Animated spewing lava geysers placed in the molten lava lakes along the track. Cyclic eruption state machine (dormant bubbling -> erupting fire column -> receding smoke) with height-aware collision (hitting erupting geyser spins the kart out).
   - Choco Valley: Animated chocolate fountains in the mud pools and rolling chocolate candy obstacles along the curves.
   - Procedural 32x32 sprites in SpriteAtlas: LAVA_GEYSER (bubbling & erupting frames), CHOCOLATE_FOUNTAIN, CHOCOLATE_CANDY.
   - 3D Billboard projection of hazards in renderSprites().
2. CRITICAL AI Terrain Mask Fix:
   - In game_canvas.cpp:1167, the condition `if (qRed(maskColor) == 255 && qBlue(maskColor) == 255)` evaluates to true for White asphalt (RGB 255, 255, 255).
   - Fix: Check White Tarmac first (just as player physics does at line 902), or require `qGreen(maskColor) == 0` for Molten Lava. This ensures AI opponents accelerate and race freely along the course instead of being permanently stuck in a lava spinout loop.
3. Deliver exact code recommendations, animation formulas, and collision handling.
Write your findings to d:\Qt\RetroKart3D\.agents\teamwork\explorer_m3_2\analysis.md and complete handoff.md.
Remember: DO NOT modify any source code files. You are an Explorer. Keep progress.md updated.
