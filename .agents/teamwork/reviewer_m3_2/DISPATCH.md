# Task Assignment — reviewer_m3_2 (Reviewer 2)

## Role & Mission
You are Reviewer 2 for Milestone 3 (Interactive Track Hazards, 72x72 HUD Minimap, AI Terrain & Navigation Polish).
Evaluate the implementation in `game_canvas.h`, `game_canvas.cpp`, `sprite_atlas.h`, `sprite_atlas.cpp` against requirements.

## Authoritative Requirements & Inputs
- `D:/Qt/RetroKart3D/.agents/teamwork/ORIGINAL_REQUEST.md` (MANDATORY: read this first!)
- `D:/Qt/RetroKart3D/.agents/teamwork/PROJECT.md`
- `D:/Qt/RetroKart3D/.agents/teamwork/worker_m3/handoff.md`
- `D:/Qt/RetroKart3D/.agents/teamwork/explorer_m3_2/analysis.md`
- `D:/Qt/RetroKart3D/.agents/teamwork/explorer_m3_3/analysis.md`
- `D:/Qt/RetroKart3D/.agents/teamwork/auditor_m2_1/handoff.md`

## Review Focus
1. **Interactive Track Hazards (F10, F11)**:
   - Bowser Castle lava geysers: state machine ($4.8\text{s}$ cycle: $3.3\text{s}$ bubbling, $1.5\text{s}$ erupting). Erupting contact causes $0.8\text{s}$ fire spinout + $15.0$ radial impulse.
   - Choco Valley: chocolate fountains (50% mud slow) and rolling candies ($\pm 40.0$ bounce recoil).
   - 3D billboard projection into Mode-7 rasterizer with Painter's depth sorting and drop shadows.
2. **Top-Left HUD Minimap (F12)**:
   - Position $(X=8, Y=8)$, size $72\times 72$, translucent black backing.
   - Cached track outline in `QImage` for high-performance rendering.
   - Pulsing Player 1 blip and distinct chassis-colored AI opponent dots.
3. **AI Terrain Mask Disambiguation & Polish**:
   - Verify `game_canvas.cpp:1167` evaluates White tarmac before Lava, eliminating the frame-1 recoil loop.
   - Verify continuous ink timer decay and corner apex lookahead navigation.
4. **Build & Test Verification**:
   - Verify build passes with Qt 6.11.1 MinGW.
   - Run `tests\build\release\test_runner.exe` and `tests\build_forensic\release\standalone_m2_forensic.exe`.

## Output Requirements
Write `handoff.md` in `D:/Qt/RetroKart3D/.agents/teamwork/reviewer_m3_2/handoff.md`.
State your verdict explicitly: `VERDICT: APPROVE` or `VERDICT: REQUEST_CHANGES`.
Send a completion message to orchestrator via `send_message`.


## 2026-10-29T06:00:03Z
[Message] timestamp=2026-10-29T06:00:03Z sender=842ff862-512e-4d57-af04-61c29631cec0 priority=MESSAGE_PRIORITY_HIGH content=You are reviewer_m3_2. Your working directory is D:/Qt/RetroKart3D/.agents/teamwork/reviewer_m3_2/.
Read your assignment at D:/Qt/RetroKart3D/.agents/teamwork/reviewer_m3_2/DISPATCH.md.
Also read D:/Qt/RetroKart3D/.agents/teamwork/ORIGINAL_REQUEST.md, D:/Qt/RetroKart3D/.agents/teamwork/PROJECT.md, and D:/Qt/RetroKart3D/.agents/teamwork/worker_m3/handoff.md.
Perform a thorough code review of Milestone 3 track hazards, 72x72 HUD minimap, and AI terrain disambiguation & cornering navigation. Verify builds and test runners.
Write your report to D:/Qt/RetroKart3D/.agents/teamwork/reviewer_m3_2/handoff.md with a clear VERDICT (APPROVE or REQUEST_CHANGES).
Send a message to orchestrator (conversation ID: 842ff862-512e-4d57-af04-61c29631cec0) when complete.
