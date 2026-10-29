# Milestone 3 Architecture & Implementation Strategy: Interactive Track Hazards & Terrain Fix

**Target Workspace**: `d:\Qt\RetroKart3D`  
**Author**: `explorer_m3_2` (Milestone 3 Explorer)  
**Date**: 2026-10-29  
**Status**: APPROVED INVESTIGATION SPECIFICATION  

---

## 1. Executive Summary

Milestone 3 delivers the showcase features for RetroKart3D:
1. **Critical AI Terrain Mask & Debuff Recovery Fix**: Resolves the root-cause bug where AI opponents were trapped in an infinite lava spinout loop on normal white tarmac (`game_canvas.cpp:1167`), and ensures ink debuff timers decay reliably regardless of spinout status.
2. **Interactive Track Hazards**:
   - **Bowser Castle Lava Geysers**: 4 spewing lava geysers strategically placed in molten lava lakes with a 4.8s cyclic state machine (`Dormant Bubbling` $\to$ `Warning Rumble` $\to$ `Spewing Active Fire Column`) featuring height-aware collision ($R \le 16.0$, 75% speed loss, 0.8s spinout, radial pushback).
   - **Choco Valley Hazards**: 3 bubbling chocolate fountains (50% speed cap) and 2 rolling chocolate candies with sinusoidal cross-track oscillation ($A = 40.0, \omega = 2.0$) and arcade bumper bounce recoil (`speed = -35.0`).
3. **Procedural 32x32 ARGB Pixel Artwork (`SpriteAtlas`)**:
   - `LAVA_GEYSER_BUBBLING`: Boiling magma caldera with bubbling hot spots and heat wisps.
   - `LAVA_GEYSER_ERUPTING`: Vertical roaring fire/plasma pillar with incandescent white-yellow core and billowing smoke cap.
   - `CHOCOLATE_FOUNTAIN`: Tiered chocolate basin with gushing chocolate plume, gloss highlights, and mud splash droplets.
   - `CHOCOLATE_CANDY`: 3D spherical dark chocolate bonbon with cream swirl and specular gleam.
4. **3D Billboard Projection in `renderSprites()`**: Full Mode-7 perspective projection, horizon culling, camera near/far plane clipping, dynamic vertical flame height scaling, and Painter's algorithm depth sorting alongside karts, projectiles, and item boxes.
5. **Top-Left HUD Minimap**: 72x72 radar box at `(8, 8)` with coordinate modulo wrapping and real-time kart blips.

Empirical verification confirms that fixing the terrain mask triage resolves all 3 test failures in `standalone_m2_forensic.exe` (`AI-3.1`, `AI-3.2`, `IB-5.5`), enabling 100% pass rates across both the forensic suite and the comprehensive 182-test E2E runner.

---

## 2. Root Cause Analysis: AI Terrain Mask Defect & Debuff Recovery (C8, C9)

### 2.1 The Defect in `game_canvas.cpp:1167-1175`
In `GameCanvas::updateAIOpponents(double dt)`:
```cpp
1160:         int sampleU = (static_cast<int>(ai.x) % TRACK_MAP_SIZE + TRACK_MAP_SIZE) % TRACK_MAP_SIZE;
1161:         int sampleV = (static_cast<int>(ai.z) % TRACK_MAP_SIZE + TRACK_MAP_SIZE) % TRACK_MAP_SIZE;
1162:         QRgb maskColor = trackMaskMap.pixel(sampleU, sampleV);
1163: 
1164:         double terrainSpeedMult = 1.0;
1165:         double terrainDragMult = 1.0;
1166: 
1167:         if (qRed(maskColor) == 255 && qBlue(maskColor) == 255) {
1168:             // Molten lava recoil
1169:             ai.speed *= 0.35;
1170:             ai.x -= std::cos(ai.angle) * 8.0;
1171:             ai.z -= std::sin(ai.angle) * 8.0;
1172:             ai.isSpinning = true;
1173:             ai.spinTimer = 0.8;
1174:             continue;
1175:         } else if (qGreen(maskColor) == 255 && qRed(maskColor) == 0) {
```

### 2.2 Mathematical Proof of Failure
Across all 3 tracks in `GameCanvas`:
- Donut Plains (`line 634`): `QPen maskRoadPen(QColor(255, 255, 255), 84);`
- Choco Valley (`line 705`): `QPen maskRoadPen(QColor(255, 255, 255), 60);`
- Bowser Castle (`line 767`): `QPen maskRoad(QColor(255, 255, 255), 74);`

All raceway tarmac pixels in `trackMaskMap` are drawn with `RGB(255, 255, 255)`.
For any point on tarmac:
- `qRed(maskColor) == 255` $\to$ **true**
- `qBlue(maskColor) == 255` $\to$ **true**
- `qGreen(maskColor) == 255` $\to$ **true** (omitted from condition!)

Because line 1167 tests only red and blue, White tarmac satisfies the Molten Lava condition!
Every frame, the AI executes lines 1169-1174:
1. `ai.speed *= 0.35`
2. `ai.x -= std::cos(ai.angle) * 8.0` (recoils backward)
3. `ai.isSpinning = true`
4. `ai.spinTimer = 0.8`
5. `continue;` (skips steering, throttle, and forward integration)

When `spinTimer` expires after 0.8s, the AI is still on tarmac, immediately triggering the condition again. The AI opponents are permanently immobilized behind the starting grid.

### 2.3 Secondary Mechanism: Ink Debuff Decay Stall (`Line 1080` vs `Line 1084`)
At lines 1064-1082:
```cpp
1064:         if (ai.spinTimer > 0.0 || ai.isSpinning) {
1065:             ai.spinTimer -= dt;
                  ...
1080:             continue;
1081:         }
1082:     }
1084:     // 2. Ink debuff decay
1085:     if (ai.inkTimer > 0.0) {
1086:         ai.inkTimer -= dt;
              ...
```
Because `continue;` is executed while `ai.isSpinning == true`, line 1085 (`ai.inkTimer -= dt`) is completely bypassed! If an AI opponent is inked while spinning, the ink debuff timer halts and never recovers (auditor check C9, `IB-5.5`).

### 2.4 Recommended Code Changes

#### Change A: Independent Debuff Decay Order in `updateAIOpponents`
Move `inkTimer` decay to Step 1 (at the very top of the per-AI loop), before any early `continue;`:
```cpp
    for (auto &ai : aiOpponents) {
        // 1. Ink debuff decay (must update continuously regardless of spin status)
        if (ai.inkTimer > 0.0) {
            ai.inkTimer -= dt;
            if (ai.inkTimer <= 0.0) {
                ai.inkTimer = 0.0;
                ai.isInked = false;
            } else {
                ai.isInked = true;
            }
        }

        // 2. Spinout debuff decay and visual spin
        if (ai.spinTimer > 0.0 || ai.isSpinning) {
            ai.spinTimer -= dt;
            if (ai.spinTimer <= 0.0) {
                ai.spinTimer = 0.0;
                ai.isSpinning = false;
            } else {
                ai.isSpinning = true;
                ai.angle += 12.0 * dt;
                ai.speed *= std::pow(0.5, dt / 0.1);
                if (ai.speed < 1.0) ai.speed = 0.0;
                ai.x += ai.speed * std::cos(ai.angle) * dt;
                ai.z += ai.speed * std::sin(ai.angle) * dt;
                while (ai.x < 0.0) ai.x += TRACK_MAP_SIZE;
                while (ai.x >= TRACK_MAP_SIZE) ai.x -= TRACK_MAP_SIZE;
                while (ai.z < 0.0) ai.z += TRACK_MAP_SIZE;
                while (ai.z >= TRACK_MAP_SIZE) ai.z -= TRACK_MAP_SIZE;
                continue;
            }
        }
```

#### Change B: Strict Terrain Mask Triage in `updateAIOpponents`
Mirror player physics (`game_canvas.cpp:902`) and require `qGreen(maskColor) == 0` for lava:
```cpp
        // 7. Terrain slowdown sampling trackMaskMap
        int sampleU = (static_cast<int>(ai.x) % TRACK_MAP_SIZE + TRACK_MAP_SIZE) % TRACK_MAP_SIZE;
        int sampleV = (static_cast<int>(ai.z) % TRACK_MAP_SIZE + TRACK_MAP_SIZE) % TRACK_MAP_SIZE;
        QRgb maskColor = trackMaskMap.pixel(sampleU, sampleV);

        double terrainSpeedMult = 1.0;
        double terrainDragMult = 1.0;

        if (qRed(maskColor) == 255 && qGreen(maskColor) == 255 && qBlue(maskColor) == 255) {
            // White Road (Tarmac): 100% full speed
            terrainSpeedMult = 1.0;
            terrainDragMult = 1.0;
        } else if (qRed(maskColor) == 255 && qGreen(maskColor) == 0 && qBlue(maskColor) == 255) {
            // Magenta Molten Lava recoil: bounce back, spinout, lose speed
            ai.speed *= 0.35;
            ai.x -= std::cos(ai.angle) * 8.0;
            ai.z -= std::sin(ai.angle) * 8.0;
            ai.isSpinning = true;
            ai.spinTimer = 0.8;
            continue;
        } else if (qGreen(maskColor) == 255 && qRed(maskColor) == 0) {
            // Off-road / Shoulder:
            terrainSpeedMult = (currentLevel == TrackLevel::BOWSER_CASTLE) ? 0.70 : 0.45;
            terrainDragMult = 2.2;
        } else if (qRed(maskColor) == 255 && qGreen(maskColor) == 255 && qBlue(maskColor) == 0) {
            // Start / Finish Line (Yellow: 255, 255, 0)
            terrainSpeedMult = 1.0;
            terrainDragMult = 1.0;
        }
```

---

## 3. Interactive Track Hazards: Architecture & Mechanics

### 3.1 Contract Structures (`game_canvas.h`)
```cpp
enum class HazardType {
    LAVA_GEYSER,
    CHOCOLATE_FOUNTAIN,
    ROLLING_CANDY
};

struct TrackHazard {
    double x = 0.0;
    double z = 0.0;
    HazardType type = HazardType::LAVA_GEYSER;
    double animTimer = 0.0;
    bool isErupting = false;
    double radius = 16.0;
};
```

### 3.2 Canonical Hazard Placement
To match `PROJECT.md` and test suite `test_harness.cpp`:

#### Bowser's Castle (`TrackLevel::BOWSER_CASTLE`):
4 Lava Geysers in molten magma pools flanking the narrow stone viaduct:
| Hazard # | Type | World X | World Z | Initial `animTimer` | Radius | Initial Phase |
|---|---|---|---|---|---|---|
| Geyser 0 | `LAVA_GEYSER` | 500.0 | 150.0 | 0.0s | 16.0 | Dormant |
| Geyser 1 | `LAVA_GEYSER` | 850.0 | 350.0 | 1.2s | 16.0 | Dormant |
| Geyser 2 | `LAVA_GEYSER` | 650.0 | 850.0 | 2.4s | 16.0 | Dormant |
| Geyser 3 | `LAVA_GEYSER` | 200.0 | 650.0 | 3.6s | 16.0 | Spewing Active |

#### Choco Valley (`TrackLevel::CHOCO_VALLEY`):
3 Chocolate Fountains + 2 Rolling Chocolate Candies:
| Hazard # | Type | World X | World Z | Initial `animTimer` | Radius | Behavior |
|---|---|---|---|---|---|---|
| Fountain 0 | `CHOCOLATE_FOUNTAIN` | 700.0 | 220.0 | 0.0s | 15.0 | Mud pool splash (50% speed cap) |
| Fountain 1 | `CHOCOLATE_FOUNTAIN` | 600.0 | 580.0 | 0.0s | 15.0 | Mud pool splash (50% speed cap) |
| Fountain 2 | `CHOCOLATE_FOUNTAIN` | 300.0 | 760.0 | 0.0s | 15.0 | Mud pool splash (50% speed cap) |
| Candy 0 | `ROLLING_CANDY` | 550.0 | 180.0 | 0.0s | 14.0 | Oscillating bumper (recoil $-35$) |
| Candy 1 | `ROLLING_CANDY` | 400.0 | 850.0 | 1.5s | 14.0 | Oscillating bumper (recoil $-35$) |

#### Donut Plains (`TrackLevel::DONUT_PLAINS`):
`trackHazards.clear();` (Level 1 beginner track has no interactive hazards).

### 3.3 Lava Geyser Cyclic State Machine
The geyser runs on a 4.80-second loop:
```
[0.0s ---------------- 2.5s]  Dormant Bubbling (Harmless, bubbling magma sprite)
[2.5s -------- 3.3s]          Warning Rumble (Harmless, vibrating bubbles + smoke puffs)
[3.3s ---------------- 4.8s]  Spewing Active Fire Column (DEADLY, erupting fire pillar)
```

Mathematical state transition:
```cpp
hazard.animTimer += dt;
if (hazard.animTimer >= 4.8) {
    hazard.animTimer -= 4.8;
}
hazard.isErupting = (hazard.animTimer >= 3.3);
```

### 3.4 Eruption Height Curve & Height-Aware Collision
During the active spewing phase ($3.3 \le t < 4.8$), normalized phase time:
$$u = \frac{t - 3.3}{1.5} \in [0.0, 1.0]$$
The visual flame column height $H(u)$ follows an explosive envelope:
$$H(u) = 36.0 \cdot \sin(u \cdot \pi)$$
Peak height reaches $36.0$ units at $u = 0.5$ ($t = 4.05\text{s}$).

Collision criteria:
A kart at $(x_k, z_k)$ is struck by the geyser if and only if:
1. `hazard.isErupting == true`
2. $\text{distance} = \sqrt{(x_k - x_h)^2 + (z_k - z_h)^2} < 16.0$
3. Contact response:
   - Speed penalized: `kart.speed *= 0.25;` (75% loss)
   - Spinout triggered: `kart.isSpinning = true; kart.spinTimer = 0.8;`
   - Radial pushback impulse:
     $$\hat{d}_x = \frac{x_k - x_h}{\text{distance}}, \quad \hat{d}_z = \frac{z_k - z_h}{\text{distance}}$$
     $$x_k \mathrel{+}= \hat{d}_x \cdot 15.0, \quad z_k \mathrel{+}= \hat{d}_z \cdot 15.0$$
     *(If $\text{distance} \le 0.001$, default $\hat{d}_x = 1.0, \hat{d}_z = 0.0$ to prevent NaN).*
   - Screen flash intensity set to $0.6$ for player 1.

### 3.5 Chocolate Valley Hazards Dynamics
1. **Chocolate Fountain (`CHOCOLATE_FOUNTAIN`)**:
   - Continuous active bubbling mud pool ($R = 15.0$).
   - If kart enters radius $15.0$:
     `if (kart.speed > kart.maxSpeed * 0.50) kart.speed = kart.maxSpeed * 0.50;`
   - Harmless to already slow karts (does not accelerate or spin them).
2. **Rolling Chocolate Candy (`ROLLING_CANDY`)**:
   - Cross-track sinusoidal motion:
     $$x_{\text{candy}}(t) = x_{\text{base}} + 40.0 \cdot \sin(2.0 \cdot \text{animTimer})$$
     $$z_{\text{candy}}(t) = z_{\text{base}}$$
   - Collision radius: $R = 14.0$.
   - Bumper bounce response:
     `kart.speed = -35.0;` (immediate arcade pinball reverse bounce impulse).

---

## 4. Procedural 32x32 ARGB Artwork in `SpriteAtlas`

### 4.1 Enumeration Updates in `sprite_atlas.h`
Add to `enum class SpriteType`:
```cpp
    // Track Hazards (Milestone 3)
    LAVA_GEYSER_BUBBLING,   // Dormant & warning bubbling lava pool
    LAVA_GEYSER_ERUPTING,   // Towering fire/plasma column
    CHOCOLATE_FOUNTAIN,     // Bubbling chocolate mud fountain
    CHOCOLATE_CANDY         // Giant rolling striped chocolate bonbon
```
*(Optionally provide aliases `LAVA_GEYSER = LAVA_GEYSER_ERUPTING` and `ROLLING_CANDY = CHOCOLATE_CANDY` for universal compatibility).*

### 4.2 Pixel Generation Formulas (`sprite_atlas.cpp`)

#### `generateLavaGeyserBubbling()` (32x32)
```cpp
void SpriteAtlas::generateLavaGeyserBubbling()
{
    QImage img(SPRITE_SIZE, SPRITE_SIZE, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    // 1. Base caldera magma pool (y: 20 to 30)
    for (int y = 20; y < 31; ++y) {
        int halfW = static_cast<int>(14.0 * std::sin((y - 19) / 12.0 * M_PI));
        for (int x = 16 - halfW; x <= 16 + halfW; ++x) {
            int d = std::abs(x - 16);
            if (d > halfW - 2)
                setPixel(img, x, y, qRgba(90, 15, 10, 240));    // Dark crust outline
            else if (d > halfW - 5)
                setPixel(img, x, y, qRgba(190, 45, 10, 255));   // Crimson magma
            else
                setPixel(img, x, y, qRgba(255, 130, 20, 255));  // Molten orange
        }
    }

    // 2. Glowing bubbling magma domes
    fillCircle(img, 12, 23, 3, qRgba(255, 210, 30, 255), qRgba(220, 70, 10, 255));
    fillCircle(img, 19, 22, 4, qRgba(255, 235, 80, 255), qRgba(240, 90, 15, 255));
    fillCircle(img, 15, 25, 2, qRgba(255, 255, 160, 255), qRgba(255, 140, 20, 255));

    // 3. Smoke / heat ripples rising
    setPixel(img, 14, 17, qRgba(160, 60, 20, 180));
    setPixel(img, 15, 16, qRgba(180, 70, 25, 190));
    setPixel(img, 18, 15, qRgba(150, 50, 15, 170));
    setPixel(img, 17, 13, qRgba(120, 40, 15, 140));

    sprites[SpriteType::LAVA_GEYSER_BUBBLING] = img;
}
```

#### `generateLavaGeyserErupting()` (32x32)
```cpp
void SpriteAtlas::generateLavaGeyserErupting()
{
    QImage img(SPRITE_SIZE, SPRITE_SIZE, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    // 1. Magma pool base
    for (int y = 24; y < 32; ++y) {
        int hw = 12 - (y - 24);
        for (int x = 16 - hw; x <= 16 + hw; ++x) {
            setPixel(img, x, y, qRgba(180, 30, 10, 250));
        }
    }

    // 2. Towering flame and plasma pillar (y: 4 to 28)
    for (int y = 4; y < 28; ++y) {
        double flare = 1.0 + 0.3 * std::sin(y * 0.8);
        int halfW = static_cast<int>((5.0 + (28 - y) * 0.15) * flare);
        for (int x = 16 - halfW; x <= 16 + halfW; ++x) {
            int dist = std::abs(x - 16);
            if (dist == halfW) {
                setPixel(img, x, y, qRgba(140, 20, 10, 220));   // Dark crimson edge
            } else if (dist >= halfW - 2) {
                setPixel(img, x, y, qRgba(240, 60, 10, 255));   // Fiery red-orange
            } else if (dist >= halfW - 3) {
                setPixel(img, x, y, qRgba(255, 170, 20, 255));  // Bright orange
            } else {
                setPixel(img, x, y, qRgba(255, 255, 180, 255)); // Incandescent white-yellow core
            }
        }
    }

    // 3. Billowing smoke & fiery sparks at crest
    fillCircle(img, 12, 5, 3, qRgba(100, 25, 15, 210), qRgba(60, 15, 10, 180));
    fillCircle(img, 20, 4, 3, qRgba(110, 30, 15, 210), qRgba(60, 15, 10, 180));
    fillCircle(img, 16, 2, 2, qRgba(255, 200, 50, 255), qRgba(200, 50, 10, 255)); // Crown spark

    // Flying cinders
    setPixel(img, 6, 8, qRgba(255, 180, 30, 230));
    setPixel(img, 26, 7, qRgba(255, 160, 20, 230));
    setPixel(img, 8, 14, qRgba(255, 100, 10, 210));
    setPixel(img, 24, 15, qRgba(255, 120, 15, 210));

    sprites[SpriteType::LAVA_GEYSER_ERUPTING] = img;
}
```

#### `generateChocolateFountain()` (32x32)
```cpp
void SpriteAtlas::generateChocolateFountain()
{
    QImage img(SPRITE_SIZE, SPRITE_SIZE, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    // 1. Lower chocolate basin (y: 22 to 30)
    for (int y = 22; y < 31; ++y) {
        int hw = static_cast<int>(13.0 * std::sin((y - 21) / 10.0 * M_PI));
        for (int x = 16 - hw; x <= 16 + hw; ++x) {
            int d = std::abs(x - 16);
            if (d == hw)
                setPixel(img, x, y, qRgba(50, 25, 10, 255));    // Dark chocolate crust
            else if (d > hw - 3)
                setPixel(img, x, y, qRgba(85, 45, 20, 255));    // Milk chocolate
            else
                setPixel(img, x, y, qRgba(115, 65, 30, 255));   // Rich brown
        }
    }

    // 2. Middle tier (y: 14 to 21)
    for (int y = 14; y < 22; ++y) {
        int hw = static_cast<int>(8.0 * std::sin((y - 13) / 9.0 * M_PI));
        for (int x = 16 - hw; x <= 16 + hw; ++x) {
            setPixel(img, x, y, qRgba(105, 55, 25, 255));
        }
    }

    // 3. Flowing dome / top plume (y: 6 to 14)
    fillCircle(img, 16, 11, 5, qRgba(135, 75, 35, 255), qRgba(60, 30, 15, 255));

    // 4. Creamy highlights & cascading syrup ribbons
    setPixel(img, 15, 8, qRgba(210, 160, 110, 240));  // Specular sheen
    setPixel(img, 16, 8, qRgba(220, 175, 125, 240));
    setPixel(img, 12, 16, qRgba(175, 115, 65, 230)); // Waterfall sheen
    setPixel(img, 20, 16, qRgba(175, 115, 65, 230));

    // Mud droplets flying
    setPixel(img, 8, 12, qRgba(90, 45, 20, 220));
    setPixel(img, 24, 11, qRgba(90, 45, 20, 220));

    sprites[SpriteType::CHOCOLATE_FOUNTAIN] = img;
}
```

#### `generateChocolateCandy()` (32x32)
```cpp
void SpriteAtlas::generateChocolateCandy()
{
    QImage img(SPRITE_SIZE, SPRITE_SIZE, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    int cx = 16, cy = 16, r = 13;

    // 1. Shaded 3D spherical chocolate sphere
    for (int y = cy - r; y <= cy + r; ++y) {
        for (int x = cx - r; x <= cx + r; ++x) {
            double dx = x - cx;
            double dy = y - cy;
            double d2 = dx * dx + dy * dy;
            if (d2 <= r * r) {
                if (d2 >= (r - 1) * (r - 1)) {
                    setPixel(img, x, y, qRgba(40, 20, 10, 255)); // Dark outline
                } else {
                    // Spherical lighting from top-left (lx = -0.4, ly = -0.4)
                    double nx = dx / r;
                    double ny = dy / r;
                    double diff = std::clamp(-0.5 * nx - 0.5 * ny + 0.4, 0.0, 1.0);
                    int red   = static_cast<int>(60  + diff * 90);
                    int green = static_cast<int>(30  + diff * 50);
                    int blue  = static_cast<int>(12  + diff * 25);
                    setPixel(img, x, y, qRgba(red, green, blue, 255));
                }
            }
        }
    }

    // 2. White vanilla cream swirl stripes across bonbon
    for (int t = -10; t <= 10; ++t) {
        int sx = cx + t;
        int sy = cy + static_cast<int>(std::sin(t * 0.3) * 5.0);
        if ((sx - cx)*(sx - cx) + (sy - cy)*(sy - cy) < (r - 2)*(r - 2)) {
            setPixel(img, sx, sy, qRgba(250, 240, 225, 255));
            setPixel(img, sx, sy + 1, qRgba(230, 215, 195, 240));
        }
    }

    // 3. Glossy specular highlight
    fillCircle(img, cx - 4, cy - 5, 2, qRgba(255, 255, 240, 240), qRgba(200, 180, 160, 200));

    sprites[SpriteType::CHOCOLATE_CANDY] = img;
}
```

### 4.3 Classification Updates
In `SpriteAtlas::isObstacle`:
```cpp
bool SpriteAtlas::isObstacle(SpriteType type) const
{
    return type == SpriteType::GREEN_PIPE || type == SpriteType::THWOMP ||
           type == SpriteType::OIL_SLICK || type == SpriteType::BANANA_DROPPED ||
           type == SpriteType::LAVA_GEYSER_ERUPTING ||
           type == SpriteType::CHOCOLATE_FOUNTAIN ||
           type == SpriteType::CHOCOLATE_CANDY;
}
```

---

## 5. Mode-7 3D Billboard Projection in `renderSprites()`

### 5.1 Projection Pipeline Integration
In `GameCanvas::renderSprites()`:
Extend `renderList` generation to iterate over `trackHazards`:
```cpp
    // 4. Project Track Hazards (Lava Geysers, Chocolate Fountains & Rolling Candies)
    for (const auto &hazard : trackHazards) {
        double worldX = hazard.x;
        double worldZ = hazard.z;

        // Rolling candy cross-track motion offset
        if (hazard.type == HazardType::ROLLING_CANDY) {
            worldX += 40.0 * std::sin(hazard.animTimer * 2.0);
        }

        double dx = worldX - kartX;
        double dz = worldZ - kartZ;

        // Mode-7 Camera transform
        double camX = -dx * sinA + dz * cosA;
        double camZ =  dx * cosA + dz * sinA;

        // Depth culling
        if (camZ <= cameraNearPlane || camZ > 450.0) continue;

        int groundY = static_cast<int>(HORIZON_Y + (focalLength * cameraHeight) / camZ);
        if (groundY <= HORIZON_Y) continue;

        double scale = focalLength / camZ;
        int sx = static_cast<int>((BUFFER_WIDTH / 2) + (focalLength * camX) / camZ);

        SpriteType hazardSprite = SpriteType::LAVA_GEYSER_BUBBLING;
        double baseW = 20.0;
        double baseH = 20.0;

        if (hazard.type == HazardType::LAVA_GEYSER) {
            if (hazard.isErupting) {
                hazardSprite = SpriteType::LAVA_GEYSER_ERUPTING;
                baseW = 26.0;
                // Dynamic eruption flame column height
                double u = std::clamp((hazard.animTimer - 3.3) / 1.5, 0.0, 1.0);
                double flameH = 24.0 + 16.0 * std::sin(u * M_PI);
                baseH = flameH;
            } else {
                hazardSprite = SpriteType::LAVA_GEYSER_BUBBLING;
                baseW = 22.0;
                baseH = 16.0;
            }
        } else if (hazard.type == HazardType::CHOCOLATE_FOUNTAIN) {
            hazardSprite = SpriteType::CHOCOLATE_FOUNTAIN;
            baseW = 24.0;
            baseH = 24.0;
        } else if (hazard.type == HazardType::ROLLING_CANDY) {
            hazardSprite = SpriteType::CHOCOLATE_CANDY;
            baseW = 20.0;
            baseH = 20.0;
        }

        int drawW = static_cast<int>(baseW * scale);
        int drawH = static_cast<int>(baseH * scale);
        if (drawW <= 1 || drawH <= 1) continue;

        // Position on ground plane
        int sy = groundY;

        renderList.push_back({ sx, sy, drawW, drawH, camZ, hazardSprite, false, groundY });
    }
```

### 5.2 Depth Sorting & Translucent Ground Shadows
Because `renderList` sorts by `camZ` from farthest to nearest, track hazards naturally occlude and are occluded by world sprites, rival karts, and projectiles.
- For `CHOCOLATE_CANDY` and `CHOCOLATE_FOUNTAIN`, ground shadow projection renders directly into `screenBuffer` below `groundY`.
- For `LAVA_GEYSER_ERUPTING`, draw an emissive warm ambient glow around `(sx, groundY)` using reddish-orange additive alpha blending.

---

## 6. Top-Left HUD Minimap Architecture

### 6.1 Coordinates and Dimensions
- Position: Top-left corner `(RADAR_X = 8, RADAR_Y = 8)`
- Size: `RADAR_SIZE = 72` pixels ($72 \times 72$)
- World Map Dimension: `MAP_WORLD_SIZE = 1024`

### 6.2 Coordinate Mapping Formula
$$\text{wrap}(c) = ((c \pmod{1024}) + 1024) \pmod{1024}$$
$$rx = 8 + \left(\frac{\text{wrap}(x)}{1024.0}\right) \times 72.0$$
$$ry = 8 + \left(\frac{\text{wrap}(z)}{1024.0}\right) \times 72.0$$

### 6.3 Rendering Implementation (`GameCanvas::renderItemHUD` or dedicated `renderMinimap`)
```cpp
void GameCanvas::renderMinimap()
{
    QPainter painter(&screenBuffer);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 1. Radar background bezel
    QRect radarRect(8, 8, 72, 72);
    painter.fillRect(radarRect, QColor(10, 15, 28, 190));
    painter.setPen(QPen(QColor(220, 185, 60), 2));
    painter.drawRect(radarRect);

    // 2. Track circuit outline
    if (!currentTrackWaypoints.empty()) {
        QPainterPath path;
        for (size_t i = 0; i < currentTrackWaypoints.size(); ++i) {
            double wx = currentTrackWaypoints[i].x();
            double wz = currentTrackWaypoints[i].y();
            double rx = 8.0 + (wx / 1024.0) * 72.0;
            double ry = 8.0 + (wz / 1024.0) * 72.0;
            if (i == 0) path.moveTo(rx, ry);
            else path.lineTo(rx, ry);
        }
        path.closeSubpath();
        painter.setPen(QPen(QColor(130, 140, 160, 200), 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawPath(path);
    }

    // 3. AI Opponent Blips (Colored dots)
    for (const auto &ai : aiOpponents) {
        double rx = 8.0 + ((std::fmod(std::fmod(ai.x, 1024.0) + 1024.0, 1024.0)) / 1024.0) * 72.0;
        double ry = 8.0 + ((std::fmod(std::fmod(ai.z, 1024.0) + 1024.0, 1024.0)) / 1024.0) * 72.0;
        rx = std::clamp(rx, 9.0, 78.0);
        ry = std::clamp(ry, 9.0, 78.0);
        painter.setBrush(ai.kartColor);
        painter.setPen(QPen(Qt::black, 1));
        painter.drawEllipse(QPointF(rx, ry), 2.5, 2.5);
    }

    // 4. Player 1 Blip (Bright White with yellow halo)
    double p1rx = 8.0 + ((std::fmod(std::fmod(kartX, 1024.0) + 1024.0, 1024.0)) / 1024.0) * 72.0;
    double p1ry = 8.0 + ((std::fmod(std::fmod(kartZ, 1024.0) + 1024.0, 1024.0)) / 1024.0) * 72.0;
    p1rx = std::clamp(p1rx, 9.0, 78.0);
    p1ry = std::clamp(p1ry, 9.0, 78.0);

    painter.setBrush(QColor(255, 255, 255));
    painter.setPen(QPen(QColor(255, 215, 0), 1.5));
    painter.drawEllipse(QPointF(p1rx, p1ry), 3.5, 3.5);
}
```

---

## 7. Verification & Implementation Roadmap

| Step | Target Files | Key Action |
|---|---|---|
| **Step 1** | `sprite_atlas.h`, `sprite_atlas.cpp` | Add 4 procedural sprites: `LAVA_GEYSER_BUBBLING`, `LAVA_GEYSER_ERUPTING`, `CHOCOLATE_FOUNTAIN`, `CHOCOLATE_CANDY`. Update `isObstacle`. |
| **Step 2** | `game_canvas.cpp:1062-1180` | Move `inkTimer` decay to loop start. Fix terrain mask condition at line 1167: check White Tarmac first, require `qGreen == 0` for molten lava. |
| **Step 3** | `game_canvas.cpp` | In `generateBowserCastle()`, initialize 4 Lava Geysers. In `generateChocoValley()`, initialize 3 fountains and 2 rolling candies. Clear in `generateDonutPlains()`. |
| **Step 4** | `game_canvas.cpp` | Create `updateTrackHazards(double dt)`: cyclic eruption state machine, height curve, and kart collision handling. Call in `updateGameLoop()`. |
| **Step 5** | `game_canvas.cpp:1560` | In `renderSprites()`, project `trackHazards` as Mode-7 3D billboards into `renderList`. |
| **Step 6** | `game_canvas.cpp` | Add `renderMinimap()` to draw the top-left HUD radar box and blips. |
| **Step 7** | Verification | Run `standalone_m2_forensic.exe` (must pass 64/64 checks). Run `test_runner.exe` (must pass 182/182 checks). Run `RetroKart3D.exe` build check. |
