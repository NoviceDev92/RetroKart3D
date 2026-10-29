#ifndef TEST_TYPES_H
#define TEST_TYPES_H

#include <QString>
#include <QColor>
#include <QPointF>
#include <vector>
#include <set>
#include <cmath>

// ============================================================================
// RetroKart3D Interface Contracts (as defined in PROJECT.md and R1 - R5)
// ============================================================================

enum class GameState {
    TITLE_MENU,
    CHARACTER_SELECT,
    RACING,
    RACE_FINISHED
};

enum class GameMode {
    PLAY_VS_AI,
    MULTIPLAYER,
    PRACTICE
};

enum class CharacterId {
    MARIO = 0,
    BOWSER = 1,
    PEACH = 2,
    TOAD = 3
};

struct CharacterDef {
    int id;
    QString name;
    QColor kartColor;
    double maxSpeed;    // 150.0 - 175.0
    double accel;       // 85.0 - 135.0
    double turnRate;    // 1.9 - 2.9
    double driftFactor; // 1.25 - 1.40
    double weight;      // 0.8 (Light) - 1.5 (Heavy)
};

enum class PlayerItem {
    NONE = 0,
    MUSHROOM,
    ROCKET,
    BANANA,
    GREEN_SHELL,
    INK_BLOOPER
};

enum class TrackLevel {
    DONUT_PLAINS = 0,
    CHOCO_VALLEY = 1,
    BOWSER_CASTLE = 2
};

enum class HazardType {
    LAVA_GEYSER,
    CHOCOLATE_FOUNTAIN,
    ROLLING_CANDY
};

struct TrackHazard {
    double x;
    double z;
    HazardType type;
    double animTimer;
    bool isErupting;
    double radius;
};

struct ShellProjectile {
    double x;
    double z;
    double vx;
    double vz;
    int ownerId;
    bool active;
    double lifeTime;
    int bouncesLeft;
};

struct DroppedBanana {
    double x;
    double z;
    int ownerId;
    double immunityTimer; // 1.2s grace timer for dropping player
    bool active;
};

struct KartEntity {
    int id = 0;
    QString name = "Kart";
    bool isAI = false;
    int characterId = 0;
    QColor bodyColor = QColor(220, 20, 20);

    // Transform and kinematics
    double x = 0.0;
    double z = 0.0;
    double angle = 0.0;       // radians
    double speed = 0.0;
    double steerAngle = 0.0;  // for wheel visualization

    // Tuning stats derived from character
    double maxSpeed = 160.0;
    double accel = 110.0;
    double brakeDecel = 240.0;
    double turnRate = 2.4;
    double driftFactor = 1.30;
    double weight = 1.0;

    // Drift and Boost mechanics
    bool isDrifting = false;
    double driftTimer = 0.0;      // accumulated drift time
    double miniTurboTimer = 0.0;   // active mini-turbo boost time

    // Debuff and Status
    bool isSpinning = false;
    double spinTimer = 0.0;
    bool isInked = false;
    double inkTimer = 0.0;

    // Items and Race progression
    PlayerItem currentItem = PlayerItem::NONE;
    int currentLap = 1;
    int currentWaypointIndex = 0;
    double lookaheadDist = 50.0;
    double totalRaceDistance = 0.0;

    // Finish state
    bool hasFinished = false;
    double finishTime = 0.0;
    int placement = 1;
};

struct MinimapHUD {
    static constexpr int RADAR_X = 8;
    static constexpr int RADAR_Y = 8;
    static constexpr int RADAR_SIZE = 72;
    static constexpr int MAP_WORLD_SIZE = 1024;

    static QPointF worldToRadar(double worldX, double worldZ) {
        // Safe modulo wrap for 0..1024
        double wrappedX = std::fmod(std::fmod(worldX, (double)MAP_WORLD_SIZE) + (double)MAP_WORLD_SIZE, (double)MAP_WORLD_SIZE);
        double wrappedZ = std::fmod(std::fmod(worldZ, (double)MAP_WORLD_SIZE) + (double)MAP_WORLD_SIZE, (double)MAP_WORLD_SIZE);
        double rx = RADAR_X + (wrappedX / (double)MAP_WORLD_SIZE) * (double)RADAR_SIZE;
        double ry = RADAR_Y + (wrappedZ / (double)MAP_WORLD_SIZE) * (double)RADAR_SIZE;
        return QPointF(rx, ry);
    }
};

struct ViewportSplit {
    static constexpr int TOTAL_WIDTH = 400;
    static constexpr int TOTAL_HEIGHT = 300;
    static constexpr int P1_TOP = 0;
    static constexpr int P1_BOTTOM = 149;
    static constexpr int DIVIDER_Y = 150;
    static constexpr int DIVIDER_HEIGHT = 2;
    static constexpr int P2_TOP = 152;
    static constexpr int P2_BOTTOM = 300;
};

#endif // TEST_TYPES_H
