#include <QApplication>
#include <QWidget>
#include <QImage>
#include <QTimer>
#include <QElapsedTimer>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QTest>
#include <iostream>
#include <iomanip>
#include <vector>
#include <random>
#include <cmath>
#include <cassert>
#include <climits>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Access GameCanvas internals for white-box adversarial verification
#define private public
#define protected public
#include "game_canvas.h"
#undef private
#undef protected
#include "sprite_atlas.h"

struct ChallengeStats {
    int total = 0;
    int passed = 0;
    int failed = 0;
    std::vector<std::string> failures;

    void record(bool condition, const std::string &testName, const std::string &detail = "") {
        total++;
        if (condition) {
            passed++;
            std::cout << "  [PASS] " << testName << "\n";
        } else {
            failed++;
            std::string msg = testName + " - FAIL: " + detail;
            failures.push_back(msg);
            std::cerr << "  [FAIL] " << testName << " -> " << detail << "\n";
        }
    }
};

static ChallengeStats g_stats;

#define CHALLENGE(cond, name, detail) g_stats.record((cond), (name), (detail))

class EmpiricalCanvas : public GameCanvas {
public:
    explicit EmpiricalCanvas(QWidget *parent = nullptr) : GameCanvas(parent) {}

    void simulateTick(double dt) {
        updatePhysics(dt);
        if (selectedGameMode == GameMode::PLAY_VS_AI) {
            updateAIOpponents(dt);
        }
        updateItemSystem(dt);
    }

    void renderDirect() {
        renderSky();
        renderGroundMode7();
        renderSprites();
        renderCockpit();
        renderItemHUD();
        renderInkOverlay();
    }
};

// ============================================================================
// SUITE 1: LONG-DISTANCE AI WAYPOINT NAVIGATION & STABILITY STRESS TEST
// ============================================================================
void testAIWaypointNavigation(EmpiricalCanvas &canvas) {
    std::cout << "\n===============================================================\n";
    std::cout << " SUITE 1: Long-Distance AI Waypoint Navigation Across 3 Tracks\n";
    std::cout << "===============================================================\n";

    canvas.setGameState(GameState::RACING);
    canvas.setGameMode(GameMode::PLAY_VS_AI);

    // Helper lambda to stress-test 100+ waypoint advances on a given track
    auto stressTrack = [&](TrackLevel level, const std::string &trackName, int expectedWpCount, int maxTicks) {
        std::cout << "\n  --- Stress-Testing " << trackName << " ---\n";
        canvas.setTrackLevel(level);
        canvas.startRace();

        const auto &wps = canvas.getTrackWaypoints();
        CHALLENGE(!wps.empty() && static_cast<int>(wps.size()) == expectedWpCount,
                  trackName + ": Persistent waypoints count matches expected (" + std::to_string(expectedWpCount) + ")",
                  "Waypoints count=" + std::to_string(wps.size()));

        CHALLENGE(canvas.aiOpponents.size() == 3,
                  trackName + ": 3 active AI rivals spawned on starting grid",
                  "AI count=" + std::to_string(canvas.aiOpponents.size()));

        std::vector<int> wpAdvances(canvas.aiOpponents.size(), 0);
        std::vector<int> prevWp(canvas.aiOpponents.size(), 0);
        for (size_t i = 0; i < canvas.aiOpponents.size(); ++i) {
            prevWp[i] = canvas.aiOpponents[i].currentWaypoint;
        }

        bool allFinite = true;
        bool stuckDetected = false;
        double dt = 0.020; // 50 Hz physics tick
        int ticksRun = 0;

        for (int tick = 0; tick < maxTicks; ++tick) {
            canvas.simulateTick(dt);
            ticksRun++;

            for (size_t i = 0; i < canvas.aiOpponents.size(); ++i) {
                const auto &ai = canvas.aiOpponents[i];
                if (!std::isfinite(ai.x) || !std::isfinite(ai.z) ||
                    !std::isfinite(ai.angle) || !std::isfinite(ai.speed)) {
                    allFinite = false;
                    break;
                }

                if (ai.currentWaypoint != prevWp[i]) {
                    wpAdvances[i]++;
                    prevWp[i] = ai.currentWaypoint;
                }
            }

            if (!allFinite) break;

            // Check if all 3 karts advanced 100+ waypoints
            if (wpAdvances[0] >= 100 && wpAdvances[1] >= 100 && wpAdvances[2] >= 100) {
                break;
            }
        }

        CHALLENGE(allFinite,
                  trackName + ": AI coordinates strictly finite across all ticks (no NaN / Inf)",
                  "Non-finite coordinates encountered");

        // Inspect if AI karts advanced 100+ waypoints or became stuck
        std::cout << "    [" << trackName << " Results] Ticks Run=" << ticksRun
                  << " | AI0 Advances=" << wpAdvances[0]
                  << " | AI1 Advances=" << wpAdvances[1]
                  << " | AI2 Advances=" << wpAdvances[2] << "\n";

        bool advanced100 = (wpAdvances[0] >= 100 && wpAdvances[1] >= 100 && wpAdvances[2] >= 100);
        if (!advanced100) {
            const auto &ai0 = canvas.aiOpponents[0];
            int sampleU = (static_cast<int>(ai0.x) % 1024 + 1024) % 1024;
            int sampleV = (static_cast<int>(ai0.z) % 1024 + 1024) % 1024;
            QRgb mask = canvas.trackMaskMap.pixel(sampleU, sampleV);
            std::cout << "    [Root Cause Diagnostic] AI0 stuck at (" << ai0.x << ", " << ai0.z
                      << ") spd=" << ai0.speed << " isSpinning=" << ai0.isSpinning
                      << " spinTimer=" << ai0.spinTimer << " wp=" << ai0.currentWaypoint
                      << " maskColor=RGB(" << qRed(mask) << "," << qGreen(mask) << "," << qBlue(mask) << ")\n";
        }

        CHALLENGE(advanced100,
                  trackName + ": All 3 AI karts completed 100+ waypoint advances without stalling",
                  "AI stalled: AI0=" + std::to_string(wpAdvances[0]) +
                  ", AI1=" + std::to_string(wpAdvances[1]) +
                  ", AI2=" + std::to_string(wpAdvances[2]) + " advances");
    };

    // 1.1 Donut Plains (16 waypoints, oval)
    stressTrack(TrackLevel::DONUT_PLAINS, "Donut Plains", 16, 5000);

    // 1.2 Choco Valley (20 waypoints, S-curves)
    stressTrack(TrackLevel::CHOCO_VALLEY, "Choco Valley", 20, 5000);

    // 1.3 Bowser Castle (24 waypoints, technical 90-degree turns)
    stressTrack(TrackLevel::BOWSER_CASTLE, "Bowser Castle", 24, 6000);
}

// ============================================================================
// SUITE 2: SHELL PROJECTILE PHYSICS & COLLISION STRESS TEST
// ============================================================================
void testShellProjectilePhysics(EmpiricalCanvas &canvas) {
    std::cout << "\n===============================================================\n";
    std::cout << " SUITE 2: Shell Projectile Physics & Collision Stress Test\n";
    std::cout << "===============================================================\n";

    canvas.setGameState(GameState::RACING);
    canvas.setGameMode(GameMode::PLAY_VS_AI);
    canvas.setTrackLevel(TrackLevel::DONUT_PLAINS);
    canvas.startRace();

    // 2.1 Multi-Angle Firing Trajectory & Velocity
    {
        const int numAngles = 8;
        bool allAnglesPass = true;

        for (int i = 0; i < numAngles; ++i) {
            double angle = (2.0 * M_PI * i) / numAngles;
            canvas.kartX = 500.0;
            canvas.kartZ = 500.0;
            canvas.kartAngle = angle;
            canvas.kartSpeed = 100.0;
            canvas.heldItem = PlayerItem::GREEN_SHELL;
            canvas.activeProjectiles.clear();

            canvas.useHeldItem();

            if (canvas.activeProjectiles.size() != 1) {
                allAnglesPass = false;
                break;
            }

            const auto &shell = canvas.activeProjectiles[0];
            double expectedSpeed = std::max(180.0, 100.0 + 75.0); // 180.0
            double calcSpeed = std::hypot(shell.vx, shell.vz);
            if (std::abs(calcSpeed - expectedSpeed) > 0.5) allAnglesPass = false;

            double launchAngle = std::atan2(shell.vz, shell.vx);
            while (launchAngle < 0.0) launchAngle += 2.0 * M_PI;
            double targetAngle = angle;
            while (targetAngle < 0.0) targetAngle += 2.0 * M_PI;
            if (std::abs(launchAngle - targetAngle) > 1e-3) allAnglesPass = false;

            if (shell.ownerId != 0 || shell.bouncesLeft != 3 || !shell.active || shell.lifeTime != 6.0) {
                allAnglesPass = false;
            }
        }

        CHALLENGE(allAnglesPass,
                  "T2.1: Green Shell fires forward at calibrated speed across 8 radial angles",
                  "Launch velocity or heading deviation");
    }

    // 2.2 Boundary Reflections: Left, Right, Top, Bottom Walls
    {
        // 2.2.1 Left wall bounce (x < 20.0)
        canvas.activeProjectiles.clear();
        Projectile sLeft;
        sLeft.x = 22.0; sLeft.z = 500.0; sLeft.vx = -100.0; sLeft.vz = 0.0;
        sLeft.bouncesLeft = 3; sLeft.active = true; sLeft.lifeTime = 6.0;
        canvas.activeProjectiles.push_back(sLeft);
        canvas.simulateTick(0.05);

        CHALLENGE(!canvas.activeProjectiles.empty() &&
                  canvas.activeProjectiles[0].vx > 0.0 &&
                  canvas.activeProjectiles[0].bouncesLeft == 2,
                  "T2.2: Left boundary reflection inverts vx to positive and decrements bouncesLeft",
                  "vx=" + std::to_string(canvas.activeProjectiles[0].vx));

        // 2.2.2 Right wall bounce (x > 1004.0)
        canvas.activeProjectiles.clear();
        Projectile sRight;
        sRight.x = 1002.0; sRight.z = 500.0; sRight.vx = 100.0; sRight.vz = 0.0;
        sRight.bouncesLeft = 3; sRight.active = true; sRight.lifeTime = 6.0;
        canvas.activeProjectiles.push_back(sRight);
        canvas.simulateTick(0.05);

        CHALLENGE(!canvas.activeProjectiles.empty() &&
                  canvas.activeProjectiles[0].vx < 0.0 &&
                  canvas.activeProjectiles[0].bouncesLeft == 2,
                  "T2.3: Right boundary reflection inverts vx to negative",
                  "vx=" + std::to_string(canvas.activeProjectiles[0].vx));

        // 2.2.3 Top wall bounce (z < 20.0)
        canvas.activeProjectiles.clear();
        Projectile sTop;
        sTop.x = 500.0; sTop.z = 22.0; sTop.vx = 0.0; sTop.vz = -100.0;
        sTop.bouncesLeft = 3; sTop.active = true; sTop.lifeTime = 6.0;
        canvas.activeProjectiles.push_back(sTop);
        canvas.simulateTick(0.05);

        CHALLENGE(!canvas.activeProjectiles.empty() &&
                  canvas.activeProjectiles[0].vz > 0.0 &&
                  canvas.activeProjectiles[0].bouncesLeft == 2,
                  "T2.4: Top boundary reflection inverts vz to positive",
                  "vz=" + std::to_string(canvas.activeProjectiles[0].vz));

        // 2.2.4 Bottom wall bounce (z > 1004.0)
        canvas.activeProjectiles.clear();
        Projectile sBottom;
        sBottom.x = 500.0; sBottom.z = 1002.0; sBottom.vx = 0.0; sBottom.vz = 100.0;
        sBottom.bouncesLeft = 3; sBottom.active = true; sBottom.lifeTime = 6.0;
        canvas.activeProjectiles.push_back(sBottom);
        canvas.simulateTick(0.05);

        CHALLENGE(!canvas.activeProjectiles.empty() &&
                  canvas.activeProjectiles[0].vz < 0.0 &&
                  canvas.activeProjectiles[0].bouncesLeft == 2,
                  "T2.5: Bottom boundary reflection inverts vz to negative",
                  "vz=" + std::to_string(canvas.activeProjectiles[0].vz));
    }

    // 2.3 Bounce Exhaustion & Lifetime Expiration
    {
        // 3rd bounce shatters shell
        canvas.activeProjectiles.clear();
        Projectile sExpire;
        sExpire.x = 22.0; sExpire.z = 500.0; sExpire.vx = -100.0; sExpire.vz = 0.0;
        sExpire.bouncesLeft = 1; sExpire.active = true; sExpire.lifeTime = 5.0;
        canvas.activeProjectiles.push_back(sExpire);
        canvas.simulateTick(0.05);

        CHALLENGE(canvas.activeProjectiles.empty(),
                  "T2.6: Shell shatters and deactivates after exhausting 3 bounces",
                  "Active shells: " + std::to_string(canvas.activeProjectiles.size()));

        // Lifetime expiration
        Projectile sTimed;
        sTimed.x = 500.0; sTimed.z = 500.0; sTimed.vx = 50.0; sTimed.vz = 0.0;
        sTimed.bouncesLeft = 3; sTimed.active = true; sTimed.lifeTime = 0.02;
        canvas.activeProjectiles.push_back(sTimed);
        canvas.simulateTick(0.05);

        CHALLENGE(canvas.activeProjectiles.empty(),
                  "T2.7: Shell deactivates upon reaching lifeTime <= 0",
                  "Active shells: " + std::to_string(canvas.activeProjectiles.size()));
    }

    // 2.4 AI Collision Hit & Spinout Trigger
    {
        canvas.activeProjectiles.clear();
        canvas.aiOpponents.clear();

        KartEntity aiTarget;
        aiTarget.id = 1;
        aiTarget.x = 600.0;
        aiTarget.z = 500.0;
        aiTarget.speed = 100.0;
        aiTarget.isSpinning = false;
        canvas.aiOpponents.push_back(aiTarget);

        Projectile shell;
        shell.x = 590.0; // 10 units away (within 16.0 collision radius)
        shell.z = 500.0;
        shell.vx = 200.0;
        shell.vz = 0.0;
        shell.active = true;
        shell.ownerId = 0;
        shell.bouncesLeft = 3;
        canvas.activeProjectiles.push_back(shell);

        canvas.simulateTick(0.01);

        CHALLENGE(canvas.activeProjectiles.empty(),
                  "T2.8: Shell consumed upon colliding with AI opponent",
                  "Shell not consumed");
        CHALLENGE(canvas.aiOpponents[0].isSpinning && canvas.aiOpponents[0].spinTimer == 1.2,
                  "T2.9: AI opponent enters 1.2s spinout on shell collision",
                  "isSpinning=" + std::to_string(canvas.aiOpponents[0].isSpinning));
        CHALLENGE(canvas.aiOpponents[0].speed < 40.0,
                  "T2.10: AI opponent speed reduced to 35% on shell collision",
                  "Speed=" + std::to_string(canvas.aiOpponents[0].speed));
    }
}

// ============================================================================
// SUITE 3: INK BLOOPER EFFECT & SCREEN OVERLAY ISOLATION STRESS TEST
// ============================================================================
void testInkBlooperEffect(EmpiricalCanvas &canvas) {
    std::cout << "\n===============================================================\n";
    std::cout << " SUITE 3: Ink Blooper Effect & Screen Overlay Isolation\n";
    std::cout << "===============================================================\n";

    canvas.setGameState(GameState::RACING);
    canvas.setGameMode(GameMode::PLAY_VS_AI);
    canvas.setTrackLevel(TrackLevel::DONUT_PLAINS);
    canvas.startRace();

    // 3.1 Player Clean Windshield Assertion
    canvas.heldItem = PlayerItem::INK_BLOOPER;
    canvas.useHeldItem();

    CHALLENGE(canvas.inkTimer == 0.0,
              "T3.1: Player 1 inkTimer strictly 0.0s after using Blooper",
              "Player inkTimer=" + std::to_string(canvas.inkTimer));
    CHALLENGE(!canvas.player1.isInked && canvas.player1.inkTimer == 0.0,
              "T3.2: Player 1 entity isInked=false and inkTimer=0.0s",
              "player1.isInked=" + std::to_string(canvas.player1.isInked));

    // 3.2 Screen Buffer Zero-Pixel Modification Assertion
    canvas.screenBuffer.fill(qRgb(220, 120, 40));
    canvas.renderInkOverlay(); // Call ink overlay renderer

    int modifiedPixels = 0;
    for (int y = 0; y < canvas.screenBuffer.height(); ++y) {
        const QRgb *scan = reinterpret_cast<const QRgb*>(canvas.screenBuffer.constScanLine(y));
        for (int x = 0; x < canvas.screenBuffer.width(); ++x) {
            if (scan[x] != qRgb(220, 120, 40)) {
                modifiedPixels++;
            }
        }
    }
    CHALLENGE(modifiedPixels == 0,
              "T3.3: Exactly 0 ink splatter pixels rendered on Player 1 screenBuffer (100% clean windshield)",
              "Modified pixels=" + std::to_string(modifiedPixels));

    // 3.3 AI Opponents Targeting & Speed Reduction
    bool allAIInked = true;
    for (const auto &ai : canvas.aiOpponents) {
        if (!ai.isInked || ai.inkTimer != 4.0) {
            allAIInked = false;
        }
    }
    CHALLENGE(allAIInked,
              "T3.4: All active AI opponents receive isInked=true and inkTimer=4.0s",
              "Not all AI opponents properly inked");

    // 3.4 Multiplayer Mode Targeting Isolation
    canvas.setGameMode(GameMode::MULTIPLAYER);
    canvas.startRace();
    canvas.player2.speed = 100.0;
    canvas.player2.isInked = false;
    canvas.player2.inkTimer = 0.0;

    canvas.heldItem = PlayerItem::INK_BLOOPER;
    canvas.useHeldItem();

    CHALLENGE(canvas.inkTimer == 0.0,
              "T3.5: Player 1 windshield remains clean in Multiplayer",
              "P1 inkTimer=" + std::to_string(canvas.inkTimer));
    CHALLENGE(canvas.player2.isInked && canvas.player2.inkTimer == 4.0 && canvas.player2.speed == 65.0,
              "T3.6: Player 2 targeted by Blooper (isInked=true, inkTimer=4.0s, speed cut by 35%)",
              "P2 targeting failed");
}

// ============================================================================
// SUITE 4: BANANA DROP & OWNER IMMUNITY WINDOW STRESS TEST
// ============================================================================
void testBananaDropAndImmunity(EmpiricalCanvas &canvas) {
    std::cout << "\n===============================================================\n";
    std::cout << " SUITE 4: Banana Drop & Owner Immunity Window Stress Test\n";
    std::cout << "===============================================================\n";

    canvas.setGameState(GameState::RACING);
    canvas.setGameMode(GameMode::PLAY_VS_AI);
    canvas.setTrackLevel(TrackLevel::DONUT_PLAINS);
    canvas.startRace();

    // 4.1 Banana Placement Geometry Behind Kart
    canvas.worldSprites.clear();
    canvas.kartX = 500.0;
    canvas.kartZ = 300.0;
    canvas.kartAngle = 0.0; // Heading East
    canvas.heldItem = PlayerItem::BANANA;

    canvas.useHeldItem();

    CHALLENGE(canvas.worldSprites.size() == 1,
              "T4.1: Deploying Banana spawns new WorldSprite in worldSprites",
              "Sprite count=" + std::to_string(canvas.worldSprites.size()));

    const auto &banana = canvas.worldSprites[0];
    double expectedX = 500.0 - 16.0; // 484.0
    double expectedZ = 300.0;
    CHALLENGE(std::abs(banana.x - expectedX) < 1e-3 && std::abs(banana.z - expectedZ) < 1e-3,
              "T4.2: Banana dropped exactly 16 units behind kart heading",
              "Coordinates (" + std::to_string(banana.x) + ", " + std::to_string(banana.z) + ")");
    CHALLENGE(banana.ownerId == 0 && banana.immunityTimer == 1.2 && banana.active,
              "T4.3: Banana initialized with ownerId=0, immunityTimer=1.2s, active=true",
              "Timer=" + std::to_string(banana.immunityTimer));

    // 4.2 Adversarial Immediate Braking / Reversing Immunity Stress Test
    // Move kart directly on top of the freshly dropped banana
    canvas.kartX = 484.0;
    canvas.kartZ = 300.0;
    canvas.kartSpeed = -20.0; // Reversing directly over banana
    canvas.isSpinning = false;
    canvas.spinTimer = 0.0;

    // Simulate 1.0s (within 1.2s immunity window)
    for (int i = 0; i < 20; ++i) { // 20 * 0.05 = 1.0s
        canvas.updatePhysics(0.05);
        canvas.updateItemSystem(0.05);
    }

    CHALLENGE(!canvas.isSpinning && canvas.worldSprites[0].active,
              "T4.4: Owner braking/reversing onto banana within 1.2s DOES NOT trigger self-spinout",
              "Self-spinout triggered during grace period");

    // Advance 0.35s more (total 1.35s > 1.2s) -> immunity expires
    for (int i = 0; i < 7; ++i) {
        canvas.updatePhysics(0.05);
        canvas.updateItemSystem(0.05);
    }

    CHALLENGE(canvas.isSpinning && !canvas.worldSprites[0].active,
              "T4.5: Owner kart spins out and consumes banana when contacting AFTER 1.2s immunity expires",
              "isSpinning=" + std::to_string(canvas.isSpinning) + ", banana active=" + std::to_string(canvas.worldSprites[0].active));

    // 4.3 Trailing AI Opponent Collision Hit & Spinout
    canvas.startRace();
    canvas.worldSprites.clear();
    canvas.kartX = 500.0; canvas.kartZ = 300.0; canvas.kartAngle = 0.0;
    canvas.heldItem = PlayerItem::BANANA;
    canvas.useHeldItem();

    // Place trailing AI directly over the banana
    canvas.aiOpponents.clear();
    KartEntity aiTrailing;
    aiTrailing.id = 1;
    aiTrailing.x = 484.0; // directly on banana
    aiTrailing.z = 300.0;
    aiTrailing.speed = 100.0;
    aiTrailing.isSpinning = false;
    canvas.aiOpponents.push_back(aiTrailing);

    // AI is ID 1 (not owner 0) -> immunity does NOT apply to AI!
    canvas.updateAIOpponents(0.05);

    CHALLENGE(!canvas.worldSprites[0].active,
              "T4.6: Banana consumed upon collision with trailing AI opponent",
              "Banana still active");
    CHALLENGE(canvas.aiOpponents[0].isSpinning && canvas.aiOpponents[0].spinTimer == 1.2,
              "T4.7: Trailing AI opponent enters 1.2s spinout upon running over banana",
              "AI isSpinning=" + std::to_string(canvas.aiOpponents[0].isSpinning));
    CHALLENGE(canvas.aiOpponents[0].speed < 40.0,
              "T4.8: Trailing AI speed reduced to 35% on banana impact",
              "AI speed=" + std::to_string(canvas.aiOpponents[0].speed));
}

// ============================================================================
// MAIN RUNNER
// ============================================================================
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    std::cout << "===============================================================\n";
    std::cout << " RETROKART 3D: EMPIRICAL CHALLENGER STRESS HARNESS (MILESTONE 2)\n";
    std::cout << " Verification of AI Waypoints, Shell Physics, Blooper & Banana\n";
    std::cout << "===============================================================\n";

    EmpiricalCanvas canvas;
    canvas.resize(400, 300);

    testAIWaypointNavigation(canvas);
    testShellProjectilePhysics(canvas);
    testInkBlooperEffect(canvas);
    testBananaDropAndImmunity(canvas);

    std::cout << "\n===============================================================\n";
    std::cout << "                 CHALLENGE SUITE SUMMARY                       \n";
    std::cout << "===============================================================\n";
    std::cout << " Total Challenges Executed: " << g_stats.total << "\n";
    std::cout << " Passed Challenges:        " << g_stats.passed << " ("
              << (g_stats.total > 0 ? (g_stats.passed * 100 / g_stats.total) : 0) << "%)\n";
    std::cout << " Failed Challenges:        " << g_stats.failed << "\n";
    std::cout << "===============================================================\n";

    if (g_stats.failed > 0) {
        std::cerr << "\n>>> VERDICT: REQUEST_CHANGES <<<\n";
        std::cerr << "Failures encountered:\n";
        for (const auto &msg : g_stats.failures) {
            std::cerr << "  * " << msg << "\n";
        }
        return 1;
    } else {
        std::cout << "\n>>> VERDICT: APPROVE <<<\n";
        std::cout << "All empirical stress tests passed with zero regressions.\n";
        return 0;
    }
}
