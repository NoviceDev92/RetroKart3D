#include "game_canvas.h"
#include "sprite_atlas.h"
#include <QApplication>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QElapsedTimer>
#include <QImage>
#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <cassert>

// Empirical Challenge Test Suite for Milestone 1
// Retrokart 3D - Independent Verification

static int g_passCount = 0;
static int g_failCount = 0;

#define CHALLENGE_ASSERT(cond, msg) \
    do { \
        if (cond) { \
            std::cout << "  [PASS] " << msg << std::endl; \
            g_passCount++; \
        } else { \
            std::cerr << "  [FAIL] " << msg << " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl; \
            g_failCount++; \
        } \
    } while(0)

void sendKey(GameCanvas &canvas, int key, bool press = true)
{
    if (press) {
        QKeyEvent ev(QEvent::KeyPress, key, Qt::NoModifier);
        QApplication::sendEvent(&canvas, &ev);
    } else {
        QKeyEvent ev(QEvent::KeyRelease, key, Qt::NoModifier);
        QApplication::sendEvent(&canvas, &ev);
    }
}

void tapKey(GameCanvas &canvas, int key)
{
    sendKey(canvas, key, true);
    sendKey(canvas, key, false);
}

void tickGameLoop(GameCanvas &canvas)
{
    QMetaObject::invokeMethod(&canvas, "updateGameLoop");
}

// -------------------------------------------------------------
// TEST 1: Performance & CPU Utilization of updateGameLoop()
// -------------------------------------------------------------
void test_PerformanceAndCPUUtilization(GameCanvas &canvas)
{
    std::cout << "\n=======================================================\n";
    std::cout << " TEST 1: CPU Utilization of updateGameLoop() in Menus\n";
    std::cout << "=======================================================\n";

    const int WARMUP_FRAMES = 50;
    const int BENCHMARK_FRAMES = 500;

    // 1. Benchmark TITLE_MENU
    canvas.setGameState(GameState::TITLE_MENU);
    for (int i = 0; i < WARMUP_FRAMES; ++i) tickGameLoop(canvas);

    auto startMenu = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < BENCHMARK_FRAMES; ++i) {
        tickGameLoop(canvas);
    }
    auto endMenu = std::chrono::high_resolution_clock::now();
    double titleMenuTotalUs = std::chrono::duration<double, std::micro>(endMenu - startMenu).count();
    double titleMenuAvgUs = titleMenuTotalUs / BENCHMARK_FRAMES;
    double titleMenuAvgMs = titleMenuAvgUs / 1000.0;

    std::cout << "  Title Menu 500 frames: " << titleMenuTotalUs / 1000.0 << " ms total ("
              << titleMenuAvgMs << " ms/frame, " << titleMenuAvgUs << " us/frame)\n";

    // 2. Benchmark CHARACTER_SELECT
    canvas.setGameState(GameState::CHARACTER_SELECT);
    for (int i = 0; i < WARMUP_FRAMES; ++i) tickGameLoop(canvas);

    auto startChar = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < BENCHMARK_FRAMES; ++i) {
        tickGameLoop(canvas);
    }
    auto endChar = std::chrono::high_resolution_clock::now();
    double charSelectTotalUs = std::chrono::duration<double, std::micro>(endChar - startChar).count();
    double charSelectAvgUs = charSelectTotalUs / BENCHMARK_FRAMES;
    double charSelectAvgMs = charSelectAvgUs / 1000.0;

    std::cout << "  Char Select 500 frames: " << charSelectTotalUs / 1000.0 << " ms total ("
              << charSelectAvgMs << " ms/frame, " << charSelectAvgUs << " us/frame)\n";

    // 3. Benchmark RACING state as baseline Mode-7 comparison
    canvas.setGameState(GameState::RACING);
    for (int i = 0; i < WARMUP_FRAMES; ++i) tickGameLoop(canvas);

    auto startRace = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < BENCHMARK_FRAMES; ++i) {
        tickGameLoop(canvas);
    }
    auto endRace = std::chrono::high_resolution_clock::now();
    double raceTotalUs = std::chrono::duration<double, std::micro>(endRace - startRace).count();
    double raceAvgUs = raceTotalUs / BENCHMARK_FRAMES;
    double raceAvgMs = raceAvgUs / 1000.0;

    std::cout << "  Active Racing 500 frames: " << raceTotalUs / 1000.0 << " ms total ("
              << raceAvgMs << " ms/frame, " << raceAvgUs << " us/frame)\n";

    // 60 FPS budget = 16.666 ms per frame
    double frameBudgetMs = 16.666;
    double titleMenuCpuPercent = (titleMenuAvgMs / frameBudgetMs) * 100.0;
    double charSelectCpuPercent = (charSelectAvgMs / frameBudgetMs) * 100.0;

    std::cout << "  Title Menu 60 FPS frame-time utilization: " << titleMenuCpuPercent << " %\n";
    std::cout << "  Char Select 60 FPS frame-time utilization: " << charSelectCpuPercent << " %\n";

    // Verify Title Menu per-frame time is well within budget (< 2.0 ms)
    CHALLENGE_ASSERT(titleMenuAvgMs < 2.0, "Title Menu frame time is negligible (< 2.0 ms)");
    // Verify Character Select per-frame time is well within budget (< 3.0 ms)
    CHALLENGE_ASSERT(charSelectAvgMs < 3.0, "Character Select frame time is negligible (< 3.0 ms)");
    // Verify negligible CPU utilization percentage (< 15% of total 16.67ms 60 FPS frame time)
    CHALLENGE_ASSERT(titleMenuCpuPercent < 15.0, "Title Menu CPU budget consumption is negligible (< 15% of 16.67ms frame time)");
    CHALLENGE_ASSERT(charSelectCpuPercent < 15.0, "Char Select CPU budget consumption is negligible (< 15% of 16.67ms frame time)");
    CHALLENGE_ASSERT(titleMenuAvgMs + charSelectAvgMs < frameBudgetMs, "Combined menu render times are well within a single frame budget");
}

// -------------------------------------------------------------
// TEST 2: SpriteAtlas Memory Safety & Bounds Verification
// -------------------------------------------------------------
void test_SpriteAtlasMemorySafety()
{
    std::cout << "\n=======================================================\n";
    std::cout << " TEST 2: SpriteAtlas Procedural Sprite Generation & Memory Safety\n";
    std::cout << "=======================================================\n";

    SpriteAtlas &atlas = SpriteAtlas::instance();

    std::vector<SpriteType> allTypes = {
        SpriteType::QUESTION_BOX,
        SpriteType::MUSHROOM,
        SpriteType::ROCKET,
        SpriteType::BANANA,
        SpriteType::GREEN_SHELL,
        SpriteType::INK_BLOOPER,
        SpriteType::GREEN_PIPE,
        SpriteType::THWOMP,
        SpriteType::OIL_SLICK,
        SpriteType::COIN,
        SpriteType::BANANA_DROPPED,
        SpriteType::SHELL_PROJECTILE,
        SpriteType::KART_MARIO,
        SpriteType::KART_BOWSER,
        SpriteType::KART_PEACH,
        SpriteType::KART_TOAD,
        SpriteType::KART_MARIO_ALT,
        SpriteType::KART_BOWSER_ALT,
        SpriteType::KART_PEACH_ALT,
        SpriteType::KART_TOAD_ALT
    };

    CHALLENGE_ASSERT(allTypes.size() == 20, "SpriteAtlas roster catalog contains all 20 canonical sprite types");

    for (auto st : allTypes) {
        const QImage &img = atlas.getSprite(st);
        CHALLENGE_ASSERT(!img.isNull(), "Sprite is non-null for enum value " + std::to_string(static_cast<int>(st)));
        CHALLENGE_ASSERT(img.width() == 32 && img.height() == 32, "Sprite size is exactly 32x32");
        CHALLENGE_ASSERT(img.format() == QImage::Format_ARGB32, "Sprite format is ARGB32");

        // Verify image has non-transparent pixels (actually drew something)
        int nonTransCount = 0;
        for (int y = 0; y < 32; ++y) {
            for (int x = 0; x < 32; ++x) {
                if (qAlpha(img.pixel(x, y)) > 0) nonTransCount++;
            }
        }
        CHALLENGE_ASSERT(nonTransCount > 50, "Sprite contains valid rasterized artwork pixels (> 50 pixels)");
    }

    // Verify Out-of-Bounds SpriteType handling
    const QImage &fallbackNeg = atlas.getSprite(static_cast<SpriteType>(-999));
    CHALLENGE_ASSERT(fallbackNeg.width() == 32 && fallbackNeg.height() == 32, "Out-of-bounds negative SpriteType returns valid 32x32 fallback");

    const QImage &fallbackLarge = atlas.getSprite(static_cast<SpriteType>(999999));
    CHALLENGE_ASSERT(fallbackLarge.width() == 32 && fallbackLarge.height() == 32, "Out-of-bounds large SpriteType returns valid 32x32 fallback");

    // Stress test: 500,000 rapid lookups across atlas
    auto startStress = std::chrono::high_resolution_clock::now();
    uint64_t dummyChecksum = 0;
    for (int i = 0; i < 500000; ++i) {
        SpriteType st = allTypes[i % allTypes.size()];
        const QImage &img = atlas.getSprite(st);
        dummyChecksum += img.width();
    }
    auto endStress = std::chrono::high_resolution_clock::now();
    double stressMs = std::chrono::duration<double, std::milli>(endStress - startStress).count();
    std::cout << "  500,000 Sprite lookups completed in " << stressMs << " ms ("
              << (500000.0 / (stressMs / 1000.0)) << " lookups/sec)\n";
    CHALLENGE_ASSERT(dummyChecksum == 500000ULL * 32ULL, "Atlas lookup integrity maintained across 500k queries");

    // Verify Palette Swapping differentiation between primary and ALT karts
    const QImage &marioNormal = atlas.getSprite(SpriteType::KART_MARIO);
    const QImage &marioAlt = atlas.getSprite(SpriteType::KART_MARIO_ALT);
    CHALLENGE_ASSERT(marioNormal.pixel(16, 21) != marioAlt.pixel(16, 21) ||
                     marioNormal.pixel(10, 21) != marioAlt.pixel(10, 21),
                     "Mario primary and Fire Mario ALT sprites have distinct palette values");

    const QImage &bowserNormal = atlas.getSprite(SpriteType::KART_BOWSER);
    const QImage &bowserAlt = atlas.getSprite(SpriteType::KART_BOWSER_ALT);
    CHALLENGE_ASSERT(bowserNormal.pixel(10, 21) != bowserAlt.pixel(10, 21),
                     "Bowser primary and Dry Bowser ALT sprites have distinct palette values");

    const QImage &peachNormal = atlas.getSprite(SpriteType::KART_PEACH);
    const QImage &peachAlt = atlas.getSprite(SpriteType::KART_PEACH_ALT);
    CHALLENGE_ASSERT(peachNormal.pixel(10, 21) != peachAlt.pixel(10, 21),
                     "Peach primary and Daisy ALT sprites have distinct palette values");

    const QImage &toadNormal = atlas.getSprite(SpriteType::KART_TOAD);
    const QImage &toadAlt = atlas.getSprite(SpriteType::KART_TOAD_ALT);
    CHALLENGE_ASSERT(toadNormal.pixel(10, 21) != toadAlt.pixel(10, 21),
                     "Toad primary and Yellow Toad ALT sprites have distinct palette values");
}

// -------------------------------------------------------------
// TEST 3: Adversarial Input Sequences During Character Selection
// -------------------------------------------------------------
void test_AdversarialInputSequences(GameCanvas &canvas)
{
    std::cout << "\n=======================================================\n";
    std::cout << " TEST 3: Adversarial Input Sequences During Character Selection\n";
    std::cout << "=======================================================\n";

    canvas.setGameState(GameState::TITLE_MENU);
    tapKey(canvas, Qt::Key_1); // Select Play vs AI
    CHALLENGE_ASSERT(canvas.getGameState() == GameState::CHARACTER_SELECT, "Transitioned to CHARACTER_SELECT");
    CHALLENGE_ASSERT(canvas.getSelectedCharIndexP1() == 0, "Initial P1 selection is index 0");

    // 1. Rapid alternating Left/Right spam (10,000 events)
    for (int i = 0; i < 5000; ++i) {
        tapKey(canvas, Qt::Key_Right);
        tapKey(canvas, Qt::Key_Left);
    }
    CHALLENGE_ASSERT(canvas.getSelectedCharIndexP1() == 0, "Index returns to 0 after balanced Left/Right spam");
    CHALLENGE_ASSERT(canvas.getSelectedCharIndexP1() >= 0 && canvas.getSelectedCharIndexP1() < 4, "Index strictly bounded in [0, 3]");

    // 2. High-frequency unidirectional cycling (5,001 Right key presses)
    for (int i = 0; i < 5001; ++i) {
        tapKey(canvas, Qt::Key_Right);
    }
    CHALLENGE_ASSERT(canvas.getSelectedCharIndexP1() == (5001 % 4), "Unidirectional cycling modulo 4 is exact (5001 % 4 == 1)");

    // 3. High-frequency WASD cycling (A and D keys)
    for (int i = 0; i < 4000; ++i) {
        tapKey(canvas, Qt::Key_D);
    }
    CHALLENGE_ASSERT(canvas.getSelectedCharIndexP1() == 1, "A/D cycling modulo 4 is exact");

    // 4. Hostile / unexpected key flood
    const int weirdKeys[] = {
        Qt::Key_Tab, Qt::Key_Backtab, Qt::Key_Shift, Qt::Key_Control, Qt::Key_Alt,
        Qt::Key_Meta, Qt::Key_CapsLock, Qt::Key_PageUp, Qt::Key_PageDown,
        Qt::Key_F1, Qt::Key_F5, Qt::Key_F12, Qt::Key_Insert, Qt::Key_Delete,
        Qt::Key_Home, Qt::Key_End, Qt::Key_Print, Qt::Key_Pause, Qt::Key_ScrollLock,
        Qt::Key_BracketLeft, Qt::Key_BraceRight, Qt::Key_Semicolon, Qt::Key_Slash,
        Qt::Key_Backslash, Qt::Key_Question, Qt::Key_QuoteLeft, Qt::Key_AsciiTilde
    };
    for (int k : weirdKeys) {
        tapKey(canvas, k);
    }
    CHALLENGE_ASSERT(canvas.getSelectedCharIndexP1() >= 0 && canvas.getSelectedCharIndexP1() < 4,
                     "Hostile non-gameplay keys safely ignored without state corruption");
    CHALLENGE_ASSERT(canvas.getGameState() == GameState::CHARACTER_SELECT,
                     "GameState remains intact after hostile key spam");

    // 5. Rapid State Transition thrashing (Enter -> Escape -> Enter -> Escape 1,000 times)
    canvas.setGameState(GameState::TITLE_MENU);
    for (int i = 0; i < 1000; ++i) {
        tapKey(canvas, Qt::Key_Return); // Goes to CHARACTER_SELECT
        tapKey(canvas, Qt::Key_Escape); // Returns to TITLE_MENU
    }
    CHALLENGE_ASSERT(canvas.getGameState() == GameState::TITLE_MENU, "1,000 rapid state flips settle cleanly at TITLE_MENU");

    // 6. Direct track selection keys (1, 2, 3) in Character Select
    canvas.setGameState(GameState::CHARACTER_SELECT);
    tapKey(canvas, Qt::Key_2); // Switch to Choco Valley
    tapKey(canvas, Qt::Key_3); // Switch to Bowser Castle
    tapKey(canvas, Qt::Key_1); // Switch to Donut Plains
    CHALLENGE_ASSERT(canvas.getGameState() == GameState::CHARACTER_SELECT, "Track keys 1-3 handled cleanly during character select");
}

// -------------------------------------------------------------
// TEST 4: Multiplayer 2P Selection & Duplicate Palette Allocation
// -------------------------------------------------------------
void test_MultiplayerDuplicatePaletteEdgeCases(GameCanvas &canvas)
{
    std::cout << "\n=======================================================\n";
    std::cout << " TEST 4: Multiplayer 2P Sequential Selection Edge Cases\n";
    std::cout << "=======================================================\n";

    // 1. Enter Multiplayer mode from Title Menu
    canvas.setGameState(GameState::TITLE_MENU);
    tapKey(canvas, Qt::Key_2); // Select 2. MULTIPLAYER
    CHALLENGE_ASSERT(canvas.getGameState() == GameState::CHARACTER_SELECT, "Multiplayer mode transition to CHARACTER_SELECT");
    CHALLENGE_ASSERT(canvas.getGameMode() == GameMode::MULTIPLAYER, "Selected game mode is MULTIPLAYER");

    // Helper lambda to test same-character pick
    auto testDuplicateRacer = [&](int racerId, const QString &racerName) {
        canvas.setGameState(GameState::TITLE_MENU);
        tapKey(canvas, Qt::Key_2); // MULTIPLAYER

        // P1 selects racerId
        canvas.setSelectedCharIndexP1(racerId);
        tapKey(canvas, Qt::Key_Return); // Confirm P1

        // P2 selects the SAME racerId
        canvas.setSelectedCharIndexP2(racerId);
        tapKey(canvas, Qt::Key_Return); // Confirm P2 and launch race

        CHALLENGE_ASSERT(canvas.getGameState() == GameState::RACING,
                         "Race launched successfully with duplicate " + racerName.toStdString());

        const CharacterDef &p1Char = canvas.getSelectedCharacterP1();
        const CharacterDef &p2Char = canvas.getSelectedCharacterP2();

        CHALLENGE_ASSERT(p1Char.id == racerId, racerName.toStdString() + ": P1 character ID matches");
        CHALLENGE_ASSERT(p2Char.id == racerId, racerName.toStdString() + ": P2 character ID matches");

        // When P1 and P2 select the same character, P2 MUST receive the alternate palette!
        CHALLENGE_ASSERT(p1Char.primaryColor != p2Char.altColor,
                         racerName.toStdString() + ": P1 primary color and P2 alt color are distinct");

        // Verify altColor is valid and has full opacity
        CHALLENGE_ASSERT(p2Char.altColor.isValid() && p2Char.altColor.alpha() == 255,
                         racerName.toStdString() + ": P2 alt color is fully opaque and valid");
    };

    // Test duplicate selection for all 4 characters
    testDuplicateRacer(0, "MARIO");
    testDuplicateRacer(1, "BOWSER");
    testDuplicateRacer(2, "PEACH");
    testDuplicateRacer(3, "TOAD");

    // 2. Test Disjoint picks (P1 Mario, P2 Peach)
    canvas.setGameState(GameState::TITLE_MENU);
    tapKey(canvas, Qt::Key_2); // MULTIPLAYER
    canvas.setSelectedCharIndexP1(0); // Mario
    tapKey(canvas, Qt::Key_Return); // P1 Confirm
    canvas.setSelectedCharIndexP2(2); // Peach
    tapKey(canvas, Qt::Key_Return); // P2 Confirm -> Launch race

    CHALLENGE_ASSERT(canvas.getGameState() == GameState::RACING, "Disjoint 2P race launched");
    const CharacterDef &p1Mario = canvas.getSelectedCharacterP1();
    const CharacterDef &p2Peach = canvas.getSelectedCharacterP2();
    CHALLENGE_ASSERT(p1Mario.id == 0 && p2Peach.id == 2, "P1 Mario and P2 Peach correctly assigned");
    CHALLENGE_ASSERT(p1Mario.primaryColor != p2Peach.primaryColor, "Disjoint characters have distinct primary colors");

    // 3. Test Back-Navigation in 2P Mode:
    // When in Phase 1 (P2 picking), Escape should step back to Phase 0 (P1 picking), not to Title Menu
    canvas.setGameState(GameState::TITLE_MENU);
    tapKey(canvas, Qt::Key_2); // MULTIPLAYER (Phase 0)
    tapKey(canvas, Qt::Key_Return); // Advance to Phase 1
    tapKey(canvas, Qt::Key_Escape); // Step back to Phase 0
    CHALLENGE_ASSERT(canvas.getGameState() == GameState::CHARACTER_SELECT,
                     "Escape in 2P Phase 1 stays in CHARACTER_SELECT (stepped back to P1 selection)");
    tapKey(canvas, Qt::Key_Escape); // Step back from Phase 0 to Title
    CHALLENGE_ASSERT(canvas.getGameState() == GameState::TITLE_MENU,
                     "Second Escape in Phase 0 returns cleanly to TITLE_MENU");
}

// -------------------------------------------------------------
// TEST 5: Character Stats Calibration & Driving Differentiation
// -------------------------------------------------------------
void test_CharacterStatsDifferentiation(GameCanvas &canvas)
{
    std::cout << "\n=======================================================\n";
    std::cout << " TEST 5: Character Stats Calibration & Differentiation\n";
    std::cout << "=======================================================\n";

    const auto &chars = canvas.getCharacters();
    CHALLENGE_ASSERT(chars.size() == 4, "Roster contains exactly 4 canonical characters");

    // Verify stats according to specification:
    // Mario: Balanced
    const CharacterDef &mario = chars[0];
    CHALLENGE_ASSERT(mario.name == "MARIO", "Character 0 is MARIO");
    CHALLENGE_ASSERT(mario.maxSpeed == 160.0, "Mario max speed is 160.0 km/h");
    CHALLENGE_ASSERT(mario.accel == 110.0, "Mario accel is 110.0");
    CHALLENGE_ASSERT(mario.turnRate == 2.80, "Mario turn rate is 2.80");
    CHALLENGE_ASSERT(mario.weight == 1.00, "Mario weight is 1.00");

    // Bowser: Heavyweight
    const CharacterDef &bowser = chars[1];
    CHALLENGE_ASSERT(bowser.name == "BOWSER", "Character 1 is BOWSER");
    CHALLENGE_ASSERT(bowser.maxSpeed == 175.0, "Bowser max speed is 175.0 km/h");
    CHALLENGE_ASSERT(bowser.accel == 85.0, "Bowser accel is 85.0 (slower initial throttle)");
    CHALLENGE_ASSERT(bowser.turnRate == 2.20, "Bowser turn rate is 2.20");
    CHALLENGE_ASSERT(bowser.weight == 1.60, "Bowser weight is 1.60 (heavyweight)");

    // Peach: Agility
    const CharacterDef &peach = chars[2];
    CHALLENGE_ASSERT(peach.name == "PEACH", "Character 2 is PEACH");
    CHALLENGE_ASSERT(peach.maxSpeed == 152.0, "Peach max speed is 152.0 km/h");
    CHALLENGE_ASSERT(peach.accel == 125.0, "Peach accel is 125.0");
    CHALLENGE_ASSERT(peach.turnRate == 3.40, "Peach turn rate is 3.40 (sharpest steering)");
    CHALLENGE_ASSERT(peach.weight == 0.75, "Peach weight is 0.75");

    // Toad: Acceleration
    const CharacterDef &toad = chars[3];
    CHALLENGE_ASSERT(toad.name == "TOAD", "Character 3 is TOAD");
    CHALLENGE_ASSERT(toad.maxSpeed == 148.0, "Toad max speed is 148.0 km/h");
    CHALLENGE_ASSERT(toad.accel == 140.0, "Toad accel is 140.0 (highest acceleration)");
    CHALLENGE_ASSERT(toad.turnRate == 3.10, "Toad turn rate is 3.10");
    CHALLENGE_ASSERT(toad.weight == 0.65, "Toad weight is 0.65");

    // Verify statistical hierarchy relationships:
    CHALLENGE_ASSERT(bowser.maxSpeed > mario.maxSpeed && mario.maxSpeed > peach.maxSpeed && peach.maxSpeed > toad.maxSpeed,
                     "Top speed hierarchy: Bowser (175) > Mario (160) > Peach (152) > Toad (148)");
    CHALLENGE_ASSERT(toad.accel > peach.accel && peach.accel > mario.accel && mario.accel > bowser.accel,
                     "Acceleration hierarchy: Toad (140) > Peach (125) > Mario (110) > Bowser (85)");
    CHALLENGE_ASSERT(peach.turnRate > toad.turnRate && toad.turnRate > mario.turnRate && mario.turnRate > bowser.turnRate,
                     "Steering turnRate hierarchy: Peach (3.4) > Toad (3.1) > Mario (2.8) > Bowser (2.2)");
    CHALLENGE_ASSERT(bowser.weight > mario.weight && mario.weight > peach.weight && peach.weight > toad.weight,
                     "Weight hierarchy: Bowser (1.6) > Mario (1.0) > Peach (0.75) > Toad (0.65)");
}

// -------------------------------------------------------------
// MAIN ENTRY POINT
// -------------------------------------------------------------
int main(int argc, char *argv[])
{
    // Initialize Qt Application for widget event pump
    QApplication app(argc, argv);

    std::cout << "=======================================================\n";
    std::cout << "   RETROKART 3D EMPIRICAL CHALLENGE HARNESS (M1)       \n";
    std::cout << "   Independent Stress-Testing & Correctness Audit       \n";
    std::cout << "=======================================================\n";

    GameCanvas canvas;

    test_PerformanceAndCPUUtilization(canvas);
    test_SpriteAtlasMemorySafety();
    test_AdversarialInputSequences(canvas);
    test_MultiplayerDuplicatePaletteEdgeCases(canvas);
    test_CharacterStatsDifferentiation(canvas);

    std::cout << "\n=======================================================\n";
    std::cout << "               EMPIRICAL CHALLENGE SUMMARY             \n";
    std::cout << "=======================================================\n";
    std::cout << "  Passed Assertions: " << g_passCount << "\n";
    std::cout << "  Failed Assertions: " << g_failCount << "\n";
    std::cout << "=======================================================\n";

    if (g_failCount == 0) {
        std::cout << " >>> VERDICT: ALL EMPIRICAL CHALLENGES PASSED! <<<\n";
        return 0;
    } else {
        std::cerr << " >>> VERDICT: EMPIRICAL DEFECTS DETECTED! <<<\n";
        return 1;
    }
}
