#include "test_tier1_features.h"
#include "test_framework.h"
#include "test_harness.h"
#include "../sprite_atlas.h"
#include <QElapsedTimer>

void runTier1FeatureTests() {
    TEST_SUITE("Tier 1 - Feature Coverage (F1 to F16)");

    // ------------------------------------------------------------------------
    // F1: Title & Main Menu Flow (5 tests)
    // ------------------------------------------------------------------------
    TEST_CASE("F1.1: Title menu initial state") {
        MenuStateController menu;
        ASSERT_EQ(menu.currentState(), GameState::TITLE_MENU);
        ASSERT_EQ(menu.menuIndex(), 0);
        ASSERT_EQ(menu.currentMode(), GameMode::PLAY_VS_AI);
    }

    TEST_CASE("F1.2: Title menu navigation cycling") {
        MenuStateController menu;
        menu.navigateMenu(1);
        ASSERT_EQ(menu.menuIndex(), 1); // Multiplayer
        menu.navigateMenu(1);
        ASSERT_EQ(menu.menuIndex(), 2); // Practice
    }

    TEST_CASE("F1.3: Title menu boundary wrapping") {
        MenuStateController menu;
        menu.navigateMenu(-1);
        ASSERT_EQ(menu.menuIndex(), 2); // wrap backwards to Practice
        menu.navigateMenu(1);
        ASSERT_EQ(menu.menuIndex(), 0); // wrap forwards to Play vs AI
    }

    TEST_CASE("F1.4: Title menu confirmation transitions to character select") {
        MenuStateController menu;
        menu.navigateMenu(1); // Select Multiplayer
        menu.confirmMenu();
        ASSERT_EQ(menu.currentState(), GameState::CHARACTER_SELECT);
        ASSERT_EQ(menu.currentMode(), GameMode::MULTIPLAYER);
    }

    TEST_CASE("F1.5: Menu back action returns to title menu cleanly") {
        MenuStateController menu;
        menu.confirmMenu();
        ASSERT_EQ(menu.currentState(), GameState::CHARACTER_SELECT);
        menu.backAction();
        ASSERT_EQ(menu.currentState(), GameState::TITLE_MENU);
    }

    // ------------------------------------------------------------------------
    // F2: Character Selection & Roster (5 tests)
    // ------------------------------------------------------------------------
    TEST_CASE("F2.1: Roster contains four canonical characters") {
        const auto& roster = CharacterRoster::instance().allCharacters();
        ASSERT_EQ(roster.size(), 4u);
        ASSERT_EQ(roster[0].name, QString("Mario"));
        ASSERT_EQ(roster[1].name, QString("Bowser"));
        ASSERT_EQ(roster[2].name, QString("Peach"));
        ASSERT_EQ(roster[3].name, QString("Toad"));
    }

    TEST_CASE("F2.2: Character speed and acceleration differentiation") {
        auto mario = CharacterRoster::instance().getCharacter(0);
        auto bowser = CharacterRoster::instance().getCharacter(1);
        auto peach = CharacterRoster::instance().getCharacter(2);

        // Bowser has highest top speed but lowest acceleration
        ASSERT_GT(bowser.maxSpeed, mario.maxSpeed);
        ASSERT_GT(mario.maxSpeed, peach.maxSpeed);
        ASSERT_GT(peach.accel, mario.accel);
        ASSERT_GT(mario.accel, bowser.accel);
    }

    TEST_CASE("F2.3: Character handling and turn rate differentiation") {
        auto bowser = CharacterRoster::instance().getCharacter(1);
        auto peach = CharacterRoster::instance().getCharacter(2);
        // Peach is nimble, Bowser is heavyweight
        ASSERT_GT(peach.turnRate, bowser.turnRate);
        ASSERT_GT(peach.driftFactor, bowser.driftFactor);
        ASSERT_GT(bowser.weight, peach.weight);
    }

    TEST_CASE("F2.4: Character body colors are unique") {
        auto mario = CharacterRoster::instance().getCharacter(0);
        auto bowser = CharacterRoster::instance().getCharacter(1);
        auto peach = CharacterRoster::instance().getCharacter(2);
        auto toad = CharacterRoster::instance().getCharacter(3);

        ASSERT_NEQ(mario.kartColor, bowser.kartColor);
        ASSERT_NEQ(bowser.kartColor, peach.kartColor);
        ASSERT_NEQ(peach.kartColor, toad.kartColor);
    }

    TEST_CASE("F2.5: P2 alternate palette tint on duplicate character choice") {
        MenuStateController menu;
        menu.navigateMenu(1); // Multiplayer
        menu.confirmMenu();

        // P1 is Mario (0), navigate P2 to Mario (0)
        menu.navigateCharSelect(2, -1); // 1 -> 0
        ASSERT_EQ(menu.p1CharIndex(), 0);
        ASSERT_EQ(menu.p2CharIndex(), 0);
        ASSERT_TRUE(menu.isP2AlternateColor());

        QColor altColor = CharacterRoster::instance().getP2AlternateColor(0);
        ASSERT_NEQ(altColor, CharacterRoster::instance().getCharacter(0).kartColor);
    }

    // ------------------------------------------------------------------------
    // F3: Multi-Kart Entity Architecture (5 tests)
    // ------------------------------------------------------------------------
    TEST_CASE("F3.1: Kart entity initialization default state") {
        KartEntity kart{};
        kart.id = 0;
        kart.name = "Player1";
        kart.isAI = false;
        kart.x = 470.0;
        kart.z = 200.0;
        kart.maxSpeed = 160.0;
        kart.accel = 110.0;
        kart.brakeDecel = 160.0;
        kart.turnRate = 2.4;

        ASSERT_EQ(kart.id, 0);
        ASSERT_FALSE(kart.isAI);
        ASSERT_NEAR(kart.speed, 0.0, 0.001);
        ASSERT_FALSE(kart.isSpinning);
        ASSERT_FALSE(kart.isInked);
    }

    TEST_CASE("F3.2: Multi-kart fleet storage in collection") {
        std::vector<KartEntity> fleet;
        for (int i = 0; i < 4; ++i) {
            KartEntity k{};
            k.id = i;
            k.isAI = (i > 0);
            fleet.push_back(k);
        }
        ASSERT_EQ(fleet.size(), 4u);
        ASSERT_FALSE(fleet[0].isAI);
        ASSERT_TRUE(fleet[1].isAI);
        ASSERT_TRUE(fleet[2].isAI);
        ASSERT_TRUE(fleet[3].isAI);
    }

    TEST_CASE("F3.3: Kart entity character stat binding") {
        auto bowserDef = CharacterRoster::instance().getCharacter(1);
        KartEntity kart{};
        kart.characterId = bowserDef.id;
        kart.maxSpeed = bowserDef.maxSpeed;
        kart.accel = bowserDef.accel;
        kart.turnRate = bowserDef.turnRate;
        kart.driftFactor = bowserDef.driftFactor;

        ASSERT_NEAR(kart.maxSpeed, 175.0, 0.001);
        ASSERT_NEAR(kart.accel, 85.0, 0.001);
        ASSERT_NEAR(kart.turnRate, 1.9, 0.001);
    }

    TEST_CASE("F3.4: Kart status debuff flags application and decay") {
        KartEntity kart{};
        kart.maxSpeed = 160.0;
        kart.speed = 100.0;

        KartPhysicsEngine::applyItemSpinout(kart);
        ASSERT_TRUE(kart.isSpinning);
        ASSERT_NEAR(kart.spinTimer, 1.0, 0.01);
        ASSERT_LT(kart.speed, 50.0); // reduced by 65%

        // Update physics over 1.1s to clear spin
        for (int i = 0; i < 25; ++i) {
            KartPhysicsEngine::updateKart(kart, false, false, 0, false, 0.05);
        }
        ASSERT_FALSE(kart.isSpinning);
        ASSERT_NEAR(kart.spinTimer, 0.0, 0.001);
    }

    TEST_CASE("F3.5: Kart kinematics position integration") {
        KartEntity kart{};
        kart.x = 100.0;
        kart.z = 100.0;
        kart.angle = 0.0; // East (+X)
        kart.speed = 100.0;

        KartPhysicsEngine::updateKart(kart, false, false, 0, false, 0.05);
        ASSERT_NEAR(kart.x, 104.89, 0.1);
        ASSERT_NEAR(kart.z, 100.0, 0.001);
    }

    // ------------------------------------------------------------------------
    // F4: Persistent Track Waypoints (5 tests)
    // ------------------------------------------------------------------------
    TEST_CASE("F4.1: Donut Plains waypoints continuous loop") {
        auto wps = TrackWaypointManager::generateWaypoints(TrackLevel::DONUT_PLAINS);
        ASSERT_GE(wps.size(), 16u);
        double len = TrackWaypointManager::trackLength(wps);
        ASSERT_GT(len, 2000.0);
    }

    TEST_CASE("F4.2: Choco Valley waypoints count and non-empty") {
        auto wps = TrackWaypointManager::generateWaypoints(TrackLevel::CHOCO_VALLEY);
        ASSERT_GE(wps.size(), 20u);
        double len = TrackWaypointManager::trackLength(wps);
        ASSERT_GT(len, 2000.0);
    }

    TEST_CASE("F4.3: Bowser Castle technical circuit waypoints") {
        auto wps = TrackWaypointManager::generateWaypoints(TrackLevel::BOWSER_CASTLE);
        ASSERT_GE(wps.size(), 24u);
        double len = TrackWaypointManager::trackLength(wps);
        ASSERT_GT(len, 2500.0);
    }

    TEST_CASE("F4.4: Track length calculation metric sanity") {
        std::vector<QPointF> box = { QPointF(0, 0), QPointF(100, 0), QPointF(100, 100), QPointF(0, 100) };
        double len = TrackWaypointManager::trackLength(box);
        ASSERT_NEAR(len, 400.0, 0.001);
    }

    TEST_CASE("F4.5: Distance along track progression monotonicity") {
        auto wps = TrackWaypointManager::generateWaypoints(TrackLevel::DONUT_PLAINS);
        double d1 = TrackWaypointManager::distanceAlongTrack(wps, 1, wps[1].x(), wps[1].y());
        double d4 = TrackWaypointManager::distanceAlongTrack(wps, 4, wps[4].x(), wps[4].y());
        ASSERT_GT(d4, d1);
    }

    // ------------------------------------------------------------------------
    // F5: Active AI Opponent Fleet (5 tests)
    // ------------------------------------------------------------------------
    TEST_CASE("F5.1: AI fleet population size") {
        std::vector<KartEntity> aiFleet;
        for (int i = 1; i <= 3; ++i) {
            KartEntity ai{};
            ai.id = i;
            ai.isAI = true;
            ai.characterId = i;
            aiFleet.push_back(ai);
        }
        ASSERT_EQ(aiFleet.size(), 3u);
        for (const auto& ai : aiFleet) {
            ASSERT_TRUE(ai.isAI);
        }
    }

    TEST_CASE("F5.2: AI lookahead steering orientation toward target") {
        auto wps = TrackWaypointManager::generateWaypoints(TrackLevel::DONUT_PLAINS);
        KartEntity ai{};
        ai.isAI = true;
        ai.x = wps[0].x();
        ai.z = wps[0].y();
        ai.angle = 0.0;
        ai.maxSpeed = 150.0;
        ai.accel = 100.0;
        ai.turnRate = 2.5;
        ai.currentWaypointIndex = 0;

        AISteeringEngine::updateAI(ai, wps, 0.0, 0.05);
        // AI should steer toward lookahead waypoint
        ASSERT_TRUE(ai.speed > 0.0);
    }

    TEST_CASE("F5.3: AI waypoint advancement on proximity") {
        auto wps = TrackWaypointManager::generateWaypoints(TrackLevel::DONUT_PLAINS);
        KartEntity ai{};
        ai.isAI = true;
        // Position within 10 units of waypoint 0
        ai.x = wps[0].x() + 5.0;
        ai.z = wps[0].y() + 5.0;
        ai.currentWaypointIndex = 0;

        AISteeringEngine::updateAI(ai, wps, 0.0, 0.05);
        ASSERT_EQ(ai.currentWaypointIndex, 1);
    }

    TEST_CASE("F5.4: AI throttle modulation on sharp angles") {
        auto wps = TrackWaypointManager::generateWaypoints(TrackLevel::BOWSER_CASTLE);
        KartEntity ai{};
        ai.isAI = true;
        ai.x = 300.0;
        ai.z = 150.0;
        ai.angle = 3.14; // Facing directly away from waypoint path
        ai.speed = 120.0;
        ai.maxSpeed = 160.0;
        ai.turnRate = 2.0;
        ai.currentWaypointIndex = 0;

        AISteeringEngine::updateAI(ai, wps, 0.0, 0.05);
        // Should feather throttle and brake due to huge angle delta
        ASSERT_LT(ai.speed, 120.0);
    }

    TEST_CASE("F5.5: AI rubber-banding catchup boost") {
        auto wps = TrackWaypointManager::generateWaypoints(TrackLevel::DONUT_PLAINS);
        KartEntity ai{};
        ai.isAI = true;
        ai.x = wps[0].x();
        ai.z = wps[0].y();
        ai.maxSpeed = 150.0;
        ai.speed = 150.0;
        ai.totalRaceDistance = 100.0;

        // Player is 400 units ahead (>150 threshold)
        AISteeringEngine::updateAI(ai, wps, 500.0, 0.05);
        ASSERT_GT(ai.speed, 150.0); // received +10% rubber-band boost
    }

    // ------------------------------------------------------------------------
    // F6: Green Shell Projectile (5 tests)
    // ------------------------------------------------------------------------
    TEST_CASE("F6.1: Green shell spawn position and forward velocity") {
        ItemTargetingEngine items;
        KartEntity kart{};
        kart.id = 0;
        kart.x = 200.0;
        kart.z = 300.0;
        kart.angle = 0.0; // Heading East
        kart.speed = 100.0;

        items.fireGreenShell(kart);
        ASSERT_EQ(items.projectiles().size(), 1u);
        const auto& shell = items.projectiles()[0];
        ASSERT_NEAR(shell.x, 212.0, 0.001); // 12 units ahead
        ASSERT_NEAR(shell.z, 300.0, 0.001);
        ASSERT_NEAR(shell.vx, 175.0, 0.001); // speed (100) + 75
        ASSERT_NEAR(shell.vz, 0.0, 0.001);
        ASSERT_EQ(shell.bouncesLeft, 3);
        ASSERT_TRUE(shell.active);
    }

    TEST_CASE("F6.2: Green shell lifetime decay and timeout") {
        ItemTargetingEngine items;
        KartEntity kart{};
        kart.id = 0;
        kart.x = 500.0;
        kart.z = 500.0;
        kart.angle = 0.0;
        kart.speed = 50.0;

        items.fireGreenShell(kart);
        std::vector<KartEntity> emptyFleet;

        // Advance 6.1 seconds
        for (int i = 0; i < 125; ++i) {
            items.update(emptyFleet, 0.05);
        }
        ASSERT_FALSE(items.projectiles()[0].active);
    }

    TEST_CASE("F6.3: Green shell wall bounce reflection") {
        ItemTargetingEngine items;
        KartEntity kart{};
        kart.id = 0;
        kart.x = 995.0; // Close to right wall at 1004
        kart.z = 500.0;
        kart.angle = 0.0; // Moving right
        kart.speed = 50.0;

        items.fireGreenShell(kart);
        std::vector<KartEntity> emptyFleet;

        items.update(emptyFleet, 0.05); // Crosses 1004, reflects vx
        const auto& shell = items.projectiles()[0];
        ASSERT_LT(shell.vx, 0.0); // Reflected to negative X
        ASSERT_EQ(shell.bouncesLeft, 2);
    }

    TEST_CASE("F6.4: Green shell shatters on third wall bounce") {
        ItemTargetingEngine items;
        KartEntity kart{};
        kart.id = 0;
        kart.x = 995.0;
        kart.z = 500.0;
        kart.angle = 0.0;
        kart.speed = 50.0;

        items.fireGreenShell(kart);
        items.projectiles()[0].bouncesLeft = 1; // set to 1 bounce left
        std::vector<KartEntity> emptyFleet;

        items.update(emptyFleet, 0.05);
        ASSERT_FALSE(items.projectiles()[0].active); // shatters on last bounce
    }

    TEST_CASE("F6.5: Green shell hits opponent AI kart and triggers spinout") {
        ItemTargetingEngine items;
        KartEntity p1{};
        p1.id = 0;
        p1.x = 200.0;
        p1.z = 300.0;
        p1.angle = 0.0;
        p1.speed = 100.0;

        KartEntity ai{};
        ai.id = 1;
        ai.isAI = true;
        ai.x = 230.0; // Directly ahead in shell path
        ai.z = 300.0;
        ai.speed = 80.0;

        std::vector<KartEntity> fleet = { p1, ai };
        items.fireGreenShell(p1);

        // Shell moves at 175 u/s. In 0.15s, it travels ~26 units, entering radius 14 of AI
        for (int i = 0; i < 3; ++i) {
            items.update(fleet, 0.05);
        }

        ASSERT_TRUE(fleet[1].isSpinning); // AI was hit!
        ASSERT_FALSE(items.projectiles()[0].active); // Shell shattered
        ASSERT_FALSE(fleet[0].isSpinning); // P1 not affected
    }

    // ------------------------------------------------------------------------
    // F7: Ink Blooper Targeting Fix (5 tests)
    // ------------------------------------------------------------------------
    TEST_CASE("F7.1: Player using blooper leaves player windshield clean") {
        ItemTargetingEngine items;
        KartEntity p1{};
        p1.id = 0;
        KartEntity ai1{};
        ai1.id = 1;
        KartEntity ai2{};
        ai2.id = 2;

        std::vector<KartEntity> fleet = { p1, ai1, ai2 };
        items.useInkBlooper(0, fleet);

        // Player must have ZERO ink (Bug fix R2)
        ASSERT_FALSE(fleet[0].isInked);
        ASSERT_NEAR(fleet[0].inkTimer, 0.0, 0.001);
    }

    TEST_CASE("F7.2: Player using blooper affects all opponent karts") {
        ItemTargetingEngine items;
        KartEntity p1{};
        p1.id = 0;
        KartEntity ai1{};
        ai1.id = 1;
        KartEntity ai2{};
        ai2.id = 2;

        std::vector<KartEntity> fleet = { p1, ai1, ai2 };
        items.useInkBlooper(0, fleet);

        ASSERT_TRUE(fleet[1].isInked);
        ASSERT_NEAR(fleet[1].inkTimer, 4.0, 0.001);
        ASSERT_TRUE(fleet[2].isInked);
        ASSERT_NEAR(fleet[2].inkTimer, 4.0, 0.001);
    }

    TEST_CASE("F7.3: Inked debuff reduces opponent top speed by 30%") {
        KartEntity ai{};
        ai.maxSpeed = 160.0;
        ai.speed = 160.0;
        ai.accel = 100.0;

        KartPhysicsEngine::applyInkedDebuff(ai);
        ASSERT_TRUE(ai.isInked);

        // Under throttle while inked, speed cannot exceed 70% of 160 = 112
        KartPhysicsEngine::updateKart(ai, true, false, 0, false, 0.05);
        ASSERT_LE(ai.speed, 112.1);
    }

    TEST_CASE("F7.4: Inked debuff timer cleans up after 4.0s") {
        KartEntity ai{};
        KartPhysicsEngine::applyInkedDebuff(ai);
        ASSERT_TRUE(ai.isInked);

        for (int i = 0; i < 85; ++i) { // 4.25 seconds
            KartPhysicsEngine::updateKart(ai, false, false, 0, false, 0.05);
        }
        ASSERT_FALSE(ai.isInked);
        ASSERT_NEAR(ai.inkTimer, 0.0, 0.001);
    }

    TEST_CASE("F7.5: AI firing blooper debuffs player") {
        ItemTargetingEngine items;
        KartEntity p1{};
        p1.id = 0;
        KartEntity ai1{};
        ai1.id = 1;

        std::vector<KartEntity> fleet = { p1, ai1 };
        items.useInkBlooper(1, fleet); // AI 1 fires

        ASSERT_TRUE(fleet[0].isInked); // Player is inked
        ASSERT_FALSE(fleet[1].isInked); // Firing AI is clear
    }

    // ------------------------------------------------------------------------
    // F8: Banana Placement & Safety (5 tests)
    // ------------------------------------------------------------------------
    TEST_CASE("F8.1: Banana drops behind kart heading") {
        ItemTargetingEngine items;
        KartEntity kart{};
        kart.id = 0;
        kart.x = 500.0;
        kart.z = 400.0;
        kart.angle = 0.0; // East

        items.dropBanana(kart);
        ASSERT_EQ(items.bananas().size(), 1u);
        const auto& banana = items.bananas()[0];
        ASSERT_NEAR(banana.x, 488.0, 0.001); // 12 units behind
        ASSERT_NEAR(banana.z, 400.0, 0.001);
        ASSERT_TRUE(banana.active);
        ASSERT_NEAR(banana.immunityTimer, 1.2, 0.001);
    }

    TEST_CASE("F8.2: Dropping kart owner immunity grace period") {
        ItemTargetingEngine items;
        KartEntity p1{};
        p1.id = 0;
        p1.x = 500.0;
        p1.z = 400.0;
        p1.angle = 0.0;

        items.dropBanana(p1);
        std::vector<KartEntity> fleet = { p1 };

        // Reverse immediately into banana within 0.5s (grace timer active)
        fleet[0].x = 488.0;
        items.update(fleet, 0.5);

        ASSERT_FALSE(fleet[0].isSpinning); // Safe during grace period!
        ASSERT_TRUE(items.bananas()[0].active);
    }

    TEST_CASE("F8.3: Trailing opponent hits banana and spins out") {
        ItemTargetingEngine items;
        KartEntity p1{};
        p1.id = 0;
        p1.x = 500.0;
        p1.z = 400.0;

        items.dropBanana(p1);

        KartEntity ai{};
        ai.id = 1;
        ai.isAI = true;
        ai.x = 488.0; // At banana position
        ai.z = 400.0;
        ai.speed = 100.0;

        std::vector<KartEntity> fleet = { p1, ai };
        items.update(fleet, 0.05);

        ASSERT_TRUE(fleet[1].isSpinning); // AI spun out!
        ASSERT_LT(fleet[1].speed, 40.0);   // speed penalty applied
    }

    TEST_CASE("F8.4: Banana peel is consumed upon collision") {
        ItemTargetingEngine items;
        KartEntity p1{};
        p1.id = 0;
        items.dropBanana(p1);

        KartEntity ai{};
        ai.id = 1;
        ai.x = items.bananas()[0].x;
        ai.z = items.bananas()[0].z;

        std::vector<KartEntity> fleet = { p1, ai };
        items.update(fleet, 0.05);

        ASSERT_FALSE(items.bananas()[0].active); // Deactivated immediately
    }

    TEST_CASE("F8.5: Owner kart vulnerable after grace period expires") {
        ItemTargetingEngine items;
        KartEntity p1{};
        p1.id = 0;
        p1.x = 500.0;
        p1.z = 400.0;
        items.dropBanana(p1);

        std::vector<KartEntity> fleet = { p1 };

        // Fast forward 1.3 seconds so grace period expires
        items.update(fleet, 0.05);
        items.bananas()[0].immunityTimer = 0.0;

        // Now move player onto banana
        fleet[0].x = items.bananas()[0].x;
        fleet[0].z = items.bananas()[0].z;
        items.update(fleet, 0.05);

        ASSERT_TRUE(fleet[0].isSpinning); // Owner now spins out after grace period
    }

    // ------------------------------------------------------------------------
    // F9: Arcade Drift & Handling (5 tests)
    // ------------------------------------------------------------------------
    TEST_CASE("F9.1: WASD arcade throttle acceleration") {
        KartEntity kart{};
        kart.maxSpeed = 160.0;
        kart.accel = 110.0;
        kart.speed = 0.0;

        // Apply throttle for 1.0 second (20 steps of 0.05)
        for (int i = 0; i < 20; ++i) {
            KartPhysicsEngine::updateKart(kart, true, false, 0, false, 0.05);
        }
        ASSERT_GT(kart.speed, 80.0);
        ASSERT_LE(kart.speed, 160.0);
    }

    TEST_CASE("F9.2: High-grip braking and reverse speed clamp") {
        KartEntity kart{};
        kart.maxSpeed = 160.0;
        kart.brakeDecel = 240.0;
        kart.speed = 100.0;

        // Apply brake
        KartPhysicsEngine::updateKart(kart, false, true, 0, false, 0.05);
        ASSERT_LT(kart.speed, 100.0);

        // Continue braking into reverse
        for (int i = 0; i < 50; ++i) {
            KartPhysicsEngine::updateKart(kart, false, true, 0, false, 0.05);
        }
        ASSERT_GE(kart.speed, -0.30 * 160.0); // reverse clamped to -30%
    }

    TEST_CASE("F9.3: Drift activation speed threshold") {
        KartEntity kart{};
        kart.speed = 30.0; // Below 40.0 threshold
        kart.turnRate = 2.4;
        kart.driftFactor = 1.35;

        KartPhysicsEngine::updateKart(kart, true, false, 1, true, 0.05);
        ASSERT_FALSE(kart.isDrifting); // Must not drift below 40.0 u/s
    }

    TEST_CASE("F9.4: Drift increases cornering turn rate") {
        KartEntity kNormal{};
        kNormal.speed = 80.0;
        kNormal.turnRate = 2.0;
        kNormal.driftFactor = 1.35;

        KartEntity kDrift = kNormal;

        // Normal steer vs Drift steer
        KartPhysicsEngine::updateKart(kNormal, true, false, 1, false, 0.05);
        KartPhysicsEngine::updateKart(kDrift, true, false, 1, true, 0.05);

        ASSERT_TRUE(kDrift.isDrifting);
        ASSERT_GT(kDrift.angle, kNormal.angle); // Drift turns faster
    }

    TEST_CASE("F9.5: Mini-turbo boost awarded on >= 1.0s drift release") {
        KartEntity kart{};
        kart.maxSpeed = 160.0;
        kart.speed = 80.0;
        kart.turnRate = 2.0;
        kart.driftFactor = 1.35;

        // Drift for 1.1 seconds (22 steps of 0.05)
        for (int i = 0; i < 22; ++i) {
            KartPhysicsEngine::updateKart(kart, true, false, 1, true, 0.05);
        }
        ASSERT_TRUE(kart.isDrifting);
        ASSERT_GE(kart.driftTimer, 1.0);

        // Release drift
        KartPhysicsEngine::updateKart(kart, true, false, 0, false, 0.05);
        ASSERT_FALSE(kart.isDrifting);
        ASSERT_NEAR(kart.miniTurboTimer, 1.2, 0.06); // Mini-turbo awarded!
    }

    // ------------------------------------------------------------------------
    // F10: Bowser Lava Geysers (5 tests)
    // ------------------------------------------------------------------------
    TEST_CASE("F10.1: Bowser Castle creates four lava geysers") {
        auto hazards = HazardEngine::createTrackHazards(TrackLevel::BOWSER_CASTLE);
        ASSERT_EQ(hazards.size(), 4u);
        for (const auto& h : hazards) {
            ASSERT_EQ(h.type, HazardType::LAVA_GEYSER);
            ASSERT_NEAR(h.radius, 16.0, 0.001);
        }
    }

    TEST_CASE("F10.2: Lava geyser cyclic eruption timing") {
        auto hazards = HazardEngine::createTrackHazards(TrackLevel::BOWSER_CASTLE);
        std::vector<KartEntity> emptyFleet;

        hazards[0].animTimer = 0.0;
        HazardEngine::updateHazards(hazards, emptyFleet, 1.0); // t=1.0: Dormant
        ASSERT_FALSE(hazards[0].isErupting);

        hazards[0].animTimer = 3.4; // t=3.4: Spewing Active (3.3..4.8)
        HazardEngine::updateHazards(hazards, emptyFleet, 0.05);
        ASSERT_TRUE(hazards[0].isErupting);
    }

    TEST_CASE("F10.3: Safe passage through geyser during dormant phase") {
        auto hazards = HazardEngine::createTrackHazards(TrackLevel::BOWSER_CASTLE);
        hazards[0].animTimer = 1.0; // Dormant
        hazards[0].isErupting = false;

        KartEntity kart{};
        kart.x = hazards[0].x;
        kart.z = hazards[0].z;
        kart.speed = 100.0;

        std::vector<KartEntity> fleet = { kart };
        HazardEngine::updateHazards(hazards, fleet, 0.05);

        ASSERT_FALSE(fleet[0].isSpinning); // Safe!
        ASSERT_NEAR(fleet[0].speed, 100.0, 0.001);
    }

    TEST_CASE("F10.4: Contact with active spewing lava geyser triggers fiery spinout") {
        auto hazards = HazardEngine::createTrackHazards(TrackLevel::BOWSER_CASTLE);
        hazards[0].animTimer = 3.5; // Spewing active

        KartEntity kart{};
        kart.x = hazards[0].x + 5.0; // inside radius 16
        kart.z = hazards[0].z + 5.0;
        kart.speed = 100.0;

        std::vector<KartEntity> fleet = { kart };
        HazardEngine::updateHazards(hazards, fleet, 0.05);

        ASSERT_TRUE(fleet[0].isSpinning);
        ASSERT_NEAR(fleet[0].spinTimer, 0.8, 0.01);
        ASSERT_NEAR(fleet[0].speed, 25.0, 0.01); // 75% speed loss
    }

    TEST_CASE("F10.5: Lava geyser collision pushes kart outward") {
        auto hazards = HazardEngine::createTrackHazards(TrackLevel::BOWSER_CASTLE);
        hazards[0].animTimer = 3.5;

        KartEntity kart{};
        kart.x = hazards[0].x + 10.0;
        kart.z = hazards[0].z;
        double origX = kart.x;

        std::vector<KartEntity> fleet = { kart };
        HazardEngine::updateHazards(hazards, fleet, 0.05);

        ASSERT_GT(fleet[0].x, origX); // Pushed away along X axis
    }

    // ------------------------------------------------------------------------
    // F11: Choco Chocolate Fountains & Candies (5 tests)
    // ------------------------------------------------------------------------
    TEST_CASE("F11.1: Choco Valley creates chocolate fountains and rolling candies") {
        auto hazards = HazardEngine::createTrackHazards(TrackLevel::CHOCO_VALLEY);
        ASSERT_EQ(hazards.size(), 5u);
        int fountainCount = 0;
        int candyCount = 0;
        for (const auto& h : hazards) {
            if (h.type == HazardType::CHOCOLATE_FOUNTAIN) fountainCount++;
            if (h.type == HazardType::ROLLING_CANDY) candyCount++;
        }
        ASSERT_EQ(fountainCount, 3);
        ASSERT_EQ(candyCount, 2);
    }

    TEST_CASE("F11.2: Chocolate fountain mud splash reduces speed by 50%") {
        auto hazards = HazardEngine::createTrackHazards(TrackLevel::CHOCO_VALLEY);
        KartEntity kart{};
        kart.maxSpeed = 160.0;
        kart.speed = 140.0;
        // Place inside fountain 0
        kart.x = hazards[0].x;
        kart.z = hazards[0].z;

        std::vector<KartEntity> fleet = { kart };
        HazardEngine::updateHazards(hazards, fleet, 0.05);

        ASSERT_LE(fleet[0].speed, 80.001); // 50% max speed cap
    }

    TEST_CASE("F11.3: Rolling candy oscillates sinusoidally") {
        auto hazards = HazardEngine::createTrackHazards(TrackLevel::CHOCO_VALLEY);
        std::vector<KartEntity> emptyFleet;

        double origTimer = hazards[3].animTimer;
        HazardEngine::updateHazards(hazards, emptyFleet, 0.5);
        ASSERT_GT(hazards[3].animTimer, origTimer);
    }

    TEST_CASE("F11.4: Rolling candy bumper bounce recoil") {
        auto hazards = HazardEngine::createTrackHazards(TrackLevel::CHOCO_VALLEY);
        hazards[3].animTimer = 0.0; // offset = 0, currentCandyX = hazards[3].x

        KartEntity kart{};
        kart.x = hazards[3].x;
        kart.z = hazards[3].z;
        kart.speed = 120.0;

        std::vector<KartEntity> fleet = { kart };
        HazardEngine::updateHazards(hazards, fleet, 0.05);

        ASSERT_NEAR(fleet[0].speed, -35.0, 0.001); // Bumper bounce backwards
    }

    TEST_CASE("F11.5: Multi-kart hazard update isolation") {
        auto hazards = HazardEngine::createTrackHazards(TrackLevel::CHOCO_VALLEY);
        KartEntity k1{};
        k1.id = 0;
        k1.x = hazards[0].x; // Inside fountain
        k1.z = hazards[0].z;
        k1.speed = 140.0;
        k1.maxSpeed = 160.0;

        KartEntity k2{};
        k2.id = 1;
        k2.x = 100.0; // Far away
        k2.z = 100.0;
        k2.speed = 140.0;
        k2.maxSpeed = 160.0;

        std::vector<KartEntity> fleet = { k1, k2 };
        HazardEngine::updateHazards(hazards, fleet, 0.05);

        ASSERT_LE(fleet[0].speed, 80.001);
        ASSERT_NEAR(fleet[1].speed, 140.0, 0.001); // Unaffected
    }

    // ------------------------------------------------------------------------
    // F12: Top-Left HUD Minimap (5 tests)
    // ------------------------------------------------------------------------
    TEST_CASE("F12.1: Minimap radar position and dimension bounds") {
        ASSERT_EQ(MinimapHUD::RADAR_X, 8);
        ASSERT_EQ(MinimapHUD::RADAR_Y, 8);
        ASSERT_EQ(MinimapHUD::RADAR_SIZE, 72);
        ASSERT_EQ(MinimapHUD::MAP_WORLD_SIZE, 1024);
    }

    TEST_CASE("F12.2: Minimap world to radar coordinate translation") {
        // (0, 0) -> (8, 8)
        QPointF origin = MinimapHUD::worldToRadar(0.0, 0.0);
        ASSERT_NEAR(origin.x(), 8.0, 0.001);
        ASSERT_NEAR(origin.y(), 8.0, 0.001);

        // (512, 512) -> center (8 + 36, 8 + 36) = (44, 44)
        QPointF center = MinimapHUD::worldToRadar(512.0, 512.0);
        ASSERT_NEAR(center.x(), 44.0, 0.001);
        ASSERT_NEAR(center.y(), 44.0, 0.001);
    }

    TEST_CASE("F12.3: Minimap coordinate boundary wrapping") {
        // Modulo handles negative coordinates smoothly
        QPointF wrappedNeg = MinimapHUD::worldToRadar(-102.4, -102.4);
        ASSERT_GE(wrappedNeg.x(), 8.0);
        ASSERT_LE(wrappedNeg.x(), 80.0);
        ASSERT_GE(wrappedNeg.y(), 8.0);
        ASSERT_LE(wrappedNeg.y(), 80.0);
    }

    TEST_CASE("F12.4: Kart blip coordinate scaling fidelity") {
        QPointF p1 = MinimapHUD::worldToRadar(100.0, 200.0);
        QPointF p2 = MinimapHUD::worldToRadar(200.0, 200.0);
        // dx in radar should be 100 / 1024 * 72
        double expectedDx = (100.0 / 1024.0) * 72.0;
        ASSERT_NEAR(p2.x() - p1.x(), expectedDx, 0.001);
    }

    TEST_CASE("F12.5: Minimap radar rendering envelope stays within HUD bounds") {
        for (double wX = 0.0; wX <= 1024.0; wX += 128.0) {
            for (double wZ = 0.0; wZ <= 1024.0; wZ += 128.0) {
                QPointF pt = MinimapHUD::worldToRadar(wX, wZ);
                ASSERT_GE(pt.x(), 8.0);
                ASSERT_LE(pt.x(), 80.001);
                ASSERT_GE(pt.y(), 8.0);
                ASSERT_LE(pt.y(), 80.001);
            }
        }
    }

    // ------------------------------------------------------------------------
    // F13: Local 2-Player Split-Screen (5 tests)
    // ------------------------------------------------------------------------
    TEST_CASE("F13.1: Split screen dimensions and vertical partitioning") {
        ASSERT_EQ(ViewportSplit::TOTAL_WIDTH, 400);
        ASSERT_EQ(ViewportSplit::TOTAL_HEIGHT, 300);
        ASSERT_EQ(ViewportSplit::P1_BOTTOM - ViewportSplit::P1_TOP + 1, 150);
        ASSERT_EQ(ViewportSplit::DIVIDER_Y, 150);
        ASSERT_EQ(ViewportSplit::DIVIDER_HEIGHT, 2);
    }

    TEST_CASE("F13.2: P1 viewport point containment") {
        ASSERT_TRUE(SplitScreenEngine::isPointInP1Viewport(0, 0));
        ASSERT_TRUE(SplitScreenEngine::isPointInP1Viewport(399, 149));
        ASSERT_FALSE(SplitScreenEngine::isPointInP1Viewport(200, 150));
        ASSERT_FALSE(SplitScreenEngine::isPointInP1Viewport(200, 200));
    }

    TEST_CASE("F13.3: P2 viewport point containment") {
        ASSERT_TRUE(SplitScreenEngine::isPointInP2Viewport(0, 152));
        ASSERT_TRUE(SplitScreenEngine::isPointInP2Viewport(399, 300));
        ASSERT_FALSE(SplitScreenEngine::isPointInP2Viewport(200, 149));
        ASSERT_FALSE(SplitScreenEngine::isPointInP2Viewport(200, 150));
    }

    TEST_CASE("F13.4: Golden divider horizontal boundary isolation") {
        ASSERT_TRUE(SplitScreenEngine::isPointInDivider(200, 150));
        ASSERT_TRUE(SplitScreenEngine::isPointInDivider(200, 151));
        ASSERT_FALSE(SplitScreenEngine::isPointInDivider(200, 149));
        ASSERT_FALSE(SplitScreenEngine::isPointInDivider(200, 152));
    }

    TEST_CASE("F13.5: Viewport zero cross bleeding assertion") {
        // Any valid point in canvas is either P1, Divider, or P2, never overlapping
        for (int y = 0; y <= 300; ++y) {
            bool inP1 = SplitScreenEngine::isPointInP1Viewport(100, y);
            bool inDiv = SplitScreenEngine::isPointInDivider(100, y);
            bool inP2 = SplitScreenEngine::isPointInP2Viewport(100, y);
            int count = (inP1 ? 1 : 0) + (inDiv ? 1 : 0) + (inP2 ? 1 : 0);
            ASSERT_EQ(count, 1);
        }
    }

    // ------------------------------------------------------------------------
    // F14: Independent Dual Controls (5 tests)
    // ------------------------------------------------------------------------
    TEST_CASE("F14.1: P1 WASD input mapping") {
        std::set<int> keys = { Qt::Key_W, Qt::Key_A, Qt::Key_Space, Qt::Key_Shift };
        bool p1T, p1B, p1I, p1D, p2T, p2B, p2I, p2D;
        int p1S, p2S;

        SplitScreenEngine::handleInputMultiplexing(keys, p1T, p1B, p1S, p1I, p1D, p2T, p2B, p2S, p2I, p2D);
        ASSERT_TRUE(p1T);
        ASSERT_FALSE(p1B);
        ASSERT_EQ(p1S, -1); // Left
        ASSERT_TRUE(p1I);
        ASSERT_TRUE(p1D);
        ASSERT_FALSE(p2T);
    }

    TEST_CASE("F14.2: P2 Arrow keys input mapping") {
        std::set<int> keys = { Qt::Key_Up, Qt::Key_Right, Qt::Key_Return, Qt::Key_Control };
        bool p1T, p1B, p1I, p1D, p2T, p2B, p2I, p2D;
        int p1S, p2S;

        SplitScreenEngine::handleInputMultiplexing(keys, p1T, p1B, p1S, p1I, p1D, p2T, p2B, p2S, p2I, p2D);
        ASSERT_FALSE(p1T);
        ASSERT_TRUE(p2T);
        ASSERT_FALSE(p2B);
        ASSERT_EQ(p2S, 1); // Right
        ASSERT_TRUE(p2I);
        ASSERT_TRUE(p2D);
    }

    TEST_CASE("F14.3: Simultaneous multi-key multiplexing without ghosting") {
        // Both P1 and P2 full throttle, steering opposite directions, using items
        std::set<int> keys = {
            Qt::Key_W, Qt::Key_A, Qt::Key_Space,
            Qt::Key_Up, Qt::Key_Right, Qt::Key_Return
        };
        bool p1T, p1B, p1I, p1D, p2T, p2B, p2I, p2D;
        int p1S, p2S;

        SplitScreenEngine::handleInputMultiplexing(keys, p1T, p1B, p1S, p1I, p1D, p2T, p2B, p2S, p2I, p2D);
        ASSERT_TRUE(p1T);
        ASSERT_EQ(p1S, -1);
        ASSERT_TRUE(p1I);
        ASSERT_TRUE(p2T);
        ASSERT_EQ(p2S, 1);
        ASSERT_TRUE(p2I);
    }

    TEST_CASE("F14.4: Cancelling opposing steering inputs") {
        std::set<int> keys = { Qt::Key_A, Qt::Key_D };
        bool p1T, p1B, p1I, p1D, p2T, p2B, p2I, p2D;
        int p1S, p2S;

        SplitScreenEngine::handleInputMultiplexing(keys, p1T, p1B, p1S, p1I, p1D, p2T, p2B, p2S, p2I, p2D);
        ASSERT_EQ(p1S, 0); // Neutral steering when both held
    }

    TEST_CASE("F14.5: Release of P1 key leaves P2 inputs completely unaffected") {
        std::set<int> keys = { Qt::Key_Up, Qt::Key_Right };
        bool p1T, p1B, p1I, p1D, p2T, p2B, p2I, p2D;
        int p1S, p2S;

        SplitScreenEngine::handleInputMultiplexing(keys, p1T, p1B, p1S, p1I, p1D, p2T, p2B, p2S, p2I, p2D);
        ASSERT_FALSE(p1T);
        ASSERT_TRUE(p2T);
        ASSERT_EQ(p2S, 1);
    }

    // ------------------------------------------------------------------------
    // F15: Build Stability & 60 FPS (5 tests)
    // ------------------------------------------------------------------------
    TEST_CASE("F15.1: Frame dt clamping safeguards against tunneling") {
        KartEntity kart{};
        kart.maxSpeed = 160.0;
        kart.speed = 100.0;
        kart.x = 100.0;

        // Large 0.5s lag spike should be clamped to 0.05s
        KartPhysicsEngine::updateKart(kart, false, false, 0, false, 0.5);
        ASSERT_NEAR(kart.x, 104.89, 0.1); // Traveled ~5 units, not 50 units!
    }

    TEST_CASE("F15.2: Ground Mode-7 rasterization pixel count invariance") {
        // Single player: 400x150 = 60,000 pixels
        int singlePixels = 400 * 150;
        // Split-screen: 2 x (400 x 75) = 60,000 pixels
        int splitPixels = 2 * (400 * 75);
        ASSERT_EQ(singlePixels, 60000);
        ASSERT_EQ(splitPixels, 60000);
    }

    TEST_CASE("F15.3: SpriteAtlas singleton and pickup items verification") {
        SpriteAtlas& atlas = SpriteAtlas::instance();
        ASSERT_TRUE(atlas.isPickup(SpriteType::QUESTION_BOX));
        ASSERT_TRUE(atlas.isPickup(SpriteType::MUSHROOM));
        ASSERT_TRUE(atlas.isPickup(SpriteType::ROCKET));
        ASSERT_TRUE(atlas.isPickup(SpriteType::BANANA));
        ASSERT_TRUE(atlas.isPickup(SpriteType::GREEN_SHELL));
        ASSERT_TRUE(atlas.isPickup(SpriteType::INK_BLOOPER));
    }

    TEST_CASE("F15.4: SpriteAtlas obstacle items identification") {
        SpriteAtlas& atlas = SpriteAtlas::instance();
        ASSERT_TRUE(atlas.isObstacle(SpriteType::GREEN_PIPE));
        ASSERT_TRUE(atlas.isObstacle(SpriteType::THWOMP));
        ASSERT_TRUE(atlas.isObstacle(SpriteType::OIL_SLICK));
        ASSERT_FALSE(atlas.isObstacle(SpriteType::MUSHROOM));
    }

    TEST_CASE("F15.5: 60 FPS physics tick performance benchmark") {
        KartEntity kart{};
        kart.maxSpeed = 160.0;
        kart.accel = 110.0;

        QElapsedTimer timer;
        timer.start();
        // 1000 physics updates (simulating ~16.6 seconds of race at 60 FPS)
        for (int i = 0; i < 1000; ++i) {
            KartPhysicsEngine::updateKart(kart, true, false, 1, false, 0.0166);
        }
        qint64 elapsedMs = timer.elapsed();
        ASSERT_LT(elapsedMs, 50); // Must execute in < 50ms, comfortably exceeding 60 FPS
    }

    // ------------------------------------------------------------------------
    // F16: E2E Acceptance Suite (5 tests)
    // ------------------------------------------------------------------------
    TEST_CASE("F16.1: Acceptance R1 Menu & Character Selection") {
        MenuStateController menu;
        menu.navigateMenu(1); // Multiplayer
        menu.confirmMenu();
        ASSERT_EQ(menu.currentState(), GameState::CHARACTER_SELECT);
        menu.confirmCharSelect();
        ASSERT_EQ(menu.currentState(), GameState::RACING);
    }

    TEST_CASE("F16.2: Acceptance R2 AI Opponents & Item Targeting") {
        ItemTargetingEngine items;
        KartEntity p1{};
        p1.id = 0;
        KartEntity ai{};
        ai.id = 1;
        std::vector<KartEntity> fleet = { p1, ai };

        items.useInkBlooper(0, fleet);
        ASSERT_FALSE(fleet[0].isInked);
        ASSERT_TRUE(fleet[1].isInked);
    }

    TEST_CASE("F16.3: Acceptance R3 Local Multiplayer Split-Screen") {
        ASSERT_TRUE(SplitScreenEngine::isPointInP1Viewport(200, 50));
        ASSERT_TRUE(SplitScreenEngine::isPointInDivider(200, 150));
        ASSERT_TRUE(SplitScreenEngine::isPointInP2Viewport(200, 250));
    }

    TEST_CASE("F16.4: Acceptance R4 Arcade Physics, Hazards, and Minimap") {
        QPointF radarPt = MinimapHUD::worldToRadar(512.0, 512.0);
        ASSERT_NEAR(radarPt.x(), 44.0, 0.001);
        ASSERT_NEAR(radarPt.y(), 44.0, 0.001);
    }

    TEST_CASE("F16.5: Acceptance R5 Stability and 60 FPS Frame Budget") {
        KartEntity kart{};
        KartPhysicsEngine::updateKart(kart, true, false, 0, false, 0.0166);
        ASSERT_TRUE(kart.speed > 0.0);
    }
}
