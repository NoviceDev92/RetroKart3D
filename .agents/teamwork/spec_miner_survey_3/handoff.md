# Handoff Report: Specification Mining (Milestone 0)
**Agent**: `spec_miner_survey_3`  
**To**: Orchestrator (`842ff862-512e-4d57-af04-61c29631cec0`)  
**Date**: 2026-10-29  
**Deliverable**: `d:\Qt\RetroKart3D\.agents\teamwork\spec_miner_survey_3\spec_report.md`

---

### 1. Observation
1. **Authoritative Specification Inputs**:
   - `ORIGINAL_REQUEST.md`: Requires R1 (Title/Main menu: Play vs AI, Multiplayer, Practice; Character selection with distinct karts & stats), R2 (Active AI opponent fleet, Item targeting bug fixes for bananas, green shells, ink blooper), R3 (Multiplayer capability), R4 (WASD arcade handling, drift/braking, Bowser Castle spewing lava geysers, Choco Valley chocolate fountains/candies, top-left HUD minimap), R5 (Desktop Qt 6.11.1 MinGW 64-bit on Windows, 60 FPS, no crashes).
   - `PROJECT_FORMULATION.md`: Details Mode-7 scanline inverse perspective mapping, pinhole camera similar triangles ($Z = \frac{H \cdot f}{y - y_{\text{horizon}}}$), 3D billboard projection, near-plane clipping ($Z_{\text{cam}} > Z_{\text{near}}$), and dual-map spatial hashing.
2. **Existing Implementation Status (`game_canvas.h`, `game_canvas.cpp`)**:
   - No title menu or character selection exists; `MainWindow` directly instantiates `GameCanvas` which boots into Donut Plains (`game_canvas.cpp:25`).
   - Single player kart state exists (`kartX`, `kartZ`, `kartAngle`, `kartSpeed`); zero AI karts or opponent data structures exist (`game_canvas.h:83-94`).
   - Item targeting bugs verbatim observed:
     - Ink Blooper (`game_canvas.cpp:602-604`): `case PlayerItem::INK_BLOOPER: inkTimer = 4.5; break;` — directly blinds the player with `renderInkOverlay()`.
     - Green Shell (`game_canvas.cpp:593-600`): Firing places a static `WorldSprite` at fixed distance `fx, fz` without velocity, heading, or update trajectory, and collision logic ignores `SpriteType::GREEN_SHELL`.
     - Banana (`game_canvas.cpp:584-591`): Drops banana behind player, but lacks AI collision tests and lacks a grace period against immediate player self-collision.
3. **Build & Toolchain Environment**:
   - Toolchain confirmed: `D:\Qt\6.11.1\mingw_64\bin\qmake.exe` and `D:\Qt\Tools\mingw1310_64\bin\g++.exe`.
   - Build test: `d:\Qt\RetroKart3D\build\Desktop_Qt_6_11_1_MinGW_64_bit_Debug\Makefile` compiles cleanly with `mingw32-make` and zero errors.

---

### 2. Logic Chain
1. *Observation 1 & 2* reveal that the current codebase is an early single-player prototype lacking game state flow (menu/character select), multi-entity management (AI karts, multiplayer karts), and functional projectile kinematics.
2. In `game_canvas.cpp:602`, the assignment `inkTimer = 4.5;` on the player violates R2's requirement ("items affect opponents rather than the player"). The fix requires shifting this debuff to AI opponents while keeping the player windshield clear.
3. In `game_canvas.cpp:598`, pushing `SpriteType::GREEN_SHELL` into `worldSprites` without dynamic velocity violates R2. A dedicated `Projectile` struct and integration loop (`updateProjectiles`) is required to support high-velocity flight, wall bounces, and collision with AI karts.
4. For R3 (Multiplayer), evaluating Option A (Local 2P Split-Screen) vs Option B (LAN Sockets):
   - Local split-screen requires zero external networking, has zero firewall interference risk, and maintains identical Mode 7 pixel rendering load ($2 \times (400 \times 75) = 60,000$ pixels = single player load).
   - Therefore, Local Split-Screen with horizontal partition ($y=0..149$ for P1, $y=151..300$ for P2) is the optimal, robust architecture for today's showcase deliverable.
5. For R4 (Physics, Hazards, Minimap):
   - Arcade drift is implemented via lateral slip angle $\beta$ and mini-turbo timers when holding `Shift`.
   - Bowser Castle lava geysers and Choco Valley chocolate fountains/candies require animated billboard sprites with cyclic timers and proximity hazard triggers.
   - Minimap HUD requires a $72 \times 72$ overlay in the top-left corner translating world coordinates $(X, Z) \to (\text{miniX}, \text{miniY})$ with real-time kart blips.

---

### 3. Caveats
- **Audio/Sound Effects**: Neither `ORIGINAL_REQUEST.md` nor `PROJECT_FORMULATION.md` require audio; the visual arcade feedback (sparks, flashes, confetti, mud splatters) provides complete sensory feedback.
- **Networking**: While LAN sockets were considered, local 2-Player split-screen is selected as the primary deliverable to eliminate demo-day connection failure risks.

---

### 4. Conclusion
A complete, exhaustive specification report has been authored and saved to:
`d:\Qt\RetroKart3D\.agents\teamwork\spec_miner_survey_3\spec_report.md`
It details:
- 23 Discoverable Features mapped across R1, R2, R3, R4, R5.
- 20 Edge Cases with defined input/output behaviors.
- Deep architectural specifications for Game State Machine, Character Roster, AI Waypoint Steering, Item Targeting Bug Fixes, Local Split-Screen Multi-Viewport, Arcade Drift Physics, Animated Track Hazards, and Top-Left Minimap HUD.
- 13 Explicit Acceptance Criteria with direct verification methods.

---

### 5. Verification Method
1. Inspect the generated specification document:
   `d:\Qt\RetroKart3D\.agents\teamwork\spec_miner_survey_3\spec_report.md`
2. Verify existing compilation baseline:
   ```powershell
   $env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH
   cd d:\Qt\RetroKart3D\build\Desktop_Qt_6_11_1_MinGW_64_bit_Debug
   mingw32-make -j4
   ```
3. Test criteria check: Compare the acceptance test matrix in §6 of `spec_report.md` against requirements R1-R5 in `ORIGINAL_REQUEST.md`.
