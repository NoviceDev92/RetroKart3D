#include <QApplication>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QTimer>
#include <QElapsedTimer>
#include <QTest>
#include <iostream>
#include <iomanip>
#include <vector>
#include <random>
#include <cmath>
#include <cassert>
#include <climits>

#include "game_canvas.h"
#include "sprite_atlas.h"

// Subclass to expose protected event handlers for precise adversarial testing
class TestableGameCanvas : public GameCanvas {
public:
    explicit TestableGameCanvas(QWidget *parent = nullptr) : GameCanvas(parent) {}

    using GameCanvas::keyPressEvent;
    using GameCanvas::keyReleaseEvent;
    using GameCanvas::focusOutEvent;
    using GameCanvas::focusInEvent;

    // Helper to trigger slot updateGameLoop
    void triggerUpdateLoop() {
        QMetaObject::invokeMethod(this, "updateGameLoop");
    }

    // Direct access to inspect internal state through signals
    double lastSpeed = 0.0;
    int lastLap = 0;
    double lastX = 0.0;
    double lastZ = 0.0;
    double lastAngleDeg = 0.0;
    QString lastSurface;

    void hookSignals() {
        connect(this, &GameCanvas::statsUpdated, this, [this](double s, int l, double x, double z, double a, QString surf) {
            lastSpeed = s;
            lastLap = l;
            lastX = x;
            lastZ = z;
            lastAngleDeg = a;
            lastSurface = surf;
        });
    }
};

struct TestStats {
    int totalTests = 0;
    int passedTests = 0;
    int failedTests = 0;
    std::vector<std::string> failureMessages;

    void recordPass(const std::string &name) {
        totalTests++;
        passedTests++;
        std::cout << "  [PASS] " << name << "\n";
    }

    void recordFail(const std::string &name, const std::string &msg) {
        totalTests++;
        failedTests++;
        failureMessages.push_back(name + ": " + msg);
        std::cout << "  [FAIL] " << name << " -> " << msg << "\n";
    }
};

static TestStats g_stats;

#define CHALLENGE_ASSERT(cond, name, msg) \
    do { \
        if (cond) { \
            g_stats.recordPass(name); \
        } else { \
            g_stats.recordFail(name, msg); \
        } \
    } while (0)

// Helper to simulate key press and release
static void sendKey(TestableGameCanvas &canvas, int key, bool press = true) {
    QKeyEvent ev(press ? QEvent::KeyPress : QEvent::KeyRelease, key, Qt::NoModifier);
    if (press) {
        canvas.keyPressEvent(&ev);
    } else {
        canvas.keyReleaseEvent(&ev);
    }
    QApplication::processEvents();
}

static void sendKeyClick(TestableGameCanvas &canvas, int key) {
    sendKey(canvas, key, true);
    sendKey(canvas, key, false);
}

// ============================================================================
// SUITE 1: RAPID MENU STATE TRANSITIONS STRESS TEST
// ============================================================================
void testRapidMenuTransitions(TestableGameCanvas &canvas) {
    std::cout << "\n--- SUITE 1: Rapid Menu State Transitions Stress Test ---\n";

    // 1.1 Canonical Transition Loop
    canvas.setGameState(GameState::TITLE_MENU);
    CHALLENGE_ASSERT(canvas.getGameState() == GameState::TITLE_MENU,
                     "T1.1: State is TITLE_MENU on init", "Expected TITLE_MENU");

    sendKeyClick(canvas, Qt::Key_1); // Select Play vs AI -> CHARACTER_SELECT
    CHALLENGE_ASSERT(canvas.getGameState() == GameState::CHARACTER_SELECT,
                     "T1.2: Menu option 1 transitions TITLE_MENU -> CHARACTER_SELECT", "Expected CHARACTER_SELECT");

    sendKeyClick(canvas, Qt::Key_Return);
    CHALLENGE_ASSERT(canvas.getGameState() == GameState::RACING,
                     "T1.3: Enter key transitions CHARACTER_SELECT -> RACING", "Expected RACING");

    sendKeyClick(canvas, Qt::Key_Escape);
    CHALLENGE_ASSERT(canvas.getGameState() == GameState::TITLE_MENU,
                     "T1.4: Escape key transitions RACING -> TITLE_MENU", "Expected TITLE_MENU");

    // 1.2 Massive Rapid Transition Stress Test (2,000 cycles)
    bool rapidSuccess = true;
    for (int i = 0; i < 2000; ++i) {
        sendKeyClick(canvas, Qt::Key_1); // TITLE -> CHAR_SELECT
        if (canvas.getGameState() != GameState::CHARACTER_SELECT) { rapidSuccess = false; break; }
        sendKeyClick(canvas, Qt::Key_Return); // CHAR_SELECT -> RACING
        if (canvas.getGameState() != GameState::RACING) { rapidSuccess = false; break; }
        sendKeyClick(canvas, Qt::Key_Escape); // RACING -> TITLE
        if (canvas.getGameState() != GameState::TITLE_MENU) { rapidSuccess = false; break; }
    }
    CHALLENGE_ASSERT(rapidSuccess,
                     "T1.5: 2,000 rapid cycles (TITLE -> CHAR_SELECT -> RACING -> TITLE) without state corruption",
                     "State cycle failed during iteration");

    // 1.3 Fuzzing Key Storm (5,000 random input events)
    std::mt19937 rng(1337);
    std::vector<int> candidateKeys = {
        Qt::Key_Up, Qt::Key_Down, Qt::Key_Left, Qt::Key_Right,
        Qt::Key_W, Qt::Key_S, Qt::Key_A, Qt::Key_D,
        Qt::Key_Return, Qt::Key_Enter, Qt::Key_Space, Qt::Key_Escape, Qt::Key_Backspace,
        Qt::Key_1, Qt::Key_2, Qt::Key_3, Qt::Key_R, Qt::Key_Shift, Qt::Key_Control,
        Qt::Key_X, Qt::Key_Z, Qt::Key_Tab
    };
    std::uniform_int_distribution<int> distKey(0, candidateKeys.size() - 1);
    std::uniform_int_distribution<int> distAction(0, 2); // 0=press, 1=release, 2=click

    bool fuzzNoCrash = true;
    for (int i = 0; i < 5000; ++i) {
        int k = candidateKeys[distKey(rng)];
        int action = distAction(rng);
        if (action == 0) sendKey(canvas, k, true);
        else if (action == 1) sendKey(canvas, k, false);
        else sendKeyClick(canvas, k);

        if (i % 20 == 0) {
            canvas.triggerUpdateLoop();
        }

        GameState s = canvas.getGameState();
        if (s != GameState::TITLE_MENU && s != GameState::CHARACTER_SELECT &&
            s != GameState::RACING && s != GameState::RACE_FINISHED) {
            fuzzNoCrash = false;
            break;
        }
    }
    CHALLENGE_ASSERT(fuzzNoCrash,
                     "T1.6: 5,000-event random fuzzing key storm survived with strictly valid state invariants",
                     "Invalid state encountered during fuzzing");

    // 1.4 State Transitions Matrix
    std::vector<GameState> allStates = {
        GameState::TITLE_MENU, GameState::CHARACTER_SELECT,
        GameState::RACING, GameState::RACE_FINISHED
    };
    bool matrixSuccess = true;
    for (GameState from : allStates) {
        for (GameState to : allStates) {
            canvas.setGameState(from);
            canvas.setGameState(to);
            if (canvas.getGameState() != to) {
                matrixSuccess = false;
            }
        }
    }
    CHALLENGE_ASSERT(matrixSuccess,
                     "T1.7: Direct setGameState() transition matrix (all 16 pairwise state changes) valid",
                     "State matrix transition failure");
}

// ============================================================================
// SUITE 2: CHARACTER INDEX BOUNDARIES & WRAP-AROUND STRESS TEST
// ============================================================================
void testCharacterIndexBoundaries(TestableGameCanvas &canvas) {
    std::cout << "\n--- SUITE 2: Character Index Boundaries & Wrap-Around Stress Test ---\n";

    // 2.1 Boundary setter values
    std::vector<int> testIndices = {
        INT_MIN, -1000000, -5, -4, -3, -2, -1,
        0, 1, 2, 3,
        4, 5, 6, 7, 8, 1000000, INT_MAX
    };

    bool p1BoundsSafe = true;
    bool p2BoundsSafe = true;

    for (int idx : testIndices) {
        canvas.setSelectedCharIndexP1(idx);
        int clampedP1 = canvas.getSelectedCharIndexP1();
        if (clampedP1 < 0 || clampedP1 >= 4) {
            p1BoundsSafe = false;
            break;
        }
        const CharacterDef &defP1 = canvas.getSelectedCharacterP1();
        if (defP1.id != clampedP1) {
            p1BoundsSafe = false;
            break;
        }

        canvas.setSelectedCharIndexP2(idx);
        int clampedP2 = canvas.getSelectedCharIndexP2();
        if (clampedP2 < 0 || clampedP2 >= 4) {
            p2BoundsSafe = false;
            break;
        }
        const CharacterDef &defP2 = canvas.getSelectedCharacterP2();
        if (defP2.id != clampedP2) {
            p2BoundsSafe = false;
            break;
        }
    }
    CHALLENGE_ASSERT(p1BoundsSafe && p2BoundsSafe,
                     "T2.1: Extreme index inputs (INT_MIN to INT_MAX) clamped safely to [0, 3]",
                     "Index out-of-bounds occurred");

    // 2.2 Exact Modulo Wrap Assertions
    canvas.setSelectedCharIndexP1(-1);
    CHALLENGE_ASSERT(canvas.getSelectedCharIndexP1() == 3 && canvas.getSelectedCharacterP1().name == "TOAD",
                     "T2.2: setSelectedCharIndexP1(-1) wraps to 3 (TOAD)", "Expected 3 (TOAD)");

    canvas.setSelectedCharIndexP1(-4);
    CHALLENGE_ASSERT(canvas.getSelectedCharIndexP1() == 0 && canvas.getSelectedCharacterP1().name == "MARIO",
                     "T2.3: setSelectedCharIndexP1(-4) wraps to 0 (MARIO)", "Expected 0 (MARIO)");

    canvas.setSelectedCharIndexP1(4);
    CHALLENGE_ASSERT(canvas.getSelectedCharIndexP1() == 0 && canvas.getSelectedCharacterP1().name == "MARIO",
                     "T2.4: setSelectedCharIndexP1(4) wraps to 0 (MARIO)", "Expected 0 (MARIO)");

    canvas.setSelectedCharIndexP1(7);
    CHALLENGE_ASSERT(canvas.getSelectedCharIndexP1() == 3 && canvas.getSelectedCharacterP1().name == "TOAD",
                     "T2.5: setSelectedCharIndexP1(7) wraps to 3 (TOAD)", "Expected 3 (TOAD)");

    // 2.3 Keyboard Navigation Wrap-Around in CHARACTER_SELECT
    canvas.setGameState(GameState::TITLE_MENU);
    sendKeyClick(canvas, Qt::Key_1); // Enters CHARACTER_SELECT with PLAY_VS_AI
    CHALLENGE_ASSERT(canvas.getGameState() == GameState::CHARACTER_SELECT,
                     "T2.6a: Entered CHARACTER_SELECT cleanly", "Expected CHARACTER_SELECT");

    canvas.setSelectedCharIndexP1(0); // MARIO

    // Press Left -> should wrap to 3 (TOAD)
    sendKeyClick(canvas, Qt::Key_Left);
    CHALLENGE_ASSERT(canvas.getSelectedCharIndexP1() == 3,
                     "T2.6b: Left arrow from Mario (0) wraps to Toad (3)", "Expected 3");

    // Press Right -> should wrap to 0 (MARIO)
    sendKeyClick(canvas, Qt::Key_Right);
    CHALLENGE_ASSERT(canvas.getSelectedCharIndexP1() == 0,
                     "T2.7: Right arrow from Toad (3) wraps to Mario (0)", "Expected 0");

    // Press 'A' -> wraps to 3 (TOAD)
    sendKeyClick(canvas, Qt::Key_A);
    CHALLENGE_ASSERT(canvas.getSelectedCharIndexP1() == 3,
                     "T2.8: 'A' key from Mario (0) wraps to Toad (3)", "Expected 3");

    // Press 'D' -> wraps to 0 (MARIO)
    sendKeyClick(canvas, Qt::Key_D);
    CHALLENGE_ASSERT(canvas.getSelectedCharIndexP1() == 0,
                     "T2.9: 'D' key from Toad (3) wraps to Mario (0)", "Expected 0");

    // Hammer Left 10,000 times
    for (int i = 0; i < 10000; ++i) {
        sendKeyClick(canvas, Qt::Key_Left);
    }
    CHALLENGE_ASSERT(canvas.getSelectedCharIndexP1() >= 0 && canvas.getSelectedCharIndexP1() < 4,
                     "T2.10: 10,000 Left-arrow navigation pulses stay strictly within [0, 3]",
                     "Index drifted out of bounds");

    // 2.4 Multiplayer 2P Selection Flow and Back Action
    canvas.setGameState(GameState::TITLE_MENU);
    // In Title Menu, press '2' to select MULTIPLAYER mode
    sendKeyClick(canvas, Qt::Key_2);
    CHALLENGE_ASSERT(canvas.getGameState() == GameState::CHARACTER_SELECT &&
                     canvas.getGameMode() == GameMode::MULTIPLAYER,
                     "T2.11: Menu '2' key entered MULTIPLAYER CHARACTER_SELECT", "Expected MULTIPLAYER");

    canvas.setSelectedCharIndexP1(0); // P1 picks Mario
    sendKeyClick(canvas, Qt::Key_Return); // Lock P1, transition to P2 select phase (charSelectPhase = 1)

    // Verify P2 defaults to (P1+1)%4 if equal, or valid
    CHALLENGE_ASSERT(canvas.getSelectedCharIndexP2() != canvas.getSelectedCharIndexP1() || canvas.getSelectedCharIndexP2() == 1,
                     "T2.12: P2 selection active and auto-assigned distinct cursor", "P2 cursor error");

    // Press Backspace in Phase 1 -> should revert to Phase 0 (P1 select), NOT Title Menu
    sendKeyClick(canvas, Qt::Key_Backspace);
    CHALLENGE_ASSERT(canvas.getGameState() == GameState::CHARACTER_SELECT,
                     "T2.13: Backspace in Phase 2 reverts to Phase 1 without exiting to title",
                     "Expected to remain in CHARACTER_SELECT");

    // Press Backspace in Phase 0 -> should exit to Title Menu
    sendKeyClick(canvas, Qt::Key_Backspace);
    CHALLENGE_ASSERT(canvas.getGameState() == GameState::TITLE_MENU,
                     "T2.14: Backspace in Phase 1 returns to TITLE_MENU", "Expected TITLE_MENU");
}

// ============================================================================
// SUITE 3: KEY-BLEED PREVENTION STRESS TEST
// ============================================================================
void testKeyBleedPrevention(TestableGameCanvas &canvas) {
    std::cout << "\n--- SUITE 3: Key-Bleed Prevention Stress Test ---\n";

    // 3.1 Throttle Key Bleed Prevention
    canvas.setGameState(GameState::TITLE_MENU);

    // Press '1' to enter Play vs AI character select
    sendKeyClick(canvas, Qt::Key_1);
    CHALLENGE_ASSERT(canvas.getGameState() == GameState::CHARACTER_SELECT,
                     "T3.1: Entered CHARACTER_SELECT for Play vs AI", "Expected CHARACTER_SELECT");

    // Simulate holding 'W' while in CHARACTER_SELECT
    sendKey(canvas, Qt::Key_W, true);

    // Confirm character to start race (still holding 'W' from character select screen)
    sendKeyClick(canvas, Qt::Key_Return);
    CHALLENGE_ASSERT(canvas.getGameState() == GameState::RACING,
                     "T3.2: Race started while W was held from menu", "Expected RACING");

    // Run 10 ticks of physics without sending fresh KeyPress(W)
    for (int i = 0; i < 10; ++i) {
        QTest::qWait(16);
    }

    // Inspect speed: kart MUST NOT have accelerated because pressedKeys was flushed on startRace!
    CHALLENGE_ASSERT(canvas.lastSpeed == 0.0,
                     "T3.3: Kart speed is strictly 0.0 — holding W in menu did not bleed into race throttle",
                     "Throttle key-bleed detected: speed > 0.0");

    // Clean up W key
    sendKey(canvas, Qt::Key_W, false);

    // 3.2 Steering Key Bleed Prevention
    canvas.setGameState(GameState::TITLE_MENU);
    sendKeyClick(canvas, Qt::Key_1); // to CHARACTER_SELECT
    sendKey(canvas, Qt::Key_A, true); // Hold A (nav / steer left)
    sendKeyClick(canvas, Qt::Key_Return); // Start race while A is held
    CHALLENGE_ASSERT(canvas.getGameState() == GameState::RACING,
                     "T3.4: Race started while A key held", "Expected RACING");

    for (int i = 0; i < 10; ++i) {
        QTest::qWait(16);
    }

    CHALLENGE_ASSERT(canvas.lastAngleDeg == 0.0,
                     "T3.5: Kart angle is strictly 0.0 deg — holding A in menu did not bleed into race steering",
                     "Steering key-bleed detected: angle != 0.0");

    sendKey(canvas, Qt::Key_A, false);

    // 3.3 Race to Title State Reset & Key Flush
    canvas.startRace();
    // Send fresh W key press to accelerate
    sendKey(canvas, Qt::Key_W, true);
    for (int i = 0; i < 20; ++i) {
        QTest::qWait(16);
    }
    double movingSpeed = canvas.lastSpeed;
    CHALLENGE_ASSERT(movingSpeed > 0.0,
                     "T3.6: Kart accelerated normally under active KeyPress(W)", "Speed was 0.0");

    // Press Escape to return to Title Menu
    sendKeyClick(canvas, Qt::Key_Escape);
    CHALLENGE_ASSERT(canvas.getGameState() == GameState::TITLE_MENU,
                     "T3.7: Returned to TITLE_MENU via Escape", "Expected TITLE_MENU");

    // Return to Racing and check that kart stopped and keys are flushed
    canvas.startRace();
    for (int i = 0; i < 5; ++i) {
        QTest::qWait(16);
    }
    CHALLENGE_ASSERT(canvas.lastSpeed == 0.0,
                     "T3.8: Restarting race cleanly reset kartSpeed to 0.0 and flushed throttle",
                     "Stale speed or un-flushed keys detected on race re-entry");

    sendKey(canvas, Qt::Key_W, false);

    // 3.4 Focus-Out Event Clears All Held Keys
    canvas.startRace();
    sendKey(canvas, Qt::Key_W, true);
    for (int i = 0; i < 15; ++i) {
        QTest::qWait(16);
    }
    double speedBeforeFocusLoss = canvas.lastSpeed;
    CHALLENGE_ASSERT(speedBeforeFocusLoss > 0.0,
                     "T3.9: Kart gained speed before focus loss", "Expected speed > 0");

    // Trigger focusOutEvent
    QFocusEvent fe(QEvent::FocusOut);
    canvas.focusOutEvent(&fe);

    // Let physics advance without releasing W: kart should decelerate due to friction!
    for (int i = 0; i < 25; ++i) {
        QTest::qWait(16);
    }
    double speedAfterFocusLoss = canvas.lastSpeed;
    CHALLENGE_ASSERT(speedAfterFocusLoss < speedBeforeFocusLoss,
                     "T3.10: Focus loss successfully cleared pressed keys (kart decelerated via friction)",
                     "Focus-out did not clear throttle key");

    sendKey(canvas, Qt::Key_W, false);
}

// ============================================================================
// SUITE 4: CHARACTER STAT SCALING & DYNAMICS VERIFICATION
// ============================================================================
void testCharacterStatScaling(TestableGameCanvas &canvas) {
    std::cout << "\n--- SUITE 4: Character Stat Scaling & Dynamics Verification ---\n";

    const auto &chars = canvas.getCharacters();
    CHALLENGE_ASSERT(chars.size() == 4,
                     "T4.1: Roster has exactly 4 canonical characters", "Expected 4 characters");

    const CharacterDef &mario  = chars[0];
    const CharacterDef &bowser = chars[1];
    const CharacterDef &peach  = chars[2];
    const CharacterDef &toad   = chars[3];

    // 4.1 Static Inequality Assertions
    CHALLENGE_ASSERT(bowser.maxSpeed > mario.maxSpeed,
                     "T4.2: Bowser maxSpeed (" + std::to_string(bowser.maxSpeed) + " km/h) strictly > Mario (" + std::to_string(mario.maxSpeed) + " km/h)",
                     "Bowser top speed must exceed Mario");

    CHALLENGE_ASSERT(toad.accel > mario.accel,
                     "T4.3: Toad accel (" + std::to_string(toad.accel) + ") strictly > Mario (" + std::to_string(mario.accel) + ")",
                     "Toad acceleration must exceed Mario");

    CHALLENGE_ASSERT(peach.turnRate > bowser.turnRate,
                     "T4.4: Peach handling/turnRate (" + std::to_string(peach.turnRate) + ") strictly > Bowser (" + std::to_string(bowser.turnRate) + ")",
                     "Peach turnRate must exceed Bowser");

    CHALLENGE_ASSERT(bowser.weight > mario.weight && mario.weight > peach.weight && peach.weight > toad.weight,
                     "T4.5: Weight ranking strictly Bowser (1.6) > Mario (1.0) > Peach (0.75) > Toad (0.65)",
                     "Weight ranking discrepancy");

    CHALLENGE_ASSERT(toad.accel > peach.accel && peach.accel > mario.accel && mario.accel > bowser.accel,
                     "T4.6: Acceleration ranking strictly Toad (140) > Peach (125) > Mario (110) > Bowser (85)",
                     "Acceleration ranking discrepancy");

    CHALLENGE_ASSERT(bowser.maxSpeed > mario.maxSpeed && mario.maxSpeed > peach.maxSpeed && peach.maxSpeed > toad.maxSpeed,
                     "T4.7: Top speed ranking strictly Bowser (175) > Mario (160) > Peach (152) > Toad (148)",
                     "Top speed ranking discrepancy");

    CHALLENGE_ASSERT(peach.turnRate > toad.turnRate && toad.turnRate > mario.turnRate && mario.turnRate > bowser.turnRate,
                     "T4.8: Turn rate ranking strictly Peach (3.4) > Toad (3.1) > Mario (2.8) > Bowser (2.2)",
                     "Turn rate ranking discrepancy");

    // 4.2 Dynamic Acceleration Measurement in Active Gameplay
    // Measure speed after 0.5 seconds of full throttle for Mario, Toad, and Bowser on tarmac
    auto measureSpeedAfterTime = [&](int charIndex, int ticks) -> double {
        canvas.setSelectedCharIndexP1(charIndex);
        canvas.startRace();
        sendKey(canvas, Qt::Key_W, true);
        for (int i = 0; i < ticks; ++i) {
            QTest::qWait(16);
        }
        sendKey(canvas, Qt::Key_W, false);
        return canvas.lastSpeed;
    };

    double toadSpeed05   = measureSpeedAfterTime(3, 30); // ~0.48s
    double marioSpeed05  = measureSpeedAfterTime(0, 30);
    double bowserSpeed05 = measureSpeedAfterTime(1, 30);

    std::cout << "    [Data] Measured speed after ~0.5s: Toad=" << toadSpeed05
              << ", Mario=" << marioSpeed05 << ", Bowser=" << bowserSpeed05 << " km/h\n";

    CHALLENGE_ASSERT(toadSpeed05 > marioSpeed05 && marioSpeed05 > bowserSpeed05,
                     "T4.9: Dynamic in-game acceleration orders: Toad > Mario > Bowser",
                     "Dynamic acceleration mismatch: Toad=" + std::to_string(toadSpeed05) +
                     ", Mario=" + std::to_string(marioSpeed05) + ", Bowser=" + std::to_string(bowserSpeed05));

    // 4.3 Dynamic Top Speed Measurement
    // Track maximum speed achieved while on tarmac straightaway before entering grass curve
    auto measureMaxRoadSpeed = [&](int charIndex) -> double {
        canvas.setSelectedCharIndexP1(charIndex);
        canvas.startRace();
        double peakSpeed = 0.0;
        sendKey(canvas, Qt::Key_W, true);
        for (int i = 0; i < 170; ++i) {
            QTest::qWait(16);
            if (canvas.lastSpeed > peakSpeed) {
                peakSpeed = canvas.lastSpeed;
            }
            if (i % 25 == 0 || i >= 135) {
                std::cout << "      [Tick " << i << "] Speed=" << canvas.lastSpeed
                          << " (Peak=" << peakSpeed << ") X=" << canvas.lastX
                          << " Z=" << canvas.lastZ << " Surf=" << canvas.lastSurface.toStdString() << "\n";
            }
        }
        sendKey(canvas, Qt::Key_W, false);
        return peakSpeed;
    };

    double bowserMaxRoad = measureMaxRoadSpeed(1);
    double marioMaxRoad  = measureMaxRoadSpeed(0);

    std::cout << "    [Data] Measured peak road speed: Bowser=" << bowserMaxRoad
              << ", Mario=" << marioMaxRoad << " km/h\n";

    CHALLENGE_ASSERT(std::abs(bowserMaxRoad - 175.0) < 0.5,
                     "T4.10: Bowser reaches full calibrated maxSpeed (175.0 km/h) on road straightaway",
                     "Expected 175.0 within 0.5, got " + std::to_string(bowserMaxRoad));

    CHALLENGE_ASSERT(std::abs(marioMaxRoad - 160.0) < 0.5,
                     "T4.11: Mario reaches full calibrated maxSpeed (160.0 km/h) on road straightaway",
                     "Expected 160.0 within 0.5, got " + std::to_string(marioMaxRoad));

    CHALLENGE_ASSERT(bowserMaxRoad > marioMaxRoad,
                     "T4.12: Bowser dynamic top speed strictly > Mario top speed",
                     "Top speed comparison failed");

    // 4.4 Dynamic Cornering Agility Measurement
    // Accelerate to speed 60, then steer left for 20 ticks (~0.32s)
    auto measureTurnDeflection = [&](int charIndex) -> double {
        canvas.setSelectedCharIndexP1(charIndex);
        canvas.startRace();
        sendKey(canvas, Qt::Key_W, true);
        for (int i = 0; i < 20; ++i) QTest::qWait(16);
        sendKey(canvas, Qt::Key_W, false);

        double angleStart = canvas.lastAngleDeg;
        sendKey(canvas, Qt::Key_A, true);
        for (int i = 0; i < 20; ++i) QTest::qWait(16);
        sendKey(canvas, Qt::Key_A, false);

        double angleEnd = canvas.lastAngleDeg;
        double diff = std::abs(angleEnd - angleStart);
        if (diff > 180.0) diff = 360.0 - diff;
        return diff;
    };

    double peachTurn  = measureTurnDeflection(2);
    double bowserTurn = measureTurnDeflection(1);

    std::cout << "    [Data] Measured steering angular deflection: Peach=" << peachTurn
              << " deg, Bowser=" << bowserTurn << " deg\n";

    CHALLENGE_ASSERT(peachTurn > bowserTurn,
                     "T4.13: Peach cornering angular deflection (" + std::to_string(peachTurn) + " deg) strictly > Bowser (" + std::to_string(bowserTurn) + " deg)",
                     "Handling dynamic test failed: Peach turn rate not greater than Bowser");
}

// ============================================================================
// SUITE 5: SPRITE ATLAS ASSETS INTEGRITY
// ============================================================================
void testSpriteAtlasIntegrity() {
    std::cout << "\n--- SUITE 5: Sprite Atlas Procedural Assets Verification ---\n";

    SpriteAtlas &atlas = SpriteAtlas::instance();

    std::vector<SpriteType> primaryKarts = {
        SpriteType::KART_MARIO, SpriteType::KART_BOWSER,
        SpriteType::KART_PEACH, SpriteType::KART_TOAD
    };
    std::vector<SpriteType> altKarts = {
        SpriteType::KART_MARIO_ALT, SpriteType::KART_BOWSER_ALT,
        SpriteType::KART_PEACH_ALT, SpriteType::KART_TOAD_ALT
    };
    std::vector<std::string> names = { "MARIO", "BOWSER", "PEACH", "TOAD" };

    for (size_t i = 0; i < 4; ++i) {
        const QImage &pri = atlas.getSprite(primaryKarts[i]);
        const QImage &alt = atlas.getSprite(altKarts[i]);

        bool dimsOk = (pri.width() == 32 && pri.height() == 32 &&
                       alt.width() == 32 && alt.height() == 32);
        CHALLENGE_ASSERT(dimsOk, "T5." + std::to_string(i*2 + 1) + ": " + names[i] + " primary & alt sprites are 32x32",
                         "Sprite dimensions invalid");

        // Count differing pixels
        int diffPixels = 0;
        int nonZeroPixels = 0;
        for (int y = 0; y < 32; ++y) {
            for (int x = 0; x < 32; ++x) {
                QRgb pCol = pri.pixel(x, y);
                QRgb aCol = alt.pixel(x, y);
                if (qAlpha(pCol) > 0) nonZeroPixels++;
                if (pCol != aCol) diffPixels++;
            }
        }
        CHALLENGE_ASSERT(nonZeroPixels > 100 && diffPixels > 20,
                         "T5." + std::to_string(i*2 + 2) + ": " + names[i] + " alt palette visibly differs from primary (" + std::to_string(diffPixels) + " px diff)",
                         "Palette variation insufficient");
    }
}

// ============================================================================
// MAIN RUNNER
// ============================================================================
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    std::cout << "===============================================================\n";
    std::cout << " RETROKART 3D: EMPIRICAL CHALLENGER STRESS HARNESS (MILESTONE 1)\n";
    std::cout << " Rigorous Verification of Game State, Characters & Input Bleed\n";
    std::cout << "===============================================================\n";

    TestableGameCanvas canvas;
    canvas.hookSignals();
    canvas.resize(400, 300);

    testRapidMenuTransitions(canvas);
    testCharacterIndexBoundaries(canvas);
    testKeyBleedPrevention(canvas);
    testCharacterStatScaling(canvas);
    testSpriteAtlasIntegrity();

    std::cout << "\n===============================================================\n";
    std::cout << "                 CHALLENGE SUITE SUMMARY                       \n";
    std::cout << "===============================================================\n";
    std::cout << " Total Challenges Executed: " << g_stats.totalTests << "\n";
    std::cout << " Passed Challenges:        " << g_stats.passedTests << " ("
              << (g_stats.totalTests > 0 ? (g_stats.passedTests * 100 / g_stats.totalTests) : 0) << "%)\n";
    std::cout << " Failed Challenges:        " << g_stats.failedTests << "\n";
    std::cout << "===============================================================\n";

    if (g_stats.failedTests > 0) {
        std::cout << "\n>>> VERDICT: REQUEST_CHANGES <<<\n";
        std::cout << "Failures encountered:\n";
        for (const auto &msg : g_stats.failureMessages) {
            std::cout << "  * " << msg << "\n";
        }
        return 1;
    } else {
        std::cout << "\n>>> VERDICT: APPROVE <<<\n";
        std::cout << "All empirical stress tests passed with zero regressions.\n";
        return 0;
    }
}
