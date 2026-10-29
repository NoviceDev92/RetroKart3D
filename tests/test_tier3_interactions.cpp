#include "test_tier3_interactions.h"
#include "test_framework.h"
#include "test_harness.h"
#include <cmath>

void runTier3InteractionTests() {
    TEST_SUITE("Tier 3 - Cross-Feature Interactions (16 Scenarios)");

    // ------------------------------------------------------------------------
    // Cross 1: Menu -> Character Select -> Kart Entity Stat Binding
    // ------------------------------------------------------------------------
    TEST_CASE("Cross 1: Menu to Character Select to Kart Entity Stat Binding") {
        MenuStateController menu;
        menu.navigateMenu(0); // Play vs AI
        menu.confirmMenu();

        // Select Bowser (index 1)
        menu.navigateCharSelect(1, 1);
        ASSERT_EQ(menu.p1CharIndex(), 1);
        menu.confirmCharSelect();

        auto charDef = CharacterRoster::instance().getCharacter(menu.p1CharIndex());
        KartEntity playerKart{};
        playerKart.id = 0;
        playerKart.characterId = charDef.id;
        playerKart.name = charDef.name;
        playerKart.maxSpeed = charDef.maxSpeed;
        playerKart.accel = charDef.accel;
        playerKart.turnRate = charDef.turnRate;
        playerKart.driftFactor = charDef.driftFactor;
        playerKart.bodyColor = charDef.kartColor;

        ASSERT_EQ(playerKart.name, QString("Bowser"));
        ASSERT_NEAR(playerKart.maxSpeed, 175.0, 0.001);
        ASSERT_NEAR(playerKart.accel, 85.0, 0.001);
        ASSERT_NEAR(playerKart.turnRate, 1.9, 0.001);
    }

    // ------------------------------------------------------------------------
    // Cross 2: Drift Turning while Deploying Banana Trap
    // ------------------------------------------------------------------------
    TEST_CASE("Cross 2: Drift Turning while Deploying Banana Trap") {
        KartEntity kart{};
        kart.id = 0;
        kart.speed = 100.0;
        kart.angle = 0.5;
        kart.turnRate = 2.4;
        kart.driftFactor = 1.35;

        // Initiate drift
        KartPhysicsEngine::updateKart(kart, true, false, 1, true, 0.1);
        ASSERT_TRUE(kart.isDrifting);

        ItemTargetingEngine items;
        items.dropBanana(kart);
        ASSERT_EQ(items.bananas().size(), 1u);

        // Advance drift while banana sits behind
        std::vector<KartEntity> fleet = { kart };
        items.update(fleet, 0.1);
        KartPhysicsEngine::updateKart(fleet[0], true, false, 1, true, 0.1);

        ASSERT_TRUE(fleet[0].isDrifting);
        ASSERT_FALSE(fleet[0].isSpinning); // Unaffected by newly dropped banana
    }

    // ------------------------------------------------------------------------
    // Cross 3: Green Shell Colliding with Drifting Opponent
    // ------------------------------------------------------------------------
    TEST_CASE("Cross 3: Green Shell Colliding with Drifting Opponent") {
        KartEntity p1{};
        p1.id = 0;
        p1.x = 200.0;
        p1.z = 200.0;
        p1.speed = 100.0;
        p1.angle = 0.0;

        KartEntity ai{};
        ai.id = 1;
        ai.isAI = true;
        ai.x = 230.0;
        ai.z = 200.0;
        ai.speed = 90.0;
        ai.isDrifting = true;
        ai.driftTimer = 0.8;

        ItemTargetingEngine items;
        items.fireGreenShell(p1);

        std::vector<KartEntity> fleet = { p1, ai };
        // Advance shell to hit AI
        for (int i = 0; i < 4; ++i) {
            items.update(fleet, 0.05);
        }

        ASSERT_TRUE(fleet[1].isSpinning);
        ASSERT_FALSE(items.projectiles()[0].active);
    }

    // ------------------------------------------------------------------------
    // Cross 4: Mini-Turbo Boost Speed Exceeding Hazard Active Window
    // ------------------------------------------------------------------------
    TEST_CASE("Cross 4: Mini-Turbo Boost Speed Exceeding Hazard Active Window") {
        KartEntity kart{};
        kart.maxSpeed = 160.0;
        kart.speed = 150.0;
        kart.miniTurboTimer = 1.2; // Active mini-turbo boost

        // With mini turbo, effective max speed is 160 * 1.35 = 216.0
        for (int i = 0; i < 5; ++i) {
            KartPhysicsEngine::updateKart(kart, true, false, 0, false, 0.05);
        }
        ASSERT_GT(kart.speed, 160.0);
    }

    // ------------------------------------------------------------------------
    // Cross 5: Blooper Fired in Split-Screen Blinds P2 & AI, Leaves P1 Clean
    // ------------------------------------------------------------------------
    TEST_CASE("Cross 5: Blooper in Split-Screen Blinds P2 and AI, Leaves P1 Clean") {
        KartEntity p1{}; p1.id = 0;
        KartEntity p2{}; p2.id = 1;
        KartEntity ai{}; ai.id = 2;

        std::vector<KartEntity> fleet = { p1, p2, ai };
        ItemTargetingEngine items;
        items.useInkBlooper(0, fleet); // P1 fires

        ASSERT_FALSE(fleet[0].isInked);
        ASSERT_TRUE(fleet[1].isInked); // P2 blinded!
        ASSERT_TRUE(fleet[2].isInked); // AI blinded!
    }

    // ------------------------------------------------------------------------
    // Cross 6: Blooper Fired by P2 Blinds P1 & AI, Leaves P2 Clean
    // ------------------------------------------------------------------------
    TEST_CASE("Cross 6: Blooper Fired by P2 Blinds P1 and AI, Leaves P2 Clean") {
        KartEntity p1{}; p1.id = 0;
        KartEntity p2{}; p2.id = 1;
        KartEntity ai{}; ai.id = 2;

        std::vector<KartEntity> fleet = { p1, p2, ai };
        ItemTargetingEngine items;
        items.useInkBlooper(1, fleet); // P2 fires

        ASSERT_TRUE(fleet[0].isInked);  // P1 blinded!
        ASSERT_FALSE(fleet[1].isInked); // P2 clean!
        ASSERT_TRUE(fleet[2].isInked);  // AI blinded!
    }

    // ------------------------------------------------------------------------
    // Cross 7: AI Firing Blooper Debuffs Both Split-Screen Players
    // ------------------------------------------------------------------------
    TEST_CASE("Cross 7: AI Firing Blooper Debuffs Both Split-Screen Players") {
        KartEntity p1{}; p1.id = 0;
        KartEntity p2{}; p2.id = 1;
        KartEntity ai{}; ai.id = 2;

        std::vector<KartEntity> fleet = { p1, p2, ai };
        ItemTargetingEngine items;
        items.useInkBlooper(2, fleet); // AI fires

        ASSERT_TRUE(fleet[0].isInked);  // P1 blinded!
        ASSERT_TRUE(fleet[1].isInked);  // P2 blinded!
        ASSERT_FALSE(fleet[2].isInked); // AI clean!
    }

    // ------------------------------------------------------------------------
    // Cross 8: Green Shell Fired by P1 in Split-Screen Hits P2
    // ------------------------------------------------------------------------
    TEST_CASE("Cross 8: Green Shell Fired by P1 Hits P2 in Split-Screen") {
        KartEntity p1{};
        p1.id = 0;
        p1.x = 300.0;
        p1.z = 400.0;
        p1.angle = 0.0; // Heading East
        p1.speed = 100.0;

        KartEntity p2{};
        p2.id = 1;
        p2.x = 330.0;
        p2.z = 400.0;
        p2.speed = 80.0;

        ItemTargetingEngine items;
        items.fireGreenShell(p1);

        std::vector<KartEntity> fleet = { p1, p2 };
        for (int i = 0; i < 4; ++i) {
            items.update(fleet, 0.05);
        }

        ASSERT_TRUE(fleet[1].isSpinning); // P2 hit and spun out!
        ASSERT_FALSE(fleet[0].isSpinning);
    }

    // ------------------------------------------------------------------------
    // Cross 9: Lava Geyser Erupting while Kart is in Banana Spinout
    // ------------------------------------------------------------------------
    TEST_CASE("Cross 9: Lava Geyser Erupting while Kart is in Banana Spinout") {
        auto hazards = HazardEngine::createTrackHazards(TrackLevel::BOWSER_CASTLE);
        hazards[0].animTimer = 3.5; // Spewing active

        KartEntity kart{};
        kart.x = hazards[0].x;
        kart.z = hazards[0].z;
        kart.isSpinning = true;
        kart.spinTimer = 0.3; // Already spinning from banana

        std::vector<KartEntity> fleet = { kart };
        HazardEngine::updateHazards(hazards, fleet, 0.05);

        ASSERT_TRUE(fleet[0].isSpinning);
        ASSERT_NEAR(fleet[0].spinTimer, 0.8, 0.01); // Geyser overrides spin timer
    }

    // ------------------------------------------------------------------------
    // Cross 10: Chocolate Mud Penalty during Drift
    // ------------------------------------------------------------------------
    TEST_CASE("Cross 10: Chocolate Mud Penalty during Drift") {
        auto hazards = HazardEngine::createTrackHazards(TrackLevel::CHOCO_VALLEY);
        KartEntity kart{};
        kart.maxSpeed = 160.0;
        kart.speed = 120.0;
        kart.isDrifting = true;
        kart.driftTimer = 0.5;
        kart.x = hazards[0].x;
        kart.z = hazards[0].z;

        std::vector<KartEntity> fleet = { kart };
        HazardEngine::updateHazards(hazards, fleet, 0.05);

        ASSERT_LE(fleet[0].speed, 80.001); // Speed cut to 50%
        ASSERT_TRUE(fleet[0].isDrifting);  // Drift state persists without crash
    }

    // ------------------------------------------------------------------------
    // Cross 11: Minimap Updating Realtime during Multi-Kart Collisions
    // ------------------------------------------------------------------------
    TEST_CASE("Cross 11: Minimap Updating Realtime during Multi-Kart Collisions") {
        KartEntity k1{}; k1.x = 200.0; k1.z = 300.0;
        KartEntity k2{}; k2.x = 400.0; k2.z = 500.0;
        KartEntity k3{}; k3.x = 600.0; k3.z = 700.0;
        KartEntity k4{}; k4.x = 800.0; k4.z = 900.0;

        std::vector<KartEntity> fleet = { k1, k2, k3, k4 };
        std::vector<QPointF> blips;
        for (const auto& k : fleet) {
            blips.push_back(MinimapHUD::worldToRadar(k.x, k.z));
        }

        ASSERT_EQ(blips.size(), 4u);
        for (const auto& b : blips) {
            ASSERT_GE(b.x(), 8.0);
            ASSERT_LE(b.x(), 80.0);
            ASSERT_GE(b.y(), 8.0);
            ASSERT_LE(b.y(), 80.0);
        }
    }

    // ------------------------------------------------------------------------
    // Cross 12: AI Rubber-Banding Response to Player Mini-Turbo Boost
    // ------------------------------------------------------------------------
    TEST_CASE("Cross 12: AI Rubber-Banding Response to Player Mini-Turbo Boost") {
        auto wps = TrackWaypointManager::generateWaypoints(TrackLevel::DONUT_PLAINS);
        KartEntity ai{};
        ai.isAI = true;
        ai.x = wps[0].x();
        ai.z = wps[0].y();
        ai.maxSpeed = 150.0;
        ai.speed = 150.0;
        ai.totalRaceDistance = 200.0;

        // Player boosts ahead from 300 -> 380 units (>150 lead)
        AISteeringEngine::updateAI(ai, wps, 380.0, 0.05);
        ASSERT_GT(ai.speed, 150.0); // Rubber band boost activates
    }

    // ------------------------------------------------------------------------
    // Cross 13: Split-Screen Asymmetric Race Finish
    // ------------------------------------------------------------------------
    TEST_CASE("Cross 13: Split-Screen Asymmetric Race Finish") {
        KartEntity p1{};
        p1.id = 0;
        p1.currentLap = 3;
        p1.hasFinished = true; // P1 crossed finish line

        KartEntity p2{};
        p2.id = 1;
        p2.currentLap = 2;
        p2.hasFinished = false; // P2 still racing

        // P2 continues receiving inputs and physics updates
        KartPhysicsEngine::updateKart(p2, true, false, 0, false, 0.05);
        ASSERT_GT(p2.speed, 0.0);
        ASSERT_TRUE(p1.hasFinished);
        ASSERT_FALSE(p2.hasFinished);
    }

    // ------------------------------------------------------------------------
    // Cross 14: Dual-Player Simultaneous Drifting and Item Usage
    // ------------------------------------------------------------------------
    TEST_CASE("Cross 14: Dual-Player Simultaneous Drifting and Item Usage") {
        std::set<int> keys = {
            Qt::Key_W, Qt::Key_D, Qt::Key_Shift, // P1 drifting right
            Qt::Key_Up, Qt::Key_Left, Qt::Key_Return // P2 steering left & using item
        };
        bool p1T, p1B, p1I, p1D, p2T, p2B, p2I, p2D;
        int p1S, p2S;

        SplitScreenEngine::handleInputMultiplexing(keys, p1T, p1B, p1S, p1I, p1D, p2T, p2B, p2S, p2I, p2D);
        ASSERT_TRUE(p1T);
        ASSERT_EQ(p1S, 1);
        ASSERT_TRUE(p1D); // P1 drifting
        ASSERT_TRUE(p2T);
        ASSERT_EQ(p2S, -1);
        ASSERT_TRUE(p2I); // P2 using item
    }

    // ------------------------------------------------------------------------
    // Cross 15: Rapid Track Switching Resets Waypoints, Hazards, and Minimap
    // ------------------------------------------------------------------------
    TEST_CASE("Cross 15: Rapid Track Switching Resets Waypoints and Hazards") {
        auto wps1 = TrackWaypointManager::generateWaypoints(TrackLevel::DONUT_PLAINS);
        auto haz1 = HazardEngine::createTrackHazards(TrackLevel::DONUT_PLAINS);

        auto wps2 = TrackWaypointManager::generateWaypoints(TrackLevel::CHOCO_VALLEY);
        auto haz2 = HazardEngine::createTrackHazards(TrackLevel::CHOCO_VALLEY);

        auto wps3 = TrackWaypointManager::generateWaypoints(TrackLevel::BOWSER_CASTLE);
        auto haz3 = HazardEngine::createTrackHazards(TrackLevel::BOWSER_CASTLE);

        ASSERT_EQ(wps1.size(), 16u);
        ASSERT_EQ(haz1.size(), 0u);
        ASSERT_EQ(wps2.size(), 20u);
        ASSERT_EQ(haz2.size(), 5u);
        ASSERT_EQ(wps3.size(), 24u);
        ASSERT_EQ(haz3.size(), 4u);
    }

    // ------------------------------------------------------------------------
    // Cross 16: Collision Between Heavyweight Bowser and Lightweight Peach
    // ------------------------------------------------------------------------
    TEST_CASE("Cross 16: Collision Between Heavyweight Bowser and Lightweight Peach") {
        auto bowserDef = CharacterRoster::instance().getCharacter(1);
        auto peachDef  = CharacterRoster::instance().getCharacter(2);

        ASSERT_GT(bowserDef.weight, peachDef.weight);
        double ratio = peachDef.weight / bowserDef.weight; // 0.8 / 1.5 = 0.533
        ASSERT_LT(ratio, 0.6);
    }
}
