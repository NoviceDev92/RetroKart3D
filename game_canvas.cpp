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

    initCharacters();
    initTrackWaypoints();
    initTrackMaps();
    generateDonutPlains();
    currentState = GameState::TITLE_MENU;

    connect(&gameTimer, &QTimer::timeout, this, &GameCanvas::updateGameLoop);
    gameTimer.start(16); // ~60 FPS
    frameTimer.start();
}

void GameCanvas::initCharacters()
{
    characters = {
        // ID 0: Mario — Balanced
        {
            0, "MARIO", "BALANCED",
            "All-around master with balanced top speed, responsive grip, and reliable acceleration.",
            QColor(220, 20, 20),    // Crimson Red
            QColor(245, 245, 245),  // Crisp White
            QColor(240, 240, 255),  // Fire Mario White
            160.0,                  // maxSpeed (km/h)
            110.0,                  // accel (m/s^2)
            2.80,                   // turnRate (rad/s)
            1.00,                   // driftFactor
            1.00,                   // weight
            3, 3, 3, 3              // Pips: 3 SPD, 3 ACC, 3 HND, 3 WGT
        },
        // ID 1: Bowser — Heavyweight
        {
            1, "BOWSER", "HEAVYWEIGHT",
            "Tremendous 175 km/h top speed and heavy collision weight; slower initial throttle.",
            QColor(35, 115, 45),    // Dark Green
            QColor(240, 140, 20),   // Fiery Orange
            QColor(60, 60, 65),     // Dry Bowser Charcoal
            175.0,                  // maxSpeed (km/h)
            85.0,                   // accel (m/s^2)
            2.20,                   // turnRate (rad/s)
            0.85,                   // driftFactor
            1.60,                   // weight
            5, 1, 1, 5              // Pips: 5 SPD, 1 ACC, 1 HND, 5 WGT
        },
        // ID 2: Peach — Agility
        {
            2, "PEACH", "AGILITY",
            "Extreme 3.4 rad/s cornering agility and high drift control for mastering hairpin bends.",
            QColor(245, 130, 175),  // Princess Pink
            QColor(255, 215, 0),    // Royal Gold
            QColor(255, 180, 50),   // Daisy Orange
            152.0,                  // maxSpeed (km/h)
            125.0,                  // accel (m/s^2)
            3.40,                   // turnRate (rad/s)
            1.25,                   // driftFactor
            0.75,                   // weight
            2, 4, 5, 2              // Pips: 2 SPD, 4 ACC, 5 HND, 2 WGT
        },
        // ID 3: Toad — Acceleration
        {
            3, "TOAD", "ACCELERATION",
            "Explosive 140 m/s^2 acceleration and lightning-fast hazard recovery off the line.",
            QColor(25, 95, 220),    // Royal Blue
            QColor(245, 245, 245),  // Mushroom White
            QColor(245, 210, 30),   // Yellow Toad
            148.0,                  // maxSpeed (km/h)
            140.0,                  // accel (m/s^2)
            3.10,                   // turnRate (rad/s)
            1.15,                   // driftFactor
            0.65,                   // weight
            2, 5, 4, 1              // Pips: 2 SPD, 5 ACC, 4 HND, 1 WGT
        }
    };
}

const CharacterDef& GameCanvas::getSelectedCharacterP1() const
{
    static CharacterDef fallback;
    if (characters.empty()) return fallback;
    int idx = (selectedCharP1 % static_cast<int>(characters.size()) + characters.size()) % characters.size();
    return characters[idx];
}

const CharacterDef& GameCanvas::getSelectedCharacterP2() const
{
    static CharacterDef fallback;
    if (characters.empty()) return fallback;
    int idx = (selectedCharP2 % static_cast<int>(characters.size()) + characters.size()) % characters.size();
    return characters[idx];
}

void GameCanvas::setGameState(GameState state)
{
    currentState = state;
    pressedKeys.clear();
    update();
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
    currentTrackWaypoints = trackWaypoints[static_cast<int>(currentLevel)];
    if (currentState == GameState::RACING || currentState == GameState::RACE_FINISHED) {
        startRace();
    }
    update();
}

void GameCanvas::startRace()
{
    if (characters.empty()) {
        initCharacters();
    }

    pressedKeys.clear();

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

    const CharacterDef &p1 = getSelectedCharacterP1();
    this->maxSpeed = p1.maxSpeed;
    this->accel = p1.accel;
    this->brakeDecel = 240.0;
    this->turnRate = p1.turnRate;
    this->driftFactor = p1.driftFactor;
    this->weight = p1.weight;

    // Reset drift state
    isDrifting = false;
    driftTimer = 0.0;
    miniTurboTimer = 0.0;
    driftHopTimer = 0.0;
    driftHopHeight = 0.0;
    lateralSlipAngle = 0.0;
    driftDirection = 0;
    driftSparks.clear();

    player1.id = 0;
    player1.isHuman = true;
    player1.characterId = p1.id;
    player1.name = p1.name;
    player1.kartColor = p1.primaryColor;
    player1.maxSpeed = p1.maxSpeed;
    player1.accel = p1.accel;
    player1.brakeDecel = 240.0;
    player1.turnRate = p1.turnRate;
    player1.driftFactor = p1.driftFactor;
    player1.weight = p1.weight;
    player1.isDrifting = false;
    player1.driftTimer = 0.0;
    player1.miniTurboTimer = 0.0;
    player1.driftHopTimer = 0.0;
    player1.lateralSlip = 0.0;
    player1.driftDirection = 0;

    double baseZ = (currentLevel == TrackLevel::BOWSER_CASTLE) ? 250.0 : 200.0;
    kartX = 470.0;
    kartZ = baseZ - 6.0;
    kartAngle = 0.0;
    kartSpeed = 0.0;
    steeringVisualAngle = 0.0;
    currentLap = 1;
    nextCheckpoint = 1;
    hasPassedHalfway = false;
    firstStartCrossed = false;

    player1.x = kartX;
    player1.z = kartZ;
    player1.angle = kartAngle;
    player1.speed = kartSpeed;
    player1.currentLap = 1;
    player1.nextCheckpoint = 1;
    player1.hasPassedHalfway = false;
    player1.firstStartCrossed = false;
    player1.raceFinished = false;
    player1.totalRaceDistance = 0.0;

    if (selectedGameMode == GameMode::MULTIPLAYER) {
        const CharacterDef &p2 = getSelectedCharacterP2();
        player2.id = 1;
        player2.isHuman = true;
        player2.isAI = false;
        player2.characterId = p2.id;
        player2.name = p2.name;
        player2.kartColor = (selectedCharP1 == selectedCharP2) ? p2.altColor : p2.primaryColor;
        player2.maxSpeed = p2.maxSpeed;
        player2.accel = p2.accel;
        player2.brakeDecel = 240.0;
        player2.friction = 45.0;
        player2.turnRate = p2.turnRate;
        player2.driftFactor = p2.driftFactor;
        player2.weight = p2.weight;
        player2.x = 470.0 - 18.0;
        player2.z = baseZ + 6.0;
        player2.angle = 0.0;
        player2.speed = 0.0;
        player2.currentLap = 1;
        player2.nextCheckpoint = 1;
        player2.hasPassedHalfway = false;
        player2.firstStartCrossed = false;
        player2.raceFinished = false;
        player2.totalRaceDistance = 0.0;
    }

    aiOpponents.clear();
    if (selectedGameMode == GameMode::PLAY_VS_AI) {
        int aiSlot = 1;
        for (int i = 0; i < 4; ++i) {
            if (i == selectedCharP1) continue;
            const CharacterDef &aiDef = characters[i];
            KartEntity aiKart;
            aiKart.id = aiSlot;
            aiKart.isHuman = false;
            aiKart.isAI = true;
            aiKart.characterId = aiDef.id;
            aiKart.name = aiDef.name;
            aiKart.kartColor = aiDef.primaryColor;
            aiKart.maxSpeed = aiDef.maxSpeed;
            aiKart.accel = aiDef.accel;
            aiKart.brakeDecel = 240.0;
            aiKart.friction = 45.0;
            aiKart.turnRate = aiDef.turnRate;
            aiKart.driftFactor = aiDef.driftFactor;
            aiKart.weight = aiDef.weight;
            aiKart.x = 470.0 - aiSlot * 18.0;
            aiKart.z = baseZ + ((aiSlot % 2 == 0) ? -6.0 : +6.0);
            aiKart.angle = 0.0;
            aiKart.speed = 0.0;
            aiKart.currentWaypoint = 0;
            aiKart.currentWaypointIndex = 0;
            aiKart.lookaheadDist = 50.0;
            aiKart.currentLap = 1;
            aiKart.placement = aiSlot + 1;
            aiKart.totalRaceDistance = 0.0;
            aiKart.isSpinning = false;
            aiKart.spinTimer = 0.0;
            aiKart.isInked = false;
            aiKart.inkTimer = 0.0;
            aiKart.hasPassedHalfway = false;
            aiKart.firstStartCrossed = false;
            aiKart.raceFinished = false;
            aiOpponents.push_back(aiKart);
            aiSlot++;
            if (aiSlot >= 4) break;
        }
    }

    heldItem = PlayerItem::NONE;
    isRouletteActive = false;
    rouletteTimer = 0.0;
    mushroomBoostTimer = 0.0;
    isSpinning = false;
    spinTimer = 0.0;
    inkTimer = 0.0;

    isCelebrationActive = false;
    isRaceFinished = false;
    celebrationTimer = 0.0;
    flashIntensity = 0.0;
    confetti.clear();

    activeProjectiles.clear();

    currentState = GameState::RACING;
    pressedKeys.clear();
    update();
}

void GameCanvas::resetKart()
{
    if (currentState == GameState::RACING || currentState == GameState::RACE_FINISHED) {
        startRace();
    } else {
        if (currentLevel == TrackLevel::BOWSER_CASTLE) {
            kartX = 470.0;
            kartZ = 250.0;
        } else {
            kartX = 470.0;
            kartZ = 200.0;
        }
        kartAngle = 0.0;
        kartSpeed = 0.0;
        steeringVisualAngle = 0.0;
        currentLap = 1;
        nextCheckpoint = 1;
        heldItem = PlayerItem::NONE;
        isRouletteActive = false;
        rouletteTimer = 0.0;
        mushroomBoostTimer = 0.0;
        isSpinning = false;
        spinTimer = 0.0;
        inkTimer = 0.0;
        isCelebrationActive = false;
        isRaceFinished = false;
        celebrationTimer = 0.0;
        flashIntensity = 0.0;
        hasPassedHalfway = false;
        firstStartCrossed = false;
        confetti.clear();
    }
}

void GameCanvas::mousePressEvent(QMouseEvent *event)
{
    setFocus();
    QWidget::mousePressEvent(event);
}

void GameCanvas::focusInEvent(QFocusEvent *event)
{
    QWidget::focusInEvent(event);
}

void GameCanvas::focusOutEvent(QFocusEvent *event)
{
    pressedKeys.clear();
    QWidget::focusOutEvent(event);
}

void GameCanvas::keyPressEvent(QKeyEvent *event)
{
    int key = event->key();

    switch (currentState) {
    case GameState::TITLE_MENU:
        handleTitleMenuInput(key);
        break;

    case GameState::CHARACTER_SELECT:
        handleCharacterSelectInput(key);
        break;

    case GameState::RACING:
    case GameState::RACE_FINISHED:
        handleRacingInput(key);
        break;
    }
}

void GameCanvas::keyReleaseEvent(QKeyEvent *event)
{
    pressedKeys.erase(event->key());
}

void GameCanvas::handleTitleMenuInput(int key)
{
    if (key == Qt::Key_Up || key == Qt::Key_W) {
        menuIndex = (menuIndex - 1 + 3) % 3;
        update();
    } else if (key == Qt::Key_Down || key == Qt::Key_S) {
        menuIndex = (menuIndex + 1) % 3;
        update();
    } else if (key == Qt::Key_1) {
        menuIndex = 0;
        confirmTitleMenuSelection();
    } else if (key == Qt::Key_2) {
        menuIndex = 1;
        confirmTitleMenuSelection();
    } else if (key == Qt::Key_3) {
        menuIndex = 2;
        confirmTitleMenuSelection();
    } else if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Space) {
        confirmTitleMenuSelection();
    }
}

void GameCanvas::confirmTitleMenuSelection()
{
    if (menuIndex == 0) {
        selectedGameMode = GameMode::PLAY_VS_AI;
    } else if (menuIndex == 1) {
        selectedGameMode = GameMode::MULTIPLAYER;
    } else {
        selectedGameMode = GameMode::PRACTICE;
    }
    charSelectPhase = 0;
    setGameState(GameState::CHARACTER_SELECT);
}

void GameCanvas::handleCharacterSelectInput(int key)
{
    if (key == Qt::Key_Escape || key == Qt::Key_Backspace) {
        if (selectedGameMode == GameMode::MULTIPLAYER && charSelectPhase == 1) {
            charSelectPhase = 0;
            update();
        } else {
            setGameState(GameState::TITLE_MENU);
        }
        return;
    }

    if (key == Qt::Key_1) {
        setTrackLevel(TrackLevel::DONUT_PLAINS);
        return;
    } else if (key == Qt::Key_2) {
        setTrackLevel(TrackLevel::CHOCO_VALLEY);
        return;
    } else if (key == Qt::Key_3) {
        setTrackLevel(TrackLevel::BOWSER_CASTLE);
        return;
    }

    if (selectedGameMode == GameMode::MULTIPLAYER) {
        if (charSelectPhase == 0) {
            if (key == Qt::Key_Left || key == Qt::Key_A) {
                selectedCharP1 = (selectedCharP1 - 1 + 4) % 4;
                update();
            } else if (key == Qt::Key_Right || key == Qt::Key_D) {
                selectedCharP1 = (selectedCharP1 + 1) % 4;
                update();
            } else if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Space) {
                charSelectPhase = 1;
                if (selectedCharP2 == selectedCharP1) {
                    selectedCharP2 = (selectedCharP1 + 1) % 4;
                }
                update();
            }
        } else {
            if (key == Qt::Key_Left || key == Qt::Key_A) {
                selectedCharP2 = (selectedCharP2 - 1 + 4) % 4;
                update();
            } else if (key == Qt::Key_Right || key == Qt::Key_D) {
                selectedCharP2 = (selectedCharP2 + 1) % 4;
                update();
            } else if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Space) {
                startRace();
            }
        }
    } else {
        if (key == Qt::Key_Left || key == Qt::Key_A) {
            selectedCharP1 = (selectedCharP1 - 1 + 4) % 4;
            update();
        } else if (key == Qt::Key_Right || key == Qt::Key_D) {
            selectedCharP1 = (selectedCharP1 + 1) % 4;
            update();
        } else if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Space) {
            startRace();
        }
    }
}

void GameCanvas::handleRacingInput(int key)
{
    if (currentState == GameState::RACE_FINISHED) {
        if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Space || key == Qt::Key_Escape) {
            setGameState(GameState::TITLE_MENU);
            return;
        } else if (key == Qt::Key_R) {
            startRace();
            return;
        }
    }

    if (key == Qt::Key_Escape) {
        setGameState(GameState::TITLE_MENU);
        return;
    }

    pressedKeys.insert(key);

    if (key == Qt::Key_Space) {
        useHeldItem();
    } else if (key == Qt::Key_R) {
        startRace();
    } else if (key == Qt::Key_F) {
        triggerCelebration("★ FINISH! ★", "COURSE CLEAR!");
    } else if (key == Qt::Key_1) {
        setTrackLevel(TrackLevel::DONUT_PLAINS);
    } else if (key == Qt::Key_2) {
        setTrackLevel(TrackLevel::CHOCO_VALLEY);
    } else if (key == Qt::Key_3) {
        setTrackLevel(TrackLevel::BOWSER_CASTLE);
    }
}

void GameCanvas::extractDonutPlainsWaypoints()
{
    std::vector<QPointF> &wps = trackWaypoints[static_cast<int>(TrackLevel::DONUT_PLAINS)];
    wps = {
        QPointF(512.0, 200.0),
        QPointF(622.9, 225.8),
        QPointF(702.1, 293.8),
        QPointF(749.7, 389.8),
        QPointF(765.5, 500.0),
        QPointF(749.7, 610.2),
        QPointF(702.1, 706.2),
        QPointF(622.9, 774.2),
        QPointF(512.0, 800.0),
        QPointF(403.1, 774.2),
        QPointF(325.2, 706.2),
        QPointF(278.6, 610.2),
        QPointF(263.0, 500.0),
        QPointF(278.6, 389.8),
        QPointF(325.2, 293.8),
        QPointF(403.1, 225.8)
    };
}

void GameCanvas::extractChocoValleyWaypoints()
{
    std::vector<QPointF> &wps = trackWaypoints[static_cast<int>(TrackLevel::CHOCO_VALLEY)];
    wps = {
        QPointF(512.0, 200.0),
        QPointF(684.2, 241.0),
        QPointF(770.4, 321.3),
        QPointF(787.3, 413.1),
        QPointF(751.6, 488.6),
        QPointF(680.0, 520.0),
        QPointF(621.1, 551.2),
        QPointF(590.0, 625.6),
        QPointF(571.7, 714.4),
        QPointF(550.8, 788.8),
        QPointF(512.0, 820.0),
        QPointF(369.3, 783.6),
        QPointF(296.2, 692.0),
        QPointF(263.2, 571.6),
        QPointF(240.9, 448.8),
        QPointF(200.0, 350.0),
        QPointF(186.2, 284.5),
        QPointF(228.6, 241.0),
        QPointF(309.2, 215.4),
        QPointF(409.7, 203.1)
    };
}

void GameCanvas::extractBowserCastleWaypoints()
{
    std::vector<QPointF> &wps = trackWaypoints[static_cast<int>(TrackLevel::BOWSER_CASTLE)];
    wps = {
        QPointF(512.0, 250.0),
        QPointF(600.3, 250.0),
        QPointF(688.7, 250.0),
        QPointF(777.0, 250.0),
        QPointF(780.0, 335.3),
        QPointF(780.0, 423.7),
        QPointF(780.0, 512.0),
        QPointF(729.7, 550.0),
        QPointF(641.3, 550.0),
        QPointF(553.0, 550.0),
        QPointF(550.0, 635.3),
        QPointF(550.0, 723.7),
        QPointF(518.0, 780.0),
        QPointF(429.7, 780.0),
        QPointF(341.3, 780.0),
        QPointF(253.0, 780.0),
        QPointF(250.0, 694.7),
        QPointF(250.0, 606.3),
        QPointF(250.0, 518.0),
        QPointF(250.0, 429.7),
        QPointF(250.0, 341.3),
        QPointF(250.0, 253.0),
        QPointF(335.3, 250.0),
        QPointF(423.7, 250.0)
    };
}

void GameCanvas::initTrackWaypoints()
{
    extractDonutPlainsWaypoints();
    extractChocoValleyWaypoints();
    extractBowserCastleWaypoints();
    currentTrackWaypoints = trackWaypoints[static_cast<int>(currentLevel)];
}

SpriteType GameCanvas::getKartSpriteType(int characterId, bool isAlt) const
{
    switch (characterId) {
    case 0: return isAlt ? SpriteType::KART_MARIO_ALT : SpriteType::KART_MARIO;
    case 1: return isAlt ? SpriteType::KART_BOWSER_ALT : SpriteType::KART_BOWSER;
    case 2: return isAlt ? SpriteType::KART_PEACH_ALT : SpriteType::KART_PEACH;
    case 3: return isAlt ? SpriteType::KART_TOAD_ALT : SpriteType::KART_TOAD;
    default: return isAlt ? SpriteType::KART_MARIO_ALT : SpriteType::KART_MARIO;
    }
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
    currentTrackWaypoints = trackWaypoints[static_cast<int>(TrackLevel::DONUT_PLAINS)];
    trackHazards.clear();
    updateMinimapTrackCache();
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
    currentTrackWaypoints = trackWaypoints[static_cast<int>(TrackLevel::CHOCO_VALLEY)];

    trackHazards = {
        { 700.0, 220.0, HazardType::CHOCOLATE_FOUNTAIN, 0.0, true, 15.0 },
        { 600.0, 580.0, HazardType::CHOCOLATE_FOUNTAIN, 0.0, true, 15.0 },
        { 300.0, 760.0, HazardType::CHOCOLATE_FOUNTAIN, 0.0, true, 15.0 },
        { 550.0, 180.0, HazardType::ROLLING_CANDY, 0.0, false, 14.0 },
        { 400.0, 850.0, HazardType::ROLLING_CANDY, 1.5, false, 14.0 }
    };
    updateMinimapTrackCache();
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
    currentTrackWaypoints = trackWaypoints[static_cast<int>(TrackLevel::BOWSER_CASTLE)];

    trackHazards = {
        { 500.0, 150.0, HazardType::LAVA_GEYSER, 0.0, false, 16.0 },
        { 850.0, 350.0, HazardType::LAVA_GEYSER, 1.2, false, 16.0 },
        { 650.0, 850.0, HazardType::LAVA_GEYSER, 2.4, false, 16.0 },
        { 200.0, 650.0, HazardType::LAVA_GEYSER, 3.6, false, 16.0 }
    };
    updateMinimapTrackCache();
}

// -------------------------------------------------------------
// GAME LOOP & PHYSICS
// -------------------------------------------------------------

void GameCanvas::updateGameLoop()
{
    double dt = frameTimer.restart() / 1000.0;
    if (dt > 0.05) dt = 0.05; // clamp delta time for stability

    globalTime += dt;

    switch (currentState) {
    case GameState::TITLE_MENU:
        renderTitleMenu();
        emit statsUpdated(0.0, 0, 0.0, 0.0, 0.0, "MENU: TITLE SCREEN");
        break;

    case GameState::CHARACTER_SELECT:
        renderCharacterSelect();
        emit statsUpdated(0.0, 0, 0.0, 0.0, 0.0, "MENU: CHARACTER SELECT");
        break;

    case GameState::RACING:
        updatePhysics(dt);
        if (selectedGameMode == GameMode::PLAY_VS_AI) {
            updateAIOpponents(dt);
        }
        updateItemSystem(dt);
        updateTrackHazards(dt);
        if (isCelebrationActive) {
            updateCelebration(dt);
        }

        renderSky();
        renderGroundMode7();
        renderSprites();
        renderCockpit();
        renderItemHUD();
        renderMinimap();
        renderInkOverlay();
        if (isCelebrationActive) {
            renderCelebration();
        }

        emit statsUpdated(kartSpeed, currentLap, kartX, kartZ, kartAngle * (180.0 / M_PI), currentSurfaceName);
        break;

    case GameState::RACE_FINISHED:
        updatePhysics(dt);
        if (selectedGameMode == GameMode::PLAY_VS_AI) {
            updateAIOpponents(dt);
        }
        updateTrackHazards(dt);
        if (isCelebrationActive) {
            updateCelebration(dt);
        }

        renderSky();
        renderGroundMode7();
        renderSprites();
        renderCockpit();
        renderItemHUD();
        renderMinimap();
        renderInkOverlay();
        if (isCelebrationActive) {
            renderCelebration();
        }
        renderRaceFinishedOverlay();

        emit statsUpdated(kartSpeed, currentLap, kartX, kartZ, kartAngle * (180.0 / M_PI), "🏆 RACE FINISHED!");
        break;
    }

    this->update(); // Request Qt paintEvent
}

void GameCanvas::updatePhysics(double dt)
{
    if (dt > 0.05) dt = 0.05;

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
        spinTimer -= dt;
        if (spinTimer <= 0.0) {
            spinTimer = 0.0;
            isSpinning = false;
        }
        isDrifting = false;
        driftTimer = 0.0;
        driftHopTimer = 0.0;
        driftHopHeight = 0.0;
        lateralSlipAngle = 0.0;
        driftSparks.clear();

        kartAngle += 14.0 * dt;
        kartSpeed = std::max(0.0, kartSpeed - 140.0 * dt);
        kartX += kartSpeed * std::cos(kartAngle) * dt;
        kartZ += kartSpeed * std::sin(kartAngle) * dt;
        currentSurfaceName = "SPINNING OUT!";
        return;
    }

    // Buff / Debuff timers countdown
    if (inkTimer > 0.0) {
        inkTimer = std::max(0.0, inkTimer - dt);
    }
    if (mushroomBoostTimer > 0.0) {
        mushroomBoostTimer = std::max(0.0, mushroomBoostTimer - dt);
    }
    if (miniTurboTimer > 0.0) {
        miniTurboTimer = std::max(0.0, miniTurboTimer - dt);
    }

    // Sample terrain mask
    int sampleU = (static_cast<int>(kartX) % TRACK_MAP_SIZE + TRACK_MAP_SIZE) % TRACK_MAP_SIZE;
    int sampleV = (static_cast<int>(kartZ) % TRACK_MAP_SIZE + TRACK_MAP_SIZE) % TRACK_MAP_SIZE;
    QRgb maskColor = trackMaskMap.pixel(sampleU, sampleV);

    double currentMaxSpeed = maxSpeed;
    double currentDrag = friction;
    double currentAccel = accel;

    // Buff / Debuff modifiers
    if (inkTimer > 0.0) {
        currentMaxSpeed *= 0.70; // 30% speed debuff
    }
    if (miniTurboTimer > 0.0) {
        currentMaxSpeed *= 1.35; // 35% mini-turbo speed boost
        currentAccel *= 1.80;
    }
    if (mushroomBoostTimer > 0.0) {
        currentMaxSpeed = maxSpeed * 1.55;
        currentAccel = accel * 2.20;
    }

    // Terrain mask effects
    bool isWhiteRoad = (qRed(maskColor) == 255 && qGreen(maskColor) == 255 && qBlue(maskColor) == 255);
    bool isStartFinish = (qRed(maskColor) == 255 && qGreen(maskColor) == 255 && qBlue(maskColor) == 0);
    bool isLava = (qRed(maskColor) == 255 && qBlue(maskColor) == 255 && qGreen(maskColor) == 0);
    bool isOffroad = (qGreen(maskColor) == 255 && qRed(maskColor) == 0);

    if (isWhiteRoad || isStartFinish) {
        if (miniTurboTimer > 0.0) {
            currentSurfaceName = "Road (⚡ MINI-TURBO! ⚡)";
        } else if (mushroomBoostTimer > 0.0) {
            currentSurfaceName = "Road (BOOST ACTIVE!)";
        } else if (isDrifting) {
            currentSurfaceName = (driftTimer >= 1.0) ? "Road (🔥 DRIFT CHARGED! 🔥)" : "Road (Drifting...)";
        } else {
            currentSurfaceName = "Road (Tarmac)";
        }
    } else if (isLava) {
        currentSurfaceName = "LAVA RECOIL! (Sizzle)";
        kartSpeed = std::max(0.0, kartSpeed * 0.35);
        kartX -= std::cos(kartAngle) * 8.0;
        kartZ -= std::sin(kartAngle) * 8.0;
        isSpinning = true;
        spinTimer = 0.45;
        flashIntensity = 0.5;
        isDrifting = false;
        driftTimer = 0.0;
        driftHopTimer = 0.0;
        driftHopHeight = 0.0;
        lateralSlipAngle = 0.0;
        driftSparks.clear();
        return;
    } else if (isOffroad) {
        currentSurfaceName = (currentLevel == TrackLevel::BOWSER_CASTLE) ? "Stone Curb (Shoulder)" : "Off-road (Grass/Sand)";
        if (mushroomBoostTimer <= 0.0 && miniTurboTimer <= 0.0) {
            currentMaxSpeed *= (currentLevel == TrackLevel::BOWSER_CASTLE) ? 0.70 : 0.45;
            currentDrag *= 2.2;
        }
    }

    // Controls
    bool forward = pressedKeys.count(Qt::Key_W) || pressedKeys.count(Qt::Key_Up);
    bool backward = pressedKeys.count(Qt::Key_S) || pressedKeys.count(Qt::Key_Down);
    bool left = pressedKeys.count(Qt::Key_A) || pressedKeys.count(Qt::Key_Left);
    bool right = pressedKeys.count(Qt::Key_D) || pressedKeys.count(Qt::Key_Right);
    bool driftKey = pressedKeys.count(Qt::Key_Shift) > 0 || pressedKeys.count(Qt::Key_Space) > 0;

    int steerDirection = 0;
    if (left && !right) steerDirection = -1;
    else if (right && !left) steerDirection = 1;

    // WASD Throttle & High-Grip Braking
    if (forward) {
        double ratio = (currentMaxSpeed > 0.0) ? (kartSpeed / currentMaxSpeed) : 0.0;
        double accelEff = currentAccel * (1.0 - 0.4 * std::clamp(ratio, 0.0, 1.0));
        kartSpeed += accelEff * dt;
        if (kartSpeed > currentMaxSpeed) kartSpeed = currentMaxSpeed;
    } else if (backward) {
        kartSpeed -= brakeDecel * dt;
        double maxReverse = -0.30 * maxSpeed;
        if (kartSpeed < maxReverse) kartSpeed = maxReverse;
    } else {
        if (kartSpeed > 0.0) {
            kartSpeed -= currentDrag * dt;
            if (kartSpeed < 0.0) kartSpeed = 0.0;
        } else if (kartSpeed < 0.0) {
            kartSpeed += currentDrag * dt;
            if (kartSpeed > 0.0) kartSpeed = 0.0;
        }
    }

    // Dedicated Drift Mechanic & Hop
    if (driftKey && steerDirection != 0 && kartSpeed > 40.0) {
        if (!isDrifting) {
            isDrifting = true;
            driftDirection = steerDirection;
            driftTimer = 0.0;
            driftHopTimer = 0.22;
        }
        driftTimer += dt;

        double driftTurnMult = this->driftFactor;
        if (steerDirection == driftDirection) {
            driftTurnMult *= 1.25;
        } else {
            driftTurnMult *= 0.45;
        }
        kartAngle += driftDirection * this->turnRate * driftTurnMult * dt;

        double targetBeta = driftDirection * 0.28 * this->driftFactor;
        lateralSlipAngle += (targetBeta - lateralSlipAngle) * 8.0 * dt;

        updateDriftSparks(dt);
    } else {
        if (isDrifting) {
            if (driftTimer >= 1.0) {
                miniTurboTimer = 1.2;
                kartSpeed = std::min(currentMaxSpeed * 1.35, kartSpeed + 25.0);
                flashIntensity = 0.35;
            }
            isDrifting = false;
            driftTimer = 0.0;
            driftDirection = 0;
            driftSparks.clear();
        }
        lateralSlipAngle += (0.0 - lateralSlipAngle) * 12.0 * dt;

        double steerSpeedFactor = (std::abs(kartSpeed) > 1.0) ? std::clamp(std::abs(kartSpeed) / 50.0, 0.70, 1.30) : 0.0;
        kartAngle += steerDirection * this->turnRate * steerSpeedFactor * dt;
    }

    // Drift Hop Parabola
    if (driftHopTimer > 0.0) {
        driftHopTimer -= dt;
        if (driftHopTimer < 0.0) driftHopTimer = 0.0;
        double tau = 1.0 - (driftHopTimer / 0.22);
        driftHopHeight = 3.5 * std::sin(M_PI * std::clamp(tau, 0.0, 1.0));
    } else {
        driftHopHeight = 0.0;
    }

    // Cockpit wheel animation lerp
    double targetWheelAngle = steerDirection * 0.35;
    if (isDrifting) targetWheelAngle = driftDirection * 0.50;
    steeringVisualAngle += (targetWheelAngle - steeringVisualAngle) * 10.0 * dt;

    // Angle wrap [0, 2*PI]
    while (kartAngle >= 2.0 * M_PI) kartAngle -= 2.0 * M_PI;
    while (kartAngle < 0.0) kartAngle += 2.0 * M_PI;

    double oldX = kartX;
    double travelAngle = kartAngle - lateralSlipAngle;
    kartX += kartSpeed * std::cos(travelAngle) * dt;
    kartZ += kartSpeed * std::sin(travelAngle) * dt;

    // Boundary wrapping
    while (kartX < 0.0) kartX += 1024.0;
    while (kartX >= 1024.0) kartX -= 1024.0;
    while (kartZ < 0.0) kartZ += 1024.0;
    while (kartZ >= 1024.0) kartZ -= 1024.0;

    // Synchronize player1 entity
    player1.x = kartX;
    player1.z = kartZ;
    player1.speed = kartSpeed;
    player1.angle = kartAngle;
    player1.isDrifting = isDrifting;
    player1.driftTimer = driftTimer;
    player1.miniTurboTimer = miniTurboTimer;
    player1.lateralSlip = lateralSlipAngle;
    player1.totalRaceDistance += kartSpeed * dt;

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
                currentState = GameState::RACE_FINISHED;
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
                if (sprite.ownerId == 0 && sprite.immunityTimer > 0.0) {
                    continue;
                }
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

void GameCanvas::updateAIOpponents(double dt)
{
    if (aiOpponents.empty()) return;
    dt = std::min(dt, 0.05);

    // Keep Player 1 state synced for rubber-banding and placement calculation
    player1.totalRaceDistance += kartSpeed * dt;
    player1.x = kartX;
    player1.z = kartZ;
    player1.speed = kartSpeed;
    player1.angle = kartAngle;
    player1.currentLap = currentLap;

    const auto &waypoints = currentTrackWaypoints;
    if (waypoints.empty()) return;
    int N = static_cast<int>(waypoints.size());

    for (auto &ai : aiOpponents) {
        // 1. Ink debuff decay (must update continuously regardless of spinout)
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
                int targetIdx = (ai.currentWaypoint + 1) % N;
                ai.angle = std::atan2(waypoints[targetIdx].y() - ai.z, waypoints[targetIdx].x() - ai.x);
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

        // 3. Collision with bananas and track hazards
        for (auto &sprite : worldSprites) {
            if (!sprite.active) continue;
            if (sprite.type == SpriteType::BANANA || sprite.type == SpriteType::BANANA_DROPPED || sprite.type == SpriteType::OIL_SLICK) {
                if (sprite.ownerId == ai.id && sprite.immunityTimer > 0.0) {
                    continue;
                }
                double dx = ai.x - sprite.x;
                double dz = ai.z - sprite.z;
                if (dx * dx + dz * dz < 15.0 * 15.0) {
                    sprite.active = false;
                    ai.isSpinning = true;
                    ai.spinTimer = 1.2;
                    ai.speed *= 0.35;
                    break;
                }
            }
        }
        if (ai.isSpinning) continue;

        // 4. Advance waypoint when proximity < 40.0 units or past waypoint towards next
        int curr = ai.currentWaypoint % N;
        int nextWp = (curr + 1) % N;
        double distCurr = std::hypot(waypoints[curr].x() - ai.x, waypoints[curr].y() - ai.z);
        double distNext = std::hypot(waypoints[nextWp].x() - ai.x, waypoints[nextWp].y() - ai.z);
        double segDx = waypoints[nextWp].x() - waypoints[curr].x();
        double segDz = waypoints[nextWp].y() - waypoints[curr].y();
        double kartDx = ai.x - waypoints[curr].x();
        double kartDz = ai.z - waypoints[curr].y();
        double proj = kartDx * segDx + kartDz * segDz;
        if (distCurr < 40.0 || (proj > 0.0 && distNext < distCurr)) {
            ai.currentWaypoint = nextWp;
            ai.currentWaypointIndex = ai.currentWaypoint;
        }

        // 5. Sample surface to determine pure pursuit lookahead
        int sampleU = (static_cast<int>(ai.x) % TRACK_MAP_SIZE + TRACK_MAP_SIZE) % TRACK_MAP_SIZE;
        int sampleV = (static_cast<int>(ai.z) % TRACK_MAP_SIZE + TRACK_MAP_SIZE) % TRACK_MAP_SIZE;
        QRgb maskColor = trackMaskMap.pixel(sampleU, sampleV);
        bool isOnRoad = (qRed(maskColor) == 255 && qGreen(maskColor) == 255 && qBlue(maskColor) == 255) ||
                        (qRed(maskColor) == 255 && qGreen(maskColor) == 255 && qBlue(maskColor) == 0);

        // Check sharpness of upcoming corner
        double curSegX = waypoints[nextWp].x() - waypoints[curr].x();
        double curSegZ = waypoints[nextWp].y() - waypoints[curr].y();
        int afterNextWp = (nextWp + 1) % N;
        double nextSegX = waypoints[afterNextWp].x() - waypoints[nextWp].x();
        double nextSegZ = waypoints[afterNextWp].y() - waypoints[nextWp].y();
        double dotProduct = curSegX * nextSegX + curSegZ * nextSegZ;
        double lenCur = std::hypot(curSegX, curSegZ);
        double lenNext = std::hypot(nextSegX, nextSegZ);
        bool isSharpCornerAhead = (lenCur > 1.0 && lenNext > 1.0 && (dotProduct / (lenCur * lenNext)) < 0.5);

        double roadAngle = std::atan2(waypoints[nextWp].y() - ai.z, waypoints[nextWp].x() - ai.x);
        double headingDiff = std::abs(ai.angle - roadAngle);
        while (headingDiff > M_PI) headingDiff = std::abs(headingDiff - 2.0 * M_PI);
        bool isTurnAligned = (headingDiff <= 0.45);

        // On road and aligned: pure pursuit lookahead ((curr + 2) % N); otherwise steer to next waypoint
        int lookaheadIdx = (isOnRoad && isTurnAligned && !isSharpCornerAhead) ? ((ai.currentWaypoint + 2) % N) : ((ai.currentWaypoint + 1) % N);
        const QPointF &targetWp = waypoints[lookaheadIdx];
        double dx = targetWp.x() - ai.x;
        double dz = targetWp.y() - ai.z;
        double desiredAngle = std::atan2(dz, dx);

        if (ai.isInked) {
            desiredAngle += std::sin(globalTime * 6.0 + ai.id * 2.0) * 0.12;
        }

        double deltaTheta = desiredAngle - ai.angle;
        while (deltaTheta > M_PI) deltaTheta -= 2.0 * M_PI;
        while (deltaTheta < -M_PI) deltaTheta += 2.0 * M_PI;

        double turnStep = ai.turnRate * dt;
        if (std::abs(deltaTheta) <= turnStep) {
            ai.angle = desiredAngle;
        } else if (deltaTheta > 0.0) {
            ai.angle += turnStep;
        } else {
            ai.angle -= turnStep;
        }
        while (ai.angle >= 2.0 * M_PI) ai.angle -= 2.0 * M_PI;
        while (ai.angle < 0.0) ai.angle += 2.0 * M_PI;

        // 6. Cornering brake/feather throttle when |delta_theta| > 0.65 rad or approaching sharp corner
        bool throttle = true;
        bool brake = false;
        double throttleScale = 1.0;
        if (isSharpCornerAhead && distNext < 50.0 && ai.speed > 85.0) {
            throttle = false;
            brake = true;
        } else if (std::abs(deltaTheta) > 0.65) {
            if (ai.speed > 115.0) {
                throttle = false;
                brake = true;
            } else {
                throttle = true;
                throttleScale = 0.65;
            }
        }

        // 7. Terrain slowdown sampling trackMaskMap
        double terrainSpeedMult = 1.0;
        double terrainDragMult = 1.0;

        if (isOnRoad) {
            // White Road (Tarmac): 100% full speed
            terrainSpeedMult = 1.0;
            terrainDragMult = 1.0;
        } else if (qRed(maskColor) == 255 && qGreen(maskColor) == 0 && qBlue(maskColor) == 255) {
            // Magenta Molten Lava recoil: bounce towards track, spinout, lose speed
            ai.speed *= 0.35;
            double toRoadX = waypoints[nextWp].x() - ai.x;
            double toRoadZ = waypoints[nextWp].y() - ai.z;
            double roadDist = std::hypot(toRoadX, toRoadZ);
            if (roadDist > 1.0) {
                ai.x += (toRoadX / roadDist) * 12.0;
                ai.z += (toRoadZ / roadDist) * 12.0;
            } else {
                ai.x -= std::cos(ai.angle) * 8.0;
                ai.z -= std::sin(ai.angle) * 8.0;
            }
            ai.angle = std::atan2(waypoints[nextWp].y() - ai.z, waypoints[nextWp].x() - ai.x);
            ai.isSpinning = true;
            ai.spinTimer = 0.8;
            continue;
        } else if (qGreen(maskColor) == 255 && qRed(maskColor) == 0) {
            // Green = shoulder (70% on Bowser Castle and Choco Valley sand, 45% grass)
            terrainSpeedMult = (currentLevel == TrackLevel::DONUT_PLAINS) ? 0.45 : 0.70;
            terrainDragMult = 2.0;
        }

        // 8. Dynamic rubber-banding speed regulation
        double distDelta = player1.totalRaceDistance - ai.totalRaceDistance;
        double rubberBandMult = 1.0;
        if (player1.totalRaceDistance > 50.0) {
            if (distDelta > 150.0) {
                double factor = std::clamp((distDelta - 150.0) / 400.0, 0.0, 1.0);
                rubberBandMult = 1.10 + factor * 0.05; // +10% to +15%
            } else if (distDelta < -150.0) {
                double factor = std::clamp((-distDelta - 150.0) / 400.0, 0.0, 1.0);
                rubberBandMult = 0.90 - factor * 0.02; // -10% to -12%
            }
        }

        double effectiveMaxSpeed = ai.maxSpeed * rubberBandMult * terrainSpeedMult;
        if (ai.isInked) {
            effectiveMaxSpeed *= 0.65; // 35% speed reduction
        }

        // 9. Throttle acceleration / braking
        if (throttle) {
            double ratio = (effectiveMaxSpeed > 0.0) ? (ai.speed / effectiveMaxSpeed) : 0.0;
            double accelEff = ai.accel * (1.0 - 0.25 * std::clamp(ratio, 0.0, 1.0)) * throttleScale;
            ai.speed += accelEff * dt;
            if (ai.speed > effectiveMaxSpeed) ai.speed = effectiveMaxSpeed;
        } else if (brake) {
            ai.speed -= ai.brakeDecel * dt;
            if (ai.speed < 0.0) ai.speed = 0.0;
        } else {
            double drag = ai.friction * terrainDragMult;
            ai.speed = std::max(0.0, ai.speed - drag * dt);
        }

        // 10. Position update along heading
        ai.x += ai.speed * std::cos(ai.angle) * dt;
        ai.z += ai.speed * std::sin(ai.angle) * dt;
        ai.totalRaceDistance += ai.speed * dt;

        while (ai.x < 0.0) ai.x += TRACK_MAP_SIZE;
        while (ai.x >= TRACK_MAP_SIZE) ai.x -= TRACK_MAP_SIZE;
        while (ai.z < 0.0) ai.z += TRACK_MAP_SIZE;
        while (ai.z >= TRACK_MAP_SIZE) ai.z -= TRACK_MAP_SIZE;

        // 11. Halfway checkpoint and lap progression
        if (ai.z > 650.0) ai.hasPassedHalfway = true;
        double trackZMin = (currentLevel == TrackLevel::BOWSER_CASTLE) ? 180.0 : 140.0;
        double trackZMax = (currentLevel == TrackLevel::BOWSER_CASTLE) ? 320.0 : 260.0;
        if (ai.x >= 508.0 && ai.x <= 516.0 && ai.z >= trackZMin && ai.z <= trackZMax && ai.hasPassedHalfway) {
            ai.hasPassedHalfway = false;
            ai.currentLap++;
            if (ai.currentLap > 3) {
                ai.raceFinished = true;
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

    // Item box respawn timers and banana immunity decay
    for (auto &sprite : worldSprites) {
        if (!sprite.active && sprite.respawnTimer > 0.0) {
            sprite.respawnTimer -= dt;
            if (sprite.respawnTimer <= 0.0) {
                sprite.active = true;
                sprite.respawnTimer = 0.0;
            }
        }
        if (sprite.active && sprite.immunityTimer > 0.0) {
            sprite.immunityTimer = std::max(0.0, sprite.immunityTimer - dt);
        }
    }

    // Dynamic projectile physics & collision
    for (auto &shell : activeProjectiles) {
        if (!shell.active) continue;

        shell.x += shell.vx * dt;
        shell.z += shell.vz * dt;
        shell.lifeTime -= dt;

        if (shell.lifeTime <= 0.0) {
            shell.active = false;
            continue;
        }

        // Boundary bounce off walls [20.0, 1004.0]
        bool bounced = false;
        if (shell.x < 20.0) {
            shell.x = 20.0;
            shell.vx = -shell.vx;
            bounced = true;
        } else if (shell.x > 1004.0) {
            shell.x = 1004.0;
            shell.vx = -shell.vx;
            bounced = true;
        }
        if (shell.z < 20.0) {
            shell.z = 20.0;
            shell.vz = -shell.vz;
            bounced = true;
        } else if (shell.z > 1004.0) {
            shell.z = 1004.0;
            shell.vz = -shell.vz;
            bounced = true;
        }

        if (bounced) {
            shell.bouncesLeft--;
            if (shell.bouncesLeft <= 0) {
                shell.active = false;
                continue;
            }
        }

        // Check collision with AI opponents (radius 16.0)
        for (auto &ai : aiOpponents) {
            if (shell.ownerId == ai.id && shell.bouncesLeft == 3) continue;

            double dx = ai.x - shell.x;
            double dz = ai.z - shell.z;
            if (dx * dx + dz * dz < 16.0 * 16.0) {
                shell.active = false;
                ai.isSpinning = true;
                ai.spinTimer = 1.2;
                ai.speed *= 0.35;
                break;
            }
        }

        // Check collision with Player 1 (if shell has bounced or was fired by opponent)
        if (shell.active && !(shell.ownerId == 0 && shell.bouncesLeft == 3)) {
            double dx = kartX - shell.x;
            double dz = kartZ - shell.z;
            if (dx * dx + dz * dz < 14.0 * 14.0) {
                shell.active = false;
                isSpinning = true;
                spinTimer = 1.0;
                kartSpeed *= 0.35;
            }
        }
    }

    activeProjectiles.erase(
        std::remove_if(activeProjectiles.begin(), activeProjectiles.end(),
                       [](const Projectile &p) { return !p.active; }),
        activeProjectiles.end());

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
        // Drop a banana peel behind the player's kart with owner immunity window
        double dropDist = 16.0;
        double bx = kartX - std::cos(kartAngle) * dropDist;
        double bz = kartZ - std::sin(kartAngle) * dropDist;
        worldSprites.push_back({ bx, bz, 0.0, SpriteType::BANANA_DROPPED, true, 0.0, 0, 1.2 });
        break;
    }

    case PlayerItem::GREEN_SHELL: {
        // Fire green shell forward as active dynamic projectile
        double fireDist = 14.0;
        double fx = kartX + std::cos(kartAngle) * fireDist;
        double fz = kartZ + std::sin(kartAngle) * fireDist;
        double launchSpeed = std::max(180.0, kartSpeed + 75.0);
        Projectile shell;
        shell.x = fx;
        shell.z = fz;
        shell.vx = std::cos(kartAngle) * launchSpeed;
        shell.vz = std::sin(kartAngle) * launchSpeed;
        shell.ownerId = 0;
        shell.active = true;
        shell.lifeTime = 6.0;
        shell.bouncesLeft = 3;
        activeProjectiles.push_back(shell);
        break;
    }

    case PlayerItem::INK_BLOOPER:
        inkTimer = 0.0; // Player 1 windshield remains 100% clean
        player1.isInked = false;
        player1.inkTimer = 0.0;
        for (auto &ai : aiOpponents) {
            ai.isInked = true;
            ai.inkTimer = 4.0;
            ai.speed *= 0.65;
        }
        if (selectedGameMode == GameMode::MULTIPLAYER) {
            player2.isInked = true;
            player2.inkTimer = 4.0;
            player2.speed *= 0.65;
        }
        flashIntensity = 0.35;
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
    double effectiveCamHeight = cameraHeight + driftHopHeight * 0.6;

    for (int y = HORIZON_Y; y < BUFFER_HEIGHT; ++y) {
        int dy = y - HORIZON_Y;
        if (dy == 0) dy = 1;

        // Similar triangles perspective: Distance to ground row
        double zGround = (effectiveCamHeight * focalLength) / dy;
        double scaleGround = effectiveCamHeight / dy;

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
    renderList.reserve(worldSprites.size() + aiOpponents.size() + activeProjectiles.size() + 4);

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

    // 2. Project Active AI Opponent Fleet Billboards
    if (selectedGameMode == GameMode::PLAY_VS_AI) {
        for (const auto &ai : aiOpponents) {
            double dx = ai.x - kartX;
            double dz = ai.z - kartZ;

            double camX = -dx * sinA + dz * cosA;
            double camZ =  dx * cosA + dz * sinA;

            if (camZ <= 5.0 || camZ > 480.0) continue;

            int sx = static_cast<int>((BUFFER_WIDTH / 2) + (focalLength * camX) / camZ);
            int groundY = static_cast<int>(HORIZON_Y + (focalLength * cameraHeight) / camZ);
            if (groundY <= HORIZON_Y) continue;

            double scale = focalLength / camZ;
            double baseSize = (ai.characterId == 1) ? 25.0 : 22.0;
            int drawW = static_cast<int>(baseSize * scale);
            int drawH = static_cast<int>(baseSize * scale);

            if (drawW <= 2 || drawH <= 2) continue;
            drawW = std::min(180, drawW);
            drawH = std::min(180, drawH);

            if (sx + drawW / 2 < 0 || sx - drawW / 2 >= BUFFER_WIDTH) continue;

            SpriteType kartSprite = getKartSpriteType(ai.characterId);
            renderList.push_back({ sx, groundY, drawW, drawH, camZ, kartSprite, false, groundY });
        }
    }

    // 3. Project Active Projectiles (Shells)
    for (const auto &shell : activeProjectiles) {
        if (!shell.active) continue;

        double dx = shell.x - kartX;
        double dz = shell.z - kartZ;

        double camX = -dx * sinA + dz * cosA;
        double camZ =  dx * cosA + dz * sinA;

        if (camZ <= 5.0 || camZ > 450.0) continue;

        int sx = static_cast<int>((BUFFER_WIDTH / 2) + (focalLength * camX) / camZ);
        int groundY = static_cast<int>(HORIZON_Y + (focalLength * cameraHeight) / camZ);
        if (groundY <= HORIZON_Y) continue;

        double scale = focalLength / camZ;
        int drawW = static_cast<int>(18.0 * scale);
        int drawH = static_cast<int>(18.0 * scale);

        if (drawW <= 1 || drawH <= 1) continue;

        renderList.push_back({ sx, groundY, drawW, drawH, camZ, SpriteType::SHELL_PROJECTILE, false, groundY });
    }

    // 4. Project Track Hazards (Lava Geysers, Chocolate Fountains & Rolling Candies)
    for (const auto &hazard : trackHazards) {
        double worldX = hazard.x;
        double worldZ = hazard.z;

        // Rolling candy cross-track sinusoidal motion offset
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

        renderList.push_back({ sx, groundY, drawW, drawH, camZ, hazardSprite, false, groundY });
    }

    // Sort farthest to nearest (Painter's Algorithm)
    std::sort(renderList.begin(), renderList.end(), [](const ProjectedSprite &a, const ProjectedSprite &b) {
        return a.depth > b.depth;
    });

    QRgb *pixels = reinterpret_cast<QRgb*>(screenBuffer.bits());

    for (const auto &item : renderList) {
        // Draw soft ground shadow beneath floating pickups and karts
        bool shouldShadow = item.isPickup ||
                            item.type == SpriteType::KART_MARIO || item.type == SpriteType::KART_BOWSER ||
                            item.type == SpriteType::KART_PEACH || item.type == SpriteType::KART_TOAD ||
                            item.type == SpriteType::KART_MARIO_ALT || item.type == SpriteType::KART_BOWSER_ALT ||
                            item.type == SpriteType::KART_PEACH_ALT || item.type == SpriteType::KART_TOAD_ALT ||
                            item.type == SpriteType::CHOCOLATE_CANDY || item.type == SpriteType::CHOCOLATE_FOUNTAIN;
        if (shouldShadow && item.groundScreenY >= HORIZON_Y && item.groundScreenY < BUFFER_HEIGHT) {
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
    const CharacterDef &c = getSelectedCharacterP1();

    // Boost flame / sparks effect when boost is active (Mushroom or Mini-Turbo)
    if (mushroomBoostTimer > 0.0 || miniTurboTimer > 0.0) {
        int cx = BUFFER_WIDTH / 2;
        bool isMiniTurbo = (miniTurboTimer > 0.0 && mushroomBoostTimer <= 0.0);
        for (int i = 0; i < 24; ++i) {
            int px = cx + (rand() % 90 - 45);
            int py = BUFFER_HEIGHT - 35 + (rand() % 35);
            if (px >= 0 && px < BUFFER_WIDTH && py >= 0 && py < BUFFER_HEIGHT) {
                if (isMiniTurbo) {
                    pixels[py * BUFFER_WIDTH + px] = (rand() % 2 == 0) ? qRgb(80, 200, 255) : qRgb(255, 160, 30);
                } else {
                    pixels[py * BUFFER_WIDTH + px] = (rand() % 2 == 0) ? qRgb(255, 230, 50) : qRgb(255, 80, 20);
                }
            }
        }
    }

    // Render drift spark particles over bottom tire areas
    for (const auto &sp : driftSparks) {
        int sx = static_cast<int>(sp.x);
        int sy = static_cast<int>(sp.y);
        if (sx >= 0 && sx < BUFFER_WIDTH && sy >= 0 && sy < BUFFER_HEIGHT) {
            pixels[sy * BUFFER_WIDTH + sx] = sp.color;
            if (sx + 1 < BUFFER_WIDTH) pixels[sy * BUFFER_WIDTH + sx + 1] = sp.color;
            if (sy + 1 < BUFFER_HEIGHT) pixels[(sy + 1) * BUFFER_WIDTH + sx] = sp.color;
        }
    }

    // First-person kart hood at bottom center (with parabolic hop vertical bob)
    int hoodCenter = BUFFER_WIDTH / 2;
    int hoodTop = BUFFER_HEIGHT - 35 - static_cast<int>(driftHopHeight * 2.2);

    QRgb baseHoodColor = c.primaryColor.rgb();
    QRgb hoodColor = (mushroomBoostTimer > 0.0 || miniTurboTimer > 0.0) ? qRgb(255, 120, 25) : baseHoodColor;
    QRgb stripeColor = c.secondaryColor.rgb();

    for (int y = hoodTop; y < BUFFER_HEIGHT; ++y) {
        if (y < 0) continue;
        int halfWidth = static_cast<int>(30.0 + (y - hoodTop) * 2.2);
        for (int x = hoodCenter - halfWidth; x < hoodCenter + halfWidth; ++x) {
            if (x >= 0 && x < BUFFER_WIDTH && y < BUFFER_HEIGHT) {
                // Center racing stripe (4px width)
                if (std::abs(x - hoodCenter) <= 2) {
                    pixels[y * BUFFER_WIDTH + x] = stripeColor;
                } else {
                    pixels[y * BUFFER_WIDTH + x] = hoodColor;
                }
            }
        }
    }

    // Steering wheel (Rotated pixel ring with hop offset)
    int wheelY = BUFFER_HEIGHT - 22 - static_cast<int>(driftHopHeight * 2.2);
    int wheelX = BUFFER_WIDTH / 2 + static_cast<int>(steeringVisualAngle * 30.0);
    int wheelR = 14;

    QRgb wheelGripColor = qRgb(240, 240, 240); // Standard white
    if (c.id == 1) wheelGripColor = qRgb(80, 85, 90);       // Bowser gunmetal
    else if (c.id == 2) wheelGripColor = qRgb(255, 215, 50); // Peach gold grip

    for (int y = wheelY - wheelR; y <= wheelY + wheelR; ++y) {
        if (y < 0 || y >= BUFFER_HEIGHT) continue;
        for (int x = wheelX - wheelR; x <= wheelX + wheelR; ++x) {
            if (x < 0 || x >= BUFFER_WIDTH) continue;
            int dist2 = (x - wheelX) * (x - wheelX) + (y - wheelY) * (y - wheelY);
            if (dist2 >= (wheelR - 3) * (wheelR - 3) && dist2 <= wheelR * wheelR) {
                pixels[y * BUFFER_WIDTH + x] = wheelGripColor;
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

void GameCanvas::renderTitleMenu()
{
    QPainter painter(&screenBuffer);
    painter.setRenderHint(QPainter::Antialiasing, false);

    // 1. Midnight Sky & Ground Horizon
    QLinearGradient skyGrad(0, 0, 0, HORIZON_Y);
    skyGrad.setColorAt(0.0, QColor(6, 10, 24));
    skyGrad.setColorAt(1.0, QColor(22, 32, 56));
    painter.fillRect(QRect(0, 0, BUFFER_WIDTH, HORIZON_Y), skyGrad);

    QLinearGradient groundGrad(0, HORIZON_Y, 0, BUFFER_HEIGHT);
    groundGrad.setColorAt(0.0, QColor(16, 8, 28));
    groundGrad.setColorAt(1.0, QColor(36, 14, 48));
    painter.fillRect(QRect(0, HORIZON_Y, BUFFER_WIDTH, BUFFER_HEIGHT - HORIZON_Y), groundGrad);

    // Dynamic Perspective Grid on ground
    painter.setPen(QColor(0, 229, 255, 60)); // Cyan perspective grid
    for (int i = 0; i < 14; ++i) {
        int groundX = -100 + i * 46;
        painter.drawLine(BUFFER_WIDTH / 2, HORIZON_Y, groundX, BUFFER_HEIGHT);
    }
    // Animated horizontal scanlines scrolling down
    double gridScroll = std::fmod(globalTime * 45.0, 18.0);
    for (int y = HORIZON_Y; y < BUFFER_HEIGHT; y += 12) {
        int lineY = y + static_cast<int>(gridScroll);
        if (lineY > HORIZON_Y && lineY < BUFFER_HEIGHT) {
            int alpha = std::min(120, static_cast<int>((lineY - HORIZON_Y) * 0.9));
            painter.setPen(QColor(0, 229, 255, alpha));
            painter.drawLine(0, lineY, BUFFER_WIDTH, lineY);
        }
    }

    // Glowing Neon Horizon Divider
    painter.setPen(QPen(QColor(0, 229, 255), 2));
    painter.drawLine(0, HORIZON_Y, BUFFER_WIDTH, HORIZON_Y);

    // Animated Top Checkered Racing Banner
    int checkSize = 10;
    double bannerOffset = std::fmod(globalTime * 25.0, static_cast<double>(checkSize * 2));
    for (int x = -checkSize * 2; x < BUFFER_WIDTH + checkSize * 2; x += checkSize) {
        int bx = static_cast<int>(x + bannerOffset);
        bool even = ((x / checkSize) % 2 == 0);
        painter.fillRect(QRect(bx, 0, checkSize, 6), even ? QColor(255, 215, 0) : QColor(20, 20, 24));
        painter.fillRect(QRect(bx, 6, checkSize, 6), even ? QColor(20, 20, 24) : QColor(255, 215, 0));
    }

    // 2. Extruded 3D Title Banner
    QFont titleFont("Arial", 22, QFont::Black);
    painter.setFont(titleFont);
    // Black Shadow
    painter.setPen(QColor(10, 5, 20));
    painter.drawText(QRect(2, 28, BUFFER_WIDTH, 40), Qt::AlignCenter, "RETRO KART 3D");
    painter.drawText(QRect(1, 27, BUFFER_WIDTH, 40), Qt::AlignCenter, "RETRO KART 3D");
    // Crimson Rim
    painter.setPen(QColor(220, 30, 25));
    painter.drawText(QRect(1, 25, BUFFER_WIDTH, 40), Qt::AlignCenter, "RETRO KART 3D");
    // Golden Front Face
    painter.setPen(QColor(255, 225, 30));
    painter.drawText(QRect(0, 24, BUFFER_WIDTH, 40), Qt::AlignCenter, "RETRO KART 3D");

    // Subtitle
    QFont subFont("Arial", 7, QFont::Bold);
    painter.setFont(subFont);
    painter.setPen(QColor(0, 240, 255));
    painter.drawText(QRect(0, 68, BUFFER_WIDTH, 14), Qt::AlignCenter, "★ 16-BIT ARCADE GRAND PRIX • MODE-7 ★");

    // 3. Interactive Menu Box
    int boxX = 75;
    int boxY = 94;
    int boxW = 250;
    int boxH = 132;
    painter.setBrush(QColor(14, 18, 32, 235));
    painter.setPen(QPen(QColor(255, 215, 0), 2));
    painter.drawRoundedRect(QRect(boxX, boxY, boxW, boxH), 8, 8);

    // Inner subtle border
    painter.setPen(QPen(QColor(255, 255, 255, 40), 1));
    painter.drawRoundedRect(QRect(boxX + 3, boxY + 3, boxW - 6, boxH - 6), 6, 6);

    // Menu Title
    painter.setFont(QFont("Arial", 7, QFont::Bold));
    painter.setPen(QColor(255, 215, 0));
    painter.drawText(QRect(boxX, boxY + 8, boxW, 14), Qt::AlignCenter, "— SELECT GAME MODE —");

    // Menu Items
    const char* menuOptions[3] = {
        "1. PLAY VS AI (GRAND PRIX)",
        "2. MULTIPLAYER (2P)",
        "3. PRACTICE (TIME TRIAL)"
    };

    int itemStartY = boxY + 28;
    int itemSpacing = 32;
    int chevronOffset = static_cast<int>(3.0 * std::sin(globalTime * 8.0));

    for (int i = 0; i < 3; ++i) {
        int itemY = itemStartY + i * itemSpacing;
        bool isSelected = (menuIndex == i);

        if (isSelected) {
            // Glowing pulsing highlight bar
            int alpha = static_cast<int>(45 + 25 * std::sin(globalTime * 8.0));
            painter.fillRect(QRect(boxX + 12, itemY - 3, boxW - 24, 26), QColor(255, 215, 0, alpha));
            painter.setPen(QPen(QColor(255, 215, 0), 1));
            painter.drawRect(QRect(boxX + 12, itemY - 3, boxW - 24, 26));

            // Animated Chevrons
            painter.setFont(QFont("Arial", 8, QFont::Bold));
            painter.setPen(QColor(255, 225, 30));
            painter.drawText(QRect(boxX + 16 + chevronOffset, itemY + 2, 20, 16), Qt::AlignLeft | Qt::AlignVCenter, "▶▶");
            painter.drawText(QRect(boxX + boxW - 36 - chevronOffset, itemY + 2, 20, 16), Qt::AlignRight | Qt::AlignVCenter, "◀◀");

            // Text
            painter.setFont(QFont("Arial", 9, QFont::Bold));
            painter.setPen(Qt::white);
            painter.drawText(QRect(boxX + 38, itemY + 2, boxW - 76, 16), Qt::AlignCenter, menuOptions[i]);
        } else {
            // Unselected Item
            painter.setFont(QFont("Arial", 9, QFont::Bold));
            painter.setPen(QColor(155, 165, 185));
            painter.drawText(QRect(boxX + 20, itemY + 2, boxW - 40, 16), Qt::AlignCenter, menuOptions[i]);
        }
    }

    // 4. Footer Hint Bar
    QRect hintRect(35, 240, 330, 38);
    painter.setBrush(QColor(16, 20, 32, 220));
    painter.setPen(QPen(QColor(50, 60, 80), 1));
    painter.drawRoundedRect(hintRect, 6, 6);

    painter.setFont(QFont("Arial", 6, QFont::Bold));
    painter.setPen(QColor(255, 215, 0));
    painter.drawText(QRect(35, 244, 330, 14), Qt::AlignCenter, "▲/▼ or W/S: NAVIGATE  •  ENTER / SPACE: SELECT");
    painter.setPen(QColor(0, 230, 255));
    painter.drawText(QRect(35, 258, 330, 14), Qt::AlignCenter, "PRESS 1, 2, 3 FOR DIRECT SELECTION");

    // Credits
    painter.setFont(QFont("Arial", 5));
    painter.setPen(QColor(110, 120, 145));
    painter.drawText(QRect(0, 284, BUFFER_WIDTH, 12), Qt::AlignCenter, "RETROKART 3D ENGINE • 60 FPS • SCANLINE MODE-7");
}

void GameCanvas::renderCharacterSelect()
{
    QPainter painter(&screenBuffer);
    painter.setRenderHint(QPainter::Antialiasing, false);

    // 1. Background
    painter.fillRect(QRect(0, 0, BUFFER_WIDTH, BUFFER_HEIGHT), QColor(16, 20, 32));

    // Top & Bottom Accent Lines
    painter.setPen(QPen(QColor(255, 215, 0), 2));
    painter.drawLine(0, 44, BUFFER_WIDTH, 44);
    painter.drawLine(0, 226, BUFFER_WIDTH, 226);

    // 2. Title & Mode Banner
    QFont titleFont("Arial", 11, QFont::Bold);
    painter.setFont(titleFont);
    painter.setPen(QColor(0, 0, 0, 180));
    painter.drawText(QRect(1, 7, BUFFER_WIDTH, 18), Qt::AlignCenter, "SELECT YOUR RACER");
    painter.setPen(QColor(255, 215, 0));
    painter.drawText(QRect(0, 6, BUFFER_WIDTH, 18), Qt::AlignCenter, "SELECT YOUR RACER");

    // Mode subtitle
    QString modeStr;
    QColor modeColor;
    if (selectedGameMode == GameMode::PLAY_VS_AI) {
        modeStr = "MODE: PLAY VS AI (GRAND PRIX)";
        modeColor = QColor(255, 200, 50);
    } else if (selectedGameMode == GameMode::MULTIPLAYER) {
        modeStr = (charSelectPhase == 0) ? "MODE: 2P MULTIPLAYER — PLAYER 1 SELECT" : "MODE: 2P MULTIPLAYER — PLAYER 2 SELECT";
        modeColor = (charSelectPhase == 0) ? QColor(255, 215, 0) : QColor(0, 230, 255);
    } else {
        modeStr = "MODE: PRACTICE (SOLO TIME TRIAL)";
        modeColor = QColor(80, 240, 120);
    }

    QFont subFont("Arial", 6, QFont::Bold);
    painter.setFont(subFont);
    painter.setPen(modeColor);
    painter.drawText(QRect(0, 22, BUFFER_WIDTH, 11), Qt::AlignCenter, modeStr);

    QString trackName = (currentLevel == TrackLevel::DONUT_PLAINS) ? "DONUT PLAINS" :
                        (currentLevel == TrackLevel::CHOCO_VALLEY) ? "CHOCO VALLEY" : "BOWSER'S CASTLE";
    painter.setPen(QColor(160, 180, 210));
    painter.drawText(QRect(0, 32, BUFFER_WIDTH, 11), Qt::AlignCenter, QString("COURSE: %1  (KEYS 1-3 TO CHANGE)").arg(trackName));

    const auto &atlas = SpriteAtlas::instance();
    double t = globalTime;

    // 3. Render 4 Character Cards
    int cardY = 48;
    int cardW = 88;
    int cardH = 174;
    int gap = 8;
    int startX = (BUFFER_WIDTH - (4 * cardW + 3 * gap)) / 2; // 12 px

    for (int i = 0; i < 4; ++i) {
        int cx = startX + i * (cardW + gap);
        QRect cardRect(cx, cardY, cardW, cardH);
        const auto &c = characters[i];

        bool isP1Here = (selectedCharP1 == i);
        bool isP2Here = (selectedGameMode == GameMode::MULTIPLAYER && selectedCharP2 == i && charSelectPhase == 1);
        bool isP1Locked = (selectedGameMode == GameMode::MULTIPLAYER && charSelectPhase == 1 && selectedCharP1 == i);

        // Card Background Fill
        painter.fillRect(cardRect, QColor(22, 26, 40, 235));

        // Border Styling
        if (isP1Here && isP2Here) {
            // Both P1 and P2 on same card
            painter.setPen(QPen(QColor(255, 215, 0), 2));
            painter.drawRect(cardRect);
            painter.setPen(QPen(QColor(0, 225, 255), 1));
            painter.drawRect(cardRect.adjusted(2, 2, -2, -2));
        } else if (isP1Here && !isP1Locked) {
            int glow = static_cast<int>(200 + 55 * std::sin(t * 10.0));
            painter.setPen(QPen(QColor(255, 215, 0, glow), 2));
            painter.drawRect(cardRect);
        } else if (isP2Here) {
            int glow = static_cast<int>(200 + 55 * std::sin(t * 10.0));
            painter.setPen(QPen(QColor(0, 225, 255, glow), 2));
            painter.drawRect(cardRect);
        } else if (isP1Locked) {
            painter.setPen(QPen(QColor(40, 200, 70), 2));
            painter.drawRect(cardRect);
        } else {
            painter.setPen(QPen(QColor(60, 68, 85), 1));
            painter.drawRect(cardRect);
        }

        // Card Header Banner (Character Name)
        QRect headerRect(cx + 1, cardY + 1, cardW - 2, 17);
        painter.fillRect(headerRect, c.primaryColor);
        painter.setFont(QFont("Arial", 7, QFont::Bold));
        painter.setPen(Qt::white);
        painter.drawText(headerRect, Qt::AlignCenter, c.name);

        // Kart Preview Box
        QRect previewRect(cx + 6, cardY + 20, cardW - 12, 46);
        painter.fillRect(previewRect, QColor(14, 16, 26));
        painter.setPen(QColor(40, 46, 62));
        painter.drawRect(previewRect);

        // Ground shadow under kart
        int bobY = static_cast<int>(std::sin(t * 8.0 + i * 1.5) * 1.8);
        painter.setBrush(QColor(0, 0, 0, 120));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(cx + 24, cardY + 54, 40, 7);

        // Billboard Kart Sprite
        SpriteType st;
        if (i == 0) st = (isP2Here && selectedCharP1 == 0) ? SpriteType::KART_MARIO_ALT : SpriteType::KART_MARIO;
        else if (i == 1) st = (isP2Here && selectedCharP1 == 1) ? SpriteType::KART_BOWSER_ALT : SpriteType::KART_BOWSER;
        else if (i == 2) st = (isP2Here && selectedCharP1 == 2) ? SpriteType::KART_PEACH_ALT : SpriteType::KART_PEACH;
        else st = (isP2Here && selectedCharP1 == 3) ? SpriteType::KART_TOAD_ALT : SpriteType::KART_TOAD;

        const QImage &kartImg = atlas.getSprite(st);
        painter.drawImage(QRect(cx + 25, cardY + 23 + bobY, 38, 38), kartImg);

        // Archetype Subtitle Badge
        QRect badgeRect(cx + 6, cardY + 68, cardW - 12, 12);
        painter.setFont(QFont("Arial", 5, QFont::Bold));
        painter.setPen(c.secondaryColor);
        painter.drawText(badgeRect, Qt::AlignCenter, c.archetype);

        // Stat Bars Helper
        auto drawPipBar = [&](int y, const QString &label, int pips, QColor barCol) {
            painter.setFont(QFont("Arial", 5, QFont::Bold));
            painter.setPen(QColor(180, 190, 205));
            painter.drawText(QRect(cx + 6, y, 22, 10), Qt::AlignLeft | Qt::AlignVCenter, label);

            int bx = cx + 30;
            int pipW = 8;
            int pipH = 5;
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

        drawPipBar(cardY + 83,  "SPD", c.speedPips,    QColor(255, 70, 50));
        drawPipBar(cardY + 98,  "ACC", c.accelPips,    QColor(255, 205, 30));
        drawPipBar(cardY + 113, "HND", c.handlingPips, QColor(40, 200, 255));
        drawPipBar(cardY + 128, "WGT", c.weightPips,   QColor(190, 110, 255));

        // Ready or Cursor Badges
        if (isP1Locked) {
            QRect readyRect(cx + 4, cardY + 148, cardW - 8, 16);
            painter.fillRect(readyRect, QColor(30, 180, 60, 230));
            painter.setFont(QFont("Arial", 6, QFont::Bold));
            painter.setPen(Qt::white);
            painter.drawText(readyRect, Qt::AlignCenter, "✓ P1 READY");
        }

        // P1 & P2 Badges above card
        if (isP1Here && !isP1Locked) {
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
    int focusIndex = (selectedGameMode == GameMode::MULTIPLAYER && charSelectPhase == 1) ? selectedCharP2 : selectedCharP1;
    const auto &focusChar = characters[focusIndex];
    QRect descRect(12, 230, BUFFER_WIDTH - 24, 32);
    painter.fillRect(descRect, QColor(20, 24, 38, 220));
    painter.setPen(QColor(50, 60, 80));
    painter.drawRect(descRect);

    painter.setFont(QFont("Arial", 6, QFont::Bold));
    painter.setPen(focusChar.secondaryColor);
    painter.drawText(QRect(18, 232, BUFFER_WIDTH - 36, 12), Qt::AlignLeft, "★ " + focusChar.name + " (" + focusChar.archetype + ")");
    painter.setFont(QFont("Arial", 6));
    painter.setPen(QColor(220, 225, 235));
    painter.drawText(QRect(18, 245, BUFFER_WIDTH - 36, 14), Qt::AlignLeft, focusChar.description);

    // 5. Footer Key Legend
    QFont legFont("Arial", 6);
    painter.setFont(legFont);
    painter.setPen(QColor(160, 175, 195));
    if (selectedGameMode == GameMode::MULTIPLAYER) {
        if (charSelectPhase == 0) {
            painter.drawText(QRect(0, 270, BUFFER_WIDTH, 14), Qt::AlignCenter,
                "P1: [◄ / ► or A / D] Choose    [Space / Enter] Confirm    [Esc] Back to Menu");
        } else {
            painter.drawText(QRect(0, 270, BUFFER_WIDTH, 14), Qt::AlignCenter,
                "P2: [◄ / ► or A / D] Choose    [Space / Enter] Start Race    [Esc] Back to P1");
        }
    } else {
        painter.drawText(QRect(0, 270, BUFFER_WIDTH, 14), Qt::AlignCenter,
            "[◄ / ► or A / D] Select Racer    [Space / Enter] Start Race    [Esc] Back to Menu");
    }
}

void GameCanvas::renderRaceFinishedOverlay()
{
    QPainter painter(&screenBuffer);
    painter.setRenderHint(QPainter::Antialiasing, false);

    QRect bannerRect(40, 105, BUFFER_WIDTH - 80, 80);
    painter.fillRect(bannerRect, QColor(14, 18, 30, 235));
    painter.setPen(QPen(QColor(255, 215, 0), 2));
    painter.drawRect(bannerRect);

    painter.setFont(QFont("Arial", 12, QFont::Bold));
    painter.setPen(QColor(255, 225, 30));
    painter.drawText(QRect(40, 115, BUFFER_WIDTH - 80, 22), Qt::AlignCenter, "🏁 COURSE FINISHED! 🏁");

    painter.setFont(QFont("Arial", 7, QFont::Bold));
    painter.setPen(Qt::white);
    painter.drawText(QRect(40, 142, BUFFER_WIDTH - 80, 16), Qt::AlignCenter, "PRESS [R] TO RESTART RACE");
    painter.setPen(QColor(0, 225, 255));
    painter.drawText(QRect(40, 158, BUFFER_WIDTH - 80, 16), Qt::AlignCenter, "PRESS [ENTER] OR [ESC] FOR MAIN MENU");
}

// -------------------------------------------------------------
// DRIFT SPARKS & MINI-TURBO (Milestone 3 F9)
// -------------------------------------------------------------

void GameCanvas::updateDriftSparks(double dt)
{
    // Update existing sparks
    for (auto it = driftSparks.begin(); it != driftSparks.end();) {
        it->life -= dt;
        it->x += it->vx * dt;
        it->y += it->vy * dt;
        if (it->life <= 0.0) {
            it = driftSparks.erase(it);
        } else {
            ++it;
        }
    }

    // Spawn new sparks if drift timer >= 0.5s
    if (driftTimer >= 0.5 && driftSparks.size() < 30) {
        bool isCharged = (driftTimer >= 1.0);
        int emitSideX = (driftDirection > 0) ? 75 : 325; // Outer rear wheel

        for (int i = 0; i < 3; ++i) {
            SparkParticle p;
            p.x = emitSideX + (rand() % 24 - 12);
            p.y = BUFFER_HEIGHT - 30 + (rand() % 16 - 8);
            p.vx = -driftDirection * (20.0 + (rand() % 50));
            p.vy = -(30.0 + (rand() % 60));
            p.life = 0.18 + (rand() % 10) * 0.01;
            p.maxLife = p.life;

            if (isCharged) {
                // Fiery orange / golden yellow
                p.color = (rand() % 2 == 0) ? qRgb(255, 140, 20) : qRgb(255, 230, 40);
            } else {
                // Electric blue / cyan white
                p.color = (rand() % 2 == 0) ? qRgb(60, 180, 255) : qRgb(160, 230, 255);
            }
            driftSparks.push_back(p);
        }
    }
}

// -------------------------------------------------------------
// TRACK HAZARDS SYSTEM (Milestone 3 F10, F11)
// -------------------------------------------------------------

void GameCanvas::updateTrackHazards(double dt)
{
    dt = std::min(dt, 0.05);

    for (auto &hazard : trackHazards) {
        if (hazard.type == HazardType::LAVA_GEYSER) {
            // Eruption cycle: Dormant (0..2.5s) -> Warning (2.5..3.3s) -> Spewing Active (3.3..4.8s)
            hazard.animTimer += dt;
            if (hazard.animTimer >= 4.8) {
                hazard.animTimer -= 4.8;
            }
            hazard.isErupting = (hazard.animTimer >= 3.3);

            if (hazard.isErupting) {
                // Check Player 1
                double distP1 = std::hypot(kartX - hazard.x, kartZ - hazard.z);
                if (distP1 < hazard.radius) {
                    double pushX = (distP1 > 0.001) ? ((kartX - hazard.x) / distP1) : 1.0;
                    double pushZ = (distP1 > 0.001) ? ((kartZ - hazard.z) / distP1) : 0.0;
                    isSpinning = true;
                    spinTimer = 0.8;
                    kartSpeed *= 0.25; // 75% speed loss
                    kartX += pushX * 15.0;
                    kartZ += pushZ * 15.0;
                    flashIntensity = 0.6;
                }

                // Check Player 2 in Multiplayer
                if (selectedGameMode == GameMode::MULTIPLAYER) {
                    double distP2 = std::hypot(player2.x - hazard.x, player2.z - hazard.z);
                    if (distP2 < hazard.radius) {
                        double pushX = (distP2 > 0.001) ? ((player2.x - hazard.x) / distP2) : 1.0;
                        double pushZ = (distP2 > 0.001) ? ((player2.z - hazard.z) / distP2) : 0.0;
                        player2.isSpinning = true;
                        player2.spinTimer = 0.8;
                        player2.speed *= 0.25;
                        player2.x += pushX * 15.0;
                        player2.z += pushZ * 15.0;
                    }
                }

                // Check AI Opponents
                for (auto &ai : aiOpponents) {
                    double distAI = std::hypot(ai.x - hazard.x, ai.z - hazard.z);
                    if (distAI < hazard.radius) {
                        double pushX = (distAI > 0.001) ? ((ai.x - hazard.x) / distAI) : 1.0;
                        double pushZ = (distAI > 0.001) ? ((ai.z - hazard.z) / distAI) : 0.0;
                        ai.isSpinning = true;
                        ai.spinTimer = 0.8;
                        ai.speed *= 0.25;
                        ai.x += pushX * 15.0;
                        ai.z += pushZ * 15.0;
                    }
                }
            }
        } else if (hazard.type == HazardType::CHOCOLATE_FOUNTAIN) {
            hazard.animTimer += dt;

            // Player 1: mud pool cuts speed by 50%
            double distP1 = std::hypot(kartX - hazard.x, kartZ - hazard.z);
            if (distP1 < hazard.radius) {
                if (kartSpeed > maxSpeed * 0.50) {
                    kartSpeed = maxSpeed * 0.50;
                }
            }

            // Player 2
            if (selectedGameMode == GameMode::MULTIPLAYER) {
                double distP2 = std::hypot(player2.x - hazard.x, player2.z - hazard.z);
                if (distP2 < hazard.radius) {
                    if (player2.speed > player2.maxSpeed * 0.50) {
                        player2.speed = player2.maxSpeed * 0.50;
                    }
                }
            }

            // AI Opponents
            for (auto &ai : aiOpponents) {
                double distAI = std::hypot(ai.x - hazard.x, ai.z - hazard.z);
                if (distAI < hazard.radius) {
                    if (ai.speed > ai.maxSpeed * 0.50) {
                        ai.speed = ai.maxSpeed * 0.50;
                    }
                }
            }
        } else if (hazard.type == HazardType::ROLLING_CANDY) {
            hazard.animTimer += dt;
            double offset = 40.0 * std::sin(hazard.animTimer * 2.0);
            double currentCandyX = hazard.x + offset;

            // Player 1: Bumper bounce recoil (-35.0 u/s)
            double distP1 = std::hypot(kartX - currentCandyX, kartZ - hazard.z);
            if (distP1 < hazard.radius) {
                kartSpeed = -35.0;
            }

            // Player 2
            if (selectedGameMode == GameMode::MULTIPLAYER) {
                double distP2 = std::hypot(player2.x - currentCandyX, player2.z - hazard.z);
                if (distP2 < hazard.radius) {
                    player2.speed = -35.0;
                }
            }

            // AI Opponents
            for (auto &ai : aiOpponents) {
                double distAI = std::hypot(ai.x - currentCandyX, ai.z - hazard.z);
                if (distAI < hazard.radius) {
                    ai.speed = -35.0;
                }
            }
        }
    }
}

// -------------------------------------------------------------
// TOP-LEFT HUD MINIMAP (Milestone 3 F12)
// -------------------------------------------------------------

void GameCanvas::updateMinimapTrackCache()
{
    minimapTrackCache = QImage(MinimapHUD::RADAR_SIZE, MinimapHUD::RADAR_SIZE, QImage::Format_ARGB32_Premultiplied);
    minimapTrackCache.fill(Qt::transparent);

    if (currentTrackWaypoints.size() < 3) return;

    QPainter p(&minimapTrackCache);
    p.setRenderHint(QPainter::Antialiasing, true);

    const double scale = static_cast<double>(MinimapHUD::RADAR_SIZE) / static_cast<double>(TRACK_MAP_SIZE);

    QPainterPath path;
    for (size_t i = 0; i < currentTrackWaypoints.size(); ++i) {
        double rx = currentTrackWaypoints[i].x() * scale;
        double ry = currentTrackWaypoints[i].y() * scale;
        if (i == 0) path.moveTo(rx, ry);
        else path.lineTo(rx, ry);
    }
    path.closeSubpath();

    // 1. Dark outer casing for contrast against background
    p.setPen(QPen(QColor(30, 35, 50, 220), 7.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawPath(path);

    // 2. High-visibility road surface ribbon
    p.setPen(QPen(QColor(220, 225, 235, 240), 4.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawPath(path);

    // 3. Gold start/finish line indicator tick at waypoint 0
    double startX = currentTrackWaypoints[0].x() * scale;
    double startY = currentTrackWaypoints[0].y() * scale;
    p.setPen(QPen(QColor(255, 215, 0, 255), 2.5));
    p.drawLine(QPointF(startX, startY - 3.5), QPointF(startX, startY + 3.5));
}

void GameCanvas::renderMinimap()
{
    QPainter painter(&screenBuffer);

    const int rx = MinimapHUD::RADAR_X;
    const int ry = MinimapHUD::RADAR_Y;
    const int size = MinimapHUD::RADAR_SIZE;
    const QRect radarRect(rx, ry, size, size);

    // 1. Strict clipping to radar bounds (satisfies F12.5)
    painter.setClipRect(radarRect);

    // 2. Semi-transparent dark background
    painter.fillRect(radarRect, QColor(0, 0, 0, 160));

    // 3. Subtle radar crosshair grid
    painter.setPen(QPen(QColor(255, 255, 255, 25), 1, Qt::DotLine));
    painter.drawLine(rx + size / 2, ry, rx + size / 2, ry + size);
    painter.drawLine(rx, ry + size / 2, rx + size, ry + size / 2);

    // 4. Pre-rendered track outline
    if (!minimapTrackCache.isNull()) {
        painter.drawImage(rx, ry, minimapTrackCache);
    }

    // 5. AI Opponent Blips (Rendered beneath Player 1)
    if (selectedGameMode == GameMode::PLAY_VS_AI) {
        for (const auto &ai : aiOpponents) {
            QPointF blipPos = MinimapHUD::worldToRadar(ai.x, ai.z);

            painter.setPen(QPen(QColor(10, 10, 15, 220), 1.0));
            painter.setBrush(ai.kartColor);
            painter.drawEllipse(blipPos, 2.2, 2.2);
        }
    } else if (selectedGameMode == GameMode::MULTIPLAYER) {
        QPointF p2Pos = MinimapHUD::worldToRadar(player2.x, player2.z);
        painter.setPen(QPen(QColor(255, 255, 255, 240), 1.0));
        painter.setBrush(player2.kartColor);
        painter.drawEllipse(p2Pos, 2.5, 2.5);
    }

    // 6. Player 1 Blip (Pulsing White / Neon Yellow Dot)
    {
        QPointF p1Pos = MinimapHUD::worldToRadar(kartX, kartZ);

        double pulse = 0.5 + 0.5 * std::sin(globalTime * 8.0);
        double outerRadius = 3.2 + pulse * 1.0;

        // Outer pulsing yellow aura
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 235, 50, static_cast<int>(120 + 100 * pulse)));
        painter.drawEllipse(p1Pos, outerRadius, outerRadius);

        // Pure white inner core with dark contour
        painter.setPen(QPen(QColor(40, 40, 40, 180), 0.8));
        painter.setBrush(QColor(255, 255, 255, 255));
        painter.drawEllipse(p1Pos, 2.2, 2.2);
    }

    // 7. Reset clipping for crisp outer retro frame
    painter.setClipping(false);

    // Outer gunmetal border
    painter.setPen(QPen(QColor(70, 75, 95, 230), 1.5));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(radarRect);

    // Inner highlight bevel (1px inset)
    painter.setPen(QPen(QColor(140, 150, 175, 100), 1.0));
    painter.drawRect(radarRect.adjusted(1, 1, -1, -1));

    // Retro corner accent brackets
    painter.setPen(QPen(QColor(255, 215, 0, 190), 1.0));
    // Top-left
    painter.drawLine(rx, ry, rx + 4, ry);
    painter.drawLine(rx, ry, rx, ry + 4);
    // Top-right
    painter.drawLine(rx + size, ry, rx + size - 4, ry);
    painter.drawLine(rx + size, ry, rx + size - 4, ry);
    // Bottom-left
    painter.drawLine(rx, ry + size, rx + 4, ry + size);
    painter.drawLine(rx, ry + size, rx, ry + size - 4);
    // Bottom-right
    painter.drawLine(rx + size, ry + size, rx + size - 4, ry + size);
    painter.drawLine(rx + size, ry + size, rx + size, ry + size - 4);
}

