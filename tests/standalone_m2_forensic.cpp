#include <QApplication>
#include <QWidget>
#include <QImage>
#include <QTimer>
#include <QElapsedTimer>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QString>
#include <QPointF>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <set>
#include <cassert>
#include "sprite_atlas.h"

#define private public
#define protected public
#include "game_canvas.h"
#undef private
#undef protected

static int g_passCount = 0;
static int g_failCount = 0;

#define FORENSIC_ASSERT(cond, testName, desc) \
    do { \
        if (cond) { \
            std::cout << "  [PASS] " << testName << ": " << desc << std::endl; \
            g_passCount++; \
        } else { \
            std::cerr << "  [FAIL] " << testName << ": " << desc \
                      << " (Line " << __LINE__ << ")" << std::endl; \
            g_failCount++; \
        } \
    } while (0)

// Helper to advance game loop by dt
void advanceCanvas(GameCanvas &canvas, double dt)
{
    if (canvas.currentState == GameState::RACING) {
        canvas.updatePhysics(dt);
        if (canvas.selectedGameMode == GameMode::PLAY_VS_AI) {
            canvas.updateAIOpponents(dt);
        }
        canvas.updateItemSystem(dt);
    }
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    std::cout << "=======================================================\n";
    std::cout << " FORENSIC INTEGRITY AUDIT: MILESTONE 2 VERIFICATION   \n";
    std::cout << " Deep Empirical Testing of AI, Projectiles & Items    \n";
    std::cout << "=======================================================\n\n";

    GameCanvas canvas;

    // -------------------------------------------------------------
    // CHECK 1: Track Waypoints Persistent Geometry
    // -------------------------------------------------------------
    std::cout << "--- Check 1: Track Waypoint Loops & Coordinate Fidelity ---\n";
    {
        const auto &wpsDonut = canvas.getTrackWaypoints(0);
        FORENSIC_ASSERT(wpsDonut.size() == 16, "WP-1.1", "Donut Plains has 16 persistent waypoints");
        
        // Verify Donut Plains forms a smooth closed loop
        double closeDist = std::hypot(wpsDonut.front().x() - wpsDonut.back().x(),
                                      wpsDonut.front().y() - wpsDonut.back().y());
        FORENSIC_ASSERT(closeDist < 120.0, "WP-1.2", "Donut Plains waypoints form a continuous closed circuit");

        for (size_t i = 0; i < wpsDonut.size(); ++i) {
            FORENSIC_ASSERT(wpsDonut[i].x() >= 0 && wpsDonut[i].x() <= 1024 &&
                            wpsDonut[i].y() >= 0 && wpsDonut[i].y() <= 1024,
                            "WP-1.3", "Donut Plains waypoints within map boundaries [0, 1024]");
        }

        const auto &wpsChoco = canvas.getTrackWaypoints(1);
        FORENSIC_ASSERT(wpsChoco.size() == 20, "WP-1.4", "Choco Valley has 20 persistent waypoints");

        const auto &wpsBowser = canvas.getTrackWaypoints(2);
        FORENSIC_ASSERT(wpsBowser.size() == 24, "WP-1.5", "Bowser Castle has 24 persistent waypoints");

        // Verify setTrackLevel dynamically updates currentTrackWaypoints
        canvas.setTrackLevel(TrackLevel::CHOCO_VALLEY);
        FORENSIC_ASSERT(canvas.getTrackWaypoints().size() == 20, "WP-1.6", "currentTrackWaypoints synchronizes on setTrackLevel");
        canvas.setTrackLevel(TrackLevel::DONUT_PLAINS);
        FORENSIC_ASSERT(canvas.getTrackWaypoints().size() == 16, "WP-1.7", "currentTrackWaypoints restores to Donut Plains");
    }

    // -------------------------------------------------------------
    // CHECK 2: AI Fleet Initialization & Archetypes
    // -------------------------------------------------------------
    std::cout << "\n--- Check 2: AI Fleet Initialization & Roster Staggering ---\n";
    {
        canvas.setGameMode(GameMode::PLAY_VS_AI);
        canvas.setSelectedCharIndexP1(0); // P1 selects Mario (id 0)
        canvas.startRace();

        const auto &ai = canvas.getAIOpponents();
        FORENSIC_ASSERT(ai.size() == 3, "AI-2.1", "Exactly 3 AI opponents spawned in PLAY_VS_AI mode");

        std::set<int> aiCharacterIds;
        for (size_t i = 0; i < ai.size(); ++i) {
            FORENSIC_ASSERT(ai[i].isAI == true && ai[i].isHuman == false, "AI-2.2", "AI opponent flags properly initialized");
            FORENSIC_ASSERT(ai[i].characterId != 0, "AI-2.3", "AI opponents do not duplicate P1's Mario selection");
            aiCharacterIds.insert(ai[i].characterId);
        }
        FORENSIC_ASSERT(aiCharacterIds.size() == 3, "AI-2.4", "All 3 AI opponents have distinct characters (Bowser, Peach, Toad)");

        // Staggered grid placement verification
        FORENSIC_ASSERT(ai[0].x < 470.0 && ai[1].x < ai[0].x && ai[2].x < ai[1].x, "AI-2.5",
                        "AI starting grid positions are staggered sequentially behind start line");
    }

    // -------------------------------------------------------------
    // CHECK 3: Pure Pursuit AI Navigation & Trigonometric Integration
    // -------------------------------------------------------------
    std::cout << "\n--- Check 3: Authentic Pure Pursuit Navigation & Physics ---\n";
    {
        canvas.setGameMode(GameMode::PLAY_VS_AI);
        canvas.setGameState(GameState::RACING);
        canvas.startRace();

        std::cout << "  [DEBUG] Before loop: AI count = " << canvas.aiOpponents.size()
                  << ", currentState = " << static_cast<int>(canvas.currentState)
                  << ", mode = " << static_cast<int>(canvas.selectedGameMode) << "\n";

        if (!canvas.aiOpponents.empty()) {
            std::cout << "  [DEBUG] AI[0]: x=" << canvas.aiOpponents[0].x
                      << ", z=" << canvas.aiOpponents[0].z
                      << ", speed=" << canvas.aiOpponents[0].speed
                      << ", wp=" << canvas.aiOpponents[0].currentWaypoint
                      << ", isSpinning=" << canvas.aiOpponents[0].isSpinning
                      << ", spinTimer=" << canvas.aiOpponents[0].spinTimer
                      << ", maxSpeed=" << canvas.aiOpponents[0].maxSpeed
                      << ", accel=" << canvas.aiOpponents[0].accel << "\n";
        }

        double initX = canvas.aiOpponents[0].x;
        double initZ = canvas.aiOpponents[0].z;

        // Step simulation forward 150 ticks (dt = 0.016s)
        for (int frame = 0; frame < 150; ++frame) {
            advanceCanvas(canvas, 0.016);
        }

        std::cout << "  [DEBUG] After 150 ticks: AI[0]: x=" << canvas.aiOpponents[0].x
                  << ", z=" << canvas.aiOpponents[0].z
                  << ", speed=" << canvas.aiOpponents[0].speed
                  << ", wp=" << canvas.aiOpponents[0].currentWaypoint << "\n";

        double movedX = canvas.aiOpponents[0].x;
        double movedZ = canvas.aiOpponents[0].z;
        double distTraveled = std::hypot(movedX - initX, movedZ - initZ);

        FORENSIC_ASSERT(distTraveled > 20.0, "AI-3.1", "AI kart actively navigates track (non-stationary, genuine movement)");
        FORENSIC_ASSERT(canvas.aiOpponents[0].speed > 20.0, "AI-3.2", "AI kart maintains authentic forward speed");

        // Verify Waypoint advancement logic: distance < 35 triggers advancement
        int targetWp = canvas.aiOpponents[0].currentWaypoint;
        const auto &wps = canvas.currentTrackWaypoints;
        // Position AI right on top of target waypoint
        canvas.aiOpponents[0].x = wps[targetWp].x();
        canvas.aiOpponents[0].z = wps[targetWp].y();
        canvas.updateAIOpponents(0.016);
        FORENSIC_ASSERT(canvas.aiOpponents[0].currentWaypoint == (targetWp + 1) % static_cast<int>(wps.size()),
                        "AI-3.3", "AI waypoint advances when proximity < 35.0 units");

        // Verify Pure Pursuit Angle Step Limitation (turnRate * dt)
        double testDt = 0.016;
        canvas.aiOpponents[0].angle = 0.0;
        // Position waypoint perpendicular to heading (dx = 0, dz = 100 -> desiredAngle = pi/2)
        int lookaheadIdx = (canvas.aiOpponents[0].currentWaypoint + 2) % static_cast<int>(wps.size());
        canvas.currentTrackWaypoints[lookaheadIdx] = QPointF(canvas.aiOpponents[0].x, canvas.aiOpponents[0].z + 100.0);
        double maxTurnAllowed = canvas.aiOpponents[0].turnRate * testDt;
        canvas.updateAIOpponents(testDt);
        FORENSIC_ASSERT(canvas.aiOpponents[0].angle <= maxTurnAllowed + 1e-4, "AI-3.4",
                        "AI heading rotation rate is properly bounded by turnRate * dt (no instantaneous snapping)");

        // Verify Cornering Throttle Feathering & Braking
        // Desired angle delta > 0.65 rad should cut throttle and engage braking when speed > 80
        canvas.aiOpponents[0].speed = 120.0;
        canvas.aiOpponents[0].angle = 0.0;
        // Target directly behind (delta = pi > 0.65 rad)
        canvas.currentTrackWaypoints[lookaheadIdx] = QPointF(canvas.aiOpponents[0].x - 100.0, canvas.aiOpponents[0].z);
        double preSpeed = canvas.aiOpponents[0].speed;
        canvas.updateAIOpponents(0.05);
        FORENSIC_ASSERT(canvas.aiOpponents[0].speed < preSpeed, "AI-3.5",
                        "AI initiates cornering braking/throttle feathering when angular deviation > 0.65 rad");

        // Restore track waypoints
        canvas.setTrackLevel(TrackLevel::DONUT_PLAINS);
    }

    // -------------------------------------------------------------
    // CHECK 4: Green Shell Projectile Physics, Boundary Bounce & AI Collision
    // -------------------------------------------------------------
    std::cout << "\n--- Check 4: Green Shell Dynamic Projectile & Bouncing ---\n";
    {
        canvas.setGameMode(GameMode::PLAY_VS_AI);
        canvas.setGameState(GameState::RACING);
        canvas.startRace();
        canvas.activeProjectiles.clear();

        // 1. Launch green shell forward
        canvas.kartX = 500.0;
        canvas.kartZ = 200.0;
        canvas.kartAngle = 0.0; // facing +X
        canvas.kartSpeed = 100.0;
        canvas.heldItem = PlayerItem::GREEN_SHELL;
        canvas.useHeldItem();

        FORENSIC_ASSERT(canvas.activeProjectiles.size() == 1, "GS-4.1", "Green shell spawned into activeProjectiles");
        auto shell = canvas.activeProjectiles[0];
        FORENSIC_ASSERT(shell.active == true, "GS-4.2", "Spawned green shell is active");
        FORENSIC_ASSERT(shell.vx >= 175.0, "GS-4.3", "Green shell has high-speed forward velocity (v >= 175)");
        FORENSIC_ASSERT(shell.bouncesLeft == 3, "GS-4.4", "Green shell starts with exactly 3 wall bounces");

        // 2. Trajectory Integration
        double prevX = canvas.activeProjectiles[0].x;
        canvas.updateItemSystem(0.05);
        FORENSIC_ASSERT(canvas.activeProjectiles[0].x > prevX + 8.0, "GS-4.5", "Green shell position genuinely integrates along velocity");

        // 3. Boundary Bouncing Test
        canvas.activeProjectiles[0].x = 1003.0; // Near outer boundary (1004.0)
        canvas.activeProjectiles[0].vx = 200.0;  // Moving right toward wall
        canvas.updateItemSystem(0.02); // x reaches 1007.0 -> bounce triggered
        FORENSIC_ASSERT(canvas.activeProjectiles[0].vx < 0.0, "GS-4.6", "Green shell reflects velocity vx on boundary wall impact");
        FORENSIC_ASSERT(canvas.activeProjectiles[0].bouncesLeft == 2, "GS-4.7", "Green shell decrements bouncesLeft upon bounce");

        // 4. Collision with AI Opponent
        canvas.activeProjectiles.clear();
        canvas.activeProjectiles.push_back({ 500.0, 300.0, 0.0, 150.0, 3, true, 0, 6.0 }); // moving +Z
        canvas.aiOpponents[0].x = 500.0;
        canvas.aiOpponents[0].z = 308.0; // Within 16.0 collision radius
        canvas.aiOpponents[0].speed = 100.0;
        canvas.aiOpponents[0].isSpinning = false;

        canvas.updateItemSystem(0.016);
        FORENSIC_ASSERT(canvas.activeProjectiles.empty() || !canvas.activeProjectiles[0].active,
                        "GS-4.8", "Green shell is consumed/deactivated on AI collision");
        FORENSIC_ASSERT(canvas.aiOpponents[0].isSpinning == true, "GS-4.9", "AI opponent enters spinout on shell collision");
        FORENSIC_ASSERT(canvas.aiOpponents[0].spinTimer >= 1.0, "GS-4.10", "AI opponent spinTimer is set to >= 1.0s");
        FORENSIC_ASSERT(canvas.aiOpponents[0].speed <= 40.0, "GS-4.11", "AI opponent speed is penalized on shell hit");
    }

    // -------------------------------------------------------------
    // CHECK 5: Ink Blooper Fixed Targeting & Debuff Redirection
    // -------------------------------------------------------------
    std::cout << "\n--- Check 5: Ink Blooper Clean Windshield & AI Debuffs ---\n";
    {
        canvas.setGameMode(GameMode::PLAY_VS_AI);
        canvas.setGameState(GameState::RACING);
        canvas.startRace();

        canvas.inkTimer = 0.0;
        canvas.player1.isInked = false;
        canvas.player1.inkTimer = 0.0;

        for (auto &ai : canvas.aiOpponents) {
            ai.speed = 100.0;
            ai.isInked = false;
            ai.inkTimer = 0.0;
        }

        canvas.heldItem = PlayerItem::INK_BLOOPER;
        canvas.useHeldItem();

        // Verification: Player 1 windshield MUST BE 100% CLEAN
        FORENSIC_ASSERT(canvas.inkTimer == 0.0, "IB-5.1", "Player windshield inkTimer is 0.0 (CLEAN windshield)");
        FORENSIC_ASSERT(canvas.player1.isInked == false, "IB-5.2", "Player1 isInked flag is FALSE");

        // Verification: ALL AI opponents must be debuffed
        bool allAiInked = true;
        bool allAiSlowed = true;
        for (const auto &ai : canvas.aiOpponents) {
            if (!ai.isInked || std::abs(ai.inkTimer - 4.0) > 0.01) allAiInked = false;
            if (ai.speed > 70.0) allAiSlowed = false; // 100 * 0.65 = 65.0
        }
        FORENSIC_ASSERT(allAiInked, "IB-5.3", "All AI opponents received isInked=true and inkTimer=4.0s");
        FORENSIC_ASSERT(allAiSlowed, "IB-5.4", "All AI opponents received immediate 35% speed penalty");

        // Verification: Timer decay & clean recovery (dt clamped to 0.05s in updateAIOpponents, so 100 frames = 5.0s)
        for (int frame = 0; frame < 100; ++frame) {
            advanceCanvas(canvas, 0.05); // 5.0 seconds of simulation
        }
        bool allRecovered = true;
        for (const auto &ai : canvas.aiOpponents) {
            if (ai.isInked || ai.inkTimer > 0.0) allRecovered = false;
        }
        FORENSIC_ASSERT(allRecovered, "IB-5.5", "AI opponents cleanly recover after ink debuff duration expires");
    }

    // -------------------------------------------------------------
    // CHECK 6: Banana Drop, Owner Immunity & AI Collision
    // -------------------------------------------------------------
    std::cout << "\n--- Check 6: Banana Drop, Owner Immunity & AI Collision ---\n";
    {
        canvas.setGameMode(GameMode::PLAY_VS_AI);
        canvas.setGameState(GameState::RACING);
        canvas.startRace();

        canvas.kartX = 500.0;
        canvas.kartZ = 200.0;
        canvas.kartAngle = 0.0; // facing +X
        canvas.isSpinning = false;
        canvas.heldItem = PlayerItem::BANANA;
        canvas.useHeldItem();

        // Verify banana dropped behind kart
        FORENSIC_ASSERT(!canvas.worldSprites.empty(), "BN-6.1", "Banana spawned into worldSprites");
        const auto &banana = canvas.worldSprites.back();
        FORENSIC_ASSERT(banana.type == SpriteType::BANANA_DROPPED, "BN-6.2", "Spawned sprite is BANANA_DROPPED");
        FORENSIC_ASSERT(banana.x < 500.0, "BN-6.3", "Banana dropped behind player kart heading");
        FORENSIC_ASSERT(banana.ownerId == 0, "BN-6.4", "Banana assigned ownerId = 0 (Player 1)");
        FORENSIC_ASSERT(banana.immunityTimer >= 1.1, "BN-6.5", "Banana initialized with owner immunity timer >= 1.1s");

        // Owner Immunity Test: player touching own banana during immunity window does NOT spin out
        canvas.kartX = banana.x;
        canvas.kartZ = banana.y;
        canvas.updatePhysics(0.1);
        FORENSIC_ASSERT(canvas.isSpinning == false, "BN-6.6", "Player does NOT spin out on own banana during immunity window");

        // AI Collision Test: AI touching banana spins out and consumes banana
        canvas.worldSprites.clear();
        canvas.worldSprites.push_back({ 500.0, 300.0, 0.0, SpriteType::BANANA_DROPPED, true, 0.0, 0, 0.0 }); // immunity expired
        canvas.aiOpponents[0].x = 500.0;
        canvas.aiOpponents[0].z = 300.0; // Direct contact
        canvas.aiOpponents[0].speed = 100.0;
        canvas.aiOpponents[0].isSpinning = false;

        canvas.updateAIOpponents(0.016);
        FORENSIC_ASSERT(canvas.worldSprites[0].active == false, "BN-6.7", "Banana peel consumed on AI collision");
        FORENSIC_ASSERT(canvas.aiOpponents[0].isSpinning == true, "BN-6.8", "AI opponent spins out on banana collision");
        FORENSIC_ASSERT(canvas.aiOpponents[0].spinTimer >= 1.0, "BN-6.9", "AI spinout timer active (>= 1.0s)");
    }

    // -------------------------------------------------------------
    // CHECK 7: Mode-7 Perspective Projection of AI Fleet
    // -------------------------------------------------------------
    std::cout << "\n--- Check 7: Mode-7 Perspective Projection of AI Fleet ---\n";
    {
        canvas.setGameMode(GameMode::PLAY_VS_AI);
        canvas.setGameState(GameState::RACING);
        canvas.startRace();

        // Align camera facing toward AI karts
        canvas.kartX = 470.0;
        canvas.kartZ = 150.0;
        canvas.kartAngle = M_PI / 2.0; // facing +Z

        // AI karts at Z = 200 (in front of camera, camZ ~ 50.0)
        canvas.screenBuffer.fill(qRgb(0, 0, 0));
        canvas.renderSprites();

        // Verify that screenBuffer contains rendered pixels from AI karts below horizon
        int nonZeroPixels = 0;
        const QRgb *pixels = reinterpret_cast<const QRgb*>(canvas.screenBuffer.constBits());
        for (int y = canvas.HORIZON_Y; y < canvas.BUFFER_HEIGHT; ++y) {
            for (int x = 0; x < canvas.BUFFER_WIDTH; ++x) {
                if (pixels[y * canvas.BUFFER_WIDTH + x] != qRgb(0, 0, 0)) {
                    nonZeroPixels++;
                }
            }
        }
        FORENSIC_ASSERT(nonZeroPixels > 50, "M7-7.1", "AI opponent billboard sprites are genuinely rendered into frame buffer");

        // Camera Culling: Face camera 180 degrees away (facing -Z)
        canvas.kartAngle = -M_PI / 2.0;
        canvas.screenBuffer.fill(qRgb(0, 0, 0));
        canvas.renderSprites();

        int pixelsBehind = 0;
        pixels = reinterpret_cast<const QRgb*>(canvas.screenBuffer.constBits());
        for (int y = canvas.HORIZON_Y; y < canvas.BUFFER_HEIGHT; ++y) {
            for (int x = 0; x < canvas.BUFFER_WIDTH; ++x) {
                if (pixels[y * canvas.BUFFER_WIDTH + x] != qRgb(0, 0, 0)) {
                    pixelsBehind++;
                }
            }
        }
        FORENSIC_ASSERT(pixelsBehind == 0, "M7-7.2", "AI karts behind camera plane are properly culled (camZ <= 5.0)");
    }

    // -------------------------------------------------------------
    // CHECK 8: Anti-Facade / Dynamic Response Under Arbitrary Waypoints
    // -------------------------------------------------------------
    std::cout << "\n--- Check 8: Anti-Facade & Dynamic Geometric Responsiveness ---\n";
    {
        canvas.setGameMode(GameMode::PLAY_VS_AI);
        canvas.setGameState(GameState::RACING);
        canvas.startRace();

        // Inject custom arbitrary waypoints to prove AI is not following a pre-recorded path
        std::vector<QPointF> customWps = {
            QPointF(100.0, 100.0),
            QPointF(200.0, 100.0),
            QPointF(300.0, 200.0),
            QPointF(200.0, 300.0),
            QPointF(100.0, 200.0)
        };
        canvas.currentTrackWaypoints = customWps;
        canvas.aiOpponents[0].x = 100.0;
        canvas.aiOpponents[0].z = 100.0;
        canvas.aiOpponents[0].currentWaypoint = 0;
        canvas.aiOpponents[0].angle = 0.0;

        // Step simulation on arbitrary geometry
        for (int f = 0; f < 30; ++f) {
            canvas.updateAIOpponents(0.016);
        }

        // Target waypoint (curr + 2) % 5 = 2 -> (300, 200). AI must steer toward positive X and positive Z
        FORENSIC_ASSERT(canvas.aiOpponents[0].x > 100.0 && canvas.aiOpponents[0].z >= 100.0, "FACADE-8.1",
                        "AI steers authentically toward arbitrary injected waypoints (no pre-baked recordings)");
    }

    std::cout << "\n=======================================================\n";
    std::cout << " FORENSIC AUDIT SUMMARY: MILESTONE 2                   \n";
    std::cout << " Total Checks:  " << (g_passCount + g_failCount) << "\n";
    std::cout << " Passed Checks: " << g_passCount << "\n";
    std::cout << " Failed Checks: " << g_failCount << "\n";
    std::cout << "=======================================================\n";

    if (g_failCount == 0) {
        std::cout << " >>> VERDICT: CLEAN - ZERO INTEGRITY VIOLATIONS <<< \n";
        return 0;
    } else {
        std::cerr << " >>> VERDICT: INTEGRITY VIOLATION DETECTED <<< \n";
        return 1;
    }
}
