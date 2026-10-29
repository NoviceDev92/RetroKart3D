# Task Assignment — challenger_m3_2 (Adversarial Challenger 2)

## Role & Mission
You are Adversarial Challenger 2 for Milestone 3.
Your mission is to empirically stress-test and challenge the **Interactive Track Hazards, 72x72 HUD Minimap, and AI Lap Completion**.

## Authoritative Requirements & Inputs
- `D:/Qt/RetroKart3D/.agents/teamwork/ORIGINAL_REQUEST.md` (MANDATORY: read this first!)
- `D:/Qt/RetroKart3D/.agents/teamwork/PROJECT.md`
- `D:/Qt/RetroKart3D/.agents/teamwork/worker_m3/handoff.md`

## Adversarial Stress Vectors
1. **Interactive Hazards Under Continuous Collision**:
   - Geyser eruption timing: verify $4.8\text{s}$ periodic cycle ($3.3\text{s}$ bubbling, $1.5\text{s}$ erupting).
   - Collision while bubbling (no spinout) vs while erupting ($0.8\text{s}$ fire spinout + $15.0$ radial impulse).
   - Chocolate fountain contact (mud slow down to $50\%$).
   - Rolling candies continuous oscillation and bounce recoil.
2. **Minimap Projection Accuracy**:
   - Verify coordinate mapping from world space $[0, 1024]\times [0, 1024]$ to minimap $[8, 80]\times [8, 80]$.
   - Ensure blips never overflow radar boundary.
   - Verify player blip pulses and AI blips reflect character colors.
3. **AI Navigation & Long-Duration Lap Completion**:
   - Stress-test AI fleet over 100+ waypoint advances across all 3 tracks (Donut Plains, Choco Valley, Bowser Castle).
   - Verify zero stalls, zero infinite lava spinouts on frame 1, and clean cornering on sharp hairpin bends.

## Output Requirements
Build or run test harnesses using Qt 6.11.1 MinGW.
Write `handoff.md` in `D:/Qt/RetroKart3D/.agents/teamwork/challenger_m3_2/handoff.md`.
Include test results, assertion counts, and explicit verdict: `VERDICT: APPROVE` or `VERDICT: REQUEST_CHANGES`.
Send a completion message to orchestrator via `send_message`.


## 2026-10-29T06:00:12Z
You are challenger_m3_2. Your working directory is D:/Qt/RetroKart3D/.agents/teamwork/challenger_m3_2/.
Read your assignment at D:/Qt/RetroKart3D/.agents/teamwork/challenger_m3_2/DISPATCH.md.
Also read D:/Qt/RetroKart3D/.agents/teamwork/ORIGINAL_REQUEST.md, D:/Qt/RetroKart3D/.agents/teamwork/PROJECT.md, and D:/Qt/RetroKart3D/.agents/teamwork/worker_m3/handoff.md.
Empirically stress-test and challenge Milestone 3 interactive hazards (Bowser geysers, Choco fountains & candies), 72x72 HUD minimap projection, and AI fleet long-duration lap completion.
Write your report to D:/Qt/RetroKart3D/.agents/teamwork/challenger_m3_2/handoff.md with a clear VERDICT (APPROVE or REQUEST_CHANGES).
Send a message to orchestrator (conversation ID: 842ff862-512e-4d57-af04-61c29631cec0) when complete.
