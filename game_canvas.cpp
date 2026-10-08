#include "game_canvas.h"
#include <QPainter>
#include <QPainterPath>
#include <QKeyEvent>
#include <QFont>
#include <QRandomGenerator>
#include <algorithm>
#include <cmath>
#include <cstdlib>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

GameCanvas::GameCanvas(QWidget *parent)
    : QWidget(parent),
      screenBuffer(BUFFER_WIDTH, BUFFER_HEIGHT, QImage::Format_RGB32),
      trackVisualMap(TRACK_MAP_SIZE, TRACK_MAP_SIZE, QImage::Format_RGB32),
      trackMaskMap(TRACK_MAP_SIZE, TRACK_MAP_SIZE, QImage::Format_RGB32)
{
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(400, 300);

    initTrackMaps();
    setTrackLevel(TrackLevel::DONUT_PLAINS);

    connect(&gameTimer, &QTimer::timeout, this, &GameCanvas::updateGameLoop);
    gameTimer.start(16); // ~60 FPS
    frameTimer.start();
}

void GameCanvas::setTrackLevel(TrackLevel level)
{
    currentLevel = level;
    switch (currentLevel) {
    case TrackLevel::DONUT_PLAINS:
        generateDonutPlains();
        break;
    case TrackLevel::CHOCO_VALLEY:
        generateChocoValley();
        break;
    case TrackLevel::BOWSER_CASTLE:
        generateBowserCastle();
        break;
    }
    resetKart();
    update();
}

void GameCanvas::resetKart()
{
    if (currentLevel == TrackLevel::BOWSER_CASTLE) {
        kartX = 470.0;
        kartZ = 250.0;
    } else {
        kartX = 470.0;
        kartZ = 200.0;
    }
    kartAngle = 0.0; // Pointing forward along the track (East) through the start line
    kartSpeed = 0.0;
    steeringVisualAngle = 0.0;
    currentLap = 1;
    nextCheckpoint = 1;

    // Reset item & status state
    heldItem = PlayerItem::NONE;
    isRouletteActive = false;
    rouletteTimer = 0.0;
    mushroomBoostTimer = 0.0;
    isSpinning = false;
    spinTimer = 0.0;
    inkTimer = 0.0;

    // Reset celebration & race finish state
    isCelebrationActive = false;
    isRaceFinished = false;
    celebrationTimer = 0.0;
    flashIntensity = 0.0;
    hasPassedHalfway = false;
    firstStartCrossed = false;
    confetti.clear();
}

void GameCanvas::focusInEvent(QFocusEvent *event)
{
    QWidget::focusInEvent(event);
}

void GameCanvas::keyPressEvent(QKeyEvent *event)
{
    pressedKeys.insert(event->key());
    if (event->key() == Qt::Key_Space) {
        useHeldItem();
    } else if (event->key() == Qt::Key_R) {
        resetKart();
    } else if (event->key() == Qt::Key_F) {
        triggerCelebration("★ FINISH! ★", "COURSE CLEAR!");
    } else if (event->key() == Qt::Key_1) {
        setTrackLevel(TrackLevel::DONUT_PLAINS);
    } else if (event->key() == Qt::Key_2) {
        setTrackLevel(TrackLevel::CHOCO_VALLEY);
    } else if (event->key() == Qt::Key_3) {
        setTrackLevel(TrackLevel::BOWSER_CASTLE);
    }
}

void GameCanvas::keyReleaseEvent(QKeyEvent *event)
{
    pressedKeys.erase(event->key());
}

void GameCanvas::initTrackMaps()
{
    generateDonutPlains();
}

// -------------------------------------------------------------
// TRACK GENERATION (PROCEDURAL TEXTURES & MASKS)
// -------------------------------------------------------------

void GameCanvas::generateDonutPlains()
{
    trackVisualMap.fill(qRgb(34, 139, 34)); // Grass green
    trackMaskMap.fill(qRgb(0, 255, 0));     // Green = Offroad

    QPainter visPainter(&trackVisualMap);
    QPainter maskPainter(&trackMaskMap);

    visPainter.setRenderHint(QPainter::Antialiasing, true);
    maskPainter.setRenderHint(QPainter::Antialiasing, true);

    // Track path (Smooth oval loop)
    QPainterPath path;
    path.moveTo(512, 200);
    path.cubicTo(850, 200, 850, 800, 512, 800);
    path.cubicTo(180, 800, 180, 200, 512, 200);

    // Draw Curbs (Red-white outer border)
    QPen curbPen(QColor(220, 20, 60), 100);
    visPainter.setPen(curbPen);
    visPainter.drawPath(path);

    // Draw Asphalt Road
    QPen roadPen(QColor(60, 64, 68), 84);
    visPainter.setPen(roadPen);
    visPainter.drawPath(path);

    // Draw Mask Road (White = 100% speed)
    QPen maskRoadPen(QColor(255, 255, 255), 84);
    maskPainter.setPen(maskRoadPen);
    maskPainter.drawPath(path);

    // Finish Line
    visPainter.setPen(Qt::NoPen);
    visPainter.setBrush(QColor(255, 255, 255));
    for (int y = 158; y < 242; y += 12) {
        visPainter.drawRect(510, y, 6, 6);
        visPainter.drawRect(516, y + 6, 6, 6);
    }

    // Checkpoint line on mask (Yellow = Checkpoint)
    maskPainter.setPen(QPen(QColor(255, 255, 0), 10));
    maskPainter.drawLine(512, 158, 512, 242);

    visPainter.end();
    maskPainter.end();

    // Sprites along track
    worldSprites.clear();
    // Item Question Boxes on straightaways
    worldSprites.push_back({ 560.0, 200.0, 0.0, SpriteType::QUESTION_BOX, true, 0.0 });
    worldSprites.push_back({ 620.0, 200.0, 0.0, SpriteType::QUESTION_BOX, true, 0.0 });
    worldSprites.push_back({ 512.0, 800.0, 0.0, SpriteType::QUESTION_BOX, true, 0.0 });
    worldSprites.push_back({ 450.0, 800.0, 0.0, SpriteType::QUESTION_BOX, true, 0.0 });

    // Roadside obstacles (Green Pipes)
    worldSprites.push_back({ 850.0, 480.0, 0.0, SpriteType::GREEN_PIPE, true, 0.0 });
    worldSprites.push_back({ 850.0, 520.0, 0.0, SpriteType::GREEN_PIPE, true, 0.0 });
    worldSprites.push_back({ 180.0, 480.0, 0.0, SpriteType::GREEN_PIPE, true, 0.0 });
    worldSprites.push_back({ 180.0, 520.0, 0.0, SpriteType::GREEN_PIPE, true, 0.0 });

    // Track coins
    worldSprites.push_back({ 400.0, 200.0, 0.0, SpriteType::COIN, true, 0.0 });
    worldSprites.push_back({ 350.0, 200.0, 0.0, SpriteType::COIN, true, 0.0 });
    worldSprites.push_back({ 600.0, 800.0, 0.0, SpriteType::COIN, true, 0.0 });

    // Hazard banana
    worldSprites.push_back({ 780.0, 320.0, 0.0, SpriteType::BANANA_DROPPED, true, 0.0 });
}

void GameCanvas::generateChocoValley()
{
    trackVisualMap.fill(qRgb(194, 142, 87)); // Tan Desert Sand
    trackMaskMap.fill(qRgb(0, 255, 0));      // Offroad sand

    QPainter visPainter(&trackVisualMap);
    QPainter maskPainter(&trackMaskMap);

    visPainter.setRenderHint(QPainter::Antialiasing, true);
    maskPainter.setRenderHint(QPainter::Antialiasing, true);

    // Choco track path with sharper S-curves
    QPainterPath path;
    path.moveTo(512, 200);
    path.cubicTo(880, 220, 820, 520, 680, 520);
    path.cubicTo(550, 520, 600, 820, 512, 820);
    path.cubicTo(200, 820, 300, 480, 200, 350);
    path.cubicTo(120, 220, 350, 200, 512, 200);

    // Darker Dirt Road
    QPen curbPen(QColor(139, 69, 19), 74);
    visPainter.setPen(curbPen);
    visPainter.drawPath(path);

    QPen roadPen(QColor(92, 51, 23), 60);
    visPainter.setPen(roadPen);
    visPainter.drawPath(path);

    QPen maskRoadPen(QColor(255, 255, 255), 60);
    maskPainter.setPen(maskRoadPen);
    maskPainter.drawPath(path);

    // Finish line
    visPainter.setPen(Qt::NoPen);
    visPainter.setBrush(QColor(255, 230, 180));
    visPainter.drawRect(508, 170, 8, 60);

    maskPainter.setPen(QPen(QColor(255, 255, 0), 8));
    maskPainter.drawLine(512, 170, 512, 230);

    visPainter.end();
    maskPainter.end();

    worldSprites.clear();
    worldSprites.push_back({ 570.0, 200.0, 0.0, SpriteType::QUESTION_BOX, true, 0.0 });
    worldSprites.push_back({ 680.0, 520.0, 0.0, SpriteType::QUESTION_BOX, true, 0.0 });
    worldSprites.push_back({ 512.0, 820.0, 0.0, SpriteType::BANANA_DROPPED, true, 0.0 });
    worldSprites.push_back({ 200.0, 350.0, 0.0, SpriteType::QUESTION_BOX, true, 0.0 });
    worldSprites.push_back({ 720.0, 380.0, 0.0, SpriteType::OIL_SLICK, true, 0.0 });
    worldSprites.push_back({ 320.0, 540.0, 0.0, SpriteType::OIL_SLICK, true, 0.0 });
    worldSprites.push_back({ 420.0, 200.0, 0.0, SpriteType::COIN, true, 0.0 });
}

void GameCanvas::generateBowserCastle()
{
    trackVisualMap.fill(qRgb(130, 20, 10)); // Molten Lava
    trackMaskMap.fill(qRgb(255, 0, 255));   // Magenta = Lava (Hazard)

    QPainter visPainter(&trackVisualMap);
    QPainter maskPainter(&trackMaskMap);

    visPainter.setRenderHint(QPainter::Antialiasing, true);
    maskPainter.setRenderHint(QPainter::Antialiasing, true);

    // Castle Stone Raceway with smoothly rounded corners
    QPolygon castlePoly;
    castlePoly << QPoint(250, 250)
               << QPoint(780, 250)
               << QPoint(780, 550)
               << QPoint(550, 550)
               << QPoint(550, 780)
               << QPoint(250, 780);

    // Wide Stone Curb / Shoulder (slows down safely, protects against lava)
    QPen stoneBorder(QColor(48, 50, 58), 96, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    visPainter.setPen(stoneBorder);
    visPainter.drawPolygon(castlePoly);

    // Mask for Curb: Green (Offroad - safe buffer zone)
    QPen maskCurb(QColor(0, 255, 0), 96, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    maskPainter.setPen(maskCurb);
    maskPainter.drawPolygon(castlePoly);

    // Wide Tarmac Stone Raceway (74px wide - plenty of room to overtake and avoid obstacles)
    QPen stoneRoad(QColor(76, 78, 88), 74, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    visPainter.setPen(stoneRoad);
    visPainter.drawPolygon(castlePoly);

    // Mask for Road: White (Full speed)
    QPen maskRoad(QColor(255, 255, 255), 74, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    maskPainter.setPen(maskRoad);
    maskPainter.drawPolygon(castlePoly);

    // Finish line across the wider raceway
    visPainter.setPen(Qt::NoPen);
    visPainter.setBrush(QColor(255, 255, 255));
    visPainter.drawRect(510, 210, 8, 80);

    maskPainter.setPen(QPen(QColor(255, 255, 0), 8));
    maskPainter.drawLine(512, 210, 512, 290);

    visPainter.end();
    maskPainter.end();

    worldSprites.clear();
    worldSprites.push_back({ 580.0, 250.0, 0.0, SpriteType::QUESTION_BOX, true, 0.0 });
    // Thwomp placed on the outer right lane (X=810), leaving the middle and inner lane wide open!
    worldSprites.push_back({ 810.0, 400.0, 0.0, SpriteType::THWOMP, true, 0.0 });
    worldSprites.push_back({ 650.0, 550.0, 0.0, SpriteType::QUESTION_BOX, true, 0.0 });
    worldSprites.push_back({ 400.0, 780.0, 0.0, SpriteType::QUESTION_BOX, true, 0.0 });
    // Green Pipe placed along the outer corner
    worldSprites.push_back({ 215.0, 600.0, 0.0, SpriteType::GREEN_PIPE, true, 0.0 });
    worldSprites.push_back({ 350.0, 250.0, 0.0, SpriteType::COIN, true, 0.0 });
}

// -------------------------------------------------------------
// GAME LOOP & PHYSICS
// -------------------------------------------------------------

void GameCanvas::updateGameLoop()
{
    double dt = frameTimer.restart() / 1000.0;
    if (dt > 0.05) dt = 0.05; // clamp delta time for stability

    updatePhysics(dt);
    updateItemSystem(dt);
    updateCelebration(dt);

    renderSky();
    renderGroundMode7();
    renderSprites();
    renderCockpit();
    renderItemHUD();
    renderInkOverlay();
    renderCelebration();

    emit statsUpdated(kartSpeed, currentLap, kartX, kartZ, kartAngle * (180.0 / M_PI), currentSurfaceName);

    this->update(); // Request Qt paintEvent
}

void GameCanvas::updatePhysics(double dt)
{
    // Handle race finish status - coast to a stop and lock controls
    if (isRaceFinished) {
        kartSpeed = std::max(0.0, kartSpeed - 55.0 * dt);
        kartX += kartSpeed * std::cos(kartAngle) * dt;
        kartZ += kartSpeed * std::sin(kartAngle) * dt;
        currentSurfaceName = "🏆 RACE FINISHED! (PRESS R TO RESTART)";
        return;
    }

    // Handle spinout status
    if (isSpinning) {
        kartAngle += 15.0 * dt;
        kartSpeed = std::max(0.0, kartSpeed - 140.0 * dt);
        kartX += kartSpeed * std::cos(kartAngle) * dt;
        kartZ += kartSpeed * std::sin(kartAngle) * dt;
        currentSurfaceName = "SPINNING OUT!";
        return;
    }

    // Sample terrain mask
    int sampleU = (static_cast<int>(kartX) % TRACK_MAP_SIZE + TRACK_MAP_SIZE) % TRACK_MAP_SIZE;
    int sampleV = (static_cast<int>(kartZ) % TRACK_MAP_SIZE + TRACK_MAP_SIZE) % TRACK_MAP_SIZE;
    QRgb maskColor = trackMaskMap.pixel(sampleU, sampleV);

    double currentMaxSpeed = maxSpeed;
    double currentDrag = friction;
    double currentAccel = accel;

    // Mushroom / Rocket speed boost override
    if (mushroomBoostTimer > 0.0) {
        currentMaxSpeed = maxSpeed * 1.55;
        currentAccel = accel * 2.2;
    }

    if (qRed(maskColor) == 255 && qGreen(maskColor) == 255 && qBlue(maskColor) == 255) {
        currentSurfaceName = (mushroomBoostTimer > 0.0) ? "Road (BOOST ACTIVE!)" : "Road (Tarmac)";
    } else if (qRed(maskColor) == 255 && qBlue(maskColor) == 255) {
        // Lava Hazard: bounce back onto track, spin briefly and lose speed without wiping your lap!
        currentSurfaceName = "LAVA RECOIL! (Sizzle)";
        kartSpeed = std::max(0.0, kartSpeed * 0.35);
        // Push kart back away from lava toward track
        kartX -= std::cos(kartAngle) * 8.0;
        kartZ -= std::sin(kartAngle) * 8.0;
        isSpinning = true;
        spinTimer = 0.45;
        flashIntensity = 0.5; // Warning flash
        return;
    } else if (qGreen(maskColor) == 255 && qRed(maskColor) == 0) {
        currentSurfaceName = (currentLevel == TrackLevel::BOWSER_CASTLE) ? "Stone Curb (Shoulder)" : "Off-road (Grass/Sand)";
        if (mushroomBoostTimer <= 0.0) {
            currentMaxSpeed *= (currentLevel == TrackLevel::BOWSER_CASTLE) ? 0.70 : 0.45;
            currentDrag *= 2.2;
        }
    } else if (qRed(maskColor) == 255 && qGreen(maskColor) == 255 && qBlue(maskColor) == 0) {
        currentSurfaceName = "Start / Finish Line";
    }

    // Controls
    bool forward = pressedKeys.count(Qt::Key_W) || pressedKeys.count(Qt::Key_Up);
    bool backward = pressedKeys.count(Qt::Key_S) || pressedKeys.count(Qt::Key_Down);
    bool left = pressedKeys.count(Qt::Key_A) || pressedKeys.count(Qt::Key_Left);
    bool right = pressedKeys.count(Qt::Key_D) || pressedKeys.count(Qt::Key_Right);

    // Throttle & Braking
    if (forward) {
        kartSpeed += currentAccel * dt;
        if (kartSpeed > currentMaxSpeed) kartSpeed = currentMaxSpeed;
    } else if (backward) {
        kartSpeed -= brakeDecel * dt;
        if (kartSpeed < -currentMaxSpeed * 0.3) kartSpeed = -currentMaxSpeed * 0.3;
    } else {
        // Rolling friction
        if (kartSpeed > 0) {
            kartSpeed -= currentDrag * dt;
            if (kartSpeed < 0) kartSpeed = 0;
        } else if (kartSpeed < 0) {
            kartSpeed += currentDrag * dt;
            if (kartSpeed > 0) kartSpeed = 0;
        }
    }

    // Steering - responsive at both high and low speeds (tight cornering)
    double steerDirection = 0.0;
    if (left) steerDirection -= 1.0;
    if (right) steerDirection += 1.0;

    double steerSpeedFactor = (std::abs(kartSpeed) > 1.0) ? std::clamp(std::abs(kartSpeed) / 50.0, 0.70, 1.30) : 0.0;
    kartAngle += steerDirection * 3.4 * steerSpeedFactor * dt;

    // Cockpit wheel animation lerp
    double targetWheelAngle = steerDirection * 0.35;
    steeringVisualAngle += (targetWheelAngle - steeringVisualAngle) * 10.0 * dt;

    // Keep angle in [0, 2*PI]
    while (kartAngle >= 2.0 * M_PI) kartAngle -= 2.0 * M_PI;
    while (kartAngle < 0.0) kartAngle += 2.0 * M_PI;

    double oldX = kartX;

    // Integrate position (heading angle: 0 points East along X, PI/2 points South along Z)
    kartX += kartSpeed * std::cos(kartAngle) * dt;
    kartZ += kartSpeed * std::sin(kartAngle) * dt;

    // Halfway checkpoint detection: opposite side of the track (Z > 650)
    if (kartZ > 650.0) {
        hasPassedHalfway = true;
    }

    // Finish line crossing detection: crossing X = 512.0 moving forward (East)
    double trackZMin = (currentLevel == TrackLevel::BOWSER_CASTLE) ? 180.0 : 140.0;
    double trackZMax = (currentLevel == TrackLevel::BOWSER_CASTLE) ? 320.0 : 260.0;

    bool crossingFinishLine = (oldX < 512.0 && kartX >= 512.0 && kartZ >= trackZMin && kartZ <= trackZMax);

    if (crossingFinishLine) {
        if (!firstStartCrossed) {
            firstStartCrossed = true;
            triggerCelebration("RACE START!", "GO GO GO!");
        } else if (hasPassedHalfway) {
            hasPassedHalfway = false;
            currentLap++;
            if (currentLap == 2) {
                triggerCelebration("★ LAP 2 ★", "SPEED UP!");
            } else if (currentLap == 3) {
                triggerCelebration("🔥 FINAL LAP! 🔥", "MAX SPEED!");
            } else if (currentLap > 3) {
                currentLap = 3;
                isRaceFinished = true;
                triggerCelebration("🏁 1ST PLACE - VICTORY! 🏁", "COURSE CLEAR! PRESS [R] TO REPLAY");
            }
        }
    }

    // Sprite collection & obstacle collision
    for (auto &sprite : worldSprites) {
        if (!sprite.active) continue;
        double dx = kartX - sprite.x;
        double dz = kartZ - sprite.z;
        double distSq = dx * dx + dz * dz;

        // Obstacles have a precise, smaller hitbox (11 units) so they are easy to bypass
        double hitRadius = (sprite.type == SpriteType::THWOMP || sprite.type == SpriteType::GREEN_PIPE) ? 11.0 : 18.0;

        if (distSq < hitRadius * hitRadius) {
            if (sprite.type == SpriteType::QUESTION_BOX) {
                sprite.active = false;
                sprite.respawnTimer = 7.0; // Respawns after 7 seconds
                if (!isRouletteActive && heldItem == PlayerItem::NONE) {
                    startItemRoulette();
                }
            } else if (sprite.type == SpriteType::COIN) {
                sprite.active = false;
                sprite.respawnTimer = 10.0;
                kartSpeed = std::min(maxSpeed, kartSpeed + 18.0); // Mini speed boost
            } else if (sprite.type == SpriteType::BANANA || sprite.type == SpriteType::BANANA_DROPPED || sprite.type == SpriteType::OIL_SLICK) {
                sprite.active = false;
                isSpinning = true;
                spinTimer = 1.0;
                kartSpeed *= 0.35;
            } else if (sprite.type == SpriteType::GREEN_PIPE || sprite.type == SpriteType::THWOMP) {
                // Solid obstacle collision bounce
                kartSpeed = -30.0;
                kartX -= std::cos(kartAngle) * 5.0;
                kartZ -= std::sin(kartAngle) * 5.0;
            } else if (sprite.type == SpriteType::MUSHROOM) {
                sprite.active = false;
                mushroomBoostTimer = 2.8;
                kartSpeed = maxSpeed * 1.4;
            }
        }
    }
}

// -------------------------------------------------------------
// ITEM ROULETTE & POWERUP SYSTEM
// -------------------------------------------------------------

void GameCanvas::updateItemSystem(double dt)
{
    globalTime += dt;

    // Item box respawn timers
    for (auto &sprite : worldSprites) {
        if (!sprite.active && sprite.respawnTimer > 0.0) {
            sprite.respawnTimer -= dt;
            if (sprite.respawnTimer <= 0.0) {
                sprite.active = true;
                sprite.respawnTimer = 0.0;
            }
        }
    }

    // Status effect countdowns
    if (mushroomBoostTimer > 0.0) {
        mushroomBoostTimer = std::max(0.0, mushroomBoostTimer - dt);
    }

    if (isSpinning) {
        spinTimer -= dt;
        if (spinTimer <= 0.0) {
            isSpinning = false;
        }
    }

    if (inkTimer > 0.0) {
        inkTimer = std::max(0.0, inkTimer - dt);
    }

    // Item Roulette Animation
    if (isRouletteActive) {
        rouletteTimer += dt;
        // Fast cycling animation through all items
        rouletteIndex = static_cast<int>(rouletteTimer * 14.0) % 5;

        if (rouletteTimer >= rouletteDuration) {
            isRouletteActive = false;
            // Land precisely on the truly randomized item
            heldItem = spriteToPlayerItem(rouletteItemAt(targetItemIndex));
        }
    }
}

void GameCanvas::startItemRoulette()
{
    if (isRouletteActive || heldItem != PlayerItem::NONE) return;
    isRouletteActive = true;
    rouletteTimer = 0.0;
    rouletteDuration = 1.8;
    // Pick uniformly at random from all 5 powerups:
    // 0: Mushroom, 1: Rocket, 2: Banana, 3: Green Shell, 4: Ink Blooper
    targetItemIndex = QRandomGenerator::global()->bounded(5);
    rouletteIndex = QRandomGenerator::global()->bounded(5);
    heldItem = PlayerItem::NONE;
}

void GameCanvas::useHeldItem()
{
    if (isRouletteActive || heldItem == PlayerItem::NONE) return;

    switch (heldItem) {
    case PlayerItem::MUSHROOM:
        mushroomBoostTimer = 3.0;
        kartSpeed = std::max(kartSpeed, maxSpeed * 1.4);
        break;

    case PlayerItem::ROCKET:
        mushroomBoostTimer = 4.5;
        kartSpeed = maxSpeed * 1.65;
        break;

    case PlayerItem::BANANA: {
        // Drop a banana peel behind the player's kart
        double dropDist = 24.0;
        double bx = kartX - std::cos(kartAngle) * dropDist;
        double bz = kartZ - std::sin(kartAngle) * dropDist;
        worldSprites.push_back({ bx, bz, 0.0, SpriteType::BANANA_DROPPED, true, 0.0 });
        break;
    }

    case PlayerItem::GREEN_SHELL: {
        // Fire green shell forward
        double fireDist = 28.0;
        double fx = kartX + std::cos(kartAngle) * fireDist;
        double fz = kartZ + std::sin(kartAngle) * fireDist;
        worldSprites.push_back({ fx, fz, 0.0, SpriteType::GREEN_SHELL, true, 0.0 });
        break;
    }

    case PlayerItem::INK_BLOOPER:
        inkTimer = 4.5;
        break;

    default:
        break;
    }

    heldItem = PlayerItem::NONE;
}

SpriteType GameCanvas::rouletteItemAt(int index) const
{
    switch ((index % 5 + 5) % 5) {
    case 0: return SpriteType::MUSHROOM;
    case 1: return SpriteType::ROCKET;
    case 2: return SpriteType::BANANA;
    case 3: return SpriteType::GREEN_SHELL;
    case 4: return SpriteType::INK_BLOOPER;
    default: return SpriteType::MUSHROOM;
    }
}

PlayerItem GameCanvas::spriteToPlayerItem(SpriteType type) const
{
    switch (type) {
    case SpriteType::MUSHROOM: return PlayerItem::MUSHROOM;
    case SpriteType::ROCKET: return PlayerItem::ROCKET;
    case SpriteType::BANANA: return PlayerItem::BANANA;
    case SpriteType::GREEN_SHELL: return PlayerItem::GREEN_SHELL;
    case SpriteType::INK_BLOOPER: return PlayerItem::INK_BLOOPER;
    default: return PlayerItem::NONE;
    }
}

// -------------------------------------------------------------
// RENDER PASSES
// -------------------------------------------------------------

void GameCanvas::renderSky()
{
    QRgb *pixels = reinterpret_cast<QRgb*>(screenBuffer.bits());

    QColor topSkyColor, bottomSkyColor;
    if (currentLevel == TrackLevel::DONUT_PLAINS) {
        topSkyColor = QColor(40, 110, 240);
        bottomSkyColor = QColor(160, 210, 255);
    } else if (currentLevel == TrackLevel::CHOCO_VALLEY) {
        topSkyColor = QColor(180, 70, 30);
        bottomSkyColor = QColor(240, 180, 110);
    } else {
        topSkyColor = QColor(20, 5, 20);
        bottomSkyColor = QColor(140, 30, 10);
    }

    for (int y = 0; y < HORIZON_Y; ++y) {
        double factor = static_cast<double>(y) / HORIZON_Y;
        int r = topSkyColor.red() + factor * (bottomSkyColor.red() - topSkyColor.red());
        int g = topSkyColor.green() + factor * (bottomSkyColor.green() - topSkyColor.green());
        int b = topSkyColor.blue() + factor * (bottomSkyColor.blue() - topSkyColor.blue());
        QRgb skyLineColor = qRgb(r, g, b);

        for (int x = 0; x < BUFFER_WIDTH; ++x) {
            pixels[y * BUFFER_WIDTH + x] = skyLineColor;
        }
    }

    // Parallax Mountain / Cloud Silhouette
    int horizonOffset = static_cast<int>((kartAngle / (2.0 * M_PI)) * BUFFER_WIDTH * 2.0);
    for (int x = 0; x < BUFFER_WIDTH; ++x) {
        int sampleX = (x + horizonOffset) % BUFFER_WIDTH;
        int mountainHeight = static_cast<int>(18.0 * std::sin(sampleX * 0.05) + 8.0 * std::sin(sampleX * 0.12));
        int peakY = HORIZON_Y - 15 - mountainHeight;
        peakY = std::clamp(peakY, 0, HORIZON_Y - 1);

        QRgb mtnColor = (currentLevel == TrackLevel::BOWSER_CASTLE)
                            ? qRgb(50, 15, 15)
                            : (currentLevel == TrackLevel::CHOCO_VALLEY ? qRgb(110, 55, 20) : qRgb(20, 80, 30));

        for (int y = peakY; y < HORIZON_Y; ++y) {
            pixels[y * BUFFER_WIDTH + x] = mtnColor;
        }
    }
}

void GameCanvas::renderGroundMode7()
{
    QRgb *pixels = reinterpret_cast<QRgb*>(screenBuffer.bits());
    const QRgb *trackPixels = reinterpret_cast<const QRgb*>(trackVisualMap.constBits());

    double cosA = std::cos(kartAngle);
    double sinA = std::sin(kartAngle);

    for (int y = HORIZON_Y; y < BUFFER_HEIGHT; ++y) {
        int dy = y - HORIZON_Y;
        if (dy == 0) dy = 1;

        // Similar triangles perspective: Distance to ground row
        double zGround = (cameraHeight * focalLength) / dy;
        double scaleGround = cameraHeight / dy;

        for (int x = 0; x < BUFFER_WIDTH; ++x) {
            int dx = x - (BUFFER_WIDTH / 2);
            double xGround = dx * scaleGround;

            // Rotate ground offset by kart angle & translate by kart position
            double u = kartX - xGround * sinA + zGround * cosA;
            double v = kartZ + xGround * cosA + zGround * sinA;

            int mapU = (static_cast<int>(u) % TRACK_MAP_SIZE + TRACK_MAP_SIZE) % TRACK_MAP_SIZE;
            int mapV = (static_cast<int>(v) % TRACK_MAP_SIZE + TRACK_MAP_SIZE) % TRACK_MAP_SIZE;

            pixels[y * BUFFER_WIDTH + x] = trackPixels[mapV * TRACK_MAP_SIZE + mapU];
        }
    }
}

void GameCanvas::renderSprites()
{
    struct ProjectedSprite {
        int screenX;
        int screenY;
        int drawW;
        int drawH;
        double depth;
        SpriteType type;
        bool isPickup;
        int groundScreenY;
    };

    std::vector<ProjectedSprite> renderList;
    renderList.reserve(worldSprites.size());

    double cosA = std::cos(kartAngle);
    double sinA = std::sin(kartAngle);
    const auto &atlas = SpriteAtlas::instance();

    for (const auto &sprite : worldSprites) {
        if (!sprite.active) continue;

        double dx = sprite.x - kartX;
        double dz = sprite.z - kartZ;

        // Camera space coordinates
        double camX = -dx * sinA + dz * cosA;
        double camZ = dx * cosA + dz * sinA;

        if (camZ <= cameraNearPlane || camZ > 450.0) continue;

        bool isPickup = atlas.isPickup(sprite.type);
        // Pickups bob up and down smoothly
        double bobOffset = isPickup ? (std::sin(globalTime * 5.0 + sprite.x * 0.1) * 2.2 + 2.8) : 0.0;
        double effectiveY = sprite.y + bobOffset;

        int sx = static_cast<int>((BUFFER_WIDTH / 2) + (focalLength * camX) / camZ);
        int sy = static_cast<int>(HORIZON_Y + (focalLength * (cameraHeight - effectiveY)) / camZ);
        int groundY = static_cast<int>(HORIZON_Y + (focalLength * cameraHeight) / camZ);

        double scale = focalLength / camZ;
        double baseSize = (sprite.type == SpriteType::THWOMP) ? 15.0 : 20.0;
        int drawW = static_cast<int>(baseSize * scale);
        int drawH = static_cast<int>(baseSize * scale);

        if (drawW <= 1 || drawH <= 1) continue;

        renderList.push_back({ sx, sy, drawW, drawH, camZ, sprite.type, isPickup, groundY });
    }

    // Sort farthest to nearest (Painter's Algorithm)
    std::sort(renderList.begin(), renderList.end(), [](const ProjectedSprite &a, const ProjectedSprite &b) {
        return a.depth > b.depth;
    });

    QRgb *pixels = reinterpret_cast<QRgb*>(screenBuffer.bits());

    for (const auto &item : renderList) {
        // Draw soft ground shadow beneath floating pickups
        if (item.isPickup && item.groundScreenY >= HORIZON_Y && item.groundScreenY < BUFFER_HEIGHT) {
            int shadowW = item.drawW * 3 / 4;
            int shadowH = std::max(2, item.drawW / 4);
            int shStartX = item.screenX - shadowW / 2;
            int shStartY = item.groundScreenY - shadowH / 2;

            for (int sy = 0; sy < shadowH; ++sy) {
                int py = shStartY + sy;
                if (py < HORIZON_Y || py >= BUFFER_HEIGHT) continue;
                for (int sx = 0; sx < shadowW; ++sx) {
                    int px = shStartX + sx;
                    if (px < 0 || px >= BUFFER_WIDTH) continue;

                    double nx = (sx - shadowW / 2.0) / (shadowW / 2.0);
                    double ny = (sy - shadowH / 2.0) / (shadowH / 2.0);
                    if (nx * nx + ny * ny <= 1.0) {
                        QRgb bg = pixels[py * BUFFER_WIDTH + px];
                        int r = (qRed(bg) * 55) / 100;
                        int g = (qGreen(bg) * 55) / 100;
                        int b = (qBlue(bg) * 55) / 100;
                        pixels[py * BUFFER_WIDTH + px] = qRgb(r, g, b);
                    }
                }
            }
        }

        // Draw textured billboard
        const QImage &texture = atlas.getSprite(item.type);
        drawTexturedBillboard(item.screenX, item.screenY, item.drawW, item.drawH, texture);
    }
}

void GameCanvas::drawTexturedBillboard(int screenX, int screenY, int drawW, int drawH, const QImage &texture)
{
    if (texture.isNull() || drawW <= 0 || drawH <= 0) return;

    QRgb *pixels = reinterpret_cast<QRgb*>(screenBuffer.bits());
    const QRgb *texPixels = reinterpret_cast<const QRgb*>(texture.constBits());
    int texW = texture.width();
    int texH = texture.height();

    int startX = screenX - drawW / 2;
    int startY = screenY - drawH;

    int clipStartX = std::max(0, startX);
    int clipEndX = std::min(BUFFER_WIDTH, startX + drawW);
    int clipStartY = std::max(0, startY);
    int clipEndY = std::min(BUFFER_HEIGHT, startY + drawH);

    for (int y = clipStartY; y < clipEndY; ++y) {
        int ty = ((y - startY) * texH) / drawH;
        if (ty < 0) ty = 0;
        if (ty >= texH) ty = texH - 1;
        const QRgb *texRow = texPixels + ty * texW;
        QRgb *bufRow = pixels + y * BUFFER_WIDTH;

        for (int x = clipStartX; x < clipEndX; ++x) {
            int tx = ((x - startX) * texW) / drawW;
            if (tx < 0) tx = 0;
            if (tx >= texW) tx = texW - 1;

            QRgb color = texRow[tx];
            int alpha = qAlpha(color);
            if (alpha < 32) continue; // transparent pixel

            if (alpha >= 250) {
                bufRow[x] = color;
            } else {
                // Alpha blend
                QRgb dst = bufRow[x];
                int invA = 255 - alpha;
                int r = (qRed(color) * alpha + qRed(dst) * invA) / 255;
                int g = (qGreen(color) * alpha + qGreen(dst) * invA) / 255;
                int b = (qBlue(color) * alpha + qBlue(dst) * invA) / 255;
                bufRow[x] = qRgb(r, g, b);
            }
        }
    }
}

void GameCanvas::renderCockpit()
{
    QRgb *pixels = reinterpret_cast<QRgb*>(screenBuffer.bits());

    // Boost flame / sparks effect when boost is active
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

    QRgb hoodColor = (mushroomBoostTimer > 0.0) ? qRgb(240, 70, 20) : qRgb(180, 20, 20); // Golden fiery red during boost

    for (int y = hoodTop; y < BUFFER_HEIGHT; ++y) {
        int halfWidth = static_cast<int>(30.0 + (y - hoodTop) * 2.2);
        for (int x = hoodCenter - halfWidth; x < hoodCenter + halfWidth; ++x) {
            if (x >= 0 && x < BUFFER_WIDTH) {
                pixels[y * BUFFER_WIDTH + x] = hoodColor;
            }
        }
    }

    // Steering wheel (Simple rotated pixel ring)
    int wheelY = BUFFER_HEIGHT - 22;
    int wheelX = BUFFER_WIDTH / 2 + static_cast<int>(steeringVisualAngle * 30.0);
    int wheelR = 14;

    for (int y = wheelY - wheelR; y <= wheelY + wheelR; ++y) {
        if (y < 0 || y >= BUFFER_HEIGHT) continue;
        for (int x = wheelX - wheelR; x <= wheelX + wheelR; ++x) {
            if (x < 0 || x >= BUFFER_WIDTH) continue;
            int dist2 = (x - wheelX) * (x - wheelX) + (y - wheelY) * (y - wheelY);
            if (dist2 >= (wheelR - 3) * (wheelR - 3) && dist2 <= wheelR * wheelR) {
                pixels[y * BUFFER_WIDTH + x] = qRgb(240, 240, 240); // White Steering Grip
            }
        }
    }
}

void GameCanvas::renderItemHUD()
{
    QPainter painter(&screenBuffer);
    painter.setRenderHint(QPainter::Antialiasing, false);

    int boxSize = 42;
    int boxX = BUFFER_WIDTH / 2 - boxSize / 2;
    int boxY = 8;
    QRect hudRect(boxX, boxY, boxSize, boxSize);

    // Box Frame
    QColor borderColor = QColor(100, 105, 120);
    QColor bgColor = QColor(20, 22, 32, 210);

    if (isRouletteActive) {
        // Flashing arcade border while rolling
        int flashVal = static_cast<int>(globalTime * 14.0) % 3;
        if (flashVal == 0) borderColor = QColor(255, 215, 0);
        else if (flashVal == 1) borderColor = QColor(0, 235, 255);
        else borderColor = QColor(255, 60, 60);
        bgColor = QColor(35, 30, 50, 230);
    } else if (heldItem != PlayerItem::NONE) {
        // Bright golden border for held item
        borderColor = QColor(255, 215, 0);
        bgColor = QColor(25, 28, 45, 235);
    }

    // Background and border
    painter.fillRect(hudRect, bgColor);
    painter.setPen(QPen(borderColor, 2));
    painter.drawRect(hudRect);

    // Inner highlight border
    painter.setPen(QPen(QColor(borderColor.red(), borderColor.green(), borderColor.blue(), 100), 1));
    painter.drawRect(hudRect.adjusted(2, 2, -2, -2));

    const auto &atlas = SpriteAtlas::instance();

    if (isRouletteActive) {
        SpriteType currentType = rouletteItemAt(rouletteIndex);
        const QImage &img = atlas.getSprite(currentType);
        painter.drawImage(QRect(boxX + 5, boxY + 5, 32, 32), img);

        // Subtitle
        QFont hintFont("Arial", 6, QFont::Bold);
        painter.setFont(hintFont);
        painter.setPen(QColor(255, 220, 0));
        painter.drawText(QRect(boxX - 20, boxY + boxSize + 1, boxSize + 40, 12), Qt::AlignCenter, "ROLLING...");
    } else if (heldItem != PlayerItem::NONE) {
        SpriteType displayType = SpriteType::QUESTION_BOX;
        switch (heldItem) {
        case PlayerItem::MUSHROOM: displayType = SpriteType::MUSHROOM; break;
        case PlayerItem::ROCKET: displayType = SpriteType::ROCKET; break;
        case PlayerItem::BANANA: displayType = SpriteType::BANANA; break;
        case PlayerItem::GREEN_SHELL: displayType = SpriteType::GREEN_SHELL; break;
        case PlayerItem::INK_BLOOPER: displayType = SpriteType::INK_BLOOPER; break;
        default: break;
        }

        const QImage &img = atlas.getSprite(displayType);
        painter.drawImage(QRect(boxX + 5, boxY + 5, 32, 32), img);

        // Pulsing "[SPACE]" hint
        QFont hintFont("Arial", 6, QFont::Bold);
        painter.setFont(hintFont);
        painter.setPen(QColor(255, 255, 255));
        painter.drawText(QRect(boxX - 30, boxY + boxSize + 1, boxSize + 60, 12), Qt::AlignCenter, "[SPACE] USE");
    } else {
        // Empty slot question mark silhouette
        QFont emptyFont("Arial", 14, QFont::Bold);
        painter.setFont(emptyFont);
        painter.setPen(QColor(70, 75, 90));
        painter.drawText(hudRect, Qt::AlignCenter, "?");

        QFont hintFont("Arial", 5, QFont::Normal);
        painter.setFont(hintFont);
        painter.setPen(QColor(130, 135, 150));
        painter.drawText(QRect(boxX - 20, boxY + boxSize + 1, boxSize + 40, 10), Qt::AlignCenter, "ITEM");
    }
}

void GameCanvas::renderInkOverlay()
{
    if (inkTimer <= 0.0) return;

    QPainter painter(&screenBuffer);
    painter.setRenderHint(QPainter::Antialiasing, true);

    int alpha = std::min(235, static_cast<int>((inkTimer / 4.5) * 230.0));
    QColor inkColor(15, 12, 28, alpha);
    painter.setBrush(inkColor);
    painter.setPen(Qt::NoPen);

    // Blooper ink splatters across windshield
    painter.drawEllipse(QPoint(110, 80), 38, 35);
    painter.drawEllipse(QPoint(125, 100), 20, 24);
    painter.drawEllipse(QPoint(85, 70), 16, 18);

    painter.drawEllipse(QPoint(290, 110), 45, 40);
    painter.drawEllipse(QPoint(265, 130), 22, 26);
    painter.drawEllipse(QPoint(315, 90), 25, 20);

    painter.drawEllipse(QPoint(190, 140), 32, 30);
    painter.drawEllipse(QPoint(205, 160), 15, 22);

    painter.drawEllipse(QPoint(60, 160), 28, 26);
    painter.drawEllipse(QPoint(340, 60), 24, 22);
}

void GameCanvas::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    // Nearest-neighbor scaling to maintain authentic pixel graphics
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter.drawImage(rect(), screenBuffer);
}

// -------------------------------------------------------------
// CELEBRATION & FINISH ANIMATION
// -------------------------------------------------------------

void GameCanvas::triggerCelebration(const QString &title, const QString &subtitle)
{
    isCelebrationActive = true;
    celebrationTimer = 0.0;
    celebrationTitle = title;
    celebrationSubtitle = subtitle;
    flashIntensity = 0.85;

    // Generate 120 confetti particles bursting from both bottom corners
    confetti.clear();
    const QColor colors[] = {
        QColor(255, 50, 50),   // Red
        QColor(255, 215, 0),   // Gold
        QColor(0, 230, 255),   // Cyan
        QColor(60, 255, 100),  // Green
        QColor(255, 105, 180), // Pink
        QColor(255, 255, 255), // Pure White
        QColor(180, 80, 255),  // Purple
        QColor(255, 140, 0)    // Orange
    };
    int numColors = sizeof(colors) / sizeof(colors[0]);

    for (int i = 0; i < 120; ++i) {
        ConfettiParticle p;
        bool leftSide = (i % 2 == 0);
        p.x = leftSide ? (rand() % 40) : (BUFFER_WIDTH - (rand() % 40));
        p.y = BUFFER_HEIGHT - 20 - (rand() % 50);

        double launchAngle = leftSide
            ? (-(35.0 + (rand() % 45)) * M_PI / 180.0)
            : (-(180.0 - (35.0 + (rand() % 45))) * M_PI / 180.0);

        double speed = 180.0 + (rand() % 160);
        p.vx = std::cos(launchAngle) * speed;
        p.vy = std::sin(launchAngle) * speed;
        p.w = 4.0 + (rand() % 4);
        p.h = 2.5 + (rand() % 4);
        p.angle = (rand() % 360) * M_PI / 180.0;
        p.vRot = ((rand() % 240) - 120) * M_PI / 180.0;
        p.color = colors[rand() % numColors];
        confetti.push_back(p);
    }
}

void GameCanvas::updateCelebration(double dt)
{
    if (!isCelebrationActive) return;

    celebrationTimer += dt;
    flashIntensity = std::max(0.0, flashIntensity - dt * 2.8);

    // Gravity and physics on confetti
    for (auto &p : confetti) {
        p.vy += 160.0 * dt; // gravity
        p.vx *= 0.985;      // air resistance
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.angle += p.vRot * dt;
    }

    if (isRaceFinished) {
        // Keep victory banner active and looping when game is won
        if (celebrationTimer >= celebrationDuration - 0.4) {
            celebrationTimer = 0.5;
        }
    } else {
        if (celebrationTimer >= celebrationDuration) {
            isCelebrationActive = false;
            confetti.clear();
        }
    }
}

void GameCanvas::renderCelebration()
{
    if (!isCelebrationActive) return;

    // 1. Screen Golden Flash
    if (flashIntensity > 0.02) {
        QRgb *pixels = reinterpret_cast<QRgb*>(screenBuffer.bits());
        int boost = static_cast<int>(flashIntensity * 130);
        for (int i = 0; i < BUFFER_WIDTH * BUFFER_HEIGHT; ++i) {
            int r = qMin(255, qRed(pixels[i]) + boost);
            int g = qMin(255, qGreen(pixels[i]) + boost);
            int b = qMin(255, qBlue(pixels[i]) + (boost / 3));
            pixels[i] = qRgb(r, g, b);
        }
    }

    QPainter painter(&screenBuffer);
    painter.setRenderHint(QPainter::Antialiasing, false);

    // 2. Render Confetti Particles
    for (const auto &p : confetti) {
        if (p.x < -10 || p.x > BUFFER_WIDTH + 10 || p.y > BUFFER_HEIGHT + 10) continue;
        painter.save();
        painter.translate(p.x, p.y);
        painter.rotate(p.angle * 180.0 / M_PI);
        painter.setPen(QPen(QColor(15, 15, 15), 1));
        painter.setBrush(p.color);
        painter.drawRect(QRectF(-p.w / 2.0, -p.h / 2.0, p.w, p.h));
        painter.restore();
    }

    // 3. Top Checkered Flag Ribbon
    int squareSize = 7;
    int scrollOffset = static_cast<int>(celebrationTimer * 30.0) % (squareSize * 2);

    for (int x = -squareSize * 2; x < BUFFER_WIDTH + squareSize * 2; x += squareSize) {
        for (int row = 0; row < 2; ++row) {
            int drawX = x + scrollOffset;
            int drawY = 8 + row * squareSize;
            bool isWhite = ((x / squareSize + row) % 2 == 0);
            painter.fillRect(drawX, drawY, squareSize, squareSize, isWhite ? QColor(250, 250, 255) : QColor(20, 20, 25));
        }
    }
    // Gold borders for ribbon
    painter.setPen(QColor(255, 215, 0));
    painter.drawLine(0, 7, BUFFER_WIDTH, 7);
    painter.drawLine(0, 8 + 2 * squareSize, BUFFER_WIDTH, 8 + 2 * squareSize);

    // 4. Retro Center Banner with Pop-in Elastic Scaling
    double scale = 1.0;
    if (celebrationTimer < 0.25) {
        scale = celebrationTimer / 0.25; // Elastic zoom in
    } else {
        scale = 1.0 + 0.04 * std::sin((celebrationTimer - 0.25) * 8.0); // Heartbeat pulse
    }

    painter.save();
    painter.translate(BUFFER_WIDTH / 2, 85);
    painter.scale(scale, scale);

    int bannerW = 280;
    int bannerH = 68;
    QRect bannerRect(-bannerW / 2, -bannerH / 2, bannerW, bannerH);

    // Banner background with thick arcade border
    painter.setPen(QPen(QColor(255, 215, 0), 2)); // Gold outer border
    painter.setBrush(QColor(15, 18, 30, 230));   // Dark translucent arcade blue
    painter.drawRoundedRect(bannerRect, 6, 6);

    // Inner orange highlight border
    painter.setPen(QPen(QColor(255, 120, 0), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(bannerRect.adjusted(3, 3, -3, -3), 4, 4);

    // Title Text (with 3D drop shadow)
    QFont titleFont("Courier New", 14, QFont::Black);
    titleFont.setBold(true);
    painter.setFont(titleFont);

    // Shadow
    painter.setPen(QColor(10, 10, 15));
    painter.drawText(QRect(-bannerW / 2 + 2, -bannerH / 2 + 6, bannerW, 32), Qt::AlignCenter, celebrationTitle);

    // Foreground Title
    QColor titleColor = (celebrationTitle.contains("FINAL") || celebrationTitle.contains("START"))
        ? QColor(255, 80, 80)
        : QColor(255, 220, 0);
    painter.setPen(titleColor);
    painter.drawText(QRect(-bannerW / 2, -bannerH / 2 + 4, bannerW, 32), Qt::AlignCenter, celebrationTitle);

    // Subtitle Text
    QFont subFont("Arial", 9, QFont::Bold);
    painter.setFont(subFont);
    painter.setPen(QColor(10, 10, 15));
    painter.drawText(QRect(-bannerW / 2 + 1, -bannerH / 2 + 37, bannerW, 24), Qt::AlignCenter, celebrationSubtitle);

    painter.setPen(QColor(0, 240, 255)); // Bright Neon Cyan
    painter.drawText(QRect(-bannerW / 2, -bannerH / 2 + 36, bannerW, 24), Qt::AlignCenter, celebrationSubtitle);

    painter.restore();
}
