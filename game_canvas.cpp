#include "game_canvas.h"
#include <QPainter>
#include <QPainterPath>
#include <QKeyEvent>
#include <algorithm>
#include <cmath>

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
    kartX = 512.0;
    kartZ = 200.0;
    kartAngle = M_PI / 2.0; // Facing east along starting straight
    kartSpeed = 0.0;
    steeringVisualAngle = 0.0;
    currentLap = 1;
    nextCheckpoint = 1;
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
    worldSprites.push_back({ 500.0, 250.0, 0.0, SpriteType::QUESTION_BOX, true });
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
    renderSky();
    renderGroundMode7();
    renderSprites();
    renderCockpit();

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

    // Integrate position (heading angle: 0 points East along X, PI/2 points South along Z)
    kartX += kartSpeed * std::cos(kartAngle) * dt;
    kartZ += kartSpeed * std::sin(kartAngle) * dt;

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
            double u = kartX + xGround * sinA + zGround * cosA;
            double v = kartZ - xGround * cosA + zGround * sinA;

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
