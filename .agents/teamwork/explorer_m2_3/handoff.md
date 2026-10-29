# Handoff Report: Milestone 2 Item Targeting System Fixes

**Author:** `explorer_m2_3` (Explorer for AI Opponents and Item Targeting Fixes)  
**Date:** 2026-10-29  
**Recipient:** Orchestrator (`842ff862-512e-4d57-af04-61c29631cec0`)  
**Deliverable File:** `d:\Qt\RetroKart3D\.agents\teamwork\explorer_m2_3\analysis.md`

---

## 1. Observation

1. **Static Shell Billboard Spawning (`game_canvas.cpp:999-1006`)**:
   ```cpp
   case PlayerItem::GREEN_SHELL: {
       // Fire green shell forward
       double fireDist = 28.0;
       double fx = kartX + std::cos(kartAngle) * fireDist;
       double fz = kartZ + std::sin(kartAngle) * fireDist;
       worldSprites.push_back({ fx, fz, 0.0, SpriteType::GREEN_SHELL, true, 0.0 });
       break;
   }
   ```
   Green shells are appended to `worldSprites` as static objects without velocity (`vx, vz`), heading, lifetime, or owner tracking. They never move, never bounce, and are never checked in the collision loop (`game_canvas.cpp:880-910`).

2. **Self-Blinding Ink Blooper (`game_canvas.cpp:1008-1010`)**:
   ```cpp
   case PlayerItem::INK_BLOOPER:
       inkTimer = 4.5;
       break;
   ```
   `inkTimer` is a scalar member of `GameCanvas` governing the player's own viewport overlay. In `renderInkOverlay()` (`game_canvas.cpp:1406-1432`), 10 large black ink ellipses are drawn directly over the human player's screen, blinding the user who pressed Space. Opponents are completely unaffected.

3. **Unprotected Banana Drops (`game_canvas.cpp:990-997`)**:
   ```cpp
   case PlayerItem::BANANA: {
       // Drop a banana peel behind the player's kart
       double dropDist = 24.0;
       double bx = kartX - std::cos(kartAngle) * dropDist;
       double bz = kartZ - std::sin(kartAngle) * dropDist;
       worldSprites.push_back({ bx, bz, 0.0, SpriteType::BANANA_DROPPED, true, 0.0 });
       break;
   }
   ```
   `WorldSprite` lacks owner identification and immunity timers. In `updatePhysics(dt)` (`game_canvas.cpp:894-899`), driving within 18 units of any banana peel immediately triggers `isSpinning = true; spinTimer = 1.0; kartSpeed *= 0.35;`. Any reverse or sharp turn immediately spins the dropping player out. AI opponents never check collisions against bananas.

4. **Existing Architectural Contracts & Assets**:
   - `game_canvas.h:101-110` already defines `struct Projectile { double x, z, vx, vz; int bouncesLeft; bool active; int ownerId; double lifeTime; };` and member `std::vector<Projectile> activeProjectiles;`.
   - `game_canvas.h:86-90` already defines `KartEntity` fields: `bool isSpinning; double spinTimer; bool isInked; double inkTimer; double bananaGraceTimer;`.
   - `game_canvas.cpp:219-247` already instantiates 3 active `KartEntity` opponents in `std::vector<KartEntity> aiOpponents;`.
   - `sprite_atlas.h:30` and `sprite_atlas.cpp:500-514` already implement `SpriteType::SHELL_PROJECTILE` with dynamic motion speed lines.
   - `tests/test_tier1_features.cpp:310-550` contains 15 verified unit tests for F6, F7, and F8 with exact assertions matching these specifications.

---

## 2. Logic Chain

1. **Green Shell Dynamic Projectile (Observation 1 & 4)**:
   - By replacing `worldSprites.push_back` with `activeProjectiles.push_back({ fx, fz, vx, vz, ownerId, active, lifeTime, bouncesLeft })`, shells become dynamic entities.
   - Launching along heading at $\max(180.0, \text{kartSpeed} + 75.0)\text{ u/s}$ ensures the shell travels faster than any kart.
   - Integrating $\mathbf{p} \mathrel{+}= \mathbf{v}\Delta t$ and inverting $v_x$ or $v_z$ at track boundaries $[20.0, 1004.0]$ satisfies elastic wall reflection with 3 allowed bounces.
   - Proximity testing against `aiOpponents` ($d < 15.0\text{ units}$) triggers `ai.isSpinning = true`, `ai.spinTimer = 1.2`, and `ai.speed *= 0.35`, while setting `shell.active = false`.
   - Adding `activeProjectiles` projection in `renderSprites()` renders the moving shell in Mode 7 using `SpriteType::SHELL_PROJECTILE`.

2. **Ink Blooper Target Inversion (Observation 2 & 4)**:
   - Resetting `inkTimer = 0.0` for the user prevents self-blinding.
   - Iterating over `aiOpponents` and assigning `ai.isInked = true; ai.inkTimer = 4.0; ai.speed *= 0.70;` debuffs all active rivals.
   - Clamping AI maximum speed to $0.70 \times \text{maxSpeed}$ during the debuff creates an observable 30% performance penalty.
   - In 2P Split-Screen multiplayer, applying the ink overlay to Player 2's viewport fulfills the showcase requirement.

3. **Banana Safe Drop & AI Collision (Observation 3 & 4)**:
   - Adding `int ownerId = -1; double immunityTimer = 0.0;` to `WorldSprite` allows dropped traps to track ownership.
   - Initializing `immunityTimer = 1.2\text{ s}` when dropping behind the kart prevents self-collision if the player brakes or turns immediately.
   - Adding an AI collision pass over `worldSprites` triggers spinout (`ai.isSpinning = true; ai.spinTimer = 1.2; ai.speed *= 0.35;`) and consumes the banana peel (`sprite.active = false`), allowing human-placed traps to hinder opponents.

4. **Visual Feedback**:
   - Introducing lightweight `ImpactParticle` bursts on shell/banana hits creates authentic retro arcade hit reactions without compromising 60 FPS performance.

---

## 3. Caveats

1. **Multi-Kart AI Synchronization**: AI opponent movement kinematics are concurrently explored by `explorer_m2_1` and `explorer_m2_2`. The item targeting code interfaces with `aiOpponents` through standardized fields (`isSpinning`, `spinTimer`, `isInked`, `inkTimer`, `speed`, `maxSpeed`) already present in `KartEntity`.
2. **Subtle Wobble Tuning**: The $\pm 0.12\text{ rad}$ steering wobble during the inked state should be tuned so AI karts remain on track without careening into walls on Donut Plains.
3. **No Project Code Modification**: In strict compliance with read-only explorer guidelines, no project source files were altered during this investigation.

---

## 4. Conclusion

The item targeting system can be completely fixed and fully verified using existing data structures and procedural sprites without adding any external libraries.
- **Green Shell**: Dynamic forward projectile ($v \approx 220\text{ u/s}$), wall bouncing, AI collision radius $15.0$, spinout debuff $1.2\text{ s}$, 3D Mode-7 rendering.
- **Ink Blooper**: User windshield 100% clean, 30% top-speed debuff and 4.0s blindness applied to all active AI rivals.
- **Banana**: 1.2-second owner grace period preventing self-spinout, AI opponent collision with peel consumption and spinout debuff.
- **Impact Bursts**: 16-particle radial spark bursts on collision.

Full technical details, mathematical formulas, and replacement code snippets are documented in `d:\Qt\RetroKart3D\.agents\teamwork\explorer_m2_3\analysis.md`.

---

## 5. Verification Method

1. **Automated Unit & Boundary Test Suite**:
   Run the test runner targeting Tier 1 (Features F6, F7, F8) and Tier 2 (Boundaries):
   ```powershell
   $env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH
   & "d:\Qt\RetroKart3D\tests\build\release\test_runner.exe" --tier=1
   & "d:\Qt\RetroKart3D\tests\build\release\test_runner.exe" --tier=2
   ```
   Verify 100% pass rate across tests `F6.1`-`F6.5`, `F7.1`-`F7.5`, and `F8.1`-`F8.5`.

2. **Empirical In-Game Verification**:
   - Launch application and start Play vs AI on Donut Plains.
   - Collect an Item Box:
     - When receiving **Green Shell**, press Space: verify shell launches forward along kart heading, reflects off boundary walls, and triggers spinout and burst particles on AI opponent ahead.
     - When receiving **Ink Blooper**, press Space: verify player screen is completely clean, and leading/trailing AI karts visibly decelerate with wobble.
     - When receiving **Banana**, press Space and immediately brake or reverse: verify kart does NOT spin out. Allow trailing AI to drive over banana: verify banana disappears and AI spins out.
