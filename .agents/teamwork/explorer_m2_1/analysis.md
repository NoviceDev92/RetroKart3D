# Milestone 2: AI Opponents & Waypoint Navigation Technical Analysis

**Author:** `explorer_m2_1` (Milestone 2 Explorer — Waypoints & AI Navigation)  
**Date:** 2026-10-29  
**Target:** RetroKart3D (`d:\Qt\RetroKart3D`)  
**Scope:** Track Waypoint Extraction & Storage, Lookahead Pursuit & Steering, Active AI Fleet Navigation, Interface Contracts  

---

## 1. Executive Summary

Milestone 2 elevates RetroKart3D from a single-player Mode-7 driving scaffold to a fully populated, competitive kart racing game. This investigation addresses the core AI navigation infrastructure:
1. **Track Waypoint Extraction & Storage**:
   - In `game_canvas.cpp`, `generateDonutPlains()`, `generateChocoValley()`, and `generateBowserCastle()` generate 1024×1024 track bitmaps using `QPainterPath` cubic Bézier splines and `QPolygon` raceways. Previously, these path definitions were local variables discarded at the end of track generation.
   - We specify a clean, persistent storage architecture (`std::vector<QPointF> trackWaypoints[3]`) populated during track initialization, providing exact closed-loop navigation curves (16 points for Donut Plains, 20 points for Choco Valley, 24 points for Bowser Castle).
   - We identify and reconcile a critical discrepancy between heuristic waypoints in early test scaffolding and the true Mode-7 rasterization mask coordinates to ensure AI opponents drive strictly on tarmac without false collisions into grass or lava.
2. **AI Kinematics & Lookahead Steering Algorithm**:
   - A lookahead Pure Pursuit controller is specified where each AI kart selects a lookahead waypoint ($W_{T} = (C + 2) \bmod N$), calculates the target heading $\theta_{\text{desired}} = \operatorname{atan2}(W_{Tz} - z, W_{Tx} - x)$, normalizes $\Delta\theta \in [-\pi, \pi]$, and smoothly interpolates its heading using its character-specific `turnRate` (2.20 to 3.40 rad/s).
   - Waypoint advancement uses a primary proximity threshold of $35.0$ units coupled with a forward-progress dot-product fail-safe.
   - Dynamic throttle modulation feathers throttle and brakes on sharp corners ($|\Delta\theta| > 0.65$ rad), while rubber-banding scales top speed by $\pm 10\%$ when distance from the player exceeds $150.0$ units.
3. **Mode-7 3D Billboard Projection**:
   - AI karts are projected onto the 400×300 Mode-7 screen using the existing camera transform and integrated into `renderSprites()`. Character-specific procedural 32×32 sprites from `SpriteAtlas` (`KART_MARIO`, `KART_BOWSER`, `KART_PEACH`, `KART_TOAD`) are depth-sorted alongside item boxes and obstacles.

---

## 2. Track Spline & Geometry Analysis

Tracks in RetroKart3D are defined in a 1024×1024 coordinate space using dual `QImage` buffers:
- `trackVisualMap`: RGB32 Mode-7 surface texture sampled per scanline.
- `trackMaskMap`: Collision and surface physics mask (White = Asphalt, Green = Offroad/Curb, Magenta = Molten Lava, Yellow = Checkpoint/Finish Line).

### 2.1 Track 1: Donut Plains (Level 0)
- **Path Construction (`game_canvas.cpp:498-502`)**:
  ```cpp
  QPainterPath path;
  path.moveTo(512, 200);
  path.cubicTo(850, 200, 850, 800, 512, 800);
  path.cubicTo(180, 800, 180, 200, 512, 200);
  ```
- **Road & Curb Geometry**:
  - Curb: Pen width 100 px (Red/White outer border).
  - Road: Pen width 84 px (Asphalt gray on visual, White `(255, 255, 255)` on mask). Half-width = 42 px.
  - Finish line: $X = 510..516, Z = 158..242$ (Yellow mask line at $X = 512$).
  - Halfway line: $Z > 650.0$.
  - Start line position: Player at $(470, 194)$, AI fleet staggered at $(470 - 18 \times i, 200 \pm 6)$, heading $\theta = 0.0$ (East).
- **Mathematical Spline Deconstruction**:
  The curve consists of two symmetric cubic Bézier curves:
  $$B(t) = (1-t)^3 P_0 + 3(1-t)^2 t P_1 + 3(1-t) t^2 P_2 + t^3 P_3, \quad t \in [0, 1]$$
  - Half 1 (East Turn): $P_0=(512, 200), P_1=(850, 200), P_2=(850, 800), P_3=(512, 800)$.
    Maximum $X$ occurs at $t = 0.5$:
    $$B_x(0.5) = \tfrac{1}{8}(512) + \tfrac{3}{8}(850) + \tfrac{3}{8}(850) + \tfrac{1}{8}(512) = 765.5$$
    $B_z(0.5) = 500.0$.
    *Note: The curve apex reaches $X = 765.5$, NOT $850$ or $880$. With road width 84, the asphalt spans $X \in [723.5, 807.5]$. Early heuristic waypoints at $X = 880$ were ~72 units into offroad grass!*
  - Half 2 (West Turn): $P_0=(512, 800), P_1=(180, 800), P_2=(180, 200), P_3=(512, 200)$.
    Minimum $X$ occurs at $t = 0.5$:
    $$B_x(0.5) = \tfrac{1}{8}(512) + \tfrac{3}{8}(180) + \tfrac{3}{8}(180) + \tfrac{1}{8}(512) = 263.0$$
    $B_z(0.5) = 500.0$. Asphalt spans $X \in [221.0, 305.0]$.

### 2.2 Track 2: Choco Valley (Level 1)
- **Path Construction (`game_canvas.cpp:568-574`)**:
  ```cpp
  QPainterPath path;
  path.moveTo(512, 200);
  path.cubicTo(880, 220, 820, 520, 680, 520);
  path.cubicTo(550, 520, 600, 820, 512, 820);
  path.cubicTo(200, 820, 300, 480, 200, 350);
  path.cubicTo(120, 220, 350, 200, 512, 200);
  ```
- **Road & Curb Geometry**:
  - Curb: Pen width 74 px (Brown dirt).
  - Road: Pen width 60 px (Dark dirt road, White on mask). Half-width = 30 px.
  - Finish line: $X = 508..516, Z = 170..230$.
  - Start line position: $(470, 194)$, heading $\theta = 0.0$.
- **Mathematical Spline Deconstruction**:
  Four continuous cubic Bézier segments with junction nodes at:
  - Node 0: $(512.0, 200.0)$ — Start/Finish line
  - Node 1: $(680.0, 520.0)$ — S-bend apex (coincides with Question Box at $680, 520$)
  - Node 2: $(512.0, 820.0)$ — South hairpin apex (coincides with Banana at $512, 820$, $Z > 650$ halfway)
  - Node 3: $(200.0, 350.0)$ — West curve apex (coincides with Question Box at $200, 350$)

### 2.3 Track 3: Bowser Castle (Level 2)
- **Path Construction (`game_canvas.cpp:621-628`)**:
  ```cpp
  QPolygon castlePoly;
  castlePoly << QPoint(250, 250)
             << QPoint(780, 250)
             << QPoint(780, 550)
             << QPoint(550, 550)
             << QPoint(550, 780)
             << QPoint(250, 780);
  ```
- **Road & Curb Geometry**:
  - Curb: Pen width 96 px (Dark stone shoulder, Green mask offroad buffer).
  - Road: Pen width 74 px (Stone tarmac raceway, White mask road). Half-width = 37 px.
  - Finish line: $X = 510..518, Z = 210..290$.
  - Molten Lava: Entire background is Magenta `(255, 0, 255)`. Touching lava causes instant sizzle recoil and spinout!
  - Start line position: $(470, 244)$, heading $\theta = 0.0$.
- **Perimeter & Alignment**:
  - Top Straight: $(250, 250) \to (780, 250)$ (Length 530 px, passes start/finish line at $X=512, Z=250$).
  - East Wall: $(780, 250) \to (780, 550)$ (Length 300 px, passes Thwomp at $X=810, Z=400$ leaving 30 px clearance).
  - Inset Horizontal: $(780, 550) \to (550, 550)$ (Length 230 px, passes Question Box at $X=650, Z=550$).
  - Inset Vertical: $(550, 550) \to (550, 780)$ (Length 230 px).
  - Bottom Straight: $(550, 780) \to (250, 780)$ (Length 300 px, passes Question Box at $X=400, Z=780$).
  - West Wall: $(250, 780) \to (250, 250)$ (Length 530 px, passes Green Pipe at $X=215, Z=600$ leaving 35 px clearance).
  - Total closed raceway perimeter = $2120.0$ px.
  *Note: Any waypoints outside the 74 px road (e.g. at $Z = 150$) hit molten lava and trigger recoil. The AI waypoints MUST trace this polygon exactly.*

---

## 3. Persistent Waypoint Extraction & Storage Architecture

### 3.1 Data Structures in `game_canvas.h`
Add persistent waypoint storage arrays and accessor to `GameCanvas`:
```cpp
// In class GameCanvas:
private:
    // Persistent waypoint loops per track (0: Donut Plains, 1: Choco Valley, 2: Bowser Castle)
    std::vector<QPointF> trackWaypoints[3];
    std::vector<QPointF> currentTrackWaypoints;

    void initTrackWaypoints();
    void extractDonutPlainsWaypoints();
    void extractChocoValleyWaypoints();
    void extractBowserCastleWaypoints();

public:
    const std::vector<QPointF>& getTrackWaypoints() const { return currentTrackWaypoints; }
```

### 3.2 Procedural Extraction Routines

#### A. Donut Plains Extraction (16 Waypoints)
```cpp
void GameCanvas::extractDonutPlainsWaypoints()
{
    std::vector<QPointF> &wps = trackWaypoints[static_cast<int>(TrackLevel::DONUT_PLAINS)];
    wps.clear();
    wps.reserve(16);

    auto evalBezier = [](double p0, double p1, double p2, double p3, double t) {
        double u = 1.0 - t;
        return u*u*u*p0 + 3.0*u*u*t*p1 + 3.0*u*t*t*p2 + t*t*t*p3;
    };

    // East curve: (512, 200) -> cubicTo(850, 200, 850, 800, 512, 800)
    for (int i = 0; i < 8; ++i) {
        double t = i / 8.0;
        wps.emplace_back(evalBezier(512.0, 850.0, 850.0, 512.0, t),
                         evalBezier(200.0, 200.0, 800.0, 800.0, t));
    }

    // West curve: (512, 800) -> cubicTo(180, 800, 180, 200, 512, 200)
    for (int i = 0; i < 8; ++i) {
        double t = i / 8.0;
        wps.emplace_back(evalBezier(512.0, 180.0, 180.0, 512.0, t),
                         evalBezier(800.0, 800.0, 200.0, 200.0, t));
    }
}
```

#### B. Choco Valley Extraction (20 Waypoints)
```cpp
void GameCanvas::extractChocoValleyWaypoints()
{
    std::vector<QPointF> &wps = trackWaypoints[static_cast<int>(TrackLevel::CHOCO_VALLEY)];
    wps.clear();
    wps.reserve(20);

    auto evalBezier = [](double p0, double p1, double p2, double p3, double t) {
        double u = 1.0 - t;
        return u*u*u*p0 + 3.0*u*u*t*p1 + 3.0*u*t*t*p2 + t*t*t*p3;
    };

    struct BezierSeg { double p0x, p0z, p1x, p1z, p2x, p2z, p3x, p3z; };
    const BezierSeg segs[4] = {
        { 512.0, 200.0, 880.0, 220.0, 820.0, 520.0, 680.0, 520.0 },
        { 680.0, 520.0, 550.0, 520.0, 600.0, 820.0, 512.0, 820.0 },
        { 512.0, 820.0, 200.0, 820.0, 300.0, 480.0, 200.0, 350.0 },
        { 200.0, 350.0, 120.0, 220.0, 350.0, 200.0, 512.0, 200.0 }
    };

    for (int s = 0; s < 4; ++s) {
        for (int i = 0; i < 5; ++i) {
            double t = i / 5.0;
            wps.emplace_back(evalBezier(segs[s].p0x, segs[s].p1x, segs[s].p2x, segs[s].p3x, t),
                             evalBezier(segs[s].p0z, segs[s].p1z, segs[s].p2z, segs[s].p3z, t));
        }
    }
}
```

#### C. Bowser Castle Extraction (24 Waypoints)
```cpp
void GameCanvas::extractBowserCastleWaypoints()
{
    std::vector<QPointF> &wps = trackWaypoints[static_cast<int>(TrackLevel::BOWSER_CASTLE)];
    wps.clear();
    wps.reserve(24);

    struct LineSeg { QPointF p1, p2; };
    const LineSeg segs[7] = {
        { QPointF(512.0, 250.0), QPointF(780.0, 250.0) },
        { QPointF(780.0, 250.0), QPointF(780.0, 550.0) },
        { QPointF(780.0, 550.0), QPointF(550.0, 550.0) },
        { QPointF(550.0, 550.0), QPointF(550.0, 780.0) },
        { QPointF(550.0, 780.0), QPointF(250.0, 780.0) },
        { QPointF(250.0, 780.0), QPointF(250.0, 250.0) },
        { QPointF(250.0, 250.0), QPointF(512.0, 250.0) }
    };

    double totalLen = 0.0;
    double segLens[7];
    for (int i = 0; i < 7; ++i) {
        segLens[i] = std::hypot(segs[i].p2.x() - segs[i].p1.x(), segs[i].p2.y() - segs[i].p1.y());
        totalLen += segLens[i];
    }

    double step = totalLen / 24.0;
    for (int i = 0; i < 24; ++i) {
        double targetDist = i * step;
        double accum = 0.0;
        for (int s = 0; s < 7; ++s) {
            if (accum + segLens[s] >= targetDist || s == 6) {
                double frac = (segLens[s] > 0.0) ? ((targetDist - accum) / segLens[s]) : 0.0;
                double wx = segs[s].p1.x() + frac * (segs[s].p2.x() - segs[s].p1.x());
                double wz = segs[s].p1.y() + frac * (segs[s].p2.y() - segs[s].p1.y());
                wps.emplace_back(wx, wz);
                break;
            }
            accum += segLens[s];
        }
    }
}
```

### 3.3 Static Coordinate Lookup Tables

For maximum deterministic execution and debugging, the precomputed coordinates:

| Index | Donut Plains (16 wps) | Choco Valley (20 wps) | Bowser Castle (24 wps) |
|:---:|:---:|:---:|:---:|
| 0 | `(512.0, 200.0)` | `(512.0, 200.0)` | `(512.0, 250.0)` |
| 1 | `(622.9, 225.8)` | `(684.2, 241.0)` | `(600.3, 250.0)` |
| 2 | `(702.1, 293.8)` | `(770.4, 321.3)` | `(688.7, 250.0)` |
| 3 | `(749.7, 389.8)` | `(787.3, 413.1)` | `(777.0, 250.0)` |
| 4 | `(765.5, 500.0)` | `(751.6, 488.6)` | `(780.0, 335.3)` |
| 5 | `(749.7, 610.2)` | `(680.0, 520.0)` | `(780.0, 423.7)` |
| 6 | `(702.1, 706.2)` | `(621.1, 551.2)` | `(780.0, 512.0)` |
| 7 | `(622.9, 774.2)` | `(590.0, 625.6)` | `(729.7, 550.0)` |
| 8 | `(512.0, 800.0)` | `(571.7, 714.4)` | `(641.3, 550.0)` |
| 9 | `(403.1, 774.2)` | `(550.8, 788.8)` | `(553.0, 550.0)` |
| 10 | `(325.2, 706.2)` | `(512.0, 820.0)` | `(550.0, 635.3)` |
| 11 | `(278.6, 610.2)` | `(369.3, 783.6)` | `(550.0, 723.7)` |
| 12 | `(263.0, 500.0)` | `(296.2, 692.0)` | `(518.0, 780.0)` |
| 13 | `(278.6, 389.8)` | `(263.2, 571.6)` | `(429.7, 780.0)` |
| 14 | `(325.2, 293.8)` | `(240.9, 448.8)` | `(341.3, 780.0)` |
| 15 | `(403.1, 225.8)` | `(200.0, 350.0)` | `(253.0, 780.0)` |
| 16 | — | `(186.2, 284.5)` | `(250.0, 694.7)` |
| 17 | — | `(228.6, 241.0)` | `(250.0, 606.3)` |
| 18 | — | `(309.2, 215.4)` | `(250.0, 518.0)` |
| 19 | — | `(409.7, 203.1)` | `(250.0, 429.7)` |
| 20 | — | — | `(250.0, 341.3)` |
| 21 | — | — | `(250.0, 253.0)` |
| 22 | — | — | `(335.3, 250.0)` |
| 23 | — | — | `(423.7, 250.0)` |

All sampled points strictly reside within the white road mask (`trackMaskMap`) and provide safe clearance from all obstacle world sprites.

---

## 4. Waypoint Tracking & Steering Algorithm

### 4.1 Lookahead Waypoint Pursuit
For kart $i$ at position $(x, z)$ with heading $\theta$:
1. **Target Selection**:
   $$T = (\text{currentWaypoint} + 2) \bmod N$$
   Looking 2 waypoints ahead (~140 to 180 units) provides smooth corner anticipation and an authentic racing line clipping the inside apex without cutting into curbs.
2. **Desired Heading**:
   $$\Delta x = W_{Tx} - x, \quad \Delta z = W_{Tz} - z$$
   $$\theta_{\text{desired}} = \operatorname{atan2}(\Delta z, \Delta x)$$
3. **Normalized Heading Error**:
   $$\Delta\theta = \theta_{\text{desired}} - \theta$$
   ```cpp
   while (dAngle > M_PI) dAngle -= 2.0 * M_PI;
   while (dAngle < -M_PI) dAngle += 2.0 * M_PI;
   ```
4. **Angular Interpolation via Character `turnRate`**:
   $$\text{turnStep} = \text{turnRate} \times dt$$
   ```cpp
   if (std::abs(dAngle) <= turnStep) {
       ai.angle = desiredAngle;
   } else if (dAngle > 0.0) {
       ai.angle += turnStep;
   } else {
       ai.angle -= turnStep;
   }
   while (ai.angle >= 2.0 * M_PI) ai.angle -= 2.0 * M_PI;
   while (ai.angle < 0.0) ai.angle += 2.0 * M_PI;
   ```

### 4.2 Waypoint Advancement
Let current waypoint be $W_C$.
Distance to current waypoint:
$$d_C = \sqrt{(W_{Cx} - x)^2 + (W_{Cz} - z)^2}$$
- **Primary Condition**: If $d_C < 35.0$ units, increment `currentWaypoint = (currentWaypoint + 1) % N`.
- **Fail-safe Condition**: If bumped off-line such that the kart passes alongside the waypoint, check distance to the next waypoint $W_{C+1}$:
  If $d_{C+1} < d_C$ and distance to line segment $W_C \to W_{C+1}$ is within road width ($< 45.0$), advance `currentWaypoint = (currentWaypoint + 1) % N`.
This guarantees AI karts never stall or reverse if bumped.

### 4.3 Throttle & Cornering Braking
```cpp
bool throttle = true;
bool brake = false;

// If heading error is sharp (> 0.65 rad ~ 37 deg), feather throttle and brake
if (std::abs(dAngle) > 0.65) {
    throttle = false;
    if (ai.speed > 80.0) {
        brake = true;
    }
}
```
This ensures AI karts slow down appropriately for sharp hairpins and technical 90° corners, maintaining stable trajectory.

### 4.4 Catchup Rubber-Banding
Compare AI progression against the human player:
$$\Delta D = D_{\text{player}} - D_{\text{AI}}$$
- Catch-up Boost: If $\Delta D > 150.0$ units:
  $$\text{effectiveMaxSpeed} = \text{baseMaxSpeed} \times 1.10$$
- Lead Moderation: If $\Delta D < -150.0$ units:
  $$\text{effectiveMaxSpeed} = \text{baseMaxSpeed} \times 0.90$$
- Normal:
  $$\text{effectiveMaxSpeed} = \text{baseMaxSpeed}$$

### 4.5 Kinematics Integration & Debuffs
```cpp
// Spinout debuff
if (ai.isSpinning) {
    ai.spinTimer -= dt;
    if (ai.spinTimer <= 0.0) {
        ai.isSpinning = false;
        ai.spinTimer = 0.0;
    }
    ai.angle += 12.0 * dt;
    ai.speed *= std::pow(0.5, dt / 0.1);
    if (ai.speed < 1.0) ai.speed = 0.0;
    ai.x += ai.speed * std::cos(ai.angle) * dt;
    ai.z += ai.speed * std::sin(ai.angle) * dt;
    return;
}

// Inked debuff
if (ai.isInked) {
    ai.inkTimer -= dt;
    if (ai.inkTimer <= 0.0) {
        ai.isInked = false;
        ai.inkTimer = 0.0;
    }
    effectiveMaxSpeed *= 0.70; // 30% speed penalty
}

// Throttle acceleration / braking
if (throttle) {
    double ratio = (effectiveMaxSpeed > 0.0) ? (ai.speed / effectiveMaxSpeed) : 0.0;
    double accelEff = ai.accel * (1.0 - 0.4 * std::clamp(ratio, 0.0, 1.0));
    ai.speed += accelEff * dt;
    if (ai.speed > effectiveMaxSpeed) ai.speed = effectiveMaxSpeed;
} else if (brake) {
    ai.speed -= ai.brakeDecel * dt;
    if (ai.speed < 0.0) ai.speed = 0.0;
} else {
    ai.speed = std::max(0.0, ai.speed - ai.friction * dt);
}

// Position integration
ai.x += ai.speed * std::cos(ai.angle) * dt;
ai.z += ai.speed * std::sin(ai.angle) * dt;
ai.totalRaceDistance += ai.speed * dt;

// Check halfway checkpoint
if (ai.z > 650.0) {
    ai.hasPassedHalfway = true;
}

// Lap advance
if (ai.hasPassedHalfway && ai.currentWaypoint == 0) {
    ai.hasPassedHalfway = false;
    ai.currentLap++;
    if (ai.currentLap > 3) {
        ai.raceFinished = true;
    }
}
```

---

## 5. Mode-7 3D Billboard Projection & Rendering Pipeline

Opponent karts are rendered directly in `GameCanvas::renderSprites()` by appending their projected screen billboards to `renderList`:

```cpp
// In GameCanvas::renderSprites():
for (const auto &ai : aiOpponents) {
    double dx = ai.x - kartX;
    double dz = ai.z - kartZ;

    // Transform into camera space
    double camX = -dx * sinA + dz * cosA;
    double camZ =  dx * cosA + dz * sinA;

    if (camZ <= cameraNearPlane || camZ > 450.0) continue;

    int sx = static_cast<int>((BUFFER_WIDTH / 2) + (focalLength * camX) / camZ);
    int groundY = static_cast<int>(HORIZON_Y + (focalLength * cameraHeight) / camZ);
    int sy = groundY;

    double scale = focalLength / camZ;
    int drawW = static_cast<int>(24.0 * scale);
    int drawH = static_cast<int>(24.0 * scale);

    if (drawW <= 1 || drawH <= 1) continue;

    // Sprite selection based on character id
    SpriteType kartType;
    switch (ai.characterId) {
    case 0: kartType = SpriteType::KART_MARIO; break;
    case 1: kartType = SpriteType::KART_BOWSER; break;
    case 2: kartType = SpriteType::KART_PEACH; break;
    case 3: kartType = SpriteType::KART_TOAD; break;
    default: kartType = SpriteType::KART_MARIO; break;
    }

    renderList.push_back({ sx, sy, drawW, drawH, camZ, kartType, false, groundY });
}
```

Because `renderList` is sorted by `camZ` from farthest to nearest (Painter's Algorithm), opponent karts naturally occlude or are occluded by world objects and are drawn with their authentic 32×32 textures generated in `SpriteAtlas`.

---

## 6. Implementation Contract & Recommendations

### 6.1 Changes Required in `game_canvas.h`
1. Add `std::vector<QPointF> trackWaypoints[3];` and `std::vector<QPointF> currentTrackWaypoints;`.
2. Add `void updateAIOpponents(double dt);`.
3. Add `void initTrackWaypoints();`.
4. Ensure `KartEntity` fields match `test_types.h` (`totalRaceDistance`, `currentWaypoint`, `spinTimer`, `inkTimer`).

### 6.2 Changes Required in `game_canvas.cpp`
1. Call `initTrackWaypoints()` inside `initTrackMaps()` and set `currentTrackWaypoints = trackWaypoints[static_cast<int>(currentLevel)]` in `setTrackLevel()`.
2. Call `updateAIOpponents(dt)` in `updateGameLoop()` when `currentState == GameState::RACING`.
3. In `renderSprites()`, iterate over `aiOpponents` and append to `renderList`.
4. In `useHeldItem()`:
   - For `GREEN_SHELL`: Spawn moving `activeProjectiles` with forward velocity $v_{\text{kart}} + 75.0$ and target detection against `aiOpponents`.
   - For `INK_BLOOPER`: Apply `ai.isInked = true; ai.inkTimer = 4.0;` to all `aiOpponents` while leaving player's `inkTimer = 0.0`.
   - For `BANANA`: Add 1.2s owner immunity window to prevent self-collision.

---

## 7. Verification Strategy

1. **Automated E2E Suite**:
   Execute the full automated test runner:
   ```powershell
   $env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH
   & "d:\Qt\RetroKart3D\tests\build\release\test_runner.exe"
   ```
   Verify 100% pass across all 182 test cases (particularly F4.1–F4.5 and F5.1–F5.5).
2. **Main Application Build**:
   ```powershell
   $env:PATH = "D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;" + $env:PATH
   mingw32-make -f Makefile.Release
   ```
   Verify clean 0-warning compilation.
3. **Gameplay Inspection**:
   - Start game in "Play vs AI".
   - Confirm 3 opponent karts (Mario/Bowser/Peach/Toad) start on grid.
   - Confirm AI karts navigate continuous closed loop smoothly along track centerline.
   - Confirm AI karts slow down on sharp turns and do not enter grass or lava.
   - Use Ink Blooper: confirm screen stays clear, AI karts slow down.
   - Fire Green Shell: confirm forward trajectory and spinout upon hitting opponent.
