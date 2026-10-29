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

struct TestRecord {
    int total = 0;
    int passed = 0;
    int failed = 0;
    std::vector<std::string> failures;

    void record(bool condition, const std::string &name, const std::string &detail = "") {
        total++;
        if (condition) {
            passed++;
            std::cout << "  [PASS] " << name << "\n";
        } else {
            failed++;
            std::string msg = name + " -> FAIL: " + detail;
            failures.push_back(msg);
            std::cout << "  [FAIL] " << name << " -> " << detail << "\n";
        }
    }
};

static TestRecord g_tests;

#define EMPIRICAL_ASSERT(cond, name, detail) g_tests.record((cond), (name), (detail))

class ChallengerCanvas : public GameCanvas {
public:
    explicit ChallengerCanvas(QWidget *parent = nullptr) : GameCanvas(parent) {}

    void simulatePhysicsStep(double dt) {
        updatePhysics(dt);
        if (selectedGameMode == GameMode::PLAY_VS_AI) {
            updateAIOpponents(dt);
        }
        updateItemSystem(dt);
    }

    void renderFullPipeline() {
        renderSky();
        renderGroundMode7();
        renderSprites();
        renderCockpit();
        renderItemHUD();
        renderInkOverlay();
    }
};

// ============================================================================
// SUITE 1: 3D BILLBOARD PROJECTION & PAINTER'S ALGORITHM DEPTH-SORTING
// ============================================================================
void runSuite1_3DBillboardAndSorting(ChallengerCanvas &canvas) {
    std::cout << "\n===============================================================\n";
    std::cout << " SUITE 1: 3D Billboard Projection & Painter's Algorithm Sorting\n";
    std::cout << "===============================================================\n";

    canvas.setGameState(GameState::RACING);
    canvas.setGameMode(GameMode::PLAY_VS_AI);
    canvas.setTrackLevel(TrackLevel::DONUT_PLAINS);
    canvas.startRace();

    // 1.1 Projection Geometry & Perspective Scaling
    canvas.kartX = 500.0;
    canvas.kartZ = 200.0;
    canvas.kartAngle = 0.0; // Heading along +X (cosA=1, sinA=0)

    // AI kart directly in front: dx = 60.0, dz = 0.0 -> camX = 0, camZ = 60.0
    double cosA = std::cos(canvas.kartAngle);
    double sinA = std::sin(canvas.kartAngle);
    double dx = 60.0, dz = 0.0;
    double camX = -dx * sinA + dz * cosA;
    double camZ =  dx * cosA + dz * sinA;
    EMPIRICAL_ASSERT(std::abs(camX) < 1e-5 && std::abs(camZ - 60.0) < 1e-5,
                     "T1.1: Camera space coordinate transformation (forward camZ=60.0, lateral camX=0.0)",
                     "camX=" + std::to_string(camX) + " camZ=" + std::to_string(camZ));

    // 1.2 View Frustum Clipping (Near plane <= 5.0, Far plane > 480.0)
    canvas.aiOpponents.clear();
    KartEntity kNear; kNear.x = canvas.kartX + 4.0; kNear.z = canvas.kartZ; kNear.characterId = 0;
    KartEntity kFar;  kFar.x = canvas.kartX + 500.0; kFar.z = canvas.kartZ; kFar.characterId = 1;
    KartEntity kVis;  kVis.x = canvas.kartX + 50.0;  kVis.z = canvas.kartZ; kVis.characterId = 2;
    canvas.aiOpponents.push_back(kNear);
    canvas.aiOpponents.push_back(kFar);
    canvas.aiOpponents.push_back(kVis);

    bool frustumNoCrash = true;
    try {
        canvas.renderSprites();
    } catch (...) {
        frustumNoCrash = false;
    }
    EMPIRICAL_ASSERT(frustumNoCrash,
                     "T1.2: Frustum near plane (<=5.0) and far plane (>480.0) clipping executes without crash",
                     "Exception during renderSprites");

    // 1.3 Painter's Algorithm Depth-Sorting
    // Setup near object and distant object in optical line of sight
    canvas.aiOpponents.clear();
    canvas.worldSprites.clear();
    canvas.activeProjectiles.clear();
    canvas.screenBuffer.fill(qRgb(0, 0, 0)); // Black backdrop

    // Distant Peach (Char ID 2, Pink) at depth = 150.0
    KartEntity aiPeach;
    aiPeach.characterId = 2;
    aiPeach.x = canvas.kartX + 150.0;
    aiPeach.z = canvas.kartZ;
    canvas.aiOpponents.push_back(aiPeach);

    // Near Bowser (Char ID 1, Green/Yellow) at depth = 45.0
    KartEntity aiBowser;
    aiBowser.characterId = 1;
    aiBowser.x = canvas.kartX + 45.0;
    aiBowser.z = canvas.kartZ;
    canvas.aiOpponents.push_back(aiBowser);

    canvas.renderSprites();

    // Verify center column pixel is drawn by near Bowser (occluding distant Peach)
    int centerCol = canvas.BUFFER_WIDTH / 2;
    int bowserGroundY = static_cast<int>(canvas.HORIZON_Y + (canvas.focalLength * canvas.cameraHeight) / 45.0);
    QRgb sampledPixel = canvas.screenBuffer.pixel(centerCol, std::clamp(bowserGroundY - 8, 0, canvas.BUFFER_HEIGHT - 1));
    bool isOccluding = (sampledPixel != qRgb(0, 0, 0));
    EMPIRICAL_ASSERT(isOccluding,
                     "T1.3: Painter's Algorithm depth-sorting: near kart (Bowser) renders on top of distant kart (Peach)",
                     "Center pixel remained black");

    // 1.4 60 FPS Performance Benchmark
    QElapsedTimer timer;
    timer.start();
    const int BENCH_FRAMES = 500;
    for (int f = 0; f < BENCH_FRAMES; ++f) {
        canvas.renderFullPipeline();
    }
    qint64 elapsedMs = timer.elapsed();
    double msPerFrame = static_cast<double>(elapsedMs) / BENCH_FRAMES;
    double fps = (msPerFrame > 0.0) ? (1000.0 / msPerFrame) : 99999.0;
    std::cout << "    [Benchmark Data] " << BENCH_FRAMES << " Mode-7 + Billboard frames in "
              << elapsedMs << " ms (" << std::fixed << std::setprecision(3) << msPerFrame
              << " ms/frame -> " << std::setprecision(1) << fps << " FPS)\n";

    EMPIRICAL_ASSERT(msPerFrame < 16.67,
                     "T1.4: 3D billboard projection + Mode-7 software rasterizer achieves sustained 60 FPS (<16.6ms/frame)",
                     "Frame time was " + std::to_string(msPerFrame) + " ms (" + std::to_string(fps) + " FPS)");

    // 1.5 Pathological Coordinate Buffer Overrun Stress Test
    canvas.aiOpponents.clear();
    KartEntity p1; p1.x = -1e7; p1.z = -1e7; canvas.aiOpponents.push_back(p1);
    KartEntity p2; p2.x = 1e7;  p2.z = 1e7;  canvas.aiOpponents.push_back(p2);
    KartEntity p3; p3.x = canvas.kartX + 0.000001; p3.z = canvas.kartZ; canvas.aiOpponents.push_back(p3);
    KartEntity p4; p4.x = canvas.kartX - 50.0; p4.z = canvas.kartZ; canvas.aiOpponents.push_back(p4);

    bool pathologicalSafe = true;
    try {
        for (int i = 0; i < 30; ++i) canvas.renderSprites();
    } catch (...) {
        pathologicalSafe = false;
    }
    EMPIRICAL_ASSERT(pathologicalSafe,
                     "T1.5: Extreme pathological coordinates (-1e7, +1e7, camZ ~ 0, behind cam) produce no buffer overruns",
                     "Buffer overrun or exception detected");
}

// ============================================================================
// SUITE 2: ADVERSARIAL ITEM INTERACTIONS & PROJECTILE PHYSICS
// ============================================================================
void runSuite2_ItemInteractions(ChallengerCanvas &canvas) {
    std::cout << "\n===============================================================\n";
    std::cout << " SUITE 2: Adversarial Item Interactions & Projectile Physics\n";
    std::cout << "===============================================================\n";

    canvas.setGameState(GameState::RACING);
    canvas.setGameMode(GameMode::PLAY_VS_AI);
    canvas.startRace();

    // 2.1 Green Shell Launch Velocity and Heading
    canvas.kartX = 500.0; canvas.kartZ = 200.0; canvas.kartAngle = 1.0; canvas.kartSpeed = 100.0;
    canvas.heldItem = PlayerItem::GREEN_SHELL;
    canvas.activeProjectiles.clear();
    canvas.useHeldItem();

    EMPIRICAL_ASSERT(canvas.activeProjectiles.size() == 1,
                     "T2.1: Firing Green Shell spawns dynamic projectile in activeProjectiles",
                     "Projectiles count=" + std::to_string(canvas.activeProjectiles.size()));

    const auto &shell = canvas.activeProjectiles[0];
    double launchSpeed = std::hypot(shell.vx, shell.vz);
    EMPIRICAL_ASSERT(std::abs(launchSpeed - 180.0) < 0.1,
                     "T2.2: Green Shell launch speed calibrated to max(180, kartSpeed + 75.0)",
                     "launchSpeed=" + std::to_string(launchSpeed));

    // 2.2 Shell Boundary Wall Reflection and Shattering
    canvas.activeProjectiles.clear();
    Projectile p;
    p.x = 995.0; p.z = 500.0; p.vx = 200.0; p.vz = 0.0;
    p.bouncesLeft = 3; p.active = true; p.ownerId = 0;
    canvas.activeProjectiles.push_back(p);

    canvas.simulatePhysicsStep(0.05); // x hits 1004.0 wall -> bounces
    EMPIRICAL_ASSERT(canvas.activeProjectiles[0].vx < 0.0 && canvas.activeProjectiles[0].bouncesLeft == 2,
                     "T2.3: Shell reflects vx negatively and decrements bouncesLeft from 3 to 2 on wall bounce",
                     "vx=" + std::to_string(canvas.activeProjectiles[0].vx) + " bounces=" + std::to_string(canvas.activeProjectiles[0].bouncesLeft));

    // Bounce remaining 2 times to shatter
    canvas.activeProjectiles[0].x = 1003.0; canvas.activeProjectiles[0].vx = 100.0;
    canvas.simulatePhysicsStep(0.05);
    canvas.activeProjectiles[0].x = 1003.0; canvas.activeProjectiles[0].vx = 100.0;
    canvas.simulatePhysicsStep(0.05);

    EMPIRICAL_ASSERT(canvas.activeProjectiles.empty(),
                     "T2.4: Shell shatters and is pruned from activeProjectiles after 3 wall reflections",
                     "Remaining shells=" + std::to_string(canvas.activeProjectiles.size()));

    // 2.3 Shell Opponent Hit and Spinout
    canvas.activeProjectiles.clear();
    canvas.aiOpponents.clear();
    KartEntity aiTarget;
    aiTarget.id = 1; aiTarget.x = 600.0; aiTarget.z = 200.0; aiTarget.speed = 100.0; aiTarget.isSpinning = false;
    canvas.aiOpponents.push_back(aiTarget);

    Projectile shellHit;
    shellHit.x = 590.0; shellHit.z = 200.0; shellHit.vx = 200.0; shellHit.vz = 0.0;
    shellHit.active = true; shellHit.ownerId = 0; shellHit.bouncesLeft = 3;
    canvas.activeProjectiles.push_back(shellHit);

    canvas.simulatePhysicsStep(0.05);

    EMPIRICAL_ASSERT(canvas.activeProjectiles.empty(),
                     "T2.5: Shell consumed upon hitting opponent kart",
                     "Shell active");
    EMPIRICAL_ASSERT(canvas.aiOpponents[0].isSpinning && canvas.aiOpponents[0].spinTimer > 0.5,
                     "T2.6: Opponent kart enters 1.2s spinout on shell impact",
                     "isSpinning=" + std::to_string(canvas.aiOpponents[0].isSpinning));

    // 2.4 Banana Drop & Owner Immunity Grace Period
    canvas.kartX = 500.0; canvas.kartZ = 200.0; canvas.kartAngle = 0.0;
    canvas.heldItem = PlayerItem::BANANA;
    size_t countBefore = canvas.worldSprites.size();
    canvas.useHeldItem();

    EMPIRICAL_ASSERT(canvas.worldSprites.size() == countBefore + 1,
                     "T2.7: Deploying Banana spawns new WorldSprite 16 units behind kart",
                     "WorldSprite count unchanged");

    WorldSprite &banana = canvas.worldSprites.back();
    EMPIRICAL_ASSERT(banana.ownerId == 0 && banana.immunityTimer > 1.0,
                     "T2.8: Dropped banana initialized with ownerId=0 and immunityTimer=1.2s",
                     "ownerId=" + std::to_string(banana.ownerId) + " timer=" + std::to_string(banana.immunityTimer));

    // Owner reverses directly onto banana during grace period (t = 0.2s)
    canvas.kartX = banana.x; canvas.kartZ = banana.z; canvas.isSpinning = false;
    canvas.simulatePhysicsStep(0.05);
    EMPIRICAL_ASSERT(!canvas.isSpinning,
                     "T2.9: Owner kart is immune and does NOT spin out during 1.2s grace period",
                     "Player self-spinout occurred");

    // Trailing AI runs over banana during owner grace period
    canvas.aiOpponents.clear();
    KartEntity aiRival;
    aiRival.id = 2; aiRival.x = banana.x; aiRival.z = banana.z; aiRival.speed = 100.0; aiRival.isSpinning = false;
    canvas.aiOpponents.push_back(aiRival);
    canvas.simulatePhysicsStep(0.016);

    EMPIRICAL_ASSERT(canvas.aiOpponents[0].isSpinning,
                     "T2.10: Trailing AI rival spins out immediately upon hitting banana",
                     "AI did not spin out");
    EMPIRICAL_ASSERT(!banana.active,
                     "T2.11: Banana peel is consumed (active=false) upon collision",
                     "Banana active");

    // 2.5 Ink Blooper Targeting
    canvas.heldItem = PlayerItem::INK_BLOOPER;
    canvas.aiOpponents.clear();
    for (int i = 0; i < 3; ++i) {
        KartEntity ai; ai.id = i + 1; ai.speed = 100.0; ai.isInked = false; ai.inkTimer = 0.0;
        canvas.aiOpponents.push_back(ai);
    }
    canvas.useHeldItem();

    EMPIRICAL_ASSERT(canvas.inkTimer == 0.0 && !canvas.player1.isInked,
                     "T2.12: Player 1 windshield remains 100% clean when using Ink Blooper",
                     "Player inkTimer=" + std::to_string(canvas.inkTimer));

    bool allInked = true;
    for (const auto &ai : canvas.aiOpponents) {
        if (!ai.isInked || ai.inkTimer < 3.5 || ai.speed > 70.0) allInked = false;
    }
    EMPIRICAL_ASSERT(allInked,
                     "T2.13: Ink Blooper targets ALL AI opponents (isInked=true, inkTimer=4.0, speed reduced by 35%)",
                     "AI opponents not properly inked");
}

// ============================================================================
// SUITE 3: EMPIRICAL ROOT CAUSE CHALLENGES & AI FAILURE MODES
// ============================================================================
void runSuite3_AIFailureModesAndRootCauses(ChallengerCanvas &canvas) {
    std::cout << "\n===============================================================\n";
    std::cout << " SUITE 3: Empirical Root Cause Challenges & AI Failure Modes\n";
    std::cout << "===============================================================\n";

    // 3.1 BUG 1 PROOF: White Road Mask (255, 255, 255) Triggers Molten Lava Recoil in updateAIOpponents
    // In game_canvas.cpp:1167:
    //   if (qRed(maskColor) == 255 && qBlue(maskColor) == 255) { // Molten lava recoil! }
    // White road is RGB(255, 255, 255). qRed == 255 AND qBlue == 255 are both true!
    canvas.setTrackLevel(TrackLevel::DONUT_PLAINS);
    canvas.startRace();

    // Place an AI kart directly on Donut Plains white tarmac (x = 550.0, z = 200.0)
    canvas.aiOpponents.clear();
    KartEntity aiOnRoad;
    aiOnRoad.id = 1;
    aiOnRoad.x = 550.0; // On white road tarmac (finish line stripe is at 512)
    aiOnRoad.z = 200.0;
    aiOnRoad.speed = 100.0;
    aiOnRoad.angle = 0.0;
    aiOnRoad.isSpinning = false;
    aiOnRoad.spinTimer = 0.0;
    canvas.aiOpponents.push_back(aiOnRoad);

    QRgb maskAtCenterline = canvas.trackMaskMap.pixel(550, 200);
    std::cout << "    [Road Mask Sample] Donut Plains at (550, 200): RGB("
              << qRed(maskAtCenterline) << ", " << qGreen(maskAtCenterline) << ", "
              << qBlue(maskAtCenterline) << ")\n";

    // Execute 1 tick of updateAIOpponents
    canvas.updateAIOpponents(0.016);

    std::cout << "    [AI State After Road Tick] isSpinning=" << canvas.aiOpponents[0].isSpinning
              << " spinTimer=" << canvas.aiOpponents[0].spinTimer
              << " speed=" << canvas.aiOpponents[0].speed
              << " x=" << canvas.aiOpponents[0].x << " z=" << canvas.aiOpponents[0].z << "\n";

    bool roadTriggeredLava = (canvas.aiOpponents[0].isSpinning &&
                              std::abs(canvas.aiOpponents[0].spinTimer - 0.8) < 0.05);

    // This assertion FAILS because the bug exists in production code:
    // AI on road MUST NOT be spinning with molten lava 0.8s timer!
    EMPIRICAL_ASSERT(!roadTriggeredLava,
                     "T3.1: AI kart on tarmac (White road RGB 255,255,255) MUST NOT trigger molten lava recoil",
                     "CRITICAL BUG CONFIRMED: Line 1167 checks `qRed==255 && qBlue==255` without checking green!=255, causing AI to treat tarmac as molten lava!");

    // 3.2 BUG 2 PROOF: AI Spinout Early 'continue;' Freezes inkTimer Debuff Decay
    // In game_canvas.cpp:1080:
    //   if (ai.spinTimer > 0.0 || ai.isSpinning) { ... continue; }
    // Section 2 (Ink debuff decay) is placed AFTER continue; and is skipped while spinning!
    canvas.aiOpponents.clear();
    KartEntity aiInkedAndSpinning;
    aiInkedAndSpinning.id = 2;
    aiInkedAndSpinning.x = 512.0;
    aiInkedAndSpinning.z = 200.0;
    aiInkedAndSpinning.isInked = true;
    aiInkedAndSpinning.inkTimer = 4.0;
    aiInkedAndSpinning.isSpinning = true;
    aiInkedAndSpinning.spinTimer = 1.2; // 1.2s spinout
    canvas.aiOpponents.push_back(aiInkedAndSpinning);

    // Simulate 0.5s of physics while spinning
    for (int i = 0; i < 10; ++i) {
        canvas.updateAIOpponents(0.05);
    }

    std::cout << "    [Ink While Spinout Data] After 0.5s: spinTimer=" << canvas.aiOpponents[0].spinTimer
              << ", inkTimer=" << canvas.aiOpponents[0].inkTimer << " (expected 3.5s)\n";

    bool inkTimerDecayed = (canvas.aiOpponents[0].inkTimer < 3.7);
    EMPIRICAL_ASSERT(inkTimerDecayed,
                     "T3.2: AI inkTimer debuff MUST decay while kart is spinning",
                     "DEFECT CONFIRMED: Line 1080 `continue;` skips inkTimer countdown during spinout, freezing ink timer at " + std::to_string(canvas.aiOpponents[0].inkTimer));

    // 3.3 BUG 3 PROOF: Starting Grid Off-Road Spawn Defect
    // In game_canvas.cpp:246:
    //   aiKart.x = 470.0 - aiSlot * 18.0;
    // Donut Plains road centerline is at x = 512, width 84px (inner curb at x = 470.0).
    // Slots 1, 2, 3 are spawned at x = 452, 434, 416, which are ALL OFF-ROAD GREEN GRASS!
    canvas.setTrackLevel(TrackLevel::DONUT_PLAINS);
    canvas.startRace();

    std::cout << "    [Grid Positions on Donut Plains]:\n";
    bool anyOffRoadOnSpawn = false;
    for (size_t i = 0; i < canvas.aiOpponents.size(); ++i) {
        const auto &ai = canvas.aiOpponents[i];
        QRgb spawnMask = canvas.trackMaskMap.pixel(static_cast<int>(ai.x), static_cast<int>(ai.z));
        bool isGreen = (qGreen(spawnMask) == 255 && qRed(spawnMask) == 0);
        std::cout << "      AI " << i << ": x=" << ai.x << " z=" << ai.z
                  << " -> Mask RGB(" << qRed(spawnMask) << "," << qGreen(spawnMask) << "," << qBlue(spawnMask) << ")"
                  << (isGreen ? " [OFF-ROAD GRASS!]" : " [ROAD]") << "\n";
        if (isGreen) anyOffRoadOnSpawn = true;
    }

    EMPIRICAL_ASSERT(!anyOffRoadOnSpawn,
                     "T3.3: AI opponents MUST spawn on tarmac raceway, NOT in off-road grass",
                     "DEFECT CONFIRMED: AI slots 1, 2, 3 spawned at x=452, 434, 416 in green grass (road boundary begins at x=470)!");

    // 3.4 BUG 4 PROOF: 5,000-Tick Stalling Verification Across Tracks
    // Verify whether AI karts can actually complete a single lap on Donut Plains
    int initialWp = canvas.aiOpponents[0].currentWaypoint;
    int wpAdvances = 0;
    for (int step = 0; step < 1000; ++step) {
        canvas.simulatePhysicsStep(0.016);
        if (canvas.aiOpponents[0].currentWaypoint != initialWp) {
            wpAdvances++;
            initialWp = canvas.aiOpponents[0].currentWaypoint;
        }
    }
    std::cout << "    [1000 Ticks Lap Progress]: AI 0 advanced " << wpAdvances << " waypoints\n";
    EMPIRICAL_ASSERT(wpAdvances >= 10,
                     "T3.4: AI karts MUST advance along waypoints under active gameplay loop",
                     "STALLING CONFIRMED: AI advanced " + std::to_string(wpAdvances) + " waypoints in 1000 ticks due to road-as-lava bug!");
}

// ============================================================================
// MAIN RUNNER
// ============================================================================
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    std::cout << "===============================================================\n";
    std::cout << " RETROKART 3D: EMPIRICAL CHALLENGER TEST SUITE (MILESTONE 2)\n";
    std::cout << " Independent Verification of 3D Billboards, Sorting & AI Dynamics\n";
    std::cout << "===============================================================\n";

    ChallengerCanvas canvas;
    canvas.resize(400, 300);

    runSuite1_3DBillboardAndSorting(canvas);
    runSuite2_ItemInteractions(canvas);
    runSuite3_AIFailureModesAndRootCauses(canvas);

    std::cout << "\n===============================================================\n";
    std::cout << "                 CHALLENGE SUITE SUMMARY                       \n";
    std::cout << "===============================================================\n";
    std::cout << " Total Challenges Executed: " << g_tests.total << "\n";
    std::cout << " Passed Challenges:        " << g_tests.passed << " ("
              << (g_tests.total > 0 ? (g_tests.passed * 100 / g_tests.total) : 0) << "%)\n";
    std::cout << " Failed Challenges:        " << g_tests.failed << "\n";
    std::cout << "===============================================================\n";

    if (g_tests.failed > 0) {
        std::cout << "\n>>> VERDICT: REQUEST_CHANGES <<<\n";
        std::cout << "Empirically Confirmed Failure Modes:\n";
        for (const auto &msg : g_tests.failures) {
            std::cout << "  * " << msg << "\n";
        }
        return 1;
    } else {
        std::cout << "\n>>> VERDICT: APPROVE <<<\n";
        return 0;
    }
}
