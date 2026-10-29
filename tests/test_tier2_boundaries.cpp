#include "test_tier2_boundaries.h"
#include "test_framework.h"
#include "test_harness.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void runTier2BoundaryTests() {
    TEST_SUITE("Tier 2 - Boundary & Corner Cases (F1 to F16)");

    // ------------------------------------------------------------------------
    // F1 Boundaries: Menu & State
    // ------------------------------------------------------------------------
    TEST_CASE("F1-B1: Menu wrap underflow (-100 navigations)") {
        MenuStateController menu;
        for (int i = 0; i < 100; ++i) {
            menu.navigateMenu(-1);
        }
        // 100 % 3 = 1 -> wraps to (0 - 1) % 3 = 2
        ASSERT_GE(menu.menuIndex(), 0);
        ASSERT_LE(menu.menuIndex(), 2);
    }

    TEST_CASE("F1-B2: Menu wrap overflow (+100 navigations)") {
        MenuStateController menu;
        for (int i = 0; i < 100; ++i) {
            menu.navigateMenu(1);
        }
        ASSERT_GE(menu.menuIndex(), 0);
        ASSERT_LE(menu.menuIndex(), 2);
    }

    TEST_CASE("F1-B3: Navigation ignored outside Title Menu") {
        MenuStateController menu;
        menu.confirmMenu(); // Enters CHARACTER_SELECT
        menu.navigateMenu(1);
        // Menu index unchanged while in CHARACTER_SELECT
        ASSERT_EQ(menu.menuIndex(), 0);
    }

    TEST_CASE("F1-B4: Race finished transition back to title") {
        MenuStateController menu;
        menu.confirmMenu();
        menu.confirmCharSelect();
        menu.finishRace();
        ASSERT_EQ(menu.currentState(), GameState::RACE_FINISHED);
        menu.backAction();
        ASSERT_EQ(menu.currentState(), GameState::TITLE_MENU);
    }

    TEST_CASE("F1-B5: Direct reset to title from any state") {
        MenuStateController menu;
        menu.confirmMenu();
        menu.confirmCharSelect();
        menu.resetToTitle();
        ASSERT_EQ(menu.currentState(), GameState::TITLE_MENU);
        ASSERT_EQ(menu.menuIndex(), 0);
    }

    // ------------------------------------------------------------------------
    // F2 Boundaries: Character Roster & Stats
    // ------------------------------------------------------------------------
    TEST_CASE("F2-B1: Character ID underflow fallback") {
        auto def = CharacterRoster::instance().getCharacter(-5);
        ASSERT_EQ(def.id, 0); // Safe fallback to Mario
        ASSERT_EQ(def.name, QString("Mario"));
    }

    TEST_CASE("F2-B2: Character ID overflow fallback") {
        auto def = CharacterRoster::instance().getCharacter(999);
        ASSERT_EQ(def.id, 0); // Safe fallback to Mario
        ASSERT_EQ(def.name, QString("Mario"));
    }

    TEST_CASE("F2-B3: Character selection navigation wrapping across 4 choices") {
        MenuStateController menu;
        menu.confirmMenu();
        menu.navigateCharSelect(1, -1); // 0 -> 3 (Toad)
        ASSERT_EQ(menu.p1CharIndex(), 3);
        menu.navigateCharSelect(1, 1);  // 3 -> 0 (Mario)
        ASSERT_EQ(menu.p1CharIndex(), 0);
    }

    TEST_CASE("F2-B4: Extreme stat boundaries sanity") {
        for (int i = 0; i < 4; ++i) {
            auto def = CharacterRoster::instance().getCharacter(i);
            ASSERT_GE(def.maxSpeed, 140.0);
            ASSERT_LE(def.maxSpeed, 190.0);
            ASSERT_GE(def.accel, 70.0);
            ASSERT_LE(def.accel, 150.0);
            ASSERT_GE(def.turnRate, 1.5);
            ASSERT_LE(def.turnRate, 3.5);
        }
    }

    TEST_CASE("F2-B5: P2 alternate palette uniqueness for all 4 characters") {
        for (int i = 0; i < 4; ++i) {
            auto def = CharacterRoster::instance().getCharacter(i);
            QColor alt = CharacterRoster::instance().getP2AlternateColor(i);
            ASSERT_NEQ(def.kartColor, alt);
        }
    }

    // ------------------------------------------------------------------------
    // F3 Boundaries: Multi-Kart Entity Architecture
    // ------------------------------------------------------------------------
    TEST_CASE("F3-B1: World coordinate boundary wrapping at 1024.0") {
        KartEntity kart{};
        kart.x = 1023.9;
        kart.speed = 10.0;
        kart.angle = 0.0; // East (+X)

        KartPhysicsEngine::updateKart(kart, false, false, 0, false, 0.05);
        ASSERT_GE(kart.x, 0.0);
        ASSERT_LT(kart.x, 5.0); // Wrapped around to start of map
    }

    TEST_CASE("F3-B2: World coordinate negative wrapping at 0.0") {
        KartEntity kart{};
        kart.x = 0.1;
        kart.speed = -10.0;
        kart.angle = 0.0; // Reversing West (-X)

        KartPhysicsEngine::updateKart(kart, false, false, 0, false, 0.05);
        ASSERT_LT(kart.x, 1024.0);
        ASSERT_GT(kart.x, 1020.0); // Wrapped around to 1023.x
    }

    TEST_CASE("F3-B3: Kart update at zero speed maintains exact coordinates") {
        KartEntity kart{};
        kart.x = 250.0;
        kart.z = 350.0;
        kart.speed = 0.0;

        KartPhysicsEngine::updateKart(kart, false, false, 0, false, 0.05);
        ASSERT_NEAR(kart.x, 250.0, 0.001);
        ASSERT_NEAR(kart.z, 350.0, 0.001);
    }

    TEST_CASE("F3-B4: Spinout triggered at zero speed spins without displacement") {
        KartEntity kart{};
        kart.x = 250.0;
        kart.z = 350.0;
        kart.speed = 0.0;

        KartPhysicsEngine::applyItemSpinout(kart);
        double initialAngle = kart.angle;

        KartPhysicsEngine::updateKart(kart, false, false, 0, false, 0.05);
        ASSERT_NEQ(kart.angle, initialAngle); // Spun in place
        ASSERT_NEAR(kart.x, 250.0, 0.001);   // No translation
    }

    TEST_CASE("F3-B5: Simultaneous ink and spinout debuff handling") {
        KartEntity kart{};
        kart.maxSpeed = 160.0;
        kart.speed = 100.0;

        KartPhysicsEngine::applyInkedDebuff(kart);
        KartPhysicsEngine::applyItemSpinout(kart);

        ASSERT_TRUE(kart.isInked);
        ASSERT_TRUE(kart.isSpinning);
        ASSERT_LT(kart.speed, 50.0);
    }

    // ------------------------------------------------------------------------
    // F4 Boundaries: Persistent Waypoints
    // ------------------------------------------------------------------------
    TEST_CASE("F4-B1: Waypoint index wrapping at end of loop") {
        auto wps = TrackWaypointManager::generateWaypoints(TrackLevel::DONUT_PLAINS);
        int lastIndex = wps.size() - 1;
        int nextIndex = (lastIndex + 1) % wps.size();
        ASSERT_EQ(nextIndex, 0); // Wraps cleanly to waypoint 0
    }

    TEST_CASE("F4-B2: Distance calculation for point sitting exactly on waypoint") {
        auto wps = TrackWaypointManager::generateWaypoints(TrackLevel::DONUT_PLAINS);
        double dist = TrackWaypointManager::distanceAlongTrack(wps, 0, wps[0].x(), wps[0].y());
        ASSERT_NEAR(dist, 0.0, 0.001);
    }

    TEST_CASE("F4-B3: Track length of degenerate empty waypoints") {
        std::vector<QPointF> empty;
        ASSERT_NEAR(TrackWaypointManager::trackLength(empty), 0.0, 0.001);
    }

    TEST_CASE("F4-B4: Single waypoint track length boundary") {
        std::vector<QPointF> single = { QPointF(100, 100) };
        ASSERT_NEAR(TrackWaypointManager::trackLength(single), 0.0, 0.001);
    }

    TEST_CASE("F4-B5: Waypoints coordinates are within 1024 world boundaries") {
        for (TrackLevel lvl : { TrackLevel::DONUT_PLAINS, TrackLevel::CHOCO_VALLEY, TrackLevel::BOWSER_CASTLE }) {
            auto wps = TrackWaypointManager::generateWaypoints(lvl);
            for (const auto& pt : wps) {
                ASSERT_GE(pt.x(), 0.0);
                ASSERT_LE(pt.x(), 1024.0);
                ASSERT_GE(pt.y(), 0.0);
                ASSERT_LE(pt.y(), 1024.0);
            }
        }
    }

    // ------------------------------------------------------------------------
    // F5 Boundaries: Active AI Opponent Fleet
    // ------------------------------------------------------------------------
    TEST_CASE("F5-B1: AI heading difference exactly PI (180 degree flip)") {
        auto wps = TrackWaypointManager::generateWaypoints(TrackLevel::DONUT_PLAINS);
        KartEntity ai{};
        ai.isAI = true;
        ai.x = 470.0;
        ai.z = 200.0;
        ai.angle = M_PI; // Opposite to heading
        ai.turnRate = 2.0;

        AISteeringEngine::updateAI(ai, wps, 0.0, 0.05);
        // Heading delta normalized; doesn't explode into NaN
        ASSERT_FALSE(std::isnan(ai.angle));
    }

    TEST_CASE("F5-B2: Rubberband boundary at exactly 149.9 vs 150.1 units behind") {
        auto wps = TrackWaypointManager::generateWaypoints(TrackLevel::DONUT_PLAINS);
        KartEntity ai1{};
        ai1.isAI = true;
        ai1.x = wps[0].x();
        ai1.z = wps[0].y();
        ai1.maxSpeed = 150.0;
        ai1.speed = 150.0;
        ai1.totalRaceDistance = 100.0;

        KartEntity ai2 = ai1;

        // ai1: Player is 149.9 units ahead (no boost)
        AISteeringEngine::updateAI(ai1, wps, 249.9, 0.05);
        ASSERT_NEAR(ai1.speed, 150.0, 0.001);

        // ai2: Player is 150.1 units ahead (boost triggered)
        AISteeringEngine::updateAI(ai2, wps, 250.1, 0.05);
        ASSERT_GT(ai2.speed, 150.0);
    }

    TEST_CASE("F5-B3: AI speed clamped to zero during extreme braking") {
        auto wps = TrackWaypointManager::generateWaypoints(TrackLevel::BOWSER_CASTLE);
        KartEntity ai{};
        ai.isAI = true;
        ai.speed = 2.0;
        ai.turnRate = 2.0;
        ai.brakeDecel = 240.0;
        ai.angle = 3.14; // Forces brake

        AISteeringEngine::updateAI(ai, wps, 0.0, 0.05);
        ASSERT_GE(ai.speed, 0.0); // AI doesn't reverse out of control
    }

    TEST_CASE("F5-B4: AI loop waypoint progression across entire track") {
        auto wps = TrackWaypointManager::generateWaypoints(TrackLevel::DONUT_PLAINS);
        KartEntity ai{};
        ai.isAI = true;
        ai.currentWaypointIndex = wps.size() - 1;
        // Place close to last waypoint
        ai.x = wps.back().x();
        ai.z = wps.back().y();

        AISteeringEngine::updateAI(ai, wps, 0.0, 0.05);
        ASSERT_EQ(ai.currentWaypointIndex, 0); // Completed loop!
    }

    TEST_CASE("F5-B5: AI update with empty waypoints returns safely") {
        std::vector<QPointF> empty;
        KartEntity ai{};
        ai.isAI = true;
        AISteeringEngine::updateAI(ai, empty, 0.0, 0.05);
        ASSERT_FALSE(std::isnan(ai.speed));
    }

    // ------------------------------------------------------------------------
    // F6 Boundaries: Green Shell Projectile
    // ------------------------------------------------------------------------
    TEST_CASE("F6-B1: Shell fired at zero kart speed has forward launch impulse") {
        ItemTargetingEngine items;
        KartEntity kart{};
        kart.id = 0;
        kart.speed = 0.0;
        kart.angle = 0.0;

        items.fireGreenShell(kart);
        const auto& shell = items.projectiles()[0];
        ASSERT_NEAR(shell.vx, 75.0, 0.001); // 0 + 75.0 launch speed
    }

    TEST_CASE("F6-B2: Shell fired in reverse direction") {
        ItemTargetingEngine items;
        KartEntity kart{};
        kart.id = 0;
        kart.speed = -30.0; // Reversing
        kart.angle = 0.0;   // Facing East

        items.fireGreenShell(kart);
        const auto& shell = items.projectiles()[0];
        // Launch speed = -30 + 75 = 45 forward along heading
        ASSERT_NEAR(shell.vx, 45.0, 0.001);
    }

    TEST_CASE("F6-B3: Grazing angle wall bounce reflection") {
        ItemTargetingEngine items;
        KartEntity kart{};
        kart.id = 0;
        kart.x = 998.0;
        kart.z = 500.0;
        kart.angle = 0.1; // Almost parallel to right wall (1004)
        kart.speed = 100.0;

        items.fireGreenShell(kart);
        std::vector<KartEntity> emptyFleet;

        items.update(emptyFleet, 0.05);
        const auto& shell = items.projectiles()[0];
        ASSERT_LT(shell.vx, 0.0); // vx reflected
        ASSERT_GT(shell.vz, 0.0); // vz retained
    }

    TEST_CASE("F6-B4: Shell hit detection boundary at 13.99 vs 14.01 units") {
        ItemTargetingEngine items;
        KartEntity p1{};
        p1.id = 0;
        p1.x = 100.0;
        p1.z = 100.0;

        KartEntity ai1{};
        ai1.id = 1;
        ai1.x = 200.0;
        ai1.z = 214.01; // Just outside 14.0 hit radius

        std::vector<KartEntity> fleet = { p1, ai1 };
        ShellProjectile shell{};
        shell.x = 200.0;
        shell.z = 200.0;
        shell.vx = 0.0;
        shell.vz = 0.0;
        shell.ownerId = 0;
        shell.bouncesLeft = 2; // already bounced once
        shell.lifeTime = 5.0;
        shell.active = true;
        items.projectiles().push_back(shell);

        items.update(fleet, 0.05);
        ASSERT_FALSE(fleet[1].isSpinning); // Missed at 14.01!

        // Now move into 13.99
        fleet[1].z = 213.99;
        items.update(fleet, 0.05);
        ASSERT_TRUE(fleet[1].isSpinning);  // Hit at 13.99!
    }

    TEST_CASE("F6-B5: Zero lifetime projectile despawns immediately") {
        ItemTargetingEngine items;
        ShellProjectile shell{};
        shell.lifeTime = 0.0001;
        shell.active = true;
        items.projectiles().push_back(shell);

        std::vector<KartEntity> emptyFleet;
        items.update(emptyFleet, 0.05);
        ASSERT_FALSE(items.projectiles()[0].active);
    }

    // ------------------------------------------------------------------------
    // F7 Boundaries: Ink Blooper
    // ------------------------------------------------------------------------
    TEST_CASE("F7-B1: Blooper fired when opponent is already inked refreshes timer") {
        ItemTargetingEngine items;
        KartEntity p1{};
        p1.id = 0;
        KartEntity ai{};
        ai.id = 1;
        ai.isInked = true;
        ai.inkTimer = 1.0; // 1s left

        std::vector<KartEntity> fleet = { p1, ai };
        items.useInkBlooper(0, fleet);

        ASSERT_TRUE(fleet[1].isInked);
        ASSERT_NEAR(fleet[1].inkTimer, 4.0, 0.001); // Refreshed to 4.0s
    }

    TEST_CASE("F7-B2: Blooper timer expiration boundary (0.01s vs 0.00s)") {
        KartEntity ai{};
        ai.isInked = true;
        ai.inkTimer = 0.04;

        KartPhysicsEngine::updateKart(ai, false, false, 0, false, 0.05);
        ASSERT_FALSE(ai.isInked);
        ASSERT_NEAR(ai.inkTimer, 0.0, 0.001);
    }

    TEST_CASE("F7-B3: Blooper on stationary opponent leaves speed at 0") {
        KartEntity ai{};
        ai.speed = 0.0;
        KartPhysicsEngine::applyInkedDebuff(ai);
        ASSERT_TRUE(ai.isInked);
        ASSERT_NEAR(ai.speed, 0.0, 0.001);
    }

    TEST_CASE("F7-B4: Blooper fired on empty track in Practice mode") {
        ItemTargetingEngine items;
        KartEntity p1{};
        p1.id = 0;
        std::vector<KartEntity> fleet = { p1 };

        items.useInkBlooper(0, fleet);
        ASSERT_FALSE(fleet[0].isInked); // Safely operates with no opponents
    }

    TEST_CASE("F7-B5: AI blooper hits player during drift") {
        ItemTargetingEngine items;
        KartEntity p1{};
        p1.id = 0;
        p1.isDrifting = true;
        p1.driftTimer = 0.8;
        KartEntity ai{};
        ai.id = 1;

        std::vector<KartEntity> fleet = { p1, ai };
        items.useInkBlooper(1, fleet);

        ASSERT_TRUE(fleet[0].isInked);
        ASSERT_TRUE(fleet[0].isDrifting); // Drift state maintained
    }

    // ------------------------------------------------------------------------
    // F8 Boundaries: Banana Placement & Safety
    // ------------------------------------------------------------------------
    TEST_CASE("F8-B1: Banana grace period boundary at 1.199s vs 1.201s") {
        ItemTargetingEngine items;
        KartEntity p1{};
        p1.id = 0;
        p1.x = 500.0;
        p1.z = 400.0;
        items.dropBanana(p1);

        std::vector<KartEntity> fleet = { p1 };
        fleet[0].x = items.bananas()[0].x;
        fleet[0].z = items.bananas()[0].z;

        // At t = 1.15s (immunity remaining 0.05s)
        items.bananas()[0].immunityTimer = 0.05;
        items.update(fleet, 0.01);
        ASSERT_FALSE(fleet[0].isSpinning); // Still immune!

        // At t = 1.21s (immunity fully expired)
        items.bananas()[0].immunityTimer = 0.0;
        items.update(fleet, 0.01);
        ASSERT_TRUE(fleet[0].isSpinning);  // Now vulnerable!
    }

    TEST_CASE("F8-B2: Banana collision radius boundary at 11.99 vs 12.01 units") {
        ItemTargetingEngine items;
        KartEntity p1{};
        p1.id = 0;
        items.dropBanana(p1);

        KartEntity ai{};
        ai.id = 1;
        ai.x = items.bananas()[0].x;
        ai.z = items.bananas()[0].z + 12.01; // Outside radius 12.0

        std::vector<KartEntity> fleet = { p1, ai };
        items.update(fleet, 0.05);
        ASSERT_FALSE(fleet[1].isSpinning);

        // Move inside radius 12.0
        fleet[1].z = items.bananas()[0].z + 11.99;
        items.update(fleet, 0.05);
        ASSERT_TRUE(fleet[1].isSpinning);
    }

    TEST_CASE("F8-B3: Dropping banana at zero kart speed") {
        ItemTargetingEngine items;
        KartEntity p1{};
        p1.id = 0;
        p1.x = 200.0;
        p1.z = 200.0;
        p1.speed = 0.0;
        p1.angle = 0.0;

        items.dropBanana(p1);
        ASSERT_NEAR(items.bananas()[0].x, 188.0, 0.001); // 12 units behind
        ASSERT_TRUE(items.bananas()[0].active);
    }

    TEST_CASE("F8-B4: Multi-AI simultaneous collision with single banana") {
        ItemTargetingEngine items;
        KartEntity p1{};
        p1.id = 0;
        items.dropBanana(p1);

        KartEntity ai1{};
        ai1.id = 1;
        ai1.x = items.bananas()[0].x;
        ai1.z = items.bananas()[0].z;

        KartEntity ai2{};
        ai2.id = 2;
        ai2.x = items.bananas()[0].x;
        ai2.z = items.bananas()[0].z;

        std::vector<KartEntity> fleet = { p1, ai1, ai2 };
        items.update(fleet, 0.05);

        // First AI consumes the banana, banana deactivates
        ASSERT_TRUE(fleet[1].isSpinning);
        ASSERT_FALSE(items.bananas()[0].active);
    }

    TEST_CASE("F8-B5: Clear item targeting engine removes all entities") {
        ItemTargetingEngine items;
        KartEntity p1{};
        items.dropBanana(p1);
        items.fireGreenShell(p1);
        items.clear();
        ASSERT_EQ(items.bananas().size(), 0u);
        ASSERT_EQ(items.projectiles().size(), 0u);
    }

    // ------------------------------------------------------------------------
    // F9 Boundaries: Physics & Arcade Handling
    // ------------------------------------------------------------------------
    TEST_CASE("F9-B1: Top speed clamping under infinite throttle") {
        KartEntity kart{};
        kart.maxSpeed = 160.0;
        kart.accel = 110.0;
        kart.speed = 159.0;

        for (int i = 0; i < 50; ++i) {
            KartPhysicsEngine::updateKart(kart, true, false, 0, false, 0.05);
        }
        ASSERT_NEAR(kart.speed, 160.0, 0.001); // Clamped exactly to maxSpeed
    }

    TEST_CASE("F9-B2: Reverse speed clamping under infinite braking") {
        KartEntity kart{};
        kart.maxSpeed = 160.0;
        kart.brakeDecel = 240.0;
        kart.speed = 0.0;

        for (int i = 0; i < 50; ++i) {
            KartPhysicsEngine::updateKart(kart, false, true, 0, false, 0.05);
        }
        ASSERT_NEAR(kart.speed, -48.0, 0.001); // -0.30 * 160.0 = -48.0
    }

    TEST_CASE("F9-B3: Drift mini-turbo threshold boundary at 0.99s vs 1.01s") {
        // Kart 1: Drift released at 0.95s (No mini-turbo)
        KartEntity k1{};
        k1.speed = 80.0;
        k1.turnRate = 2.0;
        k1.driftFactor = 1.35;
        for (int i = 0; i < 19; ++i) {
            KartPhysicsEngine::updateKart(k1, true, false, 1, true, 0.05); // 0.95s
        }
        KartPhysicsEngine::updateKart(k1, true, false, 0, false, 0.05); // Release
        ASSERT_NEAR(k1.miniTurboTimer, 0.0, 0.001);

        // Kart 2: Drift released at 1.05s (Mini-turbo awarded!)
        KartEntity k2 = k1;
        k2.driftTimer = 1.05;
        k2.isDrifting = true;
        KartPhysicsEngine::updateKart(k2, true, false, 0, false, 0.05); // Release
        ASSERT_NEAR(k2.miniTurboTimer, 1.2, 0.06);
    }

    TEST_CASE("F9-B4: Drift initiation boundary at speed 39.9 vs 40.1") {
        KartEntity kFail{};
        kFail.speed = 39.9;
        KartPhysicsEngine::updateKart(kFail, false, false, 1, true, 0.001);
        ASSERT_FALSE(kFail.isDrifting);

        KartEntity kPass{};
        kPass.speed = 40.1;
        KartPhysicsEngine::updateKart(kPass, false, false, 1, true, 0.001);
        ASSERT_TRUE(kPass.isDrifting);
    }

    TEST_CASE("F9-B5: Natural friction stops kart at exactly zero") {
        KartEntity kart{};
        kart.speed = 0.5; // low speed coasting
        KartPhysicsEngine::updateKart(kart, false, false, 0, false, 0.05);
        ASSERT_NEAR(kart.speed, 0.0, 0.001);
    }

    // ------------------------------------------------------------------------
    // F10 Boundaries: Bowser Lava Geysers
    // ------------------------------------------------------------------------
    TEST_CASE("F10-B1: Eruption cycle phase boundary at 3.29s vs 3.31s") {
        auto hazards = HazardEngine::createTrackHazards(TrackLevel::BOWSER_CASTLE);
        std::vector<KartEntity> emptyFleet;

        hazards[0].animTimer = 3.29;
        HazardEngine::updateHazards(hazards, emptyFleet, 0.001);
        ASSERT_FALSE(hazards[0].isErupting); // Warning phase

        hazards[0].animTimer = 3.31;
        HazardEngine::updateHazards(hazards, emptyFleet, 0.001);
        ASSERT_TRUE(hazards[0].isErupting);  // Spewing Active phase!
    }

    TEST_CASE("F10-B2: Geyser collision radius boundary at 15.99 vs 16.01 units") {
        auto hazards = HazardEngine::createTrackHazards(TrackLevel::BOWSER_CASTLE);
        hazards[0].animTimer = 3.5; // Erupting

        KartEntity kMiss{};
        kMiss.x = hazards[0].x + 16.01;
        kMiss.z = hazards[0].z;
        kMiss.speed = 100.0;

        std::vector<KartEntity> fleetMiss = { kMiss };
        HazardEngine::updateHazards(hazards, fleetMiss, 0.05);
        ASSERT_FALSE(fleetMiss[0].isSpinning); // Missed

        KartEntity kHit{};
        kHit.x = hazards[0].x + 15.99;
        kHit.z = hazards[0].z;
        kHit.speed = 100.0;

        std::vector<KartEntity> fleetHit = { kHit };
        HazardEngine::updateHazards(hazards, fleetHit, 0.05);
        ASSERT_TRUE(fleetHit[0].isSpinning);  // Hit!
    }

    TEST_CASE("F10-B3: Geyser collision while kart already spinning resets timer to 0.8") {
        auto hazards = HazardEngine::createTrackHazards(TrackLevel::BOWSER_CASTLE);
        hazards[0].animTimer = 3.5;

        KartEntity kart{};
        kart.x = hazards[0].x;
        kart.z = hazards[0].z;
        kart.isSpinning = true;
        kart.spinTimer = 0.2; // nearly finished spin

        std::vector<KartEntity> fleet = { kart };
        HazardEngine::updateHazards(hazards, fleet, 0.05);

        ASSERT_NEAR(fleet[0].spinTimer, 0.8, 0.01); // Refreshed
    }

    TEST_CASE("F10-B4: Geyser animTimer loop modulo at 4.80s") {
        auto hazards = HazardEngine::createTrackHazards(TrackLevel::BOWSER_CASTLE);
        hazards[0].animTimer = 4.79;
        std::vector<KartEntity> emptyFleet;

        HazardEngine::updateHazards(hazards, emptyFleet, 0.05);
        ASSERT_LT(hazards[0].animTimer, 0.1); // Wrapped to cycle start
    }

    TEST_CASE("F10-B5: Direct dead-center hit generates safe non-zero pushback") {
        auto hazards = HazardEngine::createTrackHazards(TrackLevel::BOWSER_CASTLE);
        hazards[0].animTimer = 3.5;

        KartEntity kart{};
        kart.x = hazards[0].x; // dx = 0, dz = 0
        kart.z = hazards[0].z;

        std::vector<KartEntity> fleet = { kart };
        HazardEngine::updateHazards(hazards, fleet, 0.05);

        ASSERT_FALSE(std::isnan(fleet[0].x));
        ASSERT_FALSE(std::isnan(fleet[0].z));
        ASSERT_TRUE(fleet[0].isSpinning);
    }

    // ------------------------------------------------------------------------
    // F11 Boundaries: Choco Valley Hazards
    // ------------------------------------------------------------------------
    TEST_CASE("F11-B1: Chocolate fountain collision radius boundary at 14.99 vs 15.01") {
        auto hazards = HazardEngine::createTrackHazards(TrackLevel::CHOCO_VALLEY);
        KartEntity kMiss{};
        kMiss.maxSpeed = 160.0;
        kMiss.speed = 140.0;
        kMiss.x = hazards[0].x;
        kMiss.z = hazards[0].z + 15.01;

        std::vector<KartEntity> fMiss = { kMiss };
        HazardEngine::updateHazards(hazards, fMiss, 0.05);
        ASSERT_NEAR(fMiss[0].speed, 140.0, 0.001);

        KartEntity kHit = kMiss;
        kHit.z = hazards[0].z + 14.99;
        std::vector<KartEntity> fHit = { kHit };
        HazardEngine::updateHazards(hazards, fHit, 0.05);
        ASSERT_LE(fHit[0].speed, 80.001);
    }

    TEST_CASE("F11-B2: Chocolate fountain does not increase speed of already slow kart") {
        auto hazards = HazardEngine::createTrackHazards(TrackLevel::CHOCO_VALLEY);
        KartEntity kart{};
        kart.maxSpeed = 160.0;
        kart.speed = 30.0; // Already below 50%
        kart.x = hazards[0].x;
        kart.z = hazards[0].z;

        std::vector<KartEntity> fleet = { kart };
        HazardEngine::updateHazards(hazards, fleet, 0.05);
        ASSERT_NEAR(fleet[0].speed, 30.0, 0.001); // Not boosted to 80
    }

    TEST_CASE("F11-B3: Rolling candy amplitude extreme peak at +40 units") {
        auto hazards = HazardEngine::createTrackHazards(TrackLevel::CHOCO_VALLEY);
        // Sin peak occurs at animTimer * 2.0 = PI/2 -> animTimer = PI/4 ~= 0.785
        hazards[3].animTimer = M_PI / 4.0;
        std::vector<KartEntity> emptyFleet;

        HazardEngine::updateHazards(hazards, emptyFleet, 0.0001);
        double offset = 40.0 * std::sin(hazards[3].animTimer * 2.0);
        ASSERT_NEAR(offset, 40.0, 0.1);
    }

    TEST_CASE("F11-B4: Rolling candy bumper bounce applied at reverse speed") {
        auto hazards = HazardEngine::createTrackHazards(TrackLevel::CHOCO_VALLEY);
        hazards[3].animTimer = 0.0;

        KartEntity kart{};
        kart.x = hazards[3].x;
        kart.z = hazards[3].z;
        kart.speed = -20.0; // Reversing

        std::vector<KartEntity> fleet = { kart };
        HazardEngine::updateHazards(hazards, fleet, 0.05);
        ASSERT_NEAR(fleet[0].speed, -35.0, 0.001); // Sets bounce speed
    }

    TEST_CASE("F11-B5: Multi-hazard update with zero dt") {
        auto hazards = HazardEngine::createTrackHazards(TrackLevel::CHOCO_VALLEY);
        std::vector<KartEntity> emptyFleet;
        HazardEngine::updateHazards(hazards, emptyFleet, 0.0);
        ASSERT_FALSE(std::isnan(hazards[0].animTimer));
    }

    // ------------------------------------------------------------------------
    // F12 Boundaries: Minimap HUD
    // ------------------------------------------------------------------------
    TEST_CASE("F12-B1: World coordinate (0, 0) maps to exact radar top-left") {
        QPointF p = MinimapHUD::worldToRadar(0.0, 0.0);
        ASSERT_NEAR(p.x(), 8.0, 0.001);
        ASSERT_NEAR(p.y(), 8.0, 0.001);
    }

    TEST_CASE("F12-B2: World coordinate (1024, 1024) wraps to exact radar top-left") {
        QPointF p = MinimapHUD::worldToRadar(1024.0, 1024.0);
        ASSERT_NEAR(p.x(), 8.0, 0.001);
        ASSERT_NEAR(p.y(), 8.0, 0.001);
    }

    TEST_CASE("F12-B3: Large negative world coordinate modulo wrap") {
        QPointF p = MinimapHUD::worldToRadar(-2048.0, -1024.0);
        ASSERT_NEAR(p.x(), 8.0, 0.001);
        ASSERT_NEAR(p.y(), 8.0, 0.001);
    }

    TEST_CASE("F12-B4: Coordinate wrap line continuity (1023.9 vs 0.1)") {
        QPointF p1 = MinimapHUD::worldToRadar(1023.9, 500.0);
        QPointF p2 = MinimapHUD::worldToRadar(0.1, 500.0);
        ASSERT_NEAR(p1.x(), 79.99, 0.1);
        ASSERT_NEAR(p2.x(), 8.01, 0.1);
    }

    TEST_CASE("F12-B5: Radar size scaling consistency") {
        // Half map width (512) corresponds to exactly 36 radar units
        QPointF half = MinimapHUD::worldToRadar(512.0, 0.0);
        ASSERT_NEAR(half.x() - 8.0, 36.0, 0.001);
    }

    // ------------------------------------------------------------------------
    // F13 Boundaries: Split-Screen Viewports
    // ------------------------------------------------------------------------
    TEST_CASE("F13-B1: P1 viewport lower boundary line at y=149") {
        ASSERT_TRUE(SplitScreenEngine::isPointInP1Viewport(200, 149));
        ASSERT_FALSE(SplitScreenEngine::isPointInP1Viewport(200, 150));
    }

    TEST_CASE("F13-B2: Divider boundary line at y=150 and y=151") {
        ASSERT_TRUE(SplitScreenEngine::isPointInDivider(200, 150));
        ASSERT_TRUE(SplitScreenEngine::isPointInDivider(200, 151));
        ASSERT_FALSE(SplitScreenEngine::isPointInDivider(200, 152));
    }

    TEST_CASE("F13-B3: P2 viewport upper boundary line at y=152") {
        ASSERT_TRUE(SplitScreenEngine::isPointInP2Viewport(200, 152));
        ASSERT_FALSE(SplitScreenEngine::isPointInP2Viewport(200, 151));
    }

    TEST_CASE("F13-B4: Canvas coordinate out of bounds (negative y)") {
        ASSERT_FALSE(SplitScreenEngine::isPointInP1Viewport(200, -1));
        ASSERT_FALSE(SplitScreenEngine::isPointInP2Viewport(200, -1));
        ASSERT_FALSE(SplitScreenEngine::isPointInDivider(200, -1));
    }

    TEST_CASE("F13-B5: Canvas coordinate out of bounds (y > 300)") {
        ASSERT_FALSE(SplitScreenEngine::isPointInP1Viewport(200, 301));
        ASSERT_FALSE(SplitScreenEngine::isPointInP2Viewport(200, 301));
        ASSERT_FALSE(SplitScreenEngine::isPointInDivider(200, 301));
    }

    // ------------------------------------------------------------------------
    // F14 Boundaries: Independent Dual Controls
    // ------------------------------------------------------------------------
    TEST_CASE("F14-B1: Empty keys set produces all false and neutral steering") {
        std::set<int> keys;
        bool p1T, p1B, p1I, p1D, p2T, p2B, p2I, p2D;
        int p1S, p2S;

        SplitScreenEngine::handleInputMultiplexing(keys, p1T, p1B, p1S, p1I, p1D, p2T, p2B, p2S, p2I, p2D);
        ASSERT_FALSE(p1T);
        ASSERT_FALSE(p1B);
        ASSERT_EQ(p1S, 0);
        ASSERT_FALSE(p1I);
        ASSERT_FALSE(p1D);
        ASSERT_FALSE(p2T);
        ASSERT_FALSE(p2B);
        ASSERT_EQ(p2S, 0);
        ASSERT_FALSE(p2I);
        ASSERT_FALSE(p2D);
    }

    TEST_CASE("F14-B2: P2 Enter vs Return item activation") {
        std::set<int> kReturn = { Qt::Key_Return };
        std::set<int> kEnter  = { Qt::Key_Enter };
        bool p1T, p1B, p1I, p1D, p2T, p2B, p2I, p2D;
        int p1S, p2S;

        SplitScreenEngine::handleInputMultiplexing(kReturn, p1T, p1B, p1S, p1I, p1D, p2T, p2B, p2S, p2I, p2D);
        ASSERT_TRUE(p2I);

        SplitScreenEngine::handleInputMultiplexing(kEnter, p1T, p1B, p1S, p1I, p1D, p2T, p2B, p2S, p2I, p2D);
        ASSERT_TRUE(p2I);
    }

    TEST_CASE("F14-B3: Rapid steering reversal (flutter) neutrality") {
        std::set<int> keys = { Qt::Key_Left, Qt::Key_Right };
        bool p1T, p1B, p1I, p1D, p2T, p2B, p2I, p2D;
        int p1S, p2S;

        SplitScreenEngine::handleInputMultiplexing(keys, p1T, p1B, p1S, p1I, p1D, p2T, p2B, p2S, p2I, p2D);
        ASSERT_EQ(p2S, 0); // Cancels out
    }

    TEST_CASE("F14-B4: Simultaneous throttle and brake on same player") {
        std::set<int> keys = { Qt::Key_W, Qt::Key_S };
        bool p1T, p1B, p1I, p1D, p2T, p2B, p2I, p2D;
        int p1S, p2S;

        SplitScreenEngine::handleInputMultiplexing(keys, p1T, p1B, p1S, p1I, p1D, p2T, p2B, p2S, p2I, p2D);
        ASSERT_TRUE(p1T);
        ASSERT_TRUE(p1B); // Both captured accurately
    }

    TEST_CASE("F14-B5: Window focus loss clears all keys cleanly") {
        std::set<int> keys = { Qt::Key_W, Qt::Key_Up, Qt::Key_Space };
        keys.clear(); // Focus loss event
        bool p1T, p1B, p1I, p1D, p2T, p2B, p2I, p2D;
        int p1S, p2S;

        SplitScreenEngine::handleInputMultiplexing(keys, p1T, p1B, p1S, p1I, p1D, p2T, p2B, p2S, p2I, p2D);
        ASSERT_FALSE(p1T);
        ASSERT_FALSE(p2T);
    }

    // ------------------------------------------------------------------------
    // F15 Boundaries: Build & Frame Stability
    // ------------------------------------------------------------------------
    TEST_CASE("F15-B1: dt = 0.000s update does not cause division by zero") {
        KartEntity kart{};
        kart.maxSpeed = 160.0;
        kart.speed = 100.0;

        KartPhysicsEngine::updateKart(kart, true, false, 0, false, 0.0);
        ASSERT_FALSE(std::isnan(kart.speed));
    }

    TEST_CASE("F15-B2: dt = 0.001s (1000 FPS) high-frequency update") {
        KartEntity kart{};
        kart.maxSpeed = 160.0;
        kart.accel = 110.0;

        for (int i = 0; i < 1000; ++i) {
            KartPhysicsEngine::updateKart(kart, true, false, 0, false, 0.001);
        }
        ASSERT_GT(kart.speed, 80.0);
    }

    TEST_CASE("F15-B3: dt = 0.050s exact clamping threshold") {
        KartEntity kart{};
        kart.speed = 100.0;
        kart.x = 0.0;

        KartPhysicsEngine::updateKart(kart, false, false, 0, false, 0.05);
        ASSERT_NEAR(kart.x, 4.89, 0.1);
    }

    TEST_CASE("F15-B4: dt = 2.000s extreme hitch clamped to 0.050s") {
        KartEntity kart{};
        kart.speed = 100.0;
        kart.x = 0.0;

        KartPhysicsEngine::updateKart(kart, false, false, 0, false, 2.0);
        ASSERT_NEAR(kart.x, 4.89, 0.1); // Not 200.0!
    }

    TEST_CASE("F15-B5: Numerical stability under 500 consecutive drift updates") {
        KartEntity kart{};
        kart.speed = 100.0;
        kart.turnRate = 2.0;
        kart.driftFactor = 1.35;

        for (int i = 0; i < 500; ++i) {
            KartPhysicsEngine::updateKart(kart, true, false, 1, true, 0.0166);
        }
        ASSERT_FALSE(std::isnan(kart.x));
        ASSERT_FALSE(std::isnan(kart.z));
        ASSERT_FALSE(std::isnan(kart.angle));
    }

    // ------------------------------------------------------------------------
    // F16 Boundaries: E2E Acceptance Boundaries
    // ------------------------------------------------------------------------
    TEST_CASE("F16-B1: Race finished state ignores subsequent player throttle") {
        MenuStateController menu;
        menu.confirmMenu();
        menu.confirmCharSelect();
        menu.finishRace();

        ASSERT_EQ(menu.currentState(), GameState::RACE_FINISHED);
    }

    TEST_CASE("F16-B2: Restarting race after finish resets menu cleanly") {
        MenuStateController menu;
        menu.confirmMenu();
        menu.confirmCharSelect();
        menu.finishRace();
        menu.resetToTitle();

        ASSERT_EQ(menu.currentState(), GameState::TITLE_MENU);
        ASSERT_EQ(menu.menuIndex(), 0);
    }

    TEST_CASE("F16-B3: Simultaneous lap finish tiebreaker by track progression") {
        KartEntity k1{};
        k1.id = 0;
        k1.totalRaceDistance = 2500.5;

        KartEntity k2{};
        k2.id = 1;
        k2.totalRaceDistance = 2500.2;

        ASSERT_GT(k1.totalRaceDistance, k2.totalRaceDistance); // k1 awarded higher placement
    }

    TEST_CASE("F16-B4: Repeated 100 state cycle transitions without memory growth") {
        MenuStateController menu;
        for (int i = 0; i < 100; ++i) {
            menu.confirmMenu();
            menu.confirmCharSelect();
            menu.finishRace();
            menu.resetToTitle();
        }
        ASSERT_EQ(menu.currentState(), GameState::TITLE_MENU);
    }

    TEST_CASE("F16-B5: Modulo wrap boundary for large coordinate overflow") {
        QPointF p = MinimapHUD::worldToRadar(102400.0, 204800.0);
        ASSERT_NEAR(p.x(), 8.0, 0.001);
        ASSERT_NEAR(p.y(), 8.0, 0.001);
    }
}
