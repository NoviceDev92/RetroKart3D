#include "game_canvas.h"
#include <QPainter>
#include <QPainterPath>
#include <QKeyEvent>
#include <QFont>
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

    // Reset celebration state
    isCelebrationActive = false;
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
    if (event->key() == Qt::Key_R) {
        resetKart();
    } else if (event->key() == Qt::Key_F) {
        triggerCelebration("★ FINISH! ★", "COURSE CLEAR!");
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
// TRACK GENERATION (PRODURAL TEXTURES & MASKS)
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
    worldSprites.push_back({ 560.0, 200.0, 0.0, SpriteType::QUESTION_BOX, true });
    worldSprites.push_back({ 620.0, 200.0, 0.0, SpriteType::QUESTION_BOX, true });
    worldSprites.push_back({ 850.0, 500.0, 0.0, SpriteType::GREEN_PIPE, true });
    worldSprites.push_back({ 512.0, 800.0, 0.0, SpriteType::QUESTION_BOX, true });
    worldSprites.push_back({ 180.0, 500.0, 0.0, SpriteType::GREEN_PIPE, true });
    worldSprites.push_back({ 400.0, 200.0, 0.0, SpriteType::COIN, true });
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
    worldSprites.push_back({ 570.0, 200.0, 0.0, SpriteType::QUESTION_BOX, true });
    worldSprites.push_back({ 680.0, 520.0, 0.0, SpriteType::QUESTION_BOX, true });
    worldSprites.push_back({ 512.0, 820.0, 0.0, SpriteType::BANANA, true });
    worldSprites.push_back({ 200.0, 350.0, 0.0, SpriteType::QUESTION_BOX, true });
}

void GameCanvas::generateBowserCastle()
{
    trackVisualMap.fill(qRgb(130, 20, 10)); // Molten Lava
    trackMaskMap.fill(qRgb(255, 0, 255));   // Magenta = Lava (Hazard)

    QPainter visPainter(&trackVisualMap);
    QPainter maskPainter(&trackMaskMap);

    // Castle Stone Raceway with 90 degree turns
    QPolygon castlePoly;
    castlePoly << QPoint(250, 250)
               << QPoint(780, 250)
               << QPoint(780, 550)
               << QPoint(550, 550)
               << QPoint(550, 780)
               << QPoint(250, 780);

    QPen stoneBorder(QColor(40, 40, 45), 52, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
    visPainter.setPen(stoneBorder);
    visPainter.drawPolygon(castlePoly);

    QPen stoneRoad(QColor(70, 72, 80), 40, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
    visPainter.setPen(stoneRoad);
    visPainter.drawPolygon(castlePoly);

    QPen maskRoad(QColor(255, 255, 255), 40, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
    maskPainter.setPen(maskRoad);
    maskPainter.drawPolygon(castlePoly);

    // Finish line
    visPainter.setPen(Qt::NoPen);
    visPainter.setBrush(QColor(255, 255, 255));
    visPainter.drawRect(510, 230, 8, 40);

    maskPainter.setPen(QPen(QColor(255, 255, 0), 8));
    maskPainter.drawLine(512, 230, 512, 270);

    visPainter.end();
    maskPainter.end();

    worldSprites.clear();
    worldSprites.push_back({ 580.0, 250.0, 0.0, SpriteType::QUESTION_BOX, true });
    worldSprites.push_back({ 780.0, 400.0, 0.0, SpriteType::GREEN_PIPE, true });
    worldSprites.push_back({ 650.0, 550.0, 0.0, SpriteType::QUESTION_BOX, true });
    worldSprites.push_back({ 400.0, 780.0, 0.0, SpriteType::QUESTION_BOX, true });
}

// -------------------------------------------------------------
// GAME LOOP & PHYSICS
// -------------------------------------------------------------

void GameCanvas::updateGameLoop()
{
    double dt = frameTimer.restart() / 1000.0;
    if (dt > 0.05) dt = 0.05; // clamp delta time for stability

    updatePhysics(dt);
    updateCelebration(dt);

    renderSky();
    renderGroundMode7();
    renderSprites();
    renderCockpit();
    renderCelebration();

    emit statsUpdated(kartSpeed, currentLap, kartX, kartZ, kartAngle * (180.0 / M_PI), currentSurfaceName);

    this->update(); // Request Qt paintEvent
}

void GameCanvas::updatePhysics(double dt)
{
    // Sample terrain mask
    int sampleU = (static_cast<int>(kartX) % TRACK_MAP_SIZE + TRACK_MAP_SIZE) % TRACK_MAP_SIZE;
    int sampleV = (static_cast<int>(kartZ) % TRACK_MAP_SIZE + TRACK_MAP_SIZE) % TRACK_MAP_SIZE;
    QRgb maskColor = trackMaskMap.pixel(sampleU, sampleV);

    double currentMaxSpeed = maxSpeed;
    double currentDrag = friction;

    if (qRed(maskColor) == 255 && qGreen(maskColor) == 255 && qBlue(maskColor) == 255) {
        currentSurfaceName = "Road (Tarmac)";
    } else if (qRed(maskColor) == 255 && qBlue(maskColor) == 255) {
        // Lava!
        currentSurfaceName = "LAVA! (Resetting)";
        resetKart();
        return;
    } else if (qGreen(maskColor) == 255 && qRed(maskColor) == 0) {
        currentSurfaceName = "Off-road (Grass/Sand)";
        currentMaxSpeed *= 0.45;
        currentDrag *= 3.0;
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
        kartSpeed += accel * dt;
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

    // Steering
    double steerDirection = 0.0;
    if (left) steerDirection -= 1.0;
    if (right) steerDirection += 1.0;

    double steerSpeedFactor = std::abs(kartSpeed) / maxSpeed;
    kartAngle += steerDirection * turnRate * steerSpeedFactor * dt;

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
    double trackZMin = (currentLevel == TrackLevel::BOWSER_CASTLE) ? 190.0 : 140.0;
    double trackZMax = (currentLevel == TrackLevel::BOWSER_CASTLE) ? 310.0 : 260.0;

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
                triggerCelebration("🏁 VICTORY! 🏁", "COURSE CLEAR!");
                currentLap = 3;
            }
        }
    }

    // Sprite collection collision
    for (auto &sprite : worldSprites) {
        if (!sprite.active) continue;
        double dx = kartX - sprite.x;
        double dz = kartZ - sprite.z;
        if (dx * dx + dz * dz < 18.0 * 18.0) {
            sprite.active = false; // "Collected"
        }
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
    // Transform sprites to camera space and sort them (Painter's Algorithm)
    struct ProjectedSprite {
        int screenX;
        int screenY;
        int drawW;
        int drawH;
        double depth;
        SpriteType type;
    };

    std::vector<ProjectedSprite> renderList;

    double cosA = std::cos(kartAngle);
    double sinA = std::sin(kartAngle);

    for (const auto &sprite : worldSprites) {
        if (!sprite.active) continue;

        double dx = sprite.x - kartX;
        double dz = sprite.z - kartZ;

        // Rotate into camera coordinates
        // Forward vector in world is (cosA, sinA), right vector is (-sinA, cosA)
        double camX = -dx * sinA + dz * cosA;
        double camZ = dx * cosA + dz * sinA;

        // Near-plane clipping
        if (camZ <= cameraNearPlane) continue;

        int sx = static_cast<int>((BUFFER_WIDTH / 2) + (focalLength * camX) / camZ);
        int sy = static_cast<int>(HORIZON_Y + (focalLength * (cameraHeight - sprite.y)) / camZ);

        double scale = focalLength / camZ;
        int drawW = static_cast<int>(16.0 * scale);
        int drawH = static_cast<int>(16.0 * scale);

        if (drawW <= 1 || drawH <= 1) continue;

        renderList.push_back({ sx, sy, drawW, drawH, camZ, sprite.type });
    }

    // Sort farthest to nearest
    std::sort(renderList.begin(), renderList.end(), [](const ProjectedSprite &a, const ProjectedSprite &b) {
        return a.depth > b.depth;
    });

    for (const auto &item : renderList) {
        drawSpritePixelArt(item.screenX, item.screenY, item.drawW, item.drawH, item.type);
    }
}

void GameCanvas::drawSpritePixelArt(int screenX, int screenY, int drawW, int drawH, SpriteType type)
{
    QRgb *pixels = reinterpret_cast<QRgb*>(screenBuffer.bits());

    int startX = screenX - drawW / 2;
    int startY = screenY - drawH;

    QRgb baseColor = qRgb(255, 255, 255);
    switch (type) {
    case SpriteType::QUESTION_BOX: baseColor = qRgb(255, 215, 0); break; // Gold
    case SpriteType::GREEN_PIPE:    baseColor = qRgb(40, 180, 40);  break; // Green
    case SpriteType::COIN:          baseColor = qRgb(255, 235, 100);break; // Bright Yellow
    case SpriteType::BANANA:        baseColor = qRgb(230, 210, 20); break; // Banana yellow
    }

    for (int py = 0; py < drawH; ++py) {
        int targetY = startY + py;
        if (targetY < 0 || targetY >= BUFFER_HEIGHT) continue;

        for (int px = 0; px < drawW; ++px) {
            int targetX = startX + px;
            if (targetX < 0 || targetX >= BUFFER_WIDTH) continue;

            // Border outline
            if (px == 0 || px == drawW - 1 || py == 0 || py == drawH - 1) {
                pixels[targetY * BUFFER_WIDTH + targetX] = qRgb(20, 20, 20);
            } else {
                pixels[targetY * BUFFER_WIDTH + targetX] = baseColor;
            }
        }
    }
}

void GameCanvas::renderCockpit()
{
    QRgb *pixels = reinterpret_cast<QRgb*>(screenBuffer.bits());

    // First-person kart hood at bottom center
    int hoodCenter = BUFFER_WIDTH / 2;
    int hoodTop = BUFFER_HEIGHT - 35;

    for (int y = hoodTop; y < BUFFER_HEIGHT; ++y) {
        int halfWidth = static_cast<int>(30.0 + (y - hoodTop) * 2.2);
        for (int x = hoodCenter - halfWidth; x < hoodCenter + halfWidth; ++x) {
            if (x >= 0 && x < BUFFER_WIDTH) {
                pixels[y * BUFFER_WIDTH + x] = qRgb(180, 20, 20); // Classic Red Kart
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

    if (celebrationTimer >= celebrationDuration) {
        isCelebrationActive = false;
        confetti.clear();
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
