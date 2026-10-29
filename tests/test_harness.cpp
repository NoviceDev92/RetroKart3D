#include "test_harness.h"
#include <Qt>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// MenuStateController
// ============================================================================

MenuStateController::MenuStateController()
    : state(GameState::TITLE_MENU),
      mode(GameMode::PLAY_VS_AI),
      mIndex(0),
      p1Char(0),
      p2Char(1),
      p2AltColor(false)
{
}

void MenuStateController::navigateMenu(int dir) {
    if (state != GameState::TITLE_MENU) return;
    mIndex = (mIndex + dir) % 3;
    if (mIndex < 0) mIndex += 3;
}

void MenuStateController::confirmMenu() {
    if (state == GameState::TITLE_MENU) {
        mode = static_cast<GameMode>(mIndex);
        state = GameState::CHARACTER_SELECT;
        p2AltColor = (p1Char == p2Char);
    }
}

void MenuStateController::navigateCharSelect(int playerNum, int dir) {
    if (state != GameState::CHARACTER_SELECT) return;
    if (playerNum == 1) {
        p1Char = (p1Char + dir) % 4;
        if (p1Char < 0) p1Char += 4;
    } else if (playerNum == 2) {
        p2Char = (p2Char + dir) % 4;
        if (p2Char < 0) p2Char += 4;
    }
    p2AltColor = (mode == GameMode::MULTIPLAYER && p1Char == p2Char);
}

void MenuStateController::confirmCharSelect() {
    if (state == GameState::CHARACTER_SELECT) {
        state = GameState::RACING;
    }
}

void MenuStateController::backAction() {
    if (state == GameState::CHARACTER_SELECT) {
        state = GameState::TITLE_MENU;
    } else if (state == GameState::RACE_FINISHED) {
        state = GameState::TITLE_MENU;
    }
}

void MenuStateController::finishRace() {
    if (state == GameState::RACING) {
        state = GameState::RACE_FINISHED;
    }
}

void MenuStateController::resetToTitle() {
    state = GameState::TITLE_MENU;
    mIndex = 0;
}

// ============================================================================
// CharacterRoster
// ============================================================================

CharacterRoster::CharacterRoster() {
    roster = {
        // Mario: Balanced
        { 0, "Mario", QColor(220, 20, 20), 160.0, 110.0, 2.4, 1.30, 1.0 },
        // Bowser: Heavyweight Titan (high top speed, lower accel & handling)
        { 1, "Bowser", QColor(230, 90, 20), 175.0, 85.0, 1.9, 1.25, 1.5 },
        // Peach: Nimble / Agility Queen (nimble turn rate, fast accel)
        { 2, "Peach", QColor(255, 130, 180), 150.0, 135.0, 2.9, 1.40, 0.8 },
        // Toad: Acceleration Specialist
        { 3, "Toad", QColor(40, 120, 240), 155.0, 125.0, 2.7, 1.35, 0.85 }
    };
}

CharacterRoster& CharacterRoster::instance() {
    static CharacterRoster rosterInstance;
    return rosterInstance;
}

CharacterDef CharacterRoster::getCharacter(int id) const {
    if (id >= 0 && id < (int)roster.size()) {
        return roster[id];
    }
    return roster[0]; // fallback default Mario
}

QColor CharacterRoster::getP2AlternateColor(int id) const {
    switch (id) {
        case 0: return QColor(140, 10, 10);    // Dark Crimson Mario
        case 1: return QColor(40, 40, 40);     // Dark Obsidian Bowser
        case 2: return QColor(255, 215, 0);    // Golden Dress Peach
        case 3: return QColor(160, 40, 200);   // Purple Toad
        default: return QColor(100, 100, 100);
    }
}

// ============================================================================
// TrackWaypointManager
// ============================================================================

std::vector<QPointF> TrackWaypointManager::generateWaypoints(TrackLevel level) {
    std::vector<QPointF> waypoints;
    if (level == TrackLevel::DONUT_PLAINS) {
        // 16 points smooth oval circuit
        waypoints = {
            QPointF(470, 200), QPointF(600, 200), QPointF(750, 240), QPointF(850, 350),
            QPointF(880, 500), QPointF(850, 650), QPointF(750, 760), QPointF(600, 800),
            QPointF(450, 800), QPointF(300, 760), QPointF(200, 650), QPointF(160, 500),
            QPointF(180, 350), QPointF(260, 240), QPointF(350, 200), QPointF(420, 200)
        };
    } else if (level == TrackLevel::CHOCO_VALLEY) {
        // 20 points curvy dirt track
        waypoints = {
            QPointF(400, 180), QPointF(550, 180), QPointF(700, 220), QPointF(820, 300),
            QPointF(780, 450), QPointF(650, 480), QPointF(600, 580), QPointF(700, 680),
            QPointF(820, 750), QPointF(750, 860), QPointF(550, 880), QPointF(400, 850),
            QPointF(300, 760), QPointF(350, 640), QPointF(420, 560), QPointF(320, 480),
            QPointF(220, 420), QPointF(200, 300), QPointF(260, 220), QPointF(340, 180)
        };
    } else {
        // BOWSER_CASTLE: 24 points angular technical castle circuit
        waypoints = {
            QPointF(300, 150), QPointF(500, 150), QPointF(700, 150), QPointF(850, 150),
            QPointF(850, 350), QPointF(720, 350), QPointF(720, 500), QPointF(850, 500),
            QPointF(850, 700), QPointF(850, 850), QPointF(650, 850), QPointF(500, 850),
            QPointF(500, 700), QPointF(380, 700), QPointF(380, 850), QPointF(200, 850),
            QPointF(200, 650), QPointF(320, 650), QPointF(320, 500), QPointF(200, 500),
            QPointF(200, 350), QPointF(320, 350), QPointF(320, 200), QPointF(200, 200)
        };
    }
    return waypoints;
}

double TrackWaypointManager::trackLength(const std::vector<QPointF>& waypoints) {
    if (waypoints.size() < 2) return 0.0;
    double len = 0.0;
    for (size_t i = 0; i < waypoints.size(); ++i) {
        size_t next = (i + 1) % waypoints.size();
        double dx = waypoints[next].x() - waypoints[i].x();
        double dz = waypoints[next].y() - waypoints[i].y();
        len += std::hypot(dx, dz);
    }
    return len;
}

double TrackWaypointManager::distanceAlongTrack(const std::vector<QPointF>& waypoints, int waypointIndex, double x, double z) {
    if (waypoints.empty()) return 0.0;
    int idx = std::max(0, std::min(waypointIndex, (int)waypoints.size() - 1));
    double dist = 0.0;
    for (int i = 0; i < idx; ++i) {
        int next = (i + 1) % (int)waypoints.size();
        double dx = waypoints[next].x() - waypoints[i].x();
        double dz = waypoints[next].y() - waypoints[i].y();
        dist += std::hypot(dx, dz);
    }
    double curDx = x - waypoints[idx].x();
    double curDz = z - waypoints[idx].y();
    dist += std::hypot(curDx, curDz);
    return dist;
}

// ============================================================================
// KartPhysicsEngine
// ============================================================================

void KartPhysicsEngine::updateKart(KartEntity &kart, bool throttle, bool brake, int steerDir, bool driftKey, double dt) {
    // Delta time clamped to protect against lag spike tunneling (E20)
    dt = std::min(dt, 0.05);

    // Debuff timers countdown
    if (kart.spinTimer > 0.0) {
        kart.spinTimer -= dt;
        if (kart.spinTimer <= 0.0) {
            kart.spinTimer = 0.0;
            kart.isSpinning = false;
        } else {
            kart.isSpinning = true;
        }
        kart.angle += 12.0 * dt; // 360 spin animation
        kart.speed *= std::pow(0.5, dt / 0.1);
        if (kart.speed < 1.0) kart.speed = 0.0;
        return; // No user control during spinout
    }

    if (kart.inkTimer > 0.0) {
        kart.inkTimer -= dt;
        if (kart.inkTimer <= 0.0) {
            kart.inkTimer = 0.0;
            kart.isInked = false;
        } else {
            kart.isInked = true;
        }
    }

    // Effective maximum speed calculation
    double effectiveMaxSpeed = kart.maxSpeed;
    if (kart.isInked) {
        effectiveMaxSpeed *= 0.70; // 30% speed debuff
    }
    if (kart.miniTurboTimer > 0.0) {
        kart.miniTurboTimer -= dt;
        if (kart.miniTurboTimer < 0.0) {
            kart.miniTurboTimer = 0.0;
        }
        effectiveMaxSpeed *= 1.35; // 35% mini-turbo speed boost
    }

    // Acceleration and Braking
    if (throttle) {
        // Non-linear arcade acceleration curve
        double ratio = (effectiveMaxSpeed > 0.0) ? (kart.speed / effectiveMaxSpeed) : 0.0;
        double accelEff = kart.accel * (1.0 - 0.4 * std::clamp(ratio, 0.0, 1.0));
        kart.speed += accelEff * dt;
        if (kart.speed > effectiveMaxSpeed) {
            kart.speed = effectiveMaxSpeed;
        }
    } else if (brake) {
        // Aggressive arcade braking
        kart.speed -= kart.brakeDecel * dt;
        double maxReverse = -0.30 * kart.maxSpeed;
        if (kart.speed < maxReverse) {
            kart.speed = maxReverse;
        }
    } else {
        // Natural friction coasting
        double friction = 45.0;
        if (kart.speed > 0.0) {
            kart.speed -= friction * dt;
            if (kart.speed < 0.0) kart.speed = 0.0;
        } else if (kart.speed < 0.0) {
            kart.speed += friction * dt;
            if (kart.speed > 0.0) kart.speed = 0.0;
        }
    }

    // Drift Mechanic
    if (driftKey && steerDir != 0 && kart.speed > 40.0) {
        kart.isDrifting = true;
        kart.driftTimer += dt;
        // Drift tightens cornering turn rate
        kart.angle += steerDir * kart.turnRate * kart.driftFactor * dt;
    } else {
        if (kart.isDrifting) {
            // Drift release check: mini-turbo awarded if held >= 1.0 second
            if (kart.driftTimer >= 1.0) {
                kart.miniTurboTimer = 1.2; // 1.2 seconds boost
            }
            kart.isDrifting = false;
            kart.driftTimer = 0.0;
        }
        // Standard arcade steering
        kart.angle += steerDir * kart.turnRate * dt;
    }

    // Kinematics integration
    kart.x += kart.speed * std::cos(kart.angle) * dt;
    kart.z += kart.speed * std::sin(kart.angle) * dt;

    // Boundary wrapping 0..1024
    while (kart.x < 0.0) kart.x += 1024.0;
    while (kart.x >= 1024.0) kart.x -= 1024.0;
    while (kart.z < 0.0) kart.z += 1024.0;
    while (kart.z >= 1024.0) kart.z -= 1024.0;
}

void KartPhysicsEngine::applyHazardHit(KartEntity &kart, double pushDirX, double pushDirZ, double penaltySpeedMult) {
    kart.isSpinning = true;
    kart.spinTimer = 0.8;
    kart.speed *= penaltySpeedMult;
    // Pushback impulse
    kart.x += pushDirX * 15.0;
    kart.z += pushDirZ * 15.0;
}

void KartPhysicsEngine::applyItemSpinout(KartEntity &kart) {
    kart.isSpinning = true;
    kart.spinTimer = 1.0;
    kart.speed *= 0.35;
}

void KartPhysicsEngine::applyInkedDebuff(KartEntity &kart) {
    kart.isInked = true;
    kart.inkTimer = 4.0;
    kart.speed *= 0.70;
}

// ============================================================================
// AISteeringEngine
// ============================================================================

void AISteeringEngine::updateAI(KartEntity &ai, const std::vector<QPointF> &waypoints, double leadPlayerDist, double dt) {
    if (waypoints.empty()) return;

    dt = std::min(dt, 0.05);

    // If currently spinning, delegate to physics engine
    if (ai.spinTimer > 0.0) {
        KartPhysicsEngine::updateKart(ai, false, false, 0, false, dt);
        return;
    }

    // Check distance to current waypoint; advance if reached
    QPointF currentWp = waypoints[ai.currentWaypointIndex];
    double distToCurr = std::hypot(currentWp.x() - ai.x, currentWp.y() - ai.z);
    if (distToCurr < 35.0) {
        ai.currentWaypointIndex = (ai.currentWaypointIndex + 1) % waypoints.size();
    }

    // Lookahead target waypoint (3-4 waypoints ahead)
    int targetIndex = (ai.currentWaypointIndex + 3) % waypoints.size();
    QPointF targetWp = waypoints[targetIndex];

    double dx = targetWp.x() - ai.x;
    double dz = targetWp.y() - ai.z;
    double desiredAngle = std::atan2(dz, dx);

    // Heading delta normalized to [-PI, PI]
    double dAngle = desiredAngle - ai.angle;
    while (dAngle > M_PI) dAngle -= 2.0 * M_PI;
    while (dAngle < -M_PI) dAngle += 2.0 * M_PI;

    // Steering rate limiting
    double turnStep = ai.turnRate * dt;
    int steerDir = 0;
    if (std::abs(dAngle) < turnStep) {
        ai.angle = desiredAngle;
    } else if (dAngle > 0.0) {
        steerDir = 1;
    } else {
        steerDir = -1;
    }

    // Throttle modulation: feather throttle on sharp hairpins
    bool throttle = true;
    bool brake = false;
    if (std::abs(dAngle) > 0.65) {
        throttle = false;
        if (ai.speed > 80.0) {
            brake = true;
        }
    }

    // Rubber-banding: adjust max speed if falling behind or leading too far
    double baseMaxSpeed = ai.maxSpeed;
    if (leadPlayerDist - ai.totalRaceDistance > 150.0) {
        ai.maxSpeed = baseMaxSpeed * 1.10; // catch-up boost
    } else if (ai.totalRaceDistance - leadPlayerDist > 150.0) {
        ai.maxSpeed = baseMaxSpeed * 0.90; // moderate speed
    }

    KartPhysicsEngine::updateKart(ai, throttle, brake, steerDir, false, dt);
    ai.maxSpeed = baseMaxSpeed; // restore base

    // Update total distance progression
    ai.totalRaceDistance += ai.speed * dt;
}

// ============================================================================
// ItemTargetingEngine
// ============================================================================

void ItemTargetingEngine::fireGreenShell(const KartEntity &firingKart) {
    ShellProjectile shell;
    shell.ownerId = firingKart.id;
    // Spawns 12 units ahead of kart along heading
    shell.x = firingKart.x + 12.0 * std::cos(firingKart.angle);
    shell.z = firingKart.z + 12.0 * std::sin(firingKart.angle);
    double launchSpeed = firingKart.speed + 75.0;
    shell.vx = launchSpeed * std::cos(firingKart.angle);
    shell.vz = launchSpeed * std::sin(firingKart.angle);
    shell.bouncesLeft = 3;
    shell.lifeTime = 6.0;
    shell.active = true;

    m_projectiles.push_back(shell);
}

void ItemTargetingEngine::dropBanana(const KartEntity &droppingKart) {
    DroppedBanana banana;
    banana.ownerId = droppingKart.id;
    // Spawns 12 units behind kart along heading
    banana.x = droppingKart.x - 12.0 * std::cos(droppingKart.angle);
    banana.z = droppingKart.z - 12.0 * std::sin(droppingKart.angle);
    banana.immunityTimer = 1.2; // 1.2s owner immunity
    banana.active = true;

    m_bananas.push_back(banana);
}

void ItemTargetingEngine::useInkBlooper(int firingKartId, std::vector<KartEntity> &karts) {
    for (auto &kart : karts) {
        if (kart.id == firingKartId) {
            // Player who fired Blooper is 100% immune (Bug fix R2)
            kart.isInked = false;
            kart.inkTimer = 0.0;
        } else {
            // Opponents receive ink debuff
            KartPhysicsEngine::applyInkedDebuff(kart);
        }
    }
}

void ItemTargetingEngine::update(std::vector<KartEntity> &karts, double dt) {
    dt = std::min(dt, 0.05);

    // 1. Update Projectiles
    for (auto &shell : m_projectiles) {
        if (!shell.active) continue;

        shell.x += shell.vx * dt;
        shell.z += shell.vz * dt;
        shell.lifeTime -= dt;

        if (shell.lifeTime <= 0.0) {
            shell.active = false;
            continue;
        }

        // Wall bounce reflection test (track borders: 20 and 1004)
        bool bounced = false;
        if (shell.x < 20.0) {
            shell.x = 20.0;
            shell.vx = -shell.vx;
            bounced = true;
        } else if (shell.x > 1004.0) {
            shell.x = 1004.0;
            shell.vx = -shell.vx;
            bounced = true;
        }

        if (shell.z < 20.0) {
            shell.z = 20.0;
            shell.vz = -shell.vz;
            bounced = true;
        } else if (shell.z > 1004.0) {
            shell.z = 1004.0;
            shell.vz = -shell.vz;
            bounced = true;
        }

        if (bounced) {
            shell.bouncesLeft--;
            if (shell.bouncesLeft <= 0) {
                shell.active = false;
                continue;
            }
        }

        // Hit detection against karts (radius 14.0)
        for (auto &kart : karts) {
            // If shell hasn't bounced yet, owner cannot hit own shell (grace)
            if (kart.id == shell.ownerId && shell.bouncesLeft == 3) {
                continue;
            }

            double dist = std::hypot(kart.x - shell.x, kart.z - shell.z);
            if (dist < 14.0) {
                KartPhysicsEngine::applyItemSpinout(kart);
                shell.active = false; // Shell shatters
                break;
            }
        }
    }

    // 2. Update Bananas
    for (auto &banana : m_bananas) {
        if (!banana.active) continue;

        if (banana.immunityTimer > 0.0) {
            banana.immunityTimer -= dt;
        }

        // Collision check against all karts (radius 12.0)
        for (auto &kart : karts) {
            // Owner is immune while immunityTimer > 0
            if (kart.id == banana.ownerId && banana.immunityTimer > 0.0) {
                continue;
            }

            double dist = std::hypot(kart.x - banana.x, kart.z - banana.z);
            if (dist < 12.0) {
                KartPhysicsEngine::applyItemSpinout(kart);
                banana.active = false; // Banana consumed
                break;
            }
        }
    }
}

void ItemTargetingEngine::clear() {
    m_projectiles.clear();
    m_bananas.clear();
}

// ============================================================================
// HazardEngine
// ============================================================================

std::vector<TrackHazard> HazardEngine::createTrackHazards(TrackLevel level) {
    std::vector<TrackHazard> hazards;
    if (level == TrackLevel::BOWSER_CASTLE) {
        // 4 Lava geysers along bridge / molten borders
        hazards = {
            { 500.0, 150.0, HazardType::LAVA_GEYSER, 0.0, false, 16.0 },
            { 850.0, 350.0, HazardType::LAVA_GEYSER, 1.2, false, 16.0 },
            { 650.0, 850.0, HazardType::LAVA_GEYSER, 2.4, false, 16.0 },
            { 200.0, 650.0, HazardType::LAVA_GEYSER, 3.6, false, 16.0 }
        };
    } else if (level == TrackLevel::CHOCO_VALLEY) {
        // 3 chocolate fountains + 2 rolling candies
        hazards = {
            { 700.0, 220.0, HazardType::CHOCOLATE_FOUNTAIN, 0.0, true, 15.0 },
            { 600.0, 580.0, HazardType::CHOCOLATE_FOUNTAIN, 0.0, true, 15.0 },
            { 300.0, 760.0, HazardType::CHOCOLATE_FOUNTAIN, 0.0, true, 15.0 },
            { 550.0, 180.0, HazardType::ROLLING_CANDY, 0.0, false, 14.0 },
            { 400.0, 850.0, HazardType::ROLLING_CANDY, 1.5, false, 14.0 }
        };
    }
    return hazards;
}

void HazardEngine::updateHazards(std::vector<TrackHazard> &hazards, std::vector<KartEntity> &karts, double dt) {
    dt = std::min(dt, 0.05);

    for (auto &hazard : hazards) {
        if (hazard.type == HazardType::LAVA_GEYSER) {
            // Eruption cycle: Dormant (0..2.5s) -> Warning (2.5..3.3s) -> Spewing Active (3.3..4.8s)
            hazard.animTimer += dt;
            if (hazard.animTimer >= 4.8) {
                hazard.animTimer -= 4.8;
            }
            hazard.isErupting = (hazard.animTimer >= 3.3);

            if (hazard.isErupting) {
                for (auto &kart : karts) {
                    double dist = std::hypot(kart.x - hazard.x, kart.z - hazard.z);
                    if (dist < hazard.radius) {
                        double pushX = (dist > 0.001) ? ((kart.x - hazard.x) / dist) : 1.0;
                        double pushZ = (dist > 0.001) ? ((kart.z - hazard.z) / dist) : 0.0;
                        KartPhysicsEngine::applyHazardHit(kart, pushX, pushZ, 0.25);
                    }
                }
            }
        } else if (hazard.type == HazardType::CHOCOLATE_FOUNTAIN) {
            // Persistent bubbling chocolate mud
            for (auto &kart : karts) {
                double dist = std::hypot(kart.x - hazard.x, kart.z - hazard.z);
                if (dist < hazard.radius) {
                    // Mud splatter cuts speed by 50%
                    if (kart.speed > kart.maxSpeed * 0.50) {
                        kart.speed = kart.maxSpeed * 0.50;
                    }
                }
            }
        } else if (hazard.type == HazardType::ROLLING_CANDY) {
            // Rolling candy oscillates along track cross-axis
            hazard.animTimer += dt;
            double offset = 40.0 * std::sin(hazard.animTimer * 2.0);
            double currentCandyX = hazard.x + offset;

            for (auto &kart : karts) {
                double dist = std::hypot(kart.x - currentCandyX, kart.z - hazard.z);
                if (dist < hazard.radius) {
                    // Bumper bounce recoil
                    kart.speed = -35.0;
                }
            }
        }
    }
}

// ============================================================================
// SplitScreenEngine
// ============================================================================

bool SplitScreenEngine::isPointInP1Viewport(int x, int y) {
    return (x >= 0 && x < ViewportSplit::TOTAL_WIDTH &&
            y >= ViewportSplit::P1_TOP && y <= ViewportSplit::P1_BOTTOM);
}

bool SplitScreenEngine::isPointInP2Viewport(int x, int y) {
    return (x >= 0 && x < ViewportSplit::TOTAL_WIDTH &&
            y >= ViewportSplit::P2_TOP && y <= ViewportSplit::P2_BOTTOM);
}

bool SplitScreenEngine::isPointInDivider(int x, int y) {
    return (x >= 0 && x < ViewportSplit::TOTAL_WIDTH &&
            y >= ViewportSplit::DIVIDER_Y && y < ViewportSplit::DIVIDER_Y + ViewportSplit::DIVIDER_HEIGHT);
}

void SplitScreenEngine::handleInputMultiplexing(
    const std::set<int>& pressedKeys,
    bool &p1Throttle, bool &p1Brake, int &p1Steer, bool &p1Item, bool &p1Drift,
    bool &p2Throttle, bool &p2Brake, int &p2Steer, bool &p2Item, bool &p2Drift
) {
    // Player 1: WASD + Space + Shift
    p1Throttle = (pressedKeys.count(Qt::Key_W) > 0);
    p1Brake    = (pressedKeys.count(Qt::Key_S) > 0);
    p1Steer    = 0;
    if (pressedKeys.count(Qt::Key_A) > 0) p1Steer -= 1;
    if (pressedKeys.count(Qt::Key_D) > 0) p1Steer += 1;
    p1Item     = (pressedKeys.count(Qt::Key_Space) > 0);
    p1Drift    = (pressedKeys.count(Qt::Key_Shift) > 0);

    // Player 2: Arrows + Enter + Control
    p2Throttle = (pressedKeys.count(Qt::Key_Up) > 0);
    p2Brake    = (pressedKeys.count(Qt::Key_Down) > 0);
    p2Steer    = 0;
    if (pressedKeys.count(Qt::Key_Left) > 0) p2Steer -= 1;
    if (pressedKeys.count(Qt::Key_Right) > 0) p2Steer += 1;
    p2Item     = (pressedKeys.count(Qt::Key_Return) > 0 || pressedKeys.count(Qt::Key_Enter) > 0);
    p2Drift    = (pressedKeys.count(Qt::Key_Control) > 0 || pressedKeys.count(Qt::Key_Alt) > 0);
}
