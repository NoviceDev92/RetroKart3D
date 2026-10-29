# Task Assignment — challenger_m3_1 (Adversarial Challenger 1)

## Role & Mission
You are Adversarial Challenger 1 for Milestone 3.
Your mission is to empirically stress-test and challenge the **Arcade Driving Physics, Drift & Mini-Turbo** implementation.

## Authoritative Requirements & Inputs
- `D:/Qt/RetroKart3D/.agents/teamwork/ORIGINAL_REQUEST.md` (MANDATORY: read this first!)
- `D:/Qt/RetroKart3D/.agents/teamwork/PROJECT.md`
- `D:/Qt/RetroKart3D/.agents/teamwork/worker_m3/handoff.md`

## Adversarial Stress Vectors
1. **Drift Mechanics Under Extreme Inputs**:
   - Rapid drift toggle / key hammering (prevent state corruption or infinite boost).
   - High-speed reverse into drift (ensure drift activates only when moving forward at speed $\ge 40.0$).
   - Drift hold duration: verify threshold transitions at exactly $0.5\text{s}$ (blue sparks) and $1.0\text{s}$ (orange sparks).
   - Mini-turbo boost: verify surge awards $+25.0$ impulse, $1.35\times$ speed cap, and expires cleanly after $1.2\text{s}$.
2. **Braking & Grip Feel**:
   - Full braking from top speed to full stop: verify deceleration rate ($240.0\text{ u/s}^2$).
   - Reverse speed clamping at $-30\%$ of character `maxSpeed`.
3. **Character Scaling Invariants**:
   - Test Bowser vs Toad: Bowser has lower turn rate/drift factor, Toad has higher agility. Ensure invariant $turnRate_{Toad} > turnRate_{Bowser}$.

## Output Requirements
Build or run test harnesses using Qt 6.11.1 MinGW.
Write `handoff.md` in `D:/Qt/RetroKart3D/.agents/teamwork/challenger_m3_1/handoff.md`.
Include test results, assertion counts, and explicit verdict: `VERDICT: APPROVE` or `VERDICT: REQUEST_CHANGES`.
Send a completion message to orchestrator via `send_message`.


## 2026-10-29T06:00:07Z
You are challenger_m3_1. Your working directory is D:/Qt/RetroKart3D/.agents/teamwork/challenger_m3_1/.
Read your assignment at D:/Qt/RetroKart3D/.agents/teamwork/challenger_m3_1/DISPATCH.md.
Also read D:/Qt/RetroKart3D/.agents/teamwork/ORIGINAL_REQUEST.md, D:/Qt/RetroKart3D/.agents/teamwork/PROJECT.md, and D:/Qt/RetroKart3D/.agents/teamwork/worker_m3/handoff.md.
Empirically stress-test and challenge Milestone 3 arcade driving physics, high-grip braking, drift hold duration, and mini-turbo boost surge across characters.
Write your report to D:/Qt/RetroKart3D/.agents/teamwork/challenger_m3_1/handoff.md with a clear VERDICT (APPROVE or REQUEST_CHANGES).
Send a message to orchestrator (conversation ID: 842ff862-512e-4d57-af04-61c29631cec0) when complete.
