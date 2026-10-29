#include <QApplication>
#include <QWidget>
#include <QImage>
#include <QTimer>
#include <QElapsedTimer>
#include <QKeyEvent>
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

class ChallengerM3Canvas : public GameCanvas {
public:
    explicit ChallengerM3Canvas(QWidget *parent = nullptr) : GameCanvas(parent) {
        gameTimer.stop(); // In automated tests, step physics manually
    }

    void pressKey(int key) {
        pressedKeys.insert(key);
    }

    void releaseKey(int key) {
        pressedKeys.erase(key);
    }

    void clearAllKeys() {
        pressedKeys.clear();
    }

    void simulatePhysicsStep(double dt) {
        updatePhysics(dt);
        if (selectedGameMode == GameMode::PLAY_VS_AI) {
            updateAIOpponents(dt);
        }
        updateTrackHazards(dt);
        updateItemSystem(dt);
    }
};

// ============================================================================
// SUITE 1: DRIFT MECHANICS UNDER EXTREME & ADVERSARIAL INPUTS
// ============================================================================
void runSuite1_DriftMechanicsUnderExtremeInputs(ChallengerM3Canvas &canvas) {
    std::cout << "\n===============================================================\n";
    std::cout << " SUITE 1: Drift Mechanics Under Extreme & Adversarial Inputs\n";
    std::cout << "===============================================================\n";

    canvas.setGameState(GameState::RACING);
    canvas.setGameMode(GameMode::PRACTICE);
    canvas.setTrackLevel(TrackLevel::DONUT_PLAINS);
    canvas.startRace();

    // 1.1 Rapid Drift Key Hammering (Prevent State Corruption & Infinite Boost)
    {
        canvas.clearAllKeys();
        canvas.kartSpeed = 100.0;
        canvas.kartX = 500.0;
        canvas.kartZ = 200.0;
        canvas.kartAngle = 0.0;
        canvas.isDrifting = false;
        canvas.driftTimer = 0.0;
        canvas.miniTurboTimer = 0.0;

        // Hammer Shift key on/off 100 times in rapid succession (1 frame on, 1 frame off at dt=0.016s)
        bool boostEverAwarded = false;
        bool stateCorrupted = false;

        for (int i = 0; i < 100; ++i) {
            // Press Shift + D
            canvas.pressKey(Qt::Key_Shift);
            canvas.pressKey(Qt::Key_D);
            canvas.simulatePhysicsStep(0.016);

            if (!canvas.isDrifting && canvas.kartSpeed > 40.0) {
                stateCorrupted = true;
            }

            // Release Shift
            canvas.releaseKey(Qt::Key_Shift);
            canvas.simulatePhysicsStep(0.016);

            if (canvas.isDrifting) {
                stateCorrupted = true;
            }
            if (canvas.miniTurboTimer > 0.0) {
                boostEverAwarded = true;
            }
        }

        EMPIRICAL_ASSERT(!stateCorrupted,
                         "T1.1: Rapid drift toggle (100 toggles) maintains valid state transitions without corruption",
                         "stateCorrupted=" + std::to_string(stateCorrupted));
        EMPIRICAL_ASSERT(!boostEverAwarded,
                         "T1.1: Key hammering does NOT leak or award unearned mini-turbo boost (miniTurboTimer == 0)",
                         "boostEverAwarded=" + std::to_string(boostEverAwarded));
        EMPIRICAL_ASSERT(canvas.driftTimer == 0.0,
                         "T1.1: Drift timer cleanly resets to 0.0 on key release",
                         "driftTimer=" + std::to_string(canvas.driftTimer));
    }

    // 1.2 High-Speed Reverse into Drift
    {
        canvas.clearAllKeys();
        canvas.startRace();
        canvas.pressKey(Qt::Key_S);

        // Reverse for 2 seconds to reach maximum reverse speed (-48.0 for Mario)
        for (int i = 0; i < 120; ++i) {
            canvas.simulatePhysicsStep(0.016);
        }

        double revSpeed = canvas.kartSpeed;
        EMPIRICAL_ASSERT(revSpeed < -40.0,
                         "T1.2: Kart attains high-speed reverse velocity (speed < -40.0)",
                         "speed=" + std::to_string(revSpeed));

        // Attempt to activate drift in reverse: Shift + D
        canvas.pressKey(Qt::Key_Shift);
        canvas.pressKey(Qt::Key_D);
        canvas.simulatePhysicsStep(0.016);

        EMPIRICAL_ASSERT(canvas.isDrifting == false,
                         "T1.2: Drift CANNOT activate in reverse (isDrifting == false)",
                         "isDrifting=" + std::to_string(canvas.isDrifting));
        EMPIRICAL_ASSERT(canvas.driftTimer == 0.0,
                         "T1.2: Drift timer remains 0.0 in reverse",
                         "driftTimer=" + std::to_string(canvas.driftTimer));
        EMPIRICAL_ASSERT(canvas.driftHopHeight == 0.0,
                         "T1.2: Drift hop does not trigger in reverse",
                         "hopHeight=" + std::to_string(canvas.driftHopHeight));
        EMPIRICAL_ASSERT(std::abs(canvas.lateralSlipAngle) < 1e-4,
                         "T1.2: Lateral slip angle remains zero in reverse",
                         "lateralSlip=" + std::to_string(canvas.lateralSlipAngle));
    }

    // 1.3 Drift Activation Speed Threshold Boundary (v = 40.0)
    {
        canvas.clearAllKeys();
        canvas.startRace();

        // 1.3.1 Acceleration through threshold: holding W + Shift + D from standstill
        // Drift must remain false while speed <= 40.0, then activate strictly when speed > 40.0
        canvas.kartSpeed = 0.0;
        canvas.pressKey(Qt::Key_W);
        canvas.pressKey(Qt::Key_Shift);
        canvas.pressKey(Qt::Key_D);

        bool driftActivatedBelow40 = false;
        double speedAtActivation = 0.0;
        bool didActivate = false;

        for (int i = 0; i < 60; ++i) {
            canvas.simulatePhysicsStep(0.016);
            if (canvas.isDrifting && !didActivate) {
                didActivate = true;
                speedAtActivation = canvas.kartSpeed;
                if (speedAtActivation <= 40.0) {
                    driftActivatedBelow40 = true;
                }
                break;
            }
        }

        EMPIRICAL_ASSERT(didActivate,
                         "T1.3: Drift engages dynamically upon accelerating past threshold",
                         "didActivate=" + std::to_string(didActivate));
        EMPIRICAL_ASSERT(!driftActivatedBelow40,
                         "T1.3: Drift DOES NOT activate while speed <= 40.0 u/s",
                         "speedAtActivation=" + std::to_string(speedAtActivation));
        EMPIRICAL_ASSERT(speedAtActivation > 40.0,
                         "T1.3: Drift ACTIVATES strictly above threshold (v > 40.0 u/s)",
                         "speedAtActivation=" + std::to_string(speedAtActivation));
        EMPIRICAL_ASSERT(canvas.driftDirection == 1,
                         "T1.3: Drift direction correctly matches steer direction (+1 for Right)",
                         "driftDirection=" + std::to_string(canvas.driftDirection));

        // 1.3.2 Coasting below threshold: starting at 39.5 without throttle
        canvas.clearAllKeys();
        canvas.kartSpeed = 39.5;
        canvas.pressKey(Qt::Key_Shift);
        canvas.pressKey(Qt::Key_D);
        canvas.simulatePhysicsStep(0.016);

        EMPIRICAL_ASSERT(canvas.isDrifting == false,
                         "T1.3: Sub-threshold speed (39.5 u/s) fails drift activation check",
                         "isDrifting=" + std::to_string(canvas.isDrifting));
    }

    // 1.4 Drift Hold Duration: Blue Sparks (0.5s) & Orange Sparks (1.0s)
    {
        canvas.clearAllKeys();
        canvas.startRace();
        canvas.kartSpeed = 90.0;
        // Hold throttle + drift + right turn
        canvas.pressKey(Qt::Key_W);
        canvas.pressKey(Qt::Key_Shift);
        canvas.pressKey(Qt::Key_D);

        // Drift for 0.45s (below 0.5s threshold)
        for (int i = 0; i < 28; ++i) { // 28 * 0.016 = 0.448s
            canvas.simulatePhysicsStep(0.016);
        }

        EMPIRICAL_ASSERT(canvas.isDrifting == true && canvas.driftTimer < 0.50,
                         "T1.4: Kart is actively drifting at t < 0.5s",
                         "driftTimer=" + std::to_string(canvas.driftTimer));
        EMPIRICAL_ASSERT(canvas.driftSparks.empty(),
                         "T1.4: Drift sparks are NOT spawned before 0.5s (count == 0)",
                         "sparksCount=" + std::to_string(canvas.driftSparks.size()));

        // Drift past 0.5s (up to ~0.70s)
        for (int i = 0; i < 15; ++i) { // +0.24s -> ~0.69s
            canvas.simulatePhysicsStep(0.016);
        }

        EMPIRICAL_ASSERT(canvas.driftTimer >= 0.50 && canvas.driftTimer < 1.0,
                         "T1.4: Drift timer in Stage 1 Blue Sparks window [0.5s, 1.0s)",
                         "driftTimer=" + std::to_string(canvas.driftTimer));
        EMPIRICAL_ASSERT(!canvas.driftSparks.empty(),
                         "T1.4: Drift sparks spawn actively at t >= 0.5s",
                         "sparksCount=" + std::to_string(canvas.driftSparks.size()));

        // Inspect spark particle colors: must be Electric Blue / Cyan
        bool hasBlueComponent = false;
        for (const auto &p : canvas.driftSparks) {
            int b = qBlue(p.color);
            int r = qRed(p.color);
            if (b == 255 && (r == 60 || r == 160)) {
                hasBlueComponent = true;
            }
        }
        EMPIRICAL_ASSERT(hasBlueComponent,
                         "T1.4: Stage 1 sparks exhibit authentic electric blue/cyan hue (Blue=255)",
                         "hasBlueComponent=" + std::to_string(hasBlueComponent));

        // Release drift at 0.70s (before 1.0s) -> verify NO mini-turbo boost
        canvas.releaseKey(Qt::Key_Shift);
        canvas.simulatePhysicsStep(0.016);

        EMPIRICAL_ASSERT(canvas.isDrifting == false,
                         "T1.4: Drift ends cleanly on key release at 0.70s",
                         "isDrifting=" + std::to_string(canvas.isDrifting));
        EMPIRICAL_ASSERT(canvas.miniTurboTimer == 0.0,
                         "T1.4: Releasing before 1.0s awards ZERO mini-turbo boost",
                         "miniTurboTimer=" + std::to_string(canvas.miniTurboTimer));

        // Now drift continuously with throttle for >= 1.0s to achieve Stage 2 Orange Sparks & Mini-Turbo charge
        canvas.clearAllKeys();
        canvas.startRace();
        canvas.kartSpeed = 90.0;
        canvas.pressKey(Qt::Key_W);
        canvas.pressKey(Qt::Key_Shift);
        canvas.pressKey(Qt::Key_D);

        for (int i = 0; i < 68; ++i) { // 68 * 0.016 = 1.088s
            canvas.simulatePhysicsStep(0.016);
        }

        EMPIRICAL_ASSERT(canvas.driftTimer >= 1.0,
                         "T1.4: Drift timer reaches Stage 2 threshold (t >= 1.0s)",
                         "driftTimer=" + std::to_string(canvas.driftTimer));

        bool hasOrangeComponent = false;
        for (const auto &p : canvas.driftSparks) {
            int r = qRed(p.color);
            int b = qBlue(p.color);
            if (r == 255 && (b == 20 || b == 40)) {
                hasOrangeComponent = true;
            }
        }
        EMPIRICAL_ASSERT(hasOrangeComponent,
                         "T1.4: Stage 2 sparks exhibit fiery orange/golden hue (Red=255, Blue<=40)",
                         "hasOrangeComponent=" + std::to_string(hasOrangeComponent));
        EMPIRICAL_ASSERT(canvas.currentSurfaceName.contains("DRIFT CHARGED"),
                         "T1.4: HUD surface indicator reflects charged mini-turbo status",
                         "surface=" + canvas.currentSurfaceName.toStdString());
    }

    // 1.5 Mini-Turbo Boost Release: Impulse, 1.35x Speed Cap, and Expiration
    {
        canvas.clearAllKeys();
        canvas.startRace();
        canvas.kartSpeed = 100.0;
        canvas.pressKey(Qt::Key_W);
        canvas.pressKey(Qt::Key_Shift);
        canvas.pressKey(Qt::Key_D);

        // Drift past 1.0s
        for (int i = 0; i < 68; ++i) {
            canvas.simulatePhysicsStep(0.016);
        }

        double speedBeforeRelease = canvas.kartSpeed;
        canvas.releaseKey(Qt::Key_Shift);
        canvas.simulatePhysicsStep(0.016);

        EMPIRICAL_ASSERT(canvas.miniTurboTimer > 1.15 && canvas.miniTurboTimer <= 1.20,
                         "T1.5: Mini-turbo timer initialized to 1.2s on release",
                         "miniTurboTimer=" + std::to_string(canvas.miniTurboTimer));
        EMPIRICAL_ASSERT(canvas.kartSpeed >= speedBeforeRelease + 24.0,
                         "T1.5: Instantaneous +25.0 speed impulse awarded on mini-turbo release",
                         "speedBefore=" + std::to_string(speedBeforeRelease) + " speedAfter=" + std::to_string(canvas.kartSpeed));

        // Accelerate forward with mini-turbo active: verify kart exceeds regular maxSpeed (160.0) up to 1.35x (216.0)
        for (int i = 0; i < 40; ++i) { // ~0.64s of acceleration under boost
            canvas.simulatePhysicsStep(0.016);
        }

        EMPIRICAL_ASSERT(canvas.kartSpeed > 160.0,
                         "T1.5: Kart speed exceeds normal maximum speed (160 km/h) under mini-turbo",
                         "speed=" + std::to_string(canvas.kartSpeed));
        EMPIRICAL_ASSERT(canvas.kartSpeed <= 160.0 * 1.35 + 1e-4,
                         "T1.5: Kart speed strictly capped at 1.35x maxSpeed (216.0 km/h)",
                         "speed=" + std::to_string(canvas.kartSpeed));

        // Advance until mini-turbo boost expires (total time elapsed > 1.2s)
        for (int i = 0; i < 50; ++i) { // +0.8s -> total elapsed > 1.4s
            canvas.simulatePhysicsStep(0.016);
        }

        EMPIRICAL_ASSERT(canvas.miniTurboTimer == 0.0,
                         "T1.5: Mini-turbo boost timer expires cleanly (timer == 0.0)",
                         "miniTurboTimer=" + std::to_string(canvas.miniTurboTimer));
        EMPIRICAL_ASSERT(canvas.kartSpeed <= 160.0 + 1e-4,
                         "T1.5: Kart top speed cap cleanly returns to normal (160.0 km/h) after expiration",
                         "speed=" + std::to_string(canvas.kartSpeed));
    }

    // 1.6 Drift Hop Parabolic Arc
    {
        canvas.clearAllKeys();
        canvas.startRace();
        canvas.kartSpeed = 80.0;

        // Initiate drift
        canvas.pressKey(Qt::Key_Shift);
        canvas.pressKey(Qt::Key_A); // Left drift
        canvas.simulatePhysicsStep(0.016);

        EMPIRICAL_ASSERT(canvas.driftHopTimer > 0.19 && canvas.driftHopTimer <= 0.22,
                         "T1.6: Drift hop timer initialized to 0.22s",
                         "hopTimer=" + std::to_string(canvas.driftHopTimer));

        // Advance to mid-hop (~0.11s)
        for (int i = 0; i < 6; ++i) {
            canvas.simulatePhysicsStep(0.016);
        }

        EMPIRICAL_ASSERT(canvas.driftHopHeight > 2.5 && canvas.driftHopHeight <= 3.50,
                         "T1.6: Parabolic hop reaches peak altitude (~3.5 units)",
                         "hopHeight=" + std::to_string(canvas.driftHopHeight));

        // Advance to hop completion (> 0.22s)
        for (int i = 0; i < 10; ++i) {
            canvas.simulatePhysicsStep(0.016);
        }

        EMPIRICAL_ASSERT(canvas.driftHopTimer == 0.0,
                         "T1.6: Drift hop timer completes at 0.0s",
                         "hopTimer=" + std::to_string(canvas.driftHopTimer));
        EMPIRICAL_ASSERT(canvas.driftHopHeight == 0.0,
                         "T1.6: Drift hop height returns to ground level (0.0)",
                         "hopHeight=" + std::to_string(canvas.driftHopHeight));
    }

    // 1.7 Lateral Slip Angle & Asymmetric Steering Dynamics
    {
        canvas.clearAllKeys();
        canvas.startRace();
        canvas.kartSpeed = 80.0;
        canvas.kartAngle = 0.0;

        // Right drift: driftDirection = +1
        canvas.pressKey(Qt::Key_Shift);
        canvas.pressKey(Qt::Key_D);

        // Hold for 0.5s
        for (int i = 0; i < 30; ++i) {
            canvas.simulatePhysicsStep(0.016);
        }

        double slipInDrift = canvas.lateralSlipAngle;
        EMPIRICAL_ASSERT(slipInDrift > 0.20 && slipInDrift <= 0.28 * canvas.driftFactor + 1e-4,
                         "T1.7: Lateral slip angle converges toward target beta (+0.28 * driftFactor)",
                         "lateralSlip=" + std::to_string(slipInDrift));

        // Test asymmetric turning: steer-in (+1.25x) vs counter-steer (+0.45x)
        double angleBefore = canvas.kartAngle;
        canvas.pressKey(Qt::Key_D); // Steering into the drift
        canvas.releaseKey(Qt::Key_A);
        canvas.simulatePhysicsStep(0.032);
        double deltaSteerIn = canvas.kartAngle - angleBefore;

        angleBefore = canvas.kartAngle;
        canvas.releaseKey(Qt::Key_D);
        canvas.pressKey(Qt::Key_A); // Counter-steering opposite to drift
        canvas.simulatePhysicsStep(0.032);
        double deltaCounterSteer = canvas.kartAngle - angleBefore;

        EMPIRICAL_ASSERT(deltaSteerIn > deltaCounterSteer,
                         "T1.7: Steering into drift yields significantly higher yaw rate than counter-steering",
                         "steerIn=" + std::to_string(deltaSteerIn) + " counterSteer=" + std::to_string(deltaCounterSteer));
        EMPIRICAL_ASSERT(std::abs(deltaSteerIn / deltaCounterSteer - (1.25 / 0.45)) < 0.15,
                         "T1.7: Asymmetric turn rate ratio matches theoretical 1.25 / 0.45 = 2.78",
                         "ratio=" + std::to_string(deltaSteerIn / deltaCounterSteer));
    }
}

// ============================================================================
// SUITE 2: HIGH-GRIP BRAKING & REVERSE FEEL
// ============================================================================
void runSuite2_BrakingAndGripFeel(ChallengerM3Canvas &canvas) {
    std::cout << "\n===============================================================\n";
    std::cout << " SUITE 2: High-Grip Braking & Grip Feel\n";
    std::cout << "===============================================================\n";

    // 2.1 Full Braking Deceleration Rate (240.0 units/s^2)
    {
        canvas.clearAllKeys();
        canvas.setGameState(GameState::RACING);
        canvas.setGameMode(GameMode::PRACTICE);
        canvas.setTrackLevel(TrackLevel::DONUT_PLAINS);
        canvas.startRace();

        // Mario top speed is 160.0
        canvas.kartSpeed = 160.0;
        canvas.pressKey(Qt::Key_S); // Brake

        // Test within valid un-clamped dt (dt <= 0.05s)
        double dt = 0.040;
        double speedBefore = canvas.kartSpeed;
        canvas.simulatePhysicsStep(dt);
        double speedAfter = canvas.kartSpeed;

        double measuredDecel = (speedBefore - speedAfter) / dt;
        EMPIRICAL_ASSERT(std::abs(measuredDecel - 240.0) < 1.0,
                         "T2.1: Braking deceleration rate measures exactly 240.0 u/s^2",
                         "measuredDecel=" + std::to_string(measuredDecel));

        // Verify engine anti-tunneling clamp: dt = 0.10s is clamped to 0.05s (effective decel = 12.0 u per step)
        canvas.kartSpeed = 160.0;
        speedBefore = canvas.kartSpeed;
        canvas.simulatePhysicsStep(0.10); // Exceeds 0.05s clamp
        speedAfter = canvas.kartSpeed;
        double speedLoss = speedBefore - speedAfter;
        EMPIRICAL_ASSERT(std::abs(speedLoss - (240.0 * 0.05)) < 1e-4,
                         "T2.1: Anti-tunneling clamp enforces maximum 0.05s physics step on large dt",
                         "speedLoss=" + std::to_string(speedLoss));

        // Measure time to full stop from 160.0 u/s: theoretical is 160 / 240 = 0.6667s
        double timeToStop = 0.0;
        canvas.kartSpeed = 160.0;
        while (canvas.kartSpeed > 0.0 && timeToStop < 2.0) {
            canvas.simulatePhysicsStep(0.010);
            timeToStop += 0.010;
        }

        EMPIRICAL_ASSERT(std::abs(timeToStop - (160.0 / 240.0)) < 0.02,
                         "T2.1: Kart comes to a complete stop from 160 km/h in exactly 0.67s",
                         "timeToStop=" + std::to_string(timeToStop));
    }

    // 2.2 Reverse Speed Clamping (-30% of character maxSpeed)
    {
        // Test all 4 characters
        struct CharTest {
            int id;
            std::string name;
            double maxSpeed;
            double expectedMaxReverse;
        } roster[] = {
            { 0, "MARIO",  160.0, -48.0 },
            { 1, "BOWSER", 175.0, -52.5 },
            { 2, "PEACH",  152.0, -45.6 },
            { 3, "TOAD",   148.0, -44.4 }
        };

        for (const auto &c : roster) {
            canvas.clearAllKeys();
            canvas.selectedCharP1 = c.id;
            canvas.startRace();

            canvas.pressKey(Qt::Key_S); // Hold reverse
            for (int i = 0; i < 200; ++i) { // 200 * 0.016s = 3.2s
                canvas.simulatePhysicsStep(0.016);
            }

            EMPIRICAL_ASSERT(std::abs(canvas.kartSpeed - c.expectedMaxReverse) < 1e-4,
                             "T2.2: Reverse speed for " + c.name + " clamps at strictly -30% maxSpeed (" + std::to_string(c.expectedMaxReverse) + ")",
                             "speed=" + std::to_string(canvas.kartSpeed));
        }
    }

    // 2.3 Throttle & Brake Contention (W + S Simultaneous Input)
    {
        canvas.clearAllKeys();
        canvas.selectedCharP1 = 0;
        canvas.startRace();
        canvas.kartSpeed = 50.0;

        // Press both Forward (W) and Backward (S)
        canvas.pressKey(Qt::Key_W);
        canvas.pressKey(Qt::Key_S);
        canvas.simulatePhysicsStep(0.04);

        EMPIRICAL_ASSERT(canvas.kartSpeed > 50.0,
                         "T2.3: Forward throttle has precedence over brake on simultaneous press (no lockup)",
                         "speed=" + std::to_string(canvas.kartSpeed));
    }

    // 2.4 Rolling Friction Deceleration & Clean Zero Clamping
    {
        canvas.clearAllKeys();
        canvas.startRace();
        canvas.kartSpeed = 20.0;

        // Release all controls: coast to a stop
        for (int i = 0; i < 60; ++i) {
            canvas.simulatePhysicsStep(0.016);
        }

        EMPIRICAL_ASSERT(canvas.kartSpeed == 0.0,
                         "T2.4: Rolling friction decelerates kart to exact 0.0 without oscillating into reverse",
                         "speed=" + std::to_string(canvas.kartSpeed));
    }
}

// ============================================================================
// SUITE 3: CHARACTER SCALING INVARIANTS & ADVERSARIAL CROSS-ROSTER AUDIT
// ============================================================================
void runSuite3_CharacterScalingInvariants(ChallengerM3Canvas &canvas) {
    std::cout << "\n===============================================================\n";
    std::cout << " SUITE 3: Character Scaling Invariants & Cross-Roster Audit\n";
    std::cout << "===============================================================\n";

    canvas.initCharacters();
    const auto &chars = canvas.characters;
    EMPIRICAL_ASSERT(chars.size() == 4, "T3.0: Roster contains exactly 4 canonical characters", "size=" + std::to_string(chars.size()));

    const CharacterDef &mario  = chars[0];
    const CharacterDef &bowser = chars[1];
    const CharacterDef &peach  = chars[2];
    const CharacterDef &toad   = chars[3];

    // 3.1 Bowser vs Toad Core Agility Invariant: turnRate_Toad > turnRate_Bowser
    EMPIRICAL_ASSERT(toad.turnRate > bowser.turnRate,
                     "T3.1: Invariant: turnRate_Toad (" + std::to_string(toad.turnRate) + ") > turnRate_Bowser (" + std::to_string(bowser.turnRate) + ")",
                     "diff=" + std::to_string(toad.turnRate - bowser.turnRate));

    // 3.2 Bowser vs Toad Drift Factor Invariant: driftFactor_Toad > driftFactor_Bowser
    EMPIRICAL_ASSERT(toad.driftFactor > bowser.driftFactor,
                     "T3.2: Invariant: driftFactor_Toad (" + std::to_string(toad.driftFactor) + ") > driftFactor_Bowser (" + std::to_string(bowser.driftFactor) + ")",
                     "diff=" + std::to_string(toad.driftFactor - bowser.driftFactor));

    // 3.3 Bowser vs Toad Acceleration Invariant: accel_Toad > accel_Bowser
    EMPIRICAL_ASSERT(toad.accel > bowser.accel,
                     "T3.3: Invariant: accel_Toad (" + std::to_string(toad.accel) + ") > accel_Bowser (" + std::to_string(bowser.accel) + ")",
                     "diff=" + std::to_string(toad.accel - bowser.accel));

    // 3.4 Bowser vs Toad Top Speed Invariant: maxSpeed_Bowser > maxSpeed_Toad
    EMPIRICAL_ASSERT(bowser.maxSpeed > toad.maxSpeed,
                     "T3.4: Invariant: maxSpeed_Bowser (" + std::to_string(bowser.maxSpeed) + ") > maxSpeed_Toad (" + std::to_string(toad.maxSpeed) + ")",
                     "diff=" + std::to_string(bowser.maxSpeed - toad.maxSpeed));

    // 3.5 Bowser vs Toad Weight Invariant: weight_Bowser > weight_Toad
    EMPIRICAL_ASSERT(bowser.weight > toad.weight,
                     "T3.5: Invariant: weight_Bowser (" + std::to_string(bowser.weight) + ") > weight_Toad (" + std::to_string(toad.weight) + ")",
                     "diff=" + std::to_string(bowser.weight - toad.weight));

    // 3.6 Complete Hierarchy Verification
    // Turn Rate: Peach (3.40) > Toad (3.10) > Mario (2.80) > Bowser (2.20)
    bool turnRateStrictOrder = (peach.turnRate > toad.turnRate) &&
                               (toad.turnRate > mario.turnRate) &&
                               (mario.turnRate > bowser.turnRate);
    EMPIRICAL_ASSERT(turnRateStrictOrder,
                     "T3.6: Strict turn rate hierarchy: Peach > Toad > Mario > Bowser",
                     "turnRates: Peach=" + std::to_string(peach.turnRate) + ", Toad=" + std::to_string(toad.turnRate) + ", Mario=" + std::to_string(mario.turnRate) + ", Bowser=" + std::to_string(bowser.turnRate));

    // Acceleration: Toad (140) > Peach (125) > Mario (110) > Bowser (85)
    bool accelStrictOrder = (toad.accel > peach.accel) &&
                            (peach.accel > mario.accel) &&
                            (mario.accel > bowser.accel);
    EMPIRICAL_ASSERT(accelStrictOrder,
                     "T3.6: Strict acceleration hierarchy: Toad > Peach > Mario > Bowser",
                     "accels: Toad=" + std::to_string(toad.accel) + ", Peach=" + std::to_string(peach.accel) + ", Mario=" + std::to_string(mario.accel) + ", Bowser=" + std::to_string(bowser.accel));

    // Top Speed: Bowser (175) > Mario (160) > Peach (152) > Toad (148)
    bool speedStrictOrder = (bowser.maxSpeed > mario.maxSpeed) &&
                            (mario.maxSpeed > peach.maxSpeed) &&
                            (peach.maxSpeed > toad.maxSpeed);
    EMPIRICAL_ASSERT(speedStrictOrder,
                     "T3.6: Strict top speed hierarchy: Bowser > Mario > Peach > Toad",
                     "speeds: Bowser=" + std::to_string(bowser.maxSpeed) + ", Mario=" + std::to_string(mario.maxSpeed) + ", Peach=" + std::to_string(peach.maxSpeed) + ", Toad=" + std::to_string(toad.maxSpeed));

    // 3.7 In-Engine startRace() Dynamic Stat Synchronization
    {
        canvas.clearAllKeys();
        canvas.selectedCharP1 = 1; // Bowser
        canvas.startRace();
        EMPIRICAL_ASSERT(canvas.maxSpeed == 175.0 && canvas.accel == 85.0 && canvas.turnRate == 2.20 && canvas.driftFactor == 0.85,
                         "T3.7: Canvas properties accurately synchronize to Bowser on race start",
                         "maxSpeed=" + std::to_string(canvas.maxSpeed) + " turnRate=" + std::to_string(canvas.turnRate));

        canvas.selectedCharP1 = 3; // Toad
        canvas.startRace();
        EMPIRICAL_ASSERT(canvas.maxSpeed == 148.0 && canvas.accel == 140.0 && canvas.turnRate == 3.10 && canvas.driftFactor == 1.15,
                         "T3.7: Canvas properties accurately synchronize to Toad on race start",
                         "maxSpeed=" + std::to_string(canvas.maxSpeed) + " turnRate=" + std::to_string(canvas.turnRate));
    }

    // 3.8 Empirical Sprint Benchmark: Toad vs Bowser 0 to 100 km/h
    {
        canvas.clearAllKeys();

        // Time Toad 0-100 km/h
        canvas.selectedCharP1 = 3; // Toad
        canvas.startRace();
        canvas.pressKey(Qt::Key_W);
        double timeToad100 = 0.0;
        while (canvas.kartSpeed < 100.0 && timeToad100 < 5.0) {
            canvas.simulatePhysicsStep(0.010);
            timeToad100 += 0.010;
        }

        // Time Bowser 0-100 km/h
        canvas.clearAllKeys();
        canvas.selectedCharP1 = 1; // Bowser
        canvas.startRace();
        canvas.pressKey(Qt::Key_W);
        double timeBowser100 = 0.0;
        while (canvas.kartSpeed < 100.0 && timeBowser100 < 5.0) {
            canvas.simulatePhysicsStep(0.010);
            timeBowser100 += 0.010;
        }

        EMPIRICAL_ASSERT(timeToad100 < timeBowser100,
                         "T3.8: Toad reaches 100 km/h faster than Bowser (" + std::to_string(timeToad100) + "s vs " + std::to_string(timeBowser100) + "s)",
                         "timeToad=" + std::to_string(timeToad100) + " timeBowser=" + std::to_string(timeBowser100));
    }
}

// ============================================================================
// SUITE 4: ADVERSARIAL HAZARD INTERRUPTIONS & NUMERICAL STABILITY
// ============================================================================
void runSuite4_AdversarialHazardInterruptionsAndStability(ChallengerM3Canvas &canvas) {
    std::cout << "\n===============================================================\n";
    std::cout << " SUITE 4: Adversarial Hazard Interruptions & Numerical Stability\n";
    std::cout << "===============================================================\n";

    // 4.1 Molten Lava Recoil Aborts Active Drift
    {
        canvas.clearAllKeys();
        canvas.setGameState(GameState::RACING);
        canvas.setGameMode(GameMode::PRACTICE);
        canvas.setTrackLevel(TrackLevel::BOWSER_CASTLE);
        canvas.startRace();

        // Kart drifting with charged drift
        canvas.kartSpeed = 100.0;
        canvas.pressKey(Qt::Key_W);
        canvas.pressKey(Qt::Key_Shift);
        canvas.pressKey(Qt::Key_D);
        for (int i = 0; i < 70; ++i) { // Charge drift past 1.0s
            canvas.simulatePhysicsStep(0.016);
        }

        EMPIRICAL_ASSERT(canvas.isDrifting && canvas.driftTimer >= 1.0,
                         "T4.1: Drift is fully charged before hazard encounter",
                         "driftTimer=" + std::to_string(canvas.driftTimer));

        // Move kart into lava (magenta pixel 255, 0, 255)
        // Set kart coordinate into known lava zone on Bowser Castle
        canvas.kartX = 100.0;
        canvas.kartZ = 100.0;
        canvas.simulatePhysicsStep(0.016);

        EMPIRICAL_ASSERT(canvas.isDrifting == false,
                         "T4.1: Molten lava recoil instantly breaks active drift (isDrifting == false)",
                         "isDrifting=" + std::to_string(canvas.isDrifting));
        EMPIRICAL_ASSERT(canvas.driftTimer == 0.0,
                         "T4.1: Drift timer resets to 0.0 on lava impact",
                         "driftTimer=" + std::to_string(canvas.driftTimer));
        EMPIRICAL_ASSERT(canvas.miniTurboTimer == 0.0,
                         "T4.1: No mini-turbo boost is awarded when drift is broken by lava",
                         "miniTurboTimer=" + std::to_string(canvas.miniTurboTimer));
        EMPIRICAL_ASSERT(canvas.isSpinning == true,
                         "T4.1: Kart enters spinout state on lava recoil",
                         "isSpinning=" + std::to_string(canvas.isSpinning));
    }

    // 4.2 Active Spinout Blocks Drift Initiation
    {
        canvas.clearAllKeys();
        canvas.setTrackLevel(TrackLevel::DONUT_PLAINS);
        canvas.startRace();
        canvas.kartSpeed = 80.0;
        canvas.isSpinning = true;
        canvas.spinTimer = 0.50;

        // Try to drift while spinning
        canvas.pressKey(Qt::Key_Shift);
        canvas.pressKey(Qt::Key_D);
        canvas.simulatePhysicsStep(0.016);

        EMPIRICAL_ASSERT(canvas.isDrifting == false,
                         "T4.2: Drift CANNOT be initiated while kart is spinning out",
                         "isDrifting=" + std::to_string(canvas.isDrifting));
    }

    // 4.3 High-Frequency Random Input Fuzzing (Numerical Stability)
    {
        canvas.clearAllKeys();
        canvas.startRace();
        canvas.kartSpeed = 60.0;

        std::mt19937 rng(1337);
        std::uniform_int_distribution<int> keyDist(0, 5);
        std::uniform_real_distribution<double> dtDist(0.001, 0.060);

        int keyMap[6] = { Qt::Key_W, Qt::Key_S, Qt::Key_A, Qt::Key_D, Qt::Key_Shift, Qt::Key_Space };
        bool nanOrInfDetected = false;
        bool outOfBoundsDetected = false;

        for (int step = 0; step < 1000; ++step) {
            int actionKey = keyMap[keyDist(rng)];
            bool press = (rng() % 2 == 0);
            if (press) canvas.pressKey(actionKey);
            else canvas.releaseKey(actionKey);

            double dt = dtDist(rng);
            canvas.simulatePhysicsStep(dt);

            if (std::isnan(canvas.kartX) || std::isnan(canvas.kartZ) ||
                std::isnan(canvas.kartSpeed) || std::isnan(canvas.kartAngle) ||
                std::isinf(canvas.kartX) || std::isinf(canvas.kartZ) ||
                std::isinf(canvas.kartSpeed) || std::isinf(canvas.kartAngle)) {
                nanOrInfDetected = true;
                break;
            }

            if (canvas.kartX < 0.0 || canvas.kartX >= 1024.0 ||
                canvas.kartZ < 0.0 || canvas.kartZ >= 1024.0) {
                outOfBoundsDetected = true;
                break;
            }
        }

        EMPIRICAL_ASSERT(!nanOrInfDetected,
                         "T4.3: 1,000 steps of random input fuzzing executes without NaN or Inf",
                         "nanDetected=" + std::to_string(nanOrInfDetected));
        EMPIRICAL_ASSERT(!outOfBoundsDetected,
                         "T4.3: World coordinate bounds [0, 1024) strictly maintained under fuzzing",
                         "outOfBounds=" + std::to_string(outOfBoundsDetected));
    }
}

// ============================================================================
// MAIN ENTRY POINT
// ============================================================================
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    std::cout << "===============================================================\n";
    std::cout << " RETROKART 3D - MILESTONE 3 EMPIRICAL CHALLENGER TEST SUITE    \n";
    std::cout << " Desktop Qt 6.11.1 MinGW 64-bit | Physics, Drift & Invariants   \n";
    std::cout << "===============================================================\n";

    ChallengerM3Canvas canvas;

    runSuite1_DriftMechanicsUnderExtremeInputs(canvas);
    runSuite2_BrakingAndGripFeel(canvas);
    runSuite3_CharacterScalingInvariants(canvas);
    runSuite4_AdversarialHazardInterruptionsAndStability(canvas);

    std::cout << "\n===============================================================\n";
    std::cout << "                  CHALLENGER EXECUTION SUMMARY                 \n";
    std::cout << "===============================================================\n";
    std::cout << " Total Challenges Executed: " << std::setw(4) << g_tests.total << "\n";
    std::cout << " Passed Challenges:        " << std::setw(4) << g_tests.passed << " ("
              << (g_tests.total > 0 ? (g_tests.passed * 100.0 / g_tests.total) : 0.0) << "%)\n";
    std::cout << " Failed Challenges:        " << std::setw(4) << g_tests.failed << "\n";
    std::cout << "===============================================================\n";

    if (g_tests.failed == 0) {
        std::cout << " >>> VERDICT: APPROVE <<<\n";
        std::cout << " >>> ALL MILESTONE 3 PHYSICS & INVARIANT CHALLENGES PASSED! <<<\n";
    } else {
        std::cout << " >>> VERDICT: REQUEST_CHANGES <<<\n";
        std::cout << " >>> FAILURES DETECTED IN EMPIRICAL AUDIT: <<<\n";
        for (const auto &fail : g_tests.failures) {
            std::cout << "   - " << fail << "\n";
        }
    }
    std::cout << "===============================================================\n";

    return (g_tests.failed == 0) ? 0 : 1;
}
