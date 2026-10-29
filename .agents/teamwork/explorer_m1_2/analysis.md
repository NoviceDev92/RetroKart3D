# RetroKart 3D: Character Selection, Roster & Kart Visuals Implementation Strategy
**Milestone 1: Character Selection & Visuals Analysis Report**  
**Author**: `explorer_m1_2` (Milestone 1 Explorer: Character Selection & Visuals)  
**Date**: 2026-10-29  
**Target Environment**: Desktop Qt 6.11.1 MinGW 64-bit on Windows (`D:\Qt\6.11.1\mingw_64`)  
**Workspace**: `d:\Qt\RetroKart3D`

---

## 1. Executive Summary

This report establishes the complete architectural design, numerical physics specification, procedural pixel-art asset generation, and UI presentation strategy for **Character Selection, Roster & Kart Visuals** in RetroKart3D for Milestone 1.

### Core Deliverables Detailed in this Report:
1. **Authoritative 4-Character Roster**:
   - **Mario**: The Balanced Racer (baseline speed, reliable all-around handling).
   - **Bowser**: The Heavyweight Titan (high top speed: 175 km/h, heavy collision momentum, slower acceleration).
   - **Peach**: The Agility Specialist (maximum cornering & drift factor: 3.4 turn rate, tight drift lines, light weight).
   - **Toad**: The Turbo Acceleration Rocket (explosive acceleration: 140 m/s², rapid 0.3s hazard recovery).
2. **Exact Numerical Stat Modifiers & Physics Integration**:
   - Mathematical tuning for `maxSpeed`, `accel`, `turnRate`, `driftFactor`, `weight`, and `recoveryRate`.
   - Surface interaction and collision impulse scaling based on character weight.
   - Normalized 5-pip retro stat bars for Speed, Accel, Handling, and Weight.
3. **Procedural Kart Visuals & `SpriteAtlas` Pipeline**:
   - Programmatic generation of 32x32 ARGB billboard sprites for all 4 characters (chassis, tires, spoiler, driver head/helmet/cap/horns/crown/mushroom).
   - Alternate palette variations for duplicate character picks in Multiplayer (e.g. Fire Mario white/indigo, Dry Bowser charcoal/crimson, Daisy orange/white, Yellow Toad).
   - Dynamic first-person cockpit hood customization in `renderCockpit()` with custom hood colors, racing stripes, and steering wheel emblems.
4. **Character Selection Screen UI (400x300 Buffer)**:
   - High-contrast retro arcade card layout displaying 4 characters side-by-side with animated idle engine rumble and exhaust effects.
   - Dual-cursor system supporting independent navigation for Player 1 (WASD / Space) and Player 2 (Arrows / Enter) with ready lock-in states.
   - Seamless transition to race initialization, feeding selected attributes into `GameCanvas` physics and Milestone 2 AI opponent fleets.

---

## 2. Character Roster & Numerical Stat Definitions

### 2.1 Character Specifications Matrix

| Attribute | Mario | Bowser | Peach | Toad | Baseline |
| :--- | :---: | :---: | :---: | :---: | :---: |
| **Character ID** | `0` | `1` | `2` | `3` | - |
| **Archetype** | **Balanced** | **Heavy / High Speed** | **Light / High Handling** | **Turbo Acceleration** | - |
| **Title / Subtitle** | "ALL-ROUNDER" | "HEAVYWEIGHT TITAN" | "AGILITY PRINCESS" | "TURBO ROCKET" | - |
| **Top Speed (`maxSpeed`)** | `160.0 km/h` | `175.0 km/h` (+9.4%) | `152.0 km/h` (-5.0%) | `148.0 km/h` (-7.5%) | `160.0` |
| **Acceleration (`accel`)** | `110.0 m/s²` | `85.0 m/s²` (-22.7%) | `125.0 m/s²` (+13.6%) | `140.0 m/s²` (+27.3%) | `110.0` |
| **Turn Rate (`turnRate`)** | `2.80 rad/s` | `2.20 rad/s` (-21.4%) | `3.40 rad/s` (+21.4%) | `3.10 rad/s` (+10.7%) | `2.40` |
| **Drift Factor (`driftFactor`)** | `1.00` | `0.85` | `1.25` | `1.15` | `1.00` |
| **Weight (`weight`)** | `1.00` (Medium) | `1.60` (Heavy) | `0.75` (Light) | `0.65` (Feather) | `1.00` |
| **Spinout Recovery Rate** | `1.00` (0.45s) | `0.85` (0.53s) | `1.15` (0.39s) | `1.35` (0.33s) | `1.00` |
| **Primary Kart Color** | Crimson Red (`#DC1414`) | Dark Green (`#23732D`) | Princess Pink (`#F582AF`) | Royal Blue (`#195FDC`) | `#B41414` |
| **Secondary Accent Color** | Crisp White (`#FFFFFF`) | Fiery Orange (`#F08C14`) | Crown Gold (`#FFD700`) | Mushroom White (`#FAFAFA`) | `#F0F0F0` |
| **Multiplayer Alt Color** | Fire White (`#F0F0FA`) | Charcoal Gray (`#3C3C41`) | Daisy Orange (`#FFB432`) | Yellow Toad (`#F5D21E`) | - |
| **Cockpit Grip Color** | White grip, red cap | Gunmetal with gold studs | Gold luxury grip | White grip, blue cap | White |

### 2.2 CharacterDef Data Structure

The character specification should be declared as a clean, immutable contract in `game_canvas.h`:

```cpp
struct CharacterDef {
    int id;                     // 0: Mario, 1: Bowser, 2: Peach, 3: Toad
    QString name;               // Display name ("MARIO", "BOWSER", "PEACH", "TOAD")
    QString archetype;          // Archetype label ("BALANCED", "HEAVY", "AGILITY", "ACCEL")
    QString description;        // Narrative trait summary
    QColor primaryColor;        // Main chassis/hood color
    QColor secondaryColor;      // Racing stripe / accent color
    QColor altColor;            // Alternate palette for P2 duplicate pick
    SpriteType spriteType;      // Primary 32x32 billboard sprite
    SpriteType altSpriteType;   // Alternate palette billboard sprite

    // Exact numerical physics modifiers
    double maxSpeed;            // Top speed on flat tarmac (km/h)
    double accel;               // Forward acceleration rate (units/s^2)
    double turnRate;            // Base steering rotation rate (rad/s)
    double driftFactor;         // Drift lateral slide multiplier & mini-turbo charge rate
    double weight;              // Collision pushback resistance & off-road momentum
    double recoveryRate;        // Multiplier for hazard/spinout recovery speed

    // Normalized UI Stat Pips (1 to 5)
    int speedPips;              // 1 to 5 pips
    int accelPips;              // 1 to 5 pips
    int handlingPips;           // 1 to 5 pips
    int weightPips;             // 1 to 5 pips
};

// Global immutable character roster (4 distinct racers)
inline const CharacterDef ROSTER[4] = {
    {
        0,
        "MARIO",
        "BALANCED",
        "All-around master with balanced top speed, responsive grip, and reliable acceleration.",
        QColor(220, 20, 20),
        QColor(245, 245, 245),
        QColor(240, 240, 255),
        SpriteType::KART_MARIO,
        SpriteType::KART_MARIO_ALT,
        160.0,
        110.0,
        2.80,
        1.00,
        1.00,
        1.00,
        3, 3, 3, 3   // Perfectly balanced 3/3/3/3
    },
    {
        1,
        "BOWSER",
        "HEAVYWEIGHT TITAN",
        "Tremendous 175 km/h top speed and crushing collision weight; slower initial throttle.",
        QColor(35, 115, 45),
        QColor(240, 140, 20),
        QColor(60, 60, 65),
        SpriteType::KART_BOWSER,
        SpriteType::KART_BOWSER_ALT,
        175.0,
        85.0,
        2.20,
        0.85,
        1.60,
        0.85,
        5, 1, 1, 5   // 5 Speed, 1 Accel, 1 Handling, 5 Weight
    },
    {
        2,
        "PEACH",
        "AGILITY SPECIALIST",
        "Extreme 3.4 rad/s cornering agility and high drift control for mastering hairpin bends.",
        QColor(245, 130, 175),
        QColor(255, 215, 0),
        QColor(255, 180, 50),
        SpriteType::KART_PEACH,
        SpriteType::KART_PEACH_ALT,
        152.0,
        125.0,
        3.40,
        1.25,
        0.75,
        1.15,
        2, 4, 5, 2   // 2 Speed, 4 Accel, 5 Handling, 2 Weight
    },
    {
        3,
        "TOAD",
        "TURBO ACCELERATION",
        "Explosive 140 m/s² acceleration and lightning-fast hazard recovery off the line.",
        QColor(25, 95, 220),
        QColor(245, 245, 245),
        QColor(245, 210, 30),
        SpriteType::KART_TOAD,
        SpriteType::KART_TOAD_ALT,
        148.0,
        140.0,
        3.10,
        1.15,
        0.65,
        1.35,
        2, 5, 4, 1   // 2 Speed, 5 Accel, 4 Handling, 1 Weight
    }
};
```

### 2.3 Physics Integration Mechanics

When a character is selected, their stats directly replace the current hardcoded constants in `GameCanvas`:
1. **Throttle Calculation**:
   $$\Delta v = \text{accel} \cdot \left(1.0 - 0.35 \cdot \frac{v}{\text{maxSpeed}}\right) \cdot \Delta t$$
   - Toad reaches 100 km/h in $<0.9\text{s}$, giving an immediate arcade surge off the line.
   - Bowser accelerates gradually but cruises past all other karts on the long straightaways.
2. **Steering & Cornering**:
   $$\Delta \theta = \text{steerDir} \cdot \text{turnRate} \cdot \text{std::clamp}\left(\frac{|v|}{50.0}, 0.70, 1.30\right) \cdot \Delta t$$
   - Currently, `game_canvas.cpp:421` hardcodes `3.4`. Updating this to multiply by `this->turnRate` ensures Peach can carve sharp chicanes without braking, while Bowser requires precision racing lines.
3. **Weight & Off-Road Resistance**:
   $$\text{offroadSpeedMultiplier} = 0.45 \cdot (0.8 + 0.2 \cdot \text{weight})$$
   - Heavier karts (Bowser: 1.6) carry momentum and suffer less severe drag through off-road sand/grass compared to featherweight karts (Toad: 0.65).
4. **Spinout Recovery Rate**:
   $$\text{spinDuration} = \frac{0.45\text{s}}{\text{recoveryRate}}$$
   - Toad recovers from a banana or lava clip in only $0.33\text{s}$, while Bowser takes $0.53\text{s}$.

---

## 3. Visual Customization & SpriteAtlas Generation

### 3.1 SpriteAtlas Architectural Extension

In `sprite_atlas.h`, add 8 dedicated kart sprite types:
```cpp
enum class SpriteType {
    // Pickups ...
    QUESTION_BOX, MUSHROOM, ROCKET, BANANA, GREEN_SHELL, INK_BLOOPER,
    // Obstacles ...
    GREEN_PIPE, THWOMP, OIL_SLICK, COIN, BANANA_DROPPED, SHELL_PROJECTILE,

    // Milestone 1 Kart Billboard Sprites
    KART_MARIO,         // Red chassis, white 'M' cap, blue overalls
    KART_BOWSER,        // Green spiked shell, orange hair, horns
    KART_PEACH,         // Pink chassis, blonde hair, golden crown
    KART_TOAD,          // Blue chassis, white mushroom cap with red spots

    // Alternate Palettes (Multiplayer Duplicate Support)
    KART_MARIO_ALT,     // Fire Mario: White chassis, red cap
    KART_BOWSER_ALT,    // Dry Bowser: Charcoal gray shell, crimson spikes
    KART_PEACH_ALT,     // Daisy Variant: Yellow-orange chassis, brown hair
    KART_TOAD_ALT       // Yellow Toad: Yellow chassis, yellow mushroom spots
};
```

### 3.2 32x32 Pixel-Art Anatomical Anatomy

Every kart sprite is designed with authentic 16-bit retro pixel density:

```
 y: 0..3   [ Crown / Horns / Mushroom Cap Top / Cap Top ]
 y: 4..10  [ Driver Head / Face / Cap Visor / Shell Rim ]
 y: 11..16 [ Shoulders / Racing Suit / Cockpit Lip / Steering Wheel ]
 y: 17..23 [ Kart Chassis / Side Pods / Racing Stripe / Engine Block ]
 y: 24..29 [ Rear Tires (Left: 4..8, Right: 23..27) / Twin Chrome Exhausts ]
 y: 30..31 [ Ground Contact Shadows ]
```

### 3.3 Procedural Pixel Generation Implementations

The following procedural routines should be added to `sprite_atlas.cpp`:

#### 1. Mario Kart Sprite (`generateKartMario`)
```cpp
void SpriteAtlas::generateKartMario()
{
    QImage img(SPRITE_SIZE, SPRITE_SIZE, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    QRgb red       = qRgba(220, 25, 25, 255);
    QRgb darkRed   = qRgba(150, 15, 15, 255);
    QRgb blue      = qRgba(30, 80, 200, 255);
    QRgb white     = qRgba(250, 250, 250, 255);
    QRgb skin      = qRgba(255, 200, 150, 255);
    QRgb brown     = qRgba(90, 50, 20, 255);
    QRgb tire      = qRgba(25, 25, 28, 255);
    QRgb rim       = qRgba(165, 170, 180, 255);
    QRgb chrome    = qRgba(210, 215, 225, 255);
    QRgb shadow    = qRgba(15, 15, 15, 160);

    // Tires (Left & Right)
    fillRect(img, 3, 20, 6, 10, tire);
    fillRect(img, 23, 20, 6, 10, tire);
    fillRect(img, 5, 23, 2, 4, rim);
    fillRect(img, 25, 23, 2, 4, rim);

    // Kart Chassis
    fillRect(img, 8, 20, 16, 8, red);
    fillRect(img, 7, 23, 18, 5, darkRed);
    fillRect(img, 15, 20, 2, 8, white); // Center racing stripe

    // Twin Exhausts
    fillRect(img, 9, 25, 3, 4, chrome);
    fillRect(img, 20, 25, 3, 4, chrome);

    // Overalls & Shoulders
    fillRect(img, 11, 14, 10, 6, blue);
    fillRect(img, 13, 15, 6, 5, red); // Shirt back

    // Head, Hair & Red Cap
    fillRect(img, 12, 10, 8, 4, skin);
    fillRect(img, 11, 11, 2, 3, brown); // Hair
    fillRect(img, 19, 11, 2, 3, brown);
    fillCircle(img, 16, 7, 4, red, darkRed); // Cap dome
    fillRect(img, 12, 8, 8, 2, red);         // Visor
    fillCircle(img, 16, 6, 2, white, white); // White 'M' badge
    setPixel(img, 16, 6, red);               // Red M pixel

    // Ground Shadow
    fillCircle(img, 16, 30, 8, shadow, shadow);

    sprites[SpriteType::KART_MARIO] = img;
}
```

#### 2. Bowser Kart Sprite (`generateKartBowser`)
```cpp
void SpriteAtlas::generateKartBowser()
{
    QImage img(SPRITE_SIZE, SPRITE_SIZE, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    QRgb darkGreen = qRgba(30, 105, 40, 255);
    QRgb deepGreen = qRgba(20, 75, 25, 255);
    QRgb orange    = qRgba(240, 130, 15, 255);
    QRgb hornBone  = qRgba(245, 235, 180, 255);
    QRgb rimWhite  = qRgba(245, 245, 235, 255);
    QRgb brassGold = qRgba(220, 175, 30, 255);
    QRgb tire      = qRgba(20, 20, 22, 255);
    QRgb chrome    = qRgba(180, 185, 195, 255);

    // Massive Heavy Tires
    fillRect(img, 2, 18, 7, 12, tire);
    fillRect(img, 23, 18, 7, 12, tire);
    fillRect(img, 4, 22, 3, 5, brassGold); // Studded brass hub
    fillRect(img, 25, 22, 3, 5, brassGold);

    // Spiked Kart Chassis
    fillRect(img, 8, 20, 16, 9, darkGreen);
    fillRect(img, 7, 24, 18, 5, deepGreen);
    fillRect(img, 15, 20, 2, 9, orange); // Fiery orange stripe

    // Oversized Chrome Exhausts
    fillRect(img, 9, 26, 3, 5, chrome);
    fillRect(img, 20, 26, 3, 5, chrome);

    // Spiked Shell Back
    fillCircle(img, 16, 14, 6, darkGreen, rimWhite);
    setPixel(img, 13, 13, rimWhite); // Shell bone spikes
    setPixel(img, 19, 13, rimWhite);
    setPixel(img, 16, 11, rimWhite);

    // Bowser Horns & Fiery Red-Orange Hair
    fillRect(img, 13, 4, 6, 4, orange); // Fiery hair crest
    fillRect(img, 14, 3, 4, 2, qRgba(255, 60, 20, 255));
    // Curved Horns
    fillRect(img, 9, 5, 3, 2, hornBone);
    setPixel(img, 8, 4, hornBone);
    fillRect(img, 20, 5, 3, 2, hornBone);
    setPixel(img, 23, 4, hornBone);

    sprites[SpriteType::KART_BOWSER] = img;
}
```

#### 3. Peach Kart Sprite (`generateKartPeach`)
```cpp
void SpriteAtlas::generateKartPeach()
{
    QImage img(SPRITE_SIZE, SPRITE_SIZE, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    QRgb pink      = qRgba(245, 130, 175, 255);
    QRgb darkPink  = qRgba(200, 80, 130, 255);
    QRgb gold      = qRgba(255, 215, 0, 255);
    QRgb blonde    = qRgba(255, 220, 60, 255);
    QRgb darkBlond = qRgba(215, 180, 30, 255);
    QRgb white     = qRgba(255, 255, 255, 255);
    QRgb gemBlue   = qRgba(40, 170, 255, 255);
    QRgb tire      = qRgba(30, 30, 35, 255);

    // Slim Agility Tires
    fillRect(img, 4, 21, 5, 9, tire);
    fillRect(img, 23, 21, 5, 9, tire);
    fillRect(img, 5, 24, 2, 3, gold); // Gold wheel nuts
    fillRect(img, 24, 24, 2, 3, gold);

    // Sleek Pink Chassis
    fillRect(img, 8, 20, 16, 8, pink);
    fillRect(img, 7, 23, 18, 5, darkPink);
    fillRect(img, 15, 20, 2, 8, white); // White racing stripe

    // Flowing Blonde Hair & Dress
    fillRect(img, 10, 14, 12, 6, pink); // Pink dress shoulders
    fillRect(img, 9, 10, 14, 7, blonde);
    fillRect(img, 10, 12, 12, 4, darkBlond);

    // Royal Golden Crown
    fillRect(img, 13, 5, 6, 3, gold);
    setPixel(img, 12, 4, gold); // Crown tips
    setPixel(img, 15, 3, gold);
    setPixel(img, 18, 4, gold);
    setPixel(img, 15, 5, gemBlue); // Center sapphire jewel

    sprites[SpriteType::KART_PEACH] = img;
}
```

#### 4. Toad Kart Sprite (`generateKartToad`)
```cpp
void SpriteAtlas::generateKartToad()
{
    QImage img(SPRITE_SIZE, SPRITE_SIZE, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    QRgb blue      = qRgba(25, 95, 220, 255);
    QRgb darkBlue  = qRgba(15, 65, 160, 255);
    QRgb white     = qRgba(255, 255, 255, 255);
    QRgb redSpot   = qRgba(225, 30, 30, 255);
    QRgb skin      = qRgba(255, 210, 170, 255);
    QRgb goldVest  = qRgba(255, 200, 30, 255);
    QRgb tire      = qRgba(28, 28, 30, 255);
    QRgb chrome    = qRgba(215, 220, 230, 255);

    // Lightweight Tires
    fillRect(img, 4, 21, 5, 9, tire);
    fillRect(img, 23, 21, 5, 9, tire);
    fillRect(img, 5, 24, 2, 3, chrome);
    fillRect(img, 24, 24, 2, 3, chrome);

    // Compact Blue Chassis
    fillRect(img, 8, 20, 16, 8, blue);
    fillRect(img, 7, 23, 18, 5, darkBlue);
    fillRect(img, 15, 20, 2, 8, white); // White stripe

    // Blue Vest with Gold Trim
    fillRect(img, 11, 14, 10, 5, blue);
    fillRect(img, 12, 14, 1, 5, goldVest);
    fillRect(img, 18, 14, 1, 5, goldVest);
    fillRect(img, 13, 11, 6, 4, skin); // Neck/head

    // Mushroom Cap (White dome with Red Spots)
    fillCircle(img, 16, 7, 6, white, qRgba(220, 220, 220, 255));
    // Red Spots
    fillCircle(img, 16, 5, 2, redSpot, redSpot); // Top spot
    fillRect(img, 11, 7, 2, 3, redSpot);         // Left spot
    fillRect(img, 19, 7, 2, 3, redSpot);         // Right spot

    sprites[SpriteType::KART_TOAD] = img;
}
```

### 3.4 Alternate Palette Variations

For duplicate selections in Multiplayer (e.g. Player 1 selects Mario and Player 2 also selects Mario):
- `KART_MARIO_ALT`: Fire Mario palette swap (Body: Crisp White `#F0F0FA`, Overalls: Indigo `#202050`, Cap: Crimson Red).
- `KART_BOWSER_ALT`: Dry Bowser palette swap (Body: Charcoal `#3C3C41`, Spikes: Crimson `#E02020`, Horns: Bleached Bone).
- `KART_PEACH_ALT`: Daisy palette swap (Body: Tangerine Orange `#FFB432`, Hair: Chestnut Brown, Trim: Mint White).
- `KART_TOAD_ALT`: Yellow Toad palette swap (Body: Goldenrod `#F5D21E`, Spots: Sunflower Yellow).

### 3.5 Dynamic First-Person Cockpit Rendering

In `game_canvas.cpp`, update `renderCockpit()` to draw character-specific hood colors and steering wheels:
```cpp
void GameCanvas::renderCockpit()
{
    QRgb *pixels = reinterpret_cast<QRgb*>(screenBuffer.bits());
    const auto &charDef = ROSTER[selectedCharacterIndex];

    // Boost flame / sparks effect
    if (mushroomBoostTimer > 0.0) {
        int cx = BUFFER_WIDTH / 2;
        for (int i = 0; i < 20; ++i) {
            int px = cx + (rand() % 90 - 45);
            int py = BUFFER_HEIGHT - 35 + (rand() % 35);
            if (px >= 0 && px < BUFFER_WIDTH && py >= 0 && py < BUFFER_HEIGHT) {
                pixels[py * BUFFER_WIDTH + px] = (rand() % 2 == 0) ? qRgb(255, 230, 50) : qRgb(255, 80, 20);
            }
        }
    }

    // First-person kart hood at bottom center
    int hoodCenter = BUFFER_WIDTH / 2;
    int hoodTop = BUFFER_HEIGHT - 35;

    QRgb hoodColor = (mushroomBoostTimer > 0.0) ? qRgb(255, 90, 20) : charDef.primaryColor.rgb();
    QRgb stripeColor = charDef.secondaryColor.rgb();

    for (int y = hoodTop; y < BUFFER_HEIGHT; ++y) {
        int halfWidth = static_cast<int>(30.0 + (y - hoodTop) * 2.2);
        for (int x = hoodCenter - halfWidth; x < hoodCenter + halfWidth; ++x) {
            if (x >= 0 && x < BUFFER_WIDTH) {
                // Center racing stripe (4px width)
                if (std::abs(x - hoodCenter) <= 2) {
                    pixels[y * BUFFER_WIDTH + x] = stripeColor;
                } else {
                    pixels[y * BUFFER_WIDTH + x] = hoodColor;
                }
            }
        }
    }

    // Steering wheel (Rotated pixel ring)
    int wheelY = BUFFER_HEIGHT - 22;
    int wheelX = BUFFER_WIDTH / 2 + static_cast<int>(steeringVisualAngle * 30.0);
    int wheelR = 14;

    QRgb gripColor = qRgb(240, 240, 240);
    if (selectedCharacterIndex == 1) gripColor = qRgb(80, 85, 90);       // Bowser gunmetal
    else if (selectedCharacterIndex == 2) gripColor = qRgb(255, 215, 50); // Peach gold grip

    for (int y = wheelY - wheelR; y <= wheelY + wheelR; ++y) {
        if (y < 0 || y >= BUFFER_HEIGHT) continue;
        for (int x = wheelX - wheelR; x <= wheelX + wheelR; ++x) {
            if (x < 0 || x >= BUFFER_WIDTH) continue;
            int dist2 = (x - wheelX) * (x - wheelX) + (y - wheelY) * (y - wheelY);
            if (dist2 >= (wheelR - 3) * (wheelR - 3) && dist2 <= wheelR * wheelR) {
                pixels[y * BUFFER_WIDTH + x] = gripColor;
            }
        }
    }
}
```

---

## 4. Character Selection Screen UI & Architecture

### 4.1 Screen Geometry & Layout (400x300 Buffer)

The screen is partitioned into 4 vertical zones:
1. **Header ($y = 0 \dots 42$)**: Title banner "SELECT YOUR RACER", game mode badge, and separator line.
2. **Character Cards Row ($y = 46 \dots 224$)**:
   - Total canvas width: 400 px.
   - 4 cards: Width $W = 88\text{ px}$, Height $H = 174\text{ px}$, Gap $G = 8\text{ px}$.
   - Left Margin: $\frac{400 - (4 \times 88 + 3 \times 8)}{2} = \frac{400 - 376}{2} = 12\text{ px}$.
   - Card positions:
     - Card 0 (Mario): $X = 12$
     - Card 1 (Bowser): $X = 108$
     - Card 2 (Peach): $X = 204$
     - Card 3 (Toad): $X = 300$
3. **Character Description Banner ($y = 228 \dots 264$)**:
   - Semi-transparent arcade box displaying selected racer's detailed bio, strengths, and advice.
4. **Footer Key Legend ($y = 268 \dots 298$)**:
   - Navigation instructions dynamically tailored for Single Player vs Multiplayer.

```
+-----------------------------------------------------------------------+
|  SELECT YOUR RACER                    [MODE: PLAY VS AI]   (y: 0..42) |
+=======================================================================+
| [CARD 0: MARIO]  [CARD 1: BOWSER]  [CARD 2: PEACH]  [CARD 3: TOAD]    |
| +-------------+  +--------------+  +-------------+  +------------+    |
| |   MARIO     |  |    BOWSER    |  |    PEACH    |  |    TOAD    |    |
| | [Animated]  |  |  [Animated]  |  | [Animated]  |  | [Animated] |    |
| | [  Kart  ]  |  |  [  Kart  ]  |  | [  Kart  ]  |  | [  Kart  ] |    |
| |  BALANCED   |  |  HEAVYWEIGHT |  |   AGILITY   |  |TURBO ACCEL |    |
| | SPD ■■■□□   |  | SPD ■■■■■    |  | SPD ■■□□□   |  | SPD ■■□□□  |    |
| | ACC ■■■□□   |  | ACC ■□□□□    |  | ACC ■■■■□   |  | ACC ■■■■■  |    |
| | HND ■■■□□   |  | HND ■□□□□    |  | HND ■■■■■   |  | HND ■■■■□  |    |
| | WGT ■■■□□   |  | WGT ■■■■■    |  | WGT ■■□□□   |  | WGT ■□□□□  |    |
| +-------------+  +--------------+  +-------------+  +------------+    |
|   ▲ [P1 CURSOR]                                          (y: 46..224) |
+-----------------------------------------------------------------------+
| ★ MARIO: Balanced top speed & grip. Suited for all tracks. (y: 228..264) |
+-----------------------------------------------------------------------+
| P1: [A/D or ◄/►] Choose    [Space / Enter] Confirm    [Esc] Back to Menu |
+-----------------------------------------------------------------------+
```

### 4.2 Animation Dynamics

1. **Idle Engine Rumble**:
   - Kart preview bobs up and down with sine oscillation:
     $$y_{\text{offset}} = \sin\left(\text{globalTime} \cdot 8.0 + i \cdot 1.5\right) \cdot 1.8\text{ px}$$
2. **Highlighted Card Pulsing Border**:
   - Pulsing alpha intensity for active cursor:
     $$\alpha = 180 + 75 \cdot \sin(\text{globalTime} \cdot 12.0)$$
   - Gold border for P1 (`QColor(255, 215, 0, \alpha)`).
   - Cyan border for P2 (`QColor(0, 225, 255, \alpha)`).
3. **Selection Lock-in Flash**:
   - When confirmed (`p1Ready = true`), an animated banner `✓ READY!` flashes on the card with emerald green background (`QColor(30, 180, 50, 230)`).

### 4.3 Dual-Cursor Multi-Player Support

The Character Selection screen supports simultaneous 2-Player selection on a single shared screen:
- **State Tracking Variables**:
  ```cpp
  int p1Cursor = 0;       // 0 to 3 (Default: Mario)
  bool p1Ready = false;   // Confirmed state
  int p2Cursor = 1;       // 0 to 3 (Default: Bowser)
  bool p2Ready = false;   // Confirmed state
  ```
- **Cursor Rendering Logic**:
  - If P1 points to Card $i$: Render golden `[P1]` badge with down-arrow `▼` at top-left of Card $i$.
  - If P2 points to Card $j$: Render cyan `[P2]` badge with down-arrow `▼` at top-right of Card $j$.
  - If $i == j$ (Both on same card):
    - Border alternates or splits diagonally (Gold & Cyan).
    - Top displays `[P1]` on left, `[P2]` on right.
    - Card displays indicator: `P2: (ALT PALETTE)`.
- **Ready State Resolution**:
  - In Solo Mode (`PLAY_VS_AI`, `PRACTICE`): P1 confirmation immediately starts race.
  - In Multiplayer (`MULTIPLAYER`): Both `p1Ready && p2Ready` must be `true`. A 0.8s flash animation triggers before launching the split-screen race.

### 4.4 Complete UI Rendering Routine (`renderCharacterSelect`)

```cpp
void GameCanvas::renderCharacterSelect()
{
    QPainter painter(&screenBuffer);
    painter.setRenderHint(QPainter::Antialiasing, false);

    // 1. Dark Vignette Background
    painter.fillRect(QRect(0, 0, BUFFER_WIDTH, BUFFER_HEIGHT), QColor(16, 20, 32));
    
    // Top & Bottom Accent Lines
    painter.setPen(QPen(QColor(255, 215, 0), 2));
    painter.drawLine(0, 42, BUFFER_WIDTH, 42);
    painter.drawLine(0, 226, BUFFER_WIDTH, 226);

    // 2. Title & Mode Banner
    QFont titleFont("Arial", 11, QFont::Bold);
    painter.setFont(titleFont);
    painter.setPen(QColor(0, 0, 0, 180));
    painter.drawText(QRect(1, 11, BUFFER_WIDTH, 22), Qt::AlignCenter, "SELECT YOUR RACER");
    painter.setPen(QColor(255, 215, 0));
    painter.drawText(QRect(0, 10, BUFFER_WIDTH, 22), Qt::AlignCenter, "SELECT YOUR RACER");

    QFont subFont("Arial", 6, QFont::Bold);
    painter.setFont(subFont);
    QString modeStr = (selectedGameMode == GameMode::PLAY_VS_AI) ? "MODE: PLAY VS AI (GRAND PRIX)" :
                      (selectedGameMode == GameMode::MULTIPLAYER) ? "MODE: 2-PLAYER SPLIT-SCREEN" : "MODE: PRACTICE";
    QColor modeColor = (selectedGameMode == GameMode::MULTIPLAYER) ? QColor(0, 230, 255) : QColor(255, 200, 50);
    painter.setPen(modeColor);
    painter.drawText(QRect(0, 28, BUFFER_WIDTH, 12), Qt::AlignCenter, modeStr);

    const auto &atlas = SpriteAtlas::instance();
    double t = globalTime;

    // 3. Render 4 Character Cards
    int cardY = 46;
    int cardW = 88;
    int cardH = 176;
    int gap = 8;
    int startX = (BUFFER_WIDTH - (4 * cardW + 3 * gap)) / 2; // 12 px

    for (int i = 0; i < 4; ++i) {
        int cx = startX + i * (cardW + gap);
        QRect cardRect(cx, cardY, cardW, cardH);
        const auto &c = ROSTER[i];

        bool isP1Here = (p1Cursor == i);
        bool isP2Here = (selectedGameMode == GameMode::MULTIPLAYER && p2Cursor == i);

        // Card Background Fill
        painter.fillRect(cardRect, QColor(22, 26, 40, 235));

        // Border Styling based on Cursors
        if (isP1Here && isP2Here) {
            painter.setPen(QPen(QColor(255, 215, 0), 2));
            painter.drawRect(cardRect);
            painter.setPen(QPen(QColor(0, 225, 255), 1));
            painter.drawRect(cardRect.adjusted(2, 2, -2, -2));
        } else if (isP1Here) {
            int glow = static_cast<int>(200 + 55 * std::sin(t * 12.0));
            painter.setPen(QPen(QColor(255, 215, 0, glow), 2));
            painter.drawRect(cardRect);
        } else if (isP2Here) {
            int glow = static_cast<int>(200 + 55 * std::sin(t * 12.0));
            painter.setPen(QPen(QColor(0, 225, 255, glow), 2));
            painter.drawRect(cardRect);
        } else {
            painter.setPen(QPen(QColor(60, 68, 85), 1));
            painter.drawRect(cardRect);
        }

        // Card Header Banner (Character Name)
        QRect headerRect(cx + 1, cardY + 1, cardW - 2, 18);
        painter.fillRect(headerRect, c.primaryColor);
        painter.setFont(QFont("Arial", 8, QFont::Bold));
        painter.setPen(Qt::white);
        painter.drawText(headerRect, Qt::AlignCenter, c.name);

        // Animated Kart Preview Box
        QRect previewRect(cx + 6, cardY + 22, cardW - 12, 48);
        painter.fillRect(previewRect, QColor(14, 16, 26));
        painter.setPen(QColor(40, 46, 62));
        painter.drawRect(previewRect);

        // Shadow under kart
        int bobY = static_cast<int>(std::sin(t * 8.0 + i * 1.5) * 1.8);
        painter.setBrush(QColor(0, 0, 0, 120));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(cx + 24, cardY + 58, 40, 8);

        // Billboard Kart Sprite (Scaled 1.2x to 38x38)
        SpriteType st = (isP2Here && isP1Here && p2Ready) ? c.altSpriteType : c.spriteType;
        const QImage &kartImg = atlas.getSprite(st);
        painter.drawImage(QRect(cx + 25, cardY + 26 + bobY, 38, 38), kartImg);

        // Archetype Subtitle Badge
        QRect badgeRect(cx + 6, cardY + 73, cardW - 12, 12);
        painter.setFont(QFont("Arial", 5, QFont::Bold));
        painter.setPen(c.secondaryColor);
        painter.drawText(badgeRect, Qt::AlignCenter, c.archetype);

        // Stat Bars Helper Lambda
        auto drawPipBar = [&](int y, const QString &label, int pips, QColor barCol) {
            painter.setFont(QFont("Arial", 5, QFont::Bold));
            painter.setPen(QColor(180, 190, 205));
            painter.drawText(QRect(cx + 6, y, 22, 10), Qt::AlignLeft | Qt::AlignVCenter, label);

            int bx = cx + 30;
            int pipW = 8;
            int pipH = 6;
            for (int p = 0; p < 5; ++p) {
                QRect pipRect(bx + p * (pipW + 2), y + 2, pipW, pipH);
                if (p < pips) {
                    painter.fillRect(pipRect, barCol);
                } else {
                    painter.fillRect(pipRect, QColor(36, 40, 52));
                }
                painter.setPen(QColor(18, 20, 28));
                painter.drawRect(pipRect);
            }
        };

        // Render 4 Stat Bars
        drawPipBar(cardY + 88,  "SPD", c.speedPips,    QColor(255, 70, 50));
        drawPipBar(cardY + 104, "ACC", c.accelPips,    QColor(255, 205, 30));
        drawPipBar(cardY + 120, "HND", c.handlingPips, QColor(40, 200, 255));
        drawPipBar(cardY + 136, "WGT", c.weightPips,   QColor(190, 110, 255));

        // Ready Overlay
        if ((isP1Here && p1Ready) || (isP2Here && p2Ready)) {
            QRect readyRect(cx + 4, cardY + 154, cardW - 8, 16);
            painter.fillRect(readyRect, QColor(30, 180, 60, 230));
            painter.setFont(QFont("Arial", 6, QFont::Bold));
            painter.setPen(Qt::white);
            QString rdyText = (isP1Here && p1Ready && isP2Here && p2Ready) ? "BOTH READY!" :
                              (isP1Here && p1Ready) ? "P1 READY!" : "P2 READY!";
            painter.drawText(readyRect, Qt::AlignCenter, rdyText);
        }

        // P1 & P2 Cursor Badges above card
        if (isP1Here) {
            painter.fillRect(QRect(cx + 2, cardY - 14, 28, 12), QColor(255, 215, 0));
            painter.setFont(QFont("Arial", 6, QFont::Bold));
            painter.setPen(Qt::black);
            painter.drawText(QRect(cx + 2, cardY - 14, 28, 12), Qt::AlignCenter, "P1 ▼");
        }
        if (isP2Here) {
            painter.fillRect(QRect(cx + cardW - 30, cardY - 14, 28, 12), QColor(0, 225, 255));
            painter.setFont(QFont("Arial", 6, QFont::Bold));
            painter.setPen(Qt::black);
            painter.drawText(QRect(cx + cardW - 30, cardY - 14, 28, 12), Qt::AlignCenter, "P2 ▼");
        }
    }

    // 4. Highlighted Character Description Banner
    int focusIndex = p1Cursor;
    const auto &focusChar = ROSTER[focusIndex];
    QRect descRect(12, 230, BUFFER_WIDTH - 24, 34);
    painter.fillRect(descRect, QColor(20, 24, 38, 220));
    painter.setPen(QColor(50, 60, 80));
    painter.drawRect(descRect);

    painter.setFont(QFont("Arial", 6, QFont::Bold));
    painter.setPen(focusChar.secondaryColor);
    painter.drawText(QRect(18, 233, BUFFER_WIDTH - 36, 12), Qt::AlignLeft, "★ " + focusChar.name + " (" + focusChar.archetype + ")");
    painter.setFont(QFont("Arial", 6));
    painter.setPen(QColor(220, 225, 235));
    painter.drawText(QRect(18, 246, BUFFER_WIDTH - 36, 15), Qt::AlignLeft, focusChar.description);

    // 5. Footer Key Legend
    QFont legFont("Arial", 6);
    painter.setFont(legFont);
    painter.setPen(QColor(160, 175, 195));
    if (selectedGameMode == GameMode::MULTIPLAYER) {
        painter.drawText(QRect(0, 272, BUFFER_WIDTH, 14), Qt::AlignCenter,
            "P1: [A / D] Select  [Space] OK   |   P2: [◄ / ►] Select  [Enter] OK   |   [Esc] Back");
    } else {
        painter.drawText(QRect(0, 272, BUFFER_WIDTH, 14), Qt::AlignCenter,
            "[◄ / ► or A / D] Select Racer    [Space / Enter] Confirm    [Esc] Back to Menu");
    }
}
```

---

## 5. Step-by-Step Implementation Plan

### Step 1: Update `SpriteAtlas` (`sprite_atlas.h` & `sprite_atlas.cpp`)
1. Add `KART_MARIO`, `KART_BOWSER`, `KART_PEACH`, `KART_TOAD`, and their 4 `_ALT` counterparts to `SpriteType`.
2. Add procedural generation methods: `generateKartMario()`, `generateKartBowser()`, `generateKartPeach()`, `generateKartToad()`, and `generateAltPalettes()`.
3. Call these methods inside `SpriteAtlas::generateAllSprites()`.

### Step 2: Declare Character Definitions in `game_canvas.h`
1. Define `struct CharacterDef`.
2. Instantiate `const CharacterDef ROSTER[4]`.
3. Add member variables in `GameCanvas`:
   ```cpp
   int p1Cursor = 0;
   bool p1Ready = false;
   int p2Cursor = 1;
   bool p2Ready = false;
   int selectedCharacterIndex = 0;
   int p2SelectedCharacterIndex = 1;
   ```

### Step 3: Implement `renderCharacterSelect()` in `game_canvas.cpp`
1. Add declaration in `game_canvas.h` and definition in `game_canvas.cpp`.
2. In `GameCanvas::updateGameLoop()`, branch on `currentGameState`:
   ```cpp
   if (currentGameState == GameState::CHARACTER_SELECT) {
       renderCharacterSelect();
       this->update();
       return;
   }
   ```

### Step 4: Wire Input Handling in `GameCanvas::keyPressEvent`
1. When `currentGameState == GameState::CHARACTER_SELECT`:
   - Single Player: `Key_A` / `Key_Left` wraps left; `Key_D` / `Key_Right` wraps right; `Key_Space` / `Key_Enter` confirms.
   - Multiplayer: `Key_A`/`Key_D` for P1, `Key_Left`/`Key_Right` for P2; `Key_Space` for P1 confirm, `Key_Return`/`Key_Enter` for P2 confirm.
   - `Key_Escape`: Unlocks ready state or transitions back to `GameState::TITLE_MENU`.

### Step 5: Transfer Selected Character Stats upon Race Launch
In `GameCanvas::startRace()`:
```cpp
const auto &c1 = ROSTER[p1Cursor];
this->maxSpeed = c1.maxSpeed;
this->accel = c1.accel;
this->turnRate = c1.turnRate;
this->selectedCharacterIndex = c1.id;

if (selectedGameMode == GameMode::MULTIPLAYER) {
    const auto &c2 = ROSTER[p2Cursor];
    this->p2MaxSpeed = c2.maxSpeed;
    this->p2Accel = c2.accel;
    this->p2TurnRate = c2.turnRate;
    this->p2SelectedCharacterIndex = c2.id;
}
```

### Step 6: Customize Cockpit View in `renderCockpit()`
Update `renderCockpit()` to use `ROSTER[selectedCharacterIndex].primaryColor` and `secondaryColor` for hood and steering wheel grip styling.

---

## 6. Verification & Acceptance Mapping

| Acceptance Criterion | Verification Method | Expected Outcome |
| :--- | :--- | :--- |
| **AC-02 (Menu Flow)** | Press Enter on "Play vs AI" or "Multiplayer" | Screen transitions smoothly into Character Selection UI. |
| **AC-03 (Distinct Characters)** | Inspect Character Selection cards | 4 distinct characters displayed (Mario, Bowser, Peach, Toad) with unique kart sprites, colors, and stat pips. |
| **AC-03 (Performance Differences)** | Race Mario vs Bowser | Bowser hits 175 km/h on straightaway; Mario corners tighter and accelerates faster. |
| **Edge Case E01 (Boundary Wrap)** | Press Left on Mario (0) or Right on Toad (3) | Cursor wraps cleanly between index 0 and 3 without crash or bounds violation. |
| **Edge Case E02 (Duplicate Selection)** | P1 and P2 both pick Mario in Multiplayer | P2 receives Fire Mario alternate palette with `[ALT PALETTE]` indicator. |
| **Edge Case E03 (Back Action)** | Press `Escape` on Character Selection | Returns to Main Menu; does not start race or leak input keys. |
| **Performance (60 FPS)** | Measure `dt` in `renderCharacterSelect` | Render execution time $<0.5\text{ ms}$, ensuring steady 60 FPS. |

---

## 7. Conclusion

This strategy provides an exact, production-ready blueprint for implementing Milestone 1 Character Selection. It fulfills all requirements from `ORIGINAL_REQUEST.md`, respects the `PROJECT.md` interface contracts, and establishes an effortless handoff for Milestone 2 (AI Opponents) and Milestone 4 (2P Split-Screen).
