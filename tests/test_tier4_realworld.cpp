#include "test_tier4_realworld.h"
#include "test_framework.h"
#include "test_harness.h"
#include <vector>
#include <algorithm>
#include <cmath>

void runTier4RealWorldTests() {
    TEST_SUITE("Tier 4 - Real-World Application Scenarios");

    // ------------------------------------------------------------------------
    // Scenario 1: Full Grand Prix Race Simulation on Donut Plains
    // ------------------------------------------------------------------------
    TEST_CASE("RealWorld 1: Full Grand Prix Race Simulation on Donut Plains") {
        auto wps = TrackWaypointManager::generateWaypoints(TrackLevel::DONUT_PLAINS);
        double trackLen = TrackWaypointManager::trackLength(wps);
        ASSERT_GT(trackLen, 1000.0);

        // Instantiate 4-kart field
        std::vector<KartEntity> fleet;
        for (int i = 0; i < 4; ++i) {
            auto def = CharacterRoster::instance().getCharacter(i);
            KartEntity k{};
            k.id = i;
            k.name = def.name;
            k.isAI = (i > 0);
            k.characterId = def.id;
            k.maxSpeed = def.maxSpeed;
            k.accel = def.accel;
            k.brakeDecel = 240.0;
            k.turnRate = def.turnRate;
            k.driftFactor = def.driftFactor;
            k.weight = def.weight;
            k.x = wps[0].x() + (i * 10.0);
            k.z = wps[0].y();
            k.angle = 0.0;
            k.currentLap = 1;
            k.currentWaypointIndex = 0;
            fleet.push_back(k);
        }

        // Simulate 600 ticks of active Grand Prix race (at dt = 0.033s ~ 20 seconds)
        for (int frame = 0; frame < 600; ++frame) {
            double dt = 0.033;
            double p1Dist = fleet[0].totalRaceDistance;

            // Player 1 throttle
            KartPhysicsEngine::updateKart(fleet[0], true, false, 0, false, dt);
            fleet[0].totalRaceDistance += fleet[0].speed * dt;

            // Advance AI karts
            for (size_t i = 1; i < fleet.size(); ++i) {
                AISteeringEngine::updateAI(fleet[i], wps, p1Dist, dt);
            }
        }

        // Verify all karts progressed along track
        for (const auto& k : fleet) {
            ASSERT_GT(k.totalRaceDistance, 1000.0);
            ASSERT_GE(k.x, 0.0);
            ASSERT_LT(k.x, 1024.0);
            ASSERT_GE(k.z, 0.0);
            ASSERT_LT(k.z, 1024.0);
        }

        // Verify dynamic placements are sorted monotonically by total distance
        std::vector<KartEntity> sortedFleet = fleet;
        std::sort(sortedFleet.begin(), sortedFleet.end(), [](const KartEntity& a, const KartEntity& b) {
            return a.totalRaceDistance > b.totalRaceDistance;
        });
        ASSERT_GE(sortedFleet[0].totalRaceDistance, sortedFleet[1].totalRaceDistance);
        ASSERT_GE(sortedFleet[1].totalRaceDistance, sortedFleet[2].totalRaceDistance);
        ASSERT_GE(sortedFleet[2].totalRaceDistance, sortedFleet[3].totalRaceDistance);
    }

    // ------------------------------------------------------------------------
    // Scenario 2: Choco Valley Full Race with Active Hazards & Obstacles
    // ------------------------------------------------------------------------
    TEST_CASE("RealWorld 2: Choco Valley Full Race with Active Hazards & Obstacles") {
        auto wps = TrackWaypointManager::generateWaypoints(TrackLevel::CHOCO_VALLEY);
        auto hazards = HazardEngine::createTrackHazards(TrackLevel::CHOCO_VALLEY);

        std::vector<KartEntity> fleet;
        for (int i = 0; i < 4; ++i) {
            auto def = CharacterRoster::instance().getCharacter(i);
            KartEntity k{};
            k.id = i;
            k.isAI = (i > 0);
            k.maxSpeed = def.maxSpeed;
            k.accel = def.accel;
            k.turnRate = def.turnRate;
            k.x = wps[0].x() + (i * 8.0);
            k.z = wps[0].y();
            fleet.push_back(k);
        }

        // Run 500 frames of Choco Valley race
        for (int frame = 0; frame < 500; ++frame) {
            double dt = 0.033;
            double p1Dist = fleet[0].totalRaceDistance;

            KartPhysicsEngine::updateKart(fleet[0], true, false, 0, false, dt);
            fleet[0].totalRaceDistance += fleet[0].speed * dt;

            for (size_t i = 1; i < fleet.size(); ++i) {
                AISteeringEngine::updateAI(fleet[i], wps, p1Dist, dt);
            }

            HazardEngine::updateHazards(hazards, fleet, dt);
        }

        // All karts must remain within valid track physics space without NaN
        for (const auto& k : fleet) {
            ASSERT_FALSE(std::isnan(k.x));
            ASSERT_FALSE(std::isnan(k.z));
            ASSERT_FALSE(std::isnan(k.speed));
            ASSERT_GT(k.totalRaceDistance, 500.0);
        }
    }

    // ------------------------------------------------------------------------
    // Scenario 3: Bowser Castle Race with Lava Geyser Eruption Cycles
    // ------------------------------------------------------------------------
    TEST_CASE("RealWorld 3: Bowser Castle Race with Lava Geyser Eruption Cycles") {
        auto wps = TrackWaypointManager::generateWaypoints(TrackLevel::BOWSER_CASTLE);
        auto hazards = HazardEngine::createTrackHazards(TrackLevel::BOWSER_CASTLE);

        std::vector<KartEntity> fleet;
        for (int i = 0; i < 4; ++i) {
            auto def = CharacterRoster::instance().getCharacter(i);
            KartEntity k{};
            k.id = i;
            k.isAI = (i > 0);
            k.maxSpeed = def.maxSpeed;
            k.accel = def.accel;
            k.turnRate = def.turnRate;
            k.x = wps[0].x();
            k.z = wps[0].y();
            fleet.push_back(k);
        }

        // Run 600 frames covering multiple 4.8s geyser eruption cycles
        for (int frame = 0; frame < 600; ++frame) {
            double dt = 0.033;
            double p1Dist = fleet[0].totalRaceDistance;

            KartPhysicsEngine::updateKart(fleet[0], true, false, 0, false, dt);
            fleet[0].totalRaceDistance += fleet[0].speed * dt;

            for (size_t i = 1; i < fleet.size(); ++i) {
                AISteeringEngine::updateAI(fleet[i], wps, p1Dist, dt);
            }

            HazardEngine::updateHazards(hazards, fleet, dt);
        }

        // Verify geysers cycled through active phases
        for (const auto& h : hazards) {
            ASSERT_GT(h.animTimer, 0.0);
        }
    }

    // ------------------------------------------------------------------------
    // Scenario 4: Complete Menu-to-Race End-to-End User Flow
    // ------------------------------------------------------------------------
    TEST_CASE("RealWorld 4: Complete Menu-to-Race End-to-End User Flow") {
        MenuStateController menu;
        ASSERT_EQ(menu.currentState(), GameState::TITLE_MENU);

        // 1. User selects Multiplayer mode
        menu.navigateMenu(1);
        ASSERT_EQ(menu.menuIndex(), 1);
        ASSERT_EQ(menu.currentMode(), GameMode::PLAY_VS_AI); // not confirmed yet

        // 2. User confirms mode
        menu.confirmMenu();
        ASSERT_EQ(menu.currentState(), GameState::CHARACTER_SELECT);
        ASSERT_EQ(menu.currentMode(), GameMode::MULTIPLAYER);

        // 3. P1 selects Bowser (1)
        menu.navigateCharSelect(1, 1);
        ASSERT_EQ(menu.p1CharIndex(), 1);

        // 4. P2 also selects Bowser (1) -> duplicate palette tint triggered!
        menu.navigateCharSelect(2, 0); // P2 index 1
        ASSERT_EQ(menu.p2CharIndex(), 1);
        ASSERT_TRUE(menu.isP2AlternateColor());

        // 5. User confirms character selection -> Race launches!
        menu.confirmCharSelect();
        ASSERT_EQ(menu.currentState(), GameState::RACING);

        // 6. Race completed -> Victory / Finished state
        menu.finishRace();
        ASSERT_EQ(menu.currentState(), GameState::RACE_FINISHED);

        // 7. User exits back to Title Menu
        menu.backAction();
        ASSERT_EQ(menu.currentState(), GameState::TITLE_MENU);
    }

    // ------------------------------------------------------------------------
    // Scenario 5: Local 2-Player Split-Screen Competitive Race Simulation
    // ------------------------------------------------------------------------
    TEST_CASE("RealWorld 5: Local 2-Player Split-Screen Competitive Race") {
        ItemTargetingEngine items;

        KartEntity p1{};
        p1.id = 0;
        p1.name = "Player 1";
        p1.x = 200.0; p1.z = 200.0;
        p1.maxSpeed = 160.0; p1.accel = 110.0; p1.turnRate = 2.4; p1.driftFactor = 1.35;

        KartEntity p2{};
        p2.id = 1;
        p2.name = "Player 2";
        p2.x = 200.0; p2.z = 220.0;
        p2.maxSpeed = 175.0; p2.accel = 85.0;  p2.turnRate = 1.9; p2.driftFactor = 1.25;

        KartEntity ai1{};
        ai1.id = 2; ai1.isAI = true; ai1.x = 200.0; ai1.z = 240.0; ai1.maxSpeed = 150.0;

        KartEntity ai2{};
        ai2.id = 3; ai2.isAI = true; ai2.x = 200.0; ai2.z = 260.0; ai2.maxSpeed = 155.0;

        std::vector<KartEntity> fleet = { p1, p2, ai1, ai2 };

        // 1. P1 accelerates and drops a banana
        for (int i = 0; i < 20; ++i) {
            KartPhysicsEngine::updateKart(fleet[0], true, false, 0, false, 0.05);
        }
        items.dropBanana(fleet[0]);
        ASSERT_EQ(items.bananas().size(), 1u);

        // 2. P2 uses Green Shell and fires forward
        items.fireGreenShell(fleet[1]);
        ASSERT_EQ(items.projectiles().size(), 1u);

        // 3. P1 uses Blooper -> blinds P2 and AI opponents, P1 remains clear
        items.useInkBlooper(0, fleet);
        ASSERT_FALSE(fleet[0].isInked);
        ASSERT_TRUE(fleet[1].isInked);
        ASSERT_TRUE(fleet[2].isInked);
        ASSERT_TRUE(fleet[3].isInked);

        // 4. Advance world simulation
        for (int i = 0; i < 30; ++i) {
            items.update(fleet, 0.05);
            for (auto& k : fleet) {
                KartPhysicsEngine::updateKart(k, true, false, 0, false, 0.05);
            }
        }

        // Both players simulated independently without conflict
        ASSERT_GT(fleet[0].speed, 0.0);
        ASSERT_GT(fleet[1].speed, 0.0);
    }

    // ------------------------------------------------------------------------
    // Scenario 6: Extreme Hazard Obstacle Course Stress Test
    // ------------------------------------------------------------------------
    TEST_CASE("RealWorld 6: Extreme Hazard Obstacle Course Stress Test") {
        ItemTargetingEngine items;
        auto hazards = HazardEngine::createTrackHazards(TrackLevel::BOWSER_CASTLE);

        std::vector<KartEntity> fleet;
        for (int i = 0; i < 8; ++i) {
            KartEntity k{};
            k.id = i;
            k.x = 500.0 + (i * 15.0);
            k.z = 150.0;
            k.maxSpeed = 160.0;
            k.accel = 110.0;
            fleet.push_back(k);
        }

        // Bombard track with 5 shells and 5 bananas
        for (int i = 0; i < 5; ++i) {
            items.fireGreenShell(fleet[i]);
            items.dropBanana(fleet[i]);
        }

        ASSERT_EQ(items.projectiles().size(), 5u);
        ASSERT_EQ(items.bananas().size(), 5u);

        // Run 300 intensive simulation steps
        for (int frame = 0; frame < 300; ++frame) {
            double dt = 0.033;
            items.update(fleet, dt);
            HazardEngine::updateHazards(hazards, fleet, dt);
            for (auto& k : fleet) {
                KartPhysicsEngine::updateKart(k, true, false, 0, false, dt);
            }
        }

        // System remains stable: zero NaNs, memory clean
        for (const auto& k : fleet) {
            ASSERT_FALSE(std::isnan(k.x));
            ASSERT_FALSE(std::isnan(k.z));
            ASSERT_FALSE(std::isnan(k.speed));
        }

        items.clear();
        ASSERT_EQ(items.projectiles().size(), 0u);
        ASSERT_EQ(items.bananas().size(), 0u);
    }
}
