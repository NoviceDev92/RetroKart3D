## 2026-10-29T03:52:20Z
You are test_writer_e2e, the E2E Test Suite Creator.
Your working directory is: d:\Qt\RetroKart3D\.agents\teamwork\test_writer_e2e\
Workspace directory: d:\Qt\RetroKart3D

MANDATORY: Read the original user request first:
d:\Qt\RetroKart3D\.agents\teamwork\ORIGINAL_REQUEST.md

Also read:
d:\Qt\RetroKart3D\.agents\teamwork\PROJECT.md
d:\Qt\RetroKart3D\.agents\teamwork\spec_miner_survey_3\spec_report.md

Your mission is to establish the E2E Testing Track for RetroKart3D based on requirements R1 through R5:
1. Design an opaque-box, requirement-driven test suite following the 4-tier methodology:
   - Tier 1: Feature Coverage (>=5 test cases per feature for all 16 features in PROJECT.md)
   - Tier 2: Boundary & Corner cases (>=5 test cases per feature where boundaries exist)
   - Tier 3: Cross-Feature interaction scenarios
   - Tier 4: Real-World Application scenarios (full race simulations, menu-to-race flows, multiplayer races, hazard collision sequences)
2. Build an automated test runner:
   - Create a clean test harness (e.g., in a dedicated `tests/` directory with `.pro` file or standalone C++ / Qt test executable that can be compiled with Desktop Qt 6.11.1 MinGW 64-bit or executed to systematically assert each test case).
   - Test cases should verify:
     - GameState transitions (Title -> Mode Selection -> Character Selection -> Race -> Finished)
     - Character stats (Mario, Bowser, Peach, Toad differences in speed, acceleration, handling)
     - AI opponent population, waypoint progression, lookahead steering, rubber-banding
     - Item targeting: Ink Blooper debuffing opponents/P2 (not self), Green Shell projectile movement & collision, Banana placement immunity & opponent spinout
     - Arcade physics: WASD acceleration, braking, drift hop/power-slide, mini-turbo boost timer
     - Hazards: Bowser Castle lava geyser eruption cycle and collision, Choco Valley chocolate fountains/candies
     - Minimap: 72x72 radar coordinates translation and blip rendering
     - Multiplayer: 2-player split-screen viewports and independent controls (WASD vs Arrows)
3. Create `d:\Qt\RetroKart3D\.agents\teamwork\TEST_INFRA.md` documenting test architecture and methodology.
4. When the test suite and runner are completely implemented and ready to execute, publish `d:\Qt\RetroKart3D\.agents\teamwork\TEST_READY.md` with:
   - Command to run the full test suite
   - Coverage summary table (Tiers 1-4)
   - Feature checklist
5. Write your handoff.md and notify the orchestrator via send_message.
Do NOT modify core game files (game_canvas.*, mainwindow.*, sprite_atlas.*) — write test files in tests/ and metadata in your directory. Keep progress.md updated.
