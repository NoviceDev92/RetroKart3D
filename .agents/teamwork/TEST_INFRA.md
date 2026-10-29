# RetroKart 3D: Test Infrastructure & Architecture Specification

**Author**: `test_writer_e2e` (E2E Test Suite Creator)  
**Date**: 2026-10-29  
**Platform**: Desktop Qt 6.11.1 MinGW 64-bit on Windows  
**Workspace**: `d:\Qt\RetroKart3D`  

---

## 1. Executive Overview

This document specifies the comprehensive End-to-End (E2E) testing architecture and automated test runner for **RetroKart 3D**. Built upon the core requirements **R1 through R5** and the 16 features cataloged in `PROJECT.md`, this test infrastructure provides an opaque-box, requirement-driven verification track that executes independently of manual GUI interactions.

The test suite is structured around a rigorous **4-Tier Testing Methodology**:
1. **Tier 1: Feature Coverage** (>=5 test cases per feature across all 16 features, totaling 80 tests).
2. **Tier 2: Boundary & Corner Cases** (>=5 test cases per boundary domain across all 16 features, totaling 80 tests).
3. **Tier 3: Cross-Feature Interactions** (16 multi-system scenario tests).
4. **Tier 4: Real-World Application Scenarios** (6 comprehensive race simulations, menu workflows, multiplayer sessions, and hazard obstacle courses).

**Total Test Suite Volume**: **182 distinct automated test cases** with over **300 fine-grained assertions**.

---

## 2. 4-Tier Testing Methodology

```
+-----------------------------------------------------------------------------------+
|                           RETROKART 3D TEST MATRIX                                |
+-----------------------------------------------------------------------------------+
| Tier 4: Real-World Scenarios  | Full GP Races (Donut, Choco, Bowser), 2P Splits   |
| Tier 3: Cross-Feature Tests   | Drift + Items, Blooper + SplitScreen, Hazards + AI|
| Tier 2: Boundary & Corner     | Coordinate wrap, grace timers, drift charge, dt   |
| Tier 1: Feature Coverage      | F1 to F16 (>=5 test cases per feature)            |
+-----------------------------------------------------------------------------------+
```

### Tier 1: Feature Coverage (F1 - F16)
Every single feature from `PROJECT.md` is covered by at least 5 dedicated unit and integration tests:
- **F1: Title & Main Menu Flow**: Initial state, cursor navigation, boundary wrapping, mode confirmation, back action.
- **F2: Character Selection & Roster**: 4-character roster (Mario, Bowser, Peach, Toad), top speed & acceleration differentiation, handling turn rates, unique kart colors, P2 alternate palette tint on duplicate choice.
- **F3: Multi-Kart Entity Architecture**: KartEntity model, multi-kart collection storage, character profile stat binding, status debuff flags, kinematics integration.
- **F4: Persistent Track Waypoints**: Waypoint loops for Donut Plains (16 wps), Choco Valley (20 wps), Bowser Castle (24 wps), closed loop length accumulation, distance along track progression.
- **F5: Active AI Opponent Fleet**: 3 active AI rivals, lookahead waypoint pursuit, steering rate-limiting, hairpin throttle modulation, rubber-banding catch-up boost.
- **F6: Green Shell Projectile**: Forward launch velocity ($v + 75.0$), lifetime decay, wall bounce reflection, 3rd bounce destruction, AI hit detection and spinout.
- **F7: Ink Blooper Targeting Fix**: Player immunity (zero self-blinding), opponent debuffing, 30% top speed reduction, 4.0s timer cleanup, AI firing against player.
- **F8: Banana Placement & Safety**: Spawn behind kart, 1.2s owner immunity grace period, trailing opponent spinout, immediate consumption on hit, vulnerability after grace expiration.
- **F9: Arcade Drift & Handling**: WASD throttle curve, high-grip braking and -30% reverse clamp, drift speed threshold (>40.0 u/s), cornering turn rate multiplier ($1.35\times$), 1.2s mini-turbo boost award after $\ge 1.0s$ drift.
- **F10: Bowser Lava Geysers**: 4 lava geysers, 4.8s eruption cycle (Dormant 2.5s $\to$ Warning 0.8s $\to$ Spewing 1.5s), safe passage during dormancy, fiery spinout during spewing active, radial pushback impulse.
- **F11: Choco Chocolate Fountains & Candies**: 3 chocolate fountains + 2 rolling candies, mud splash 50% speed penalty, sinusoidal candy motion, pinball bumper bounce (-35.0 speed recoil).
- **F12: Top-Left HUD Minimap**: 72x72 radar box at (8, 8), world $(0..1024) \to$ radar $(8..80)$ coordinate translation, modulo boundary wrapping, color-coded blips, HUD bounds clipping.
- **F13: Local 2-Player Split-Screen**: 400x300 horizontal split, Top Viewport ($y \in [0, 149]$), Golden Divider ($y \in [150, 151]$), Bottom Viewport ($y \in [152, 300]$), zero cross-bleeding.
- **F14: Independent Dual Controls**: P1 (WASD + Space + Shift) and P2 (Arrows + Enter + Control) simultaneous key parsing, opposing steering cancellation, zero key ghosting.
- **F15: Build Stability & 60 FPS**: Delta time clamping ($\le 0.05s$), Mode-7 ground pixel budget invariance (60,000 pixels), SpriteAtlas pickup and obstacle classification, 1000-tick performance benchmark (<50ms).
- **F16: E2E Acceptance Suite**: Full traceability verifying R1, R2, R3, R4, R5 acceptance criteria.

### Tier 2: Boundary & Corner Cases
Targets boundary conditions, numerical extremes, and edge cases:
- Coordinate wrapping across $1024.0 \leftrightarrow 0.0$ and negative coordinates.
- Menu navigation wrap under extreme input bursts (+100, -100 steps).
- Character ID fallback handling ($<0$ or $>3$ falls back cleanly to default character).
- Shell wall bounce grazing angles and exact 3-bounce termination.
- Shell and banana collision radii exact thresholds ($14.0$ and $12.0$ units).
- Banana grace period transition at exactly $1.200s$.
- Drift mini-turbo threshold boundary ($0.99s$ fails vs $1.01s$ awards mini-turbo).
- Lava geyser phase transitions ($3.29s$ warning vs $3.31s$ erupting).
- Split-screen boundary lines ($y=149, 150, 151, 152$).
- Frame delta time hitch clamping at $0.050s$ up to $2.0s$ lag spikes.

### Tier 3: Cross-Feature Interactions
Validates systemic integration across multiple interacting subsystems:
- Menu selection $\to$ character selection $\to$ kart entity stat initialization.
- Drifting around waypoint curves while deploying bananas.
- Green shell colliding with an opponent currently drifting or mini-turbo boosted.
- Blooper fired in 2-player split screen: P1 clean, P2 and AI blinded.
- Lava geyser eruption while kart is already in spinout from a banana peel.
- Chocolate fountain mud speed penalty during drift power-slide.
- AI rubber-banding response when player gets a mini-turbo boost.
- Asymmetric split-screen race finish (P1 finishes, P2 still active).
- Heavyweight vs lightweight kart collision momentum transfer.

### Tier 4: Real-World Application Scenarios
Simulates complete real-world sessions:
- **Scenario 1**: Full 600-frame Grand Prix on Donut Plains with 4 karts, dynamic lap distance tracking, and race placement sorting.
- **Scenario 2**: Choco Valley race with active chocolate mud fountains and rolling candy obstacles.
- **Scenario 3**: Bowser Castle race with multiple 4.8s cyclic geyser eruptions.
- **Scenario 4**: Full menu-to-race end-to-end user flow: Title $\to$ Mode $\to$ Character Select with P2 duplicate color resolution $\to$ Race $\to$ Finish $\to$ Title.
- **Scenario 5**: Local 2-Player competitive head-to-head race with independent input streams and item warfare.
- **Scenario 6**: Extreme hazard bombardment stress test (multiple shells, bananas, geysers, 8 karts).

---

## 3. Directory Layout & Architecture

```
d:\Qt\RetroKart3D\tests\
├── tests.pro                  # QMake project file for Desktop Qt 6.11.1 MinGW 64-bit
├── main.cpp                   # Automated test runner with CLI argument parsing
├── test_framework.h           # Assertion macros, test context, and scorecard printer
├── test_types.h               # Authoritative interface contracts (PROJECT.md & R1-R5)
├── test_harness.h             # Controller, Roster, Physics, AI, Items, Hazard engines
├── test_harness.cpp           # Simulation engine implementations
├── test_tier1_features.h      # Tier 1 header
├── test_tier1_features.cpp    # 80 feature coverage test cases
├── test_tier2_boundaries.h    # Tier 2 header
├── test_tier2_boundaries.cpp  # 80 boundary and corner test cases
├── test_tier3_interactions.h  # Tier 3 header
├── test_tier3_interactions.cpp# 16 cross-feature interaction scenarios
├── test_tier4_realworld.h     # Tier 4 header
├── test_tier4_realworld.cpp   # 6 real-world simulation scenarios
└── build\                     # Compilation build directory
    ├── Makefile
    ├── test_runner.exe        # Compiled test runner executable
    └── ...
```

---

## 4. How to Build and Run

### Command to Build:
```powershell
$env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH
New-Item -ItemType Directory -Force -Path "d:\Qt\RetroKart3D\tests\build"
Set-Location "d:\Qt\RetroKart3D\tests\build"
& "D:\Qt\6.11.1\mingw_64\bin\qmake.exe" ..\tests.pro
& "D:\Qt\Tools\mingw1310_64\bin\mingw32-make.exe" -j4
```

### Command to Run Full Test Suite:
```powershell
& "d:\Qt\RetroKart3D\tests\build\test_runner.exe"
```

### Command to Run Specific Tiers:
```powershell
& "d:\Qt\RetroKart3D\tests\build\test_runner.exe" --tier=1   # Feature Coverage only
& "d:\Qt\RetroKart3D\tests\build\test_runner.exe" --tier=2   # Boundary Cases only
& "d:\Qt\RetroKart3D\tests\build\test_runner.exe" --tier=3   # Cross-Feature only
& "d:\Qt\RetroKart3D\tests\build\test_runner.exe" --tier=4   # Real-World Simulations
```
