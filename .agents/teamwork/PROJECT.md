# Project: RetroKart3D

## Architecture
RetroKart3D is a Mode-7 pseudo-3D kart racer built in C++ using Qt 6.11.1 (Desktop MinGW 64-bit).
It employs software scanline inverse perspective mapping onto a 400x300 RGB32 frame buffer scaled to the Qt window.
The architecture consists of:
1. **Game State Management**:
   - `GameState`: `TITLE_MENU`, `CHARACTER_SELECT`, `RACING`, `RACE_FINISHED`.
   - Title/Menu controller handling mode selection: Play vs AI (Grand Prix), Multiplayer (2P Split-Screen), Practice (Solo Time Trial).
   - Character selection controller with 4 distinct characters (Mario/Standard, Bowser/Heavy, Peach/Light, Toad/Acceleration) with differing kart body colors and physics tuning attributes.
2. **Entity & Kart System**:
   - `KartEntity`: Unified struct/class encapsulating position (`x, z`), heading (`angle`), velocity (`speed`), acceleration, max speed, handling, drift state, mini-turbo charge, item inventory, and status debuffs (spinout, ink-blindness).
   - Multi-kart collection: Support for Player 1, Player 2, and AI opponents (3 opponent karts in Play vs AI mode, total 4 karts on track).
3. **AI Navigation & Waypoint System**:
   - Track spline centerlines retained as persistent waypoints (`std::vector<QPointF>`) for all 3 tracks: Donut Plains, Choco Valley, Bowser's Castle.
   - Lookahead waypoint pursuit, steering interpolation, and rubber-banding to keep AI karts competitive.
4. **Item & Projectile System**:
   - Items: Banana, Green Shell, Ink Blooper, Mushroom.
   - Dynamic projectiles (`ShellProjectile`) with velocity, trajectory, wall bounce, and target collision.
   - Fixed item targeting: Ink Blooper blinds AI/opponents, Green Shell moves forward to hit opponents, Banana includes owner immunity grace period.
5. **Physics & Arcade Handling (M3 Deliverable)**:
   - Responsive WASD arcade controls.
   - Dedicated drift mechanic (holding Shift/Space executes hop, enters power slide with lateral slip angle beta, charges mini-turbo boost sparks, and releases speed surge).
   - Braking feel and reverse logic.
   - Corrected terrain mask physics (White tarmac = 100% road, Green = off-road drag, Magenta = Molten Lava recoil).
6. **Track Hazards & Minimap (M3 Deliverable)**:
   - Bowser Castle: Animated spewing lava geyser columns with eruption cycle and kart collision hazard.
   - Choco Valley: Animated chocolate fountains and rolling chocolate candies.
   - HUD Minimap: 72x72 radar in top-left showing track outline and real-time kart position blips.

## Feature Inventory
| # | Feature | Description | Milestone | Source |
|---|---------|-------------|-----------|--------|
| F1 | Title & Main Menu Flow | Startup title screen with options: Play vs AI, Multiplayer, Practice | M1 | R1 |
| F2 | Character Selection & Roster | Screen with 4 distinct characters, observable kart colors, and stats | M1 | R1 |
| F3 | Multi-Kart Entity Architecture | Unified KartEntity struct/model for multi-kart management | M2 | R2 |
| F4 | Persistent Track Waypoints | Waypoint loops for Donut Plains, Choco Valley, Bowser Castle | M2 | R2 |
| F5 | Active AI Opponent Fleet | 3 active AI opponent karts navigating courses with lookahead steering | M2 | R2 |
| F6 | Green Shell Projectile | Forward-moving projectile with velocity, collision, and opponent hit | M2 | R2 |
| F7 | Ink Blooper Targeting Fix | Blinds AI opponents (speed debuff/spin) or P2 instead of self-blinding | M2 | R2 |
| F8 | Banana Placement & Safety | Drop behind with owner immunity grace period to prevent self-spinout | M2 | R2 |
| F9 | Arcade Drift & Handling | WASD arcade handling, drift hop, lateral slip, and mini-turbo | M3 | R4 |
| F10 | Bowser Lava Geysers | Spewing animated lava geysers with eruption timer and collision | M3 | R4 |
| F11 | Choco Chocolate Fountains | Animated chocolate fountains/candies hazard with collision | M3 | R4 |
| F12 | Top-Left HUD Minimap | Real-time 72x72 radar showing track outline and kart dots | M3 | R4 |
| F13 | Build Stability & 60 FPS | Qt 6.11.1 MinGW 64-bit clean compilation, 0 warnings, 60 FPS | M3 | R5 |
| F14 | E2E Acceptance Suite | Opaque-box test suite verifying showcase requirements | M3 | R5 |

## Milestones
| # | Name | Scope | Dependencies | Status |
|---|------|-------|-------------|--------|
| M1 | Game State Flow & Character Selection | F1, F2 (Menu states, navigation, character roster & stat scaling) | none | DONE |
| M2 | AI Opponents & Item Targeting Fixes | F3, F4, F5, F6, F7, F8 (Multi-kart entity, waypoints, AI fleet, projectile shell, Blooper & Banana fixes) | M1 | MERGED_INTO_M3 |
| M3 | Showcase Deliverable: Physics, Drift, Hazards, Minimap & AI Polish | F9, F10, F11, F12, F13, F14 (Arcade drift/mini-turbo, lava geysers, chocolate fountains, HUD minimap, terrain mask fix for active AI racing) | M1 | IN_PROGRESS |

*User Directive 2026-10-29T05:08:37Z: Milestone 3 serves as the final showcase deliverable for today. Complete M3 implementation and verification gate in full, then stop.*

## Interface Contracts
### Drift & Handling
- `bool isDrifting`: Activated when turning and holding drift key (Shift or Space).
- `double driftTimer`: Charges mini-turbo sparks (0.0 to 1.5s).
- `double miniTurboTimer`: Released on drift exit to grant 1.25x speed boost for 0.8s.
- `double lateralSlip`: Sideways slide angle $\beta$ modifying velocity vector during power slide.

### Track Hazards & Minimap
- `enum class HazardType { NONE, LAVA_GEYSER, CHOCOLATE_FOUNTAIN, CHOCOLATE_CANDY };`
- `struct TrackHazard { double x, z; HazardType type; double animTimer; bool isErupting; double radius; };`
- Minimap: 72x72 top-left radar with track outline thumbnail, player white dot, and 3 colored dots for AI opponents.

## Code Layout
- `game_canvas.h`, `game_canvas.cpp`: Core canvas, Mode-7 renderer, state machine, physics & drift mechanics, track hazards, minimap HUD, AI opponent updates.
- `sprite_atlas.h`, `sprite_atlas.cpp`: Procedural sprite generators for karts, shells, bananas, lava geysers, chocolate fountains/candies, minimap blips.
- `mainwindow.h`, `mainwindow.cpp`: Qt window container.
- `RetroKart3D.pro`: Project build file.
