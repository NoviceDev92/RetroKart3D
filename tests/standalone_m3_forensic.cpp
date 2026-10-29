#include <QApplication>
#include <QWidget>
#include <QImage>
#include <QTimer>
#include <QElapsedTimer>
#include <QKeyEvent>
#include <QPainter>
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

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    std::cout << "=======================================================\n";
    std::cout << " FORENSIC INTEGRITY AUDIT: MILESTONE 3 VERIFICATION   \n";
    std::cout << " Arcade Physics, Drift, Hazards, Minimap & AI Polish   \n";
    std::cout << "=======================================================\n\n";

    GameCanvas canvas;

    // -------------------------------------------------------------
    // CHECK 1: Arcade WASD Physics & High-Grip Braking (F9)
    // -------------------------------------------------------------
    std::cout << "--- Check 1: Arcade WASD Physics & Braking Dynamics ---\n";
    {
        canvas.setGameMode(GameMode::PLAY_VS_AI);
        canvas.setSelectedCharIndexP1(0); // Mario
        canvas.setGameState(GameState::RACING);
        canvas.startRace();

        canvas.kartSpeed = 0.0;
        canvas.kartX = 500.0;
        canvas.kartZ = 200.0;
        canvas.kartAngle = 0.0; // facing +X
        canvas.pressedKeys.clear();

        // 1. Forward throttle (W / Up)
        canvas.pressedKeys.insert(Qt::Key_W);
        canvas.updatePhysics(0.05);
        canvas.updatePhysics(0.05);
        FORENSIC_ASSERT(canvas.kartSpeed > 5.0, "PHY-1.1", "WASD forward throttle accelerates kart from standstill");
        FORENSIC_ASSERT(canvas.kartX > 500.0, "PHY-1.2", "Kart integrates position forward along heading vector");

        // 2. High-grip braking (S / Down)
        canvas.pressedKeys.clear();
        canvas.pressedKeys.insert(Qt::Key_S);
        canvas.kartSpeed = 100.0;
        // Two 0.05s steps (0.10s total): brakeDecel = 240.0 -> 100 - 24 = 76.0
        canvas.updatePhysics(0.05);
        canvas.updatePhysics(0.05);
        FORENSIC_ASSERT(canvas.kartSpeed < 80.0 && canvas.kartSpeed > 70.0, "PHY-1.3",
                        "High-grip braking rapidly decelerates kart with brakeDecel >= 240.0 u/s^2");

        // 3. Reverse clamp (-30% maxSpeed)
        for (int i = 0; i < 20; ++i) {
            canvas.updatePhysics(0.05);
        }
        double maxReverse = -0.30 * canvas.maxSpeed;
        FORENSIC_ASSERT(canvas.kartSpeed >= maxReverse - 1e-4, "PHY-1.4",
                        "Reverse speed is strictly clamped to -30% of max speed");

        // 4. Natural friction coasting
        canvas.pressedKeys.clear();
        canvas.kartSpeed = 50.0;
        canvas.updatePhysics(0.05);
        FORENSIC_ASSERT(canvas.kartSpeed < 50.0, "PHY-1.5", "Coasting without throttle or brake applies friction drag");
    }

    // -------------------------------------------------------------
    // CHECK 2: Dedicated Drift Mechanics, Hop & Mini-Turbo (F9)
    // -------------------------------------------------------------
    std::cout << "\n--- Check 2: Dedicated Drift Mechanics, Hop & Mini-Turbo ---\n";
    {
        canvas.setGameMode(GameMode::PLAY_VS_AI);
        canvas.setGameState(GameState::RACING);
        canvas.startRace();
        canvas.pressedKeys.clear();

        // 1. Drift threshold check (must be > 40.0 speed)
        canvas.kartSpeed = 35.0; // below threshold
        canvas.pressedKeys.insert(Qt::Key_Shift);
        canvas.pressedKeys.insert(Qt::Key_D); // right turn
        canvas.updatePhysics(0.05);
        FORENSIC_ASSERT(canvas.isDrifting == false, "DRF-2.1", "Drift does NOT activate below 40.0 speed threshold");

        // 2. Drift activation above threshold
        canvas.kartSpeed = 80.0;
        canvas.pressedKeys.insert(Qt::Key_W); // throttle while drifting
        canvas.updatePhysics(0.05);
        FORENSIC_ASSERT(canvas.isDrifting == true, "DRF-2.2", "Drift activates when holding Shift + steering at speed > 40.0");
        FORENSIC_ASSERT(canvas.driftDirection == 1, "DRF-2.3", "Drift locks direction to initial steer direction (Right = +1)");
        FORENSIC_ASSERT(canvas.driftHopTimer > 0.0, "DRF-2.4", "Drift hop timer initiated (0.22s)");
        FORENSIC_ASSERT(canvas.driftHopHeight > 0.0, "DRF-2.5", "Drift hop executes vertical parabolic arc (h > 0)");

        // 3. Counter-steering vs with-drift turn multipliers
        // Steer with drift (+1): turnMult = 1.25
        double angleBefore = canvas.kartAngle;
        canvas.updatePhysics(0.05);
        double deltaTurnWith = canvas.kartAngle - angleBefore;

        // Counter-steer (-1): turnMult = 0.45
        canvas.pressedKeys.erase(Qt::Key_D);
        canvas.pressedKeys.insert(Qt::Key_A);
        angleBefore = canvas.kartAngle;
        canvas.updatePhysics(0.05);
        double deltaTurnCounter = canvas.kartAngle - angleBefore;
        FORENSIC_ASSERT(deltaTurnWith > deltaTurnCounter, "DRF-2.6",
                        "Steering into drift yields higher yaw rate (1.25x) than counter-steering (0.45x)");

        // 4. Lateral slip angle (beta)
        FORENSIC_ASSERT(canvas.lateralSlipAngle > 0.05, "DRF-2.7", "Lateral slip angle beta genuinely develops during power slide");

        // 5. Dual-stage sparks check (steering with drift + throttle)
        canvas.pressedKeys.erase(Qt::Key_A);
        canvas.pressedKeys.insert(Qt::Key_D);
        for (int step = 0; step < 12; ++step) {
            canvas.updatePhysics(0.05);
        }
        FORENSIC_ASSERT(canvas.driftTimer >= 0.5, "DRF-2.8", "Drift timer charges continuously");
        FORENSIC_ASSERT(!canvas.driftSparks.empty(), "DRF-2.9", "Drift sparks spawn after driftTimer >= 0.5s");

        // Advance to full charge (>= 1.0s)
        for (int step = 0; step < 12; ++step) {
            canvas.updatePhysics(0.05);
        }
        FORENSIC_ASSERT(canvas.driftTimer >= 1.0, "DRF-2.10", "Drift timer reaches full charge (>= 1.0s)");

        // 6. Mini-turbo boost release on drift exit
        double preSpeed = canvas.kartSpeed;
        canvas.pressedKeys.erase(Qt::Key_Shift);
        canvas.updatePhysics(0.05);
        FORENSIC_ASSERT(canvas.isDrifting == false, "DRF-2.11", "Drift exits immediately upon releasing drift key");
        FORENSIC_ASSERT(canvas.miniTurboTimer > 1.0, "DRF-2.12", "Mini-turbo boost awarded for 1.2s on charged drift exit");
        FORENSIC_ASSERT(canvas.kartSpeed > preSpeed + 15.0, "DRF-2.13", "Mini-turbo grants instantaneous +25.0 speed impulse");

        // 7. Mini-turbo boost speed boost
        canvas.pressedKeys.insert(Qt::Key_W);
        for (int i = 0; i < 20; ++i) canvas.updatePhysics(0.05);
        FORENSIC_ASSERT(canvas.kartSpeed > canvas.maxSpeed * 1.15, "DRF-2.14",
                        "Mini-turbo boosts top speed up to 1.35x maximum speed");

        // 8. Collision / spinout cancels drift
        canvas.pressedKeys.clear();
        canvas.pressedKeys.insert(Qt::Key_Shift);
        canvas.pressedKeys.insert(Qt::Key_D);
        canvas.pressedKeys.insert(Qt::Key_W);
        canvas.kartSpeed = 80.0;
        canvas.updatePhysics(0.05);
        FORENSIC_ASSERT(canvas.isDrifting == true, "DRF-2.15", "Re-entered drift");
        canvas.isSpinning = true;
        canvas.updatePhysics(0.05);
        FORENSIC_ASSERT(canvas.isDrifting == false, "DRF-2.16", "Drift cancelled immediately when kart spins out");
    }

    // -------------------------------------------------------------
    // CHECK 3: Bowser Castle Lava Geysers (F10)
    // -------------------------------------------------------------
    std::cout << "\n--- Check 3: Bowser Castle Lava Geysers (F10) ---\n";
    {
        canvas.setTrackLevel(TrackLevel::BOWSER_CASTLE);
        canvas.setGameMode(GameMode::PLAY_VS_AI);
        canvas.startRace();

        // 1. Geyser count in Bowser Castle
        const auto &hazards = canvas.getTrackHazards();
        int geyserCount = 0;
        for (const auto &h : hazards) {
            if (h.type == HazardType::LAVA_GEYSER) geyserCount++;
        }
        FORENSIC_ASSERT(geyserCount == 4, "GEY-3.1", "Bowser Castle initializes exactly 4 lava geysers");

        // 2. Eruption cycle timing: 4.8s period (3.3s dormant, 1.5s erupting)
        canvas.trackHazards[0].animTimer = 1.0;
        canvas.updateTrackHazards(0.05);
        FORENSIC_ASSERT(canvas.trackHazards[0].isErupting == false, "GEY-3.2",
                        "Lava geyser is dormant/bubbling when animTimer < 3.3s");

        // 3. Safe passage through dormant geyser
        canvas.kartX = canvas.trackHazards[0].x;
        canvas.kartZ = canvas.trackHazards[0].z;
        canvas.kartSpeed = 100.0;
        canvas.isSpinning = false;
        canvas.updateTrackHazards(0.05);
        FORENSIC_ASSERT(canvas.isSpinning == false, "GEY-3.3",
                        "Kart passes safely through dormant lava geyser without collision or spinout");

        // 4. Eruption trigger at 3.3s
        canvas.trackHazards[0].animTimer = 3.35;
        canvas.updateTrackHazards(0.05);
        FORENSIC_ASSERT(canvas.trackHazards[0].isErupting == true, "GEY-3.4",
                        "Lava geyser transitions to isErupting=true at animTimer >= 3.3s");

        // 5. Fiery spinout and pushback on erupting geyser contact
        canvas.kartX = canvas.trackHazards[0].x + 2.0;
        canvas.kartZ = canvas.trackHazards[0].z;
        canvas.kartSpeed = 100.0;
        canvas.isSpinning = false;
        canvas.updateTrackHazards(0.05);
        FORENSIC_ASSERT(canvas.isSpinning == true, "GEY-3.5",
                        "Contact with erupting geyser triggers fiery spinout (isSpinning=true)");
        FORENSIC_ASSERT(std::abs(canvas.spinTimer - 0.8) < 0.05, "GEY-3.6",
                        "Erupting geyser spinTimer set to 0.8s");
        FORENSIC_ASSERT(canvas.kartSpeed <= 30.0, "GEY-3.7",
                        "Erupting geyser penalizes speed by 75% (speed *= 0.25)");
        FORENSIC_ASSERT(canvas.kartX > canvas.trackHazards[0].x + 10.0, "GEY-3.8",
                        "Erupting geyser applies outward radial pushback (+15.0 units)");

        // 6. AI opponent also collides with erupting geyser
        if (!canvas.aiOpponents.empty()) {
            canvas.aiOpponents[0].x = canvas.trackHazards[0].x + 1.0;
            canvas.aiOpponents[0].z = canvas.trackHazards[0].z;
            canvas.aiOpponents[0].speed = 100.0;
            canvas.aiOpponents[0].isSpinning = false;
            canvas.trackHazards[0].animTimer = 3.4;
            canvas.updateTrackHazards(0.05);
            FORENSIC_ASSERT(canvas.aiOpponents[0].isSpinning == true, "GEY-3.9",
                            "AI opponents also collide with and spin out from erupting lava geysers");
        }
    }

    // -------------------------------------------------------------
    // CHECK 4: Choco Valley Hazards (F11)
    // -------------------------------------------------------------
    std::cout << "\n--- Check 4: Choco Valley Chocolate Fountains & Candies (F11) ---\n";
    {
        canvas.setTrackLevel(TrackLevel::CHOCO_VALLEY);
        canvas.setGameMode(GameMode::PLAY_VS_AI);
        canvas.startRace();

        const auto &hazards = canvas.getTrackHazards();
        int fountainCount = 0;
        int candyCount = 0;
        for (const auto &h : hazards) {
            if (h.type == HazardType::CHOCOLATE_FOUNTAIN) fountainCount++;
            if (h.type == HazardType::ROLLING_CANDY) candyCount++;
        }
        FORENSIC_ASSERT(fountainCount == 3, "CHO-4.1", "Choco Valley initializes exactly 3 chocolate fountains");
        FORENSIC_ASSERT(candyCount == 2, "CHO-4.2", "Choco Valley initializes exactly 2 rolling candies");

        // 1. Chocolate fountain mud splash cuts speed to 50%
        int fIdx = -1;
        for (size_t i = 0; i < canvas.trackHazards.size(); ++i) {
            if (canvas.trackHazards[i].type == HazardType::CHOCOLATE_FOUNTAIN) {
                fIdx = static_cast<int>(i);
                break;
            }
        }
        assert(fIdx >= 0);

        canvas.kartX = canvas.trackHazards[fIdx].x;
        canvas.kartZ = canvas.trackHazards[fIdx].z;
        canvas.kartSpeed = canvas.maxSpeed; // 100% max speed
        canvas.updateTrackHazards(0.05);
        FORENSIC_ASSERT(canvas.kartSpeed <= canvas.maxSpeed * 0.50 + 1e-4, "CHO-4.3",
                        "Chocolate fountain mud splash caps speed to 50% max speed");

        // 2. Rolling candy sinusoidal oscillation
        int cIdx = -1;
        for (size_t i = 0; i < canvas.trackHazards.size(); ++i) {
            if (canvas.trackHazards[i].type == HazardType::ROLLING_CANDY) {
                cIdx = static_cast<int>(i);
                break;
            }
        }
        assert(cIdx >= 0);

        canvas.trackHazards[cIdx].animTimer = 0.0;
        double origX = canvas.trackHazards[cIdx].x;
        canvas.updateTrackHazards(0.39); // 0.39 * 2.0 = 0.78 rad -> sin(0.78) ~ 0.7
        double offset = 40.0 * std::sin(canvas.trackHazards[cIdx].animTimer * 2.0);
        FORENSIC_ASSERT(std::abs(offset) > 10.0, "CHO-4.4",
                        "Rolling candy oscillates sinusoidally across track with amplitude 40.0 units");

        // 3. Rolling candy bumper bounce recoil
        double currentCandyX = origX + offset;
        canvas.kartX = currentCandyX;
        canvas.kartZ = canvas.trackHazards[cIdx].z;
        canvas.kartSpeed = 80.0;
        canvas.updateTrackHazards(0.05);
        FORENSIC_ASSERT(canvas.kartSpeed == -35.0, "CHO-4.5",
                        "Contact with rolling candy triggers bumper bounce recoil (speed = -35.0 u/s)");
    }

    // -------------------------------------------------------------
    // CHECK 5: Top-Left HUD Minimap (F12)
    // -------------------------------------------------------------
    std::cout << "\n--- Check 5: Top-Left HUD Minimap (F12) ---\n";
    {
        canvas.setTrackLevel(TrackLevel::DONUT_PLAINS);
        canvas.setGameMode(GameMode::PLAY_VS_AI);
        canvas.startRace();

        // 1. Radar dimensions & positioning
        FORENSIC_ASSERT(MinimapHUD::RADAR_X == 8, "MAP-5.1", "Minimap radar positioned at X=8 (top-left margin)");
        FORENSIC_ASSERT(MinimapHUD::RADAR_Y == 8, "MAP-5.2", "Minimap radar positioned at Y=8 (top-left margin)");
        FORENSIC_ASSERT(MinimapHUD::RADAR_SIZE == 72, "MAP-5.3", "Minimap radar size is exactly 72x72 pixels");

        // 2. Pre-rendered track layout cache
        FORENSIC_ASSERT(!canvas.minimapTrackCache.isNull(), "MAP-5.4",
                        "Minimap track cache is pre-rendered into QImage (minimapTrackCache is valid)");
        FORENSIC_ASSERT(canvas.minimapTrackCache.width() == 72 && canvas.minimapTrackCache.height() == 72,
                        "MAP-5.5", "Minimap track cache matches radar size 72x72");

        // 3. World to Radar coordinate translation
        QPointF p0 = MinimapHUD::worldToRadar(0.0, 0.0);
        FORENSIC_ASSERT(std::abs(p0.x() - 8.0) < 1e-4 && std::abs(p0.y() - 8.0) < 1e-4,
                        "MAP-5.6", "World coordinate (0, 0) maps to radar top-left (8, 8)");

        QPointF pCenter = MinimapHUD::worldToRadar(512.0, 512.0);
        FORENSIC_ASSERT(std::abs(pCenter.x() - 44.0) < 1e-4 && std::abs(pCenter.y() - 44.0) < 1e-4,
                        "MAP-5.7", "World coordinate (512, 512) maps to radar center (44, 44)");

        QPointF pWrap = MinimapHUD::worldToRadar(1024.0, 1024.0);
        FORENSIC_ASSERT(std::abs(pWrap.x() - 8.0) < 1e-4 && std::abs(pWrap.y() - 8.0) < 1e-4,
                        "MAP-5.8", "World coordinate (1024, 1024) wraps modulo 1024 to (8, 8)");

        // 4. Render radar onto screenBuffer
        canvas.renderMinimap();
        QRgb sampleRadar = canvas.screenBuffer.pixel(12, 12);
        FORENSIC_ASSERT(qAlpha(sampleRadar) == 255, "MAP-5.9",
                        "Minimap radar renders cleanly into screenBuffer with dark backing");

        // 5. All 3 tracks generate valid track caches
        canvas.setTrackLevel(TrackLevel::CHOCO_VALLEY);
        FORENSIC_ASSERT(!canvas.minimapTrackCache.isNull(), "MAP-5.10", "Choco Valley minimap cache generated");
        canvas.setTrackLevel(TrackLevel::BOWSER_CASTLE);
        FORENSIC_ASSERT(!canvas.minimapTrackCache.isNull(), "MAP-5.11", "Bowser Castle minimap cache generated");
    }

    // -------------------------------------------------------------
    // CHECK 6: AI Polish & Terrain Disambiguation (M2 Defect Resolution)
    // -------------------------------------------------------------
    std::cout << "\n--- Check 6: AI Polish & Terrain Mask Disambiguation ---\n";
    {
        canvas.setTrackLevel(TrackLevel::DONUT_PLAINS);
        canvas.setGameMode(GameMode::PLAY_VS_AI);
        canvas.setGameState(GameState::RACING);
        canvas.startRace();

        // 1. Verify White Tarmac vs Molten Lava disambiguation
        // On White Tarmac (RGB 255, 255, 255), AI MUST NOT spin out or lose speed
        canvas.aiOpponents[0].x = 512.0;
        canvas.aiOpponents[0].z = 200.0; // Road waypoint 0
        canvas.aiOpponents[0].speed = 100.0;
        canvas.aiOpponents[0].isSpinning = false;
        canvas.aiOpponents[0].spinTimer = 0.0;

        canvas.updateAIOpponents(0.05);
        FORENSIC_ASSERT(canvas.aiOpponents[0].isSpinning == false, "AID-6.1",
                        "AI on White Tarmac (RGB 255, 255, 255) DOES NOT trigger molten lava recoil");
        FORENSIC_ASSERT(canvas.aiOpponents[0].speed >= 95.0, "AID-6.2",
                        "AI on White Tarmac maintains 100% full speed");

        // 2. Verify Molten Lava (Magenta RGB 255, 0, 255) DOES trigger recoil
        canvas.setTrackLevel(TrackLevel::BOWSER_CASTLE);
        canvas.startRace();
        canvas.aiOpponents[0].x = 100.0;
        canvas.aiOpponents[0].z = 100.0; // In molten lava lake
        canvas.aiOpponents[0].speed = 100.0;
        canvas.aiOpponents[0].isSpinning = false;
        canvas.updateAIOpponents(0.05);
        FORENSIC_ASSERT(canvas.aiOpponents[0].isSpinning == true, "AID-6.3",
                        "AI on Magenta Molten Lava lake (RGB 255, 0, 255) correctly triggers lava recoil");
        FORENSIC_ASSERT(canvas.aiOpponents[0].speed <= 35.0, "AID-6.4",
                        "AI speed heavily penalized by molten lava lake");

        // 3. Verify inkTimer decay persistence even during spinout
        canvas.aiOpponents[0].isInked = true;
        canvas.aiOpponents[0].inkTimer = 4.0;
        canvas.aiOpponents[0].isSpinning = true;
        canvas.aiOpponents[0].spinTimer = 2.0;

        canvas.updateAIOpponents(0.05);
        FORENSIC_ASSERT(canvas.aiOpponents[0].inkTimer < 4.0 - 0.04, "AID-6.5",
                        "AI inkTimer decays continuously even while kart is spinning");

        // 4. Multi-tick autonomous navigation verification on Bowser Castle
        canvas.setTrackLevel(TrackLevel::BOWSER_CASTLE);
        canvas.startRace();
        double startX = canvas.aiOpponents[0].x;
        double startZ = canvas.aiOpponents[0].z;

        for (int frame = 0; frame < 200; ++frame) {
            canvas.updatePhysics(0.016);
            canvas.updateAIOpponents(0.016);
            canvas.updateTrackHazards(0.016);
        }

        double movedDist = std::hypot(canvas.aiOpponents[0].x - startX, canvas.aiOpponents[0].z - startZ);
        FORENSIC_ASSERT(movedDist > 40.0, "AID-6.6",
                        "AI opponents successfully navigate Bowser Castle raceway without getting permanently stuck");
        FORENSIC_ASSERT(canvas.aiOpponents[0].speed > 25.0, "AID-6.7",
                        "AI maintains strong racing velocity on technical circuit");
    }

    std::cout << "\n=======================================================\n";
    std::cout << " FORENSIC AUDIT SUMMARY: MILESTONE 3                   \n";
    std::cout << " Total Checks:  " << (g_passCount + g_failCount) << "\n";
    std::cout << " Passed Checks: " << g_passCount << "\n";
    std::cout << " Failed Checks: " << g_failCount << "\n";
    std::cout << "=======================================================\n";

    if (g_failCount == 0) {
        std::cout << " >>> VERDICT: CLEAN - ZERO INTEGRITY VIOLATIONS <<< \n";
        return 0;
    } else {
        std::cerr << " >>> VERDICT: INTEGRITY VIOLATION <<< \n";
        return 1;
    }
}
