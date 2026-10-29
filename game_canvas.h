#ifndef GAME_CANVAS_H
#define GAME_CANVAS_H

#include <QWidget>
#include <QImage>
#include <QTimer>
#include <QElapsedTimer>
#include <QMouseEvent>
#include <QString>
#include <QPointF>
#include <vector>
#include <set>
#include <cmath>
#include "sprite_atlas.h"

enum class GameState {
    TITLE_MENU,         // Startup title screen and game mode selection
    CHARACTER_SELECT,   // Character roster selection with kart stats
    RACING,             // Active Mode-7 gameplay loop
    RACE_FINISHED       // 3 laps complete or victory celebration with restart options
};

enum class GameMode {
    PLAY_VS_AI,         // Grand Prix with active AI opponent fleet (3 rivals)
    MULTIPLAYER,        // Local 2-Player Split-Screen head-to-head racing
    PRACTICE            // Solo time trial and course exploration
};

struct CharacterDef {
    int id;                     // 0: Mario, 1: Bowser, 2: Peach, 3: Toad
    QString name;               // "MARIO", "BOWSER", "PEACH", "TOAD"
    QString archetype;          // "BALANCED", "HEAVYWEIGHT", "AGILITY", "ACCEL"
    QString description;        // Summary trait / narrative
    QColor primaryColor;        // Main chassis / hood color
    QColor secondaryColor;      // Racing stripe / accent color
    QColor altColor;            // Alt palette color for duplicate picks
    double maxSpeed;            // Top speed on flat tarmac (km/h)
    double accel;               // Forward acceleration rate (units/s^2)
    double turnRate;            // Base steering rotation rate (rad/s)
    double driftFactor;         // Drift lateral slide multiplier
    double weight;              // Collision pushback resistance
    int speedPips;              // 1 to 5 pips
    int accelPips;              // 1 to 5 pips
    int handlingPips;           // 1 to 5 pips
    int weightPips;             // 1 to 5 pips
};

enum class TrackLevel {
    DONUT_PLAINS = 0,   // Level 1: Easy
    CHOCO_VALLEY = 1,   // Level 2: Medium
    BOWSER_CASTLE = 2   // Level 3: Hard
};

// Player's held item
enum class PlayerItem {
    NONE,
    MUSHROOM,
    ROCKET,
    BANANA,
    GREEN_SHELL,
    INK_BLOOPER
};

// Downstream multi-kart contract for Milestone 2 & 4
struct KartEntity {
    int id = 0;
    bool isHuman = true;
    bool isAI = false;
    int characterId = 0;
    QString name = "MARIO";
    QColor kartColor = QColor(220, 20, 20);
    double x = 470.0;
    double z = 200.0;
    double angle = 0.0;
    double speed = 0.0;
    double steerAngle = 0.0;
    double maxSpeed = 160.0;
    double accel = 110.0;
    double brakeDecel = 240.0;
    double friction = 45.0;
    double turnRate = 2.8;
    double driftFactor = 1.0;
    double weight = 1.0;
    bool isDrifting = false;
    double driftTimer = 0.0;
    double miniTurboTimer = 0.0;
    double driftHopTimer = 0.0;
    double lateralSlip = 0.0;
    int driftDirection = 0;
    double mushroomBoostTimer = 0.0;
    PlayerItem heldItem = PlayerItem::NONE;
    bool isSpinning = false;
    double spinTimer = 0.0;
    bool isInked = false;
    double inkTimer = 0.0;
    double bananaGraceTimer = 0.0;
    int currentWaypoint = 0;
    int currentWaypointIndex = 0;
    double lookaheadDist = 50.0;
    int currentLap = 1;
    int nextCheckpoint = 1;
    bool hasPassedHalfway = false;
    bool firstStartCrossed = false;
    bool raceFinished = false;
    int placement = 1;
    double totalRaceDistance = 0.0;
};

// Projectile contract for Milestone 2
struct Projectile {
    double x = 0.0;
    double z = 0.0;
    double vx = 0.0;
    double vz = 0.0;
    int bouncesLeft = 3;
    bool active = true;
    int ownerId = 0;
    double lifeTime = 6.0;
};
using ShellProjectile = Projectile;

// Track hazard contract for Milestone 3
enum class HazardType {
    LAVA_GEYSER,
    CHOCOLATE_FOUNTAIN,
    ROLLING_CANDY
};

struct TrackHazard {
    double x = 0.0;
    double z = 0.0;
    HazardType type = HazardType::LAVA_GEYSER;
    double animTimer = 0.0;
    bool isErupting = false;
    double radius = 16.0;
};

struct WorldSprite {
    double x;
    double z;
    double y;           // height offset above ground
    SpriteType type;
    bool active;
    double respawnTimer; // time remaining before respawn (for item boxes)
    int ownerId = -1;
    double immunityTimer = 0.0;
};

#ifndef MINIMAP_HUD_DEFINED
#define MINIMAP_HUD_DEFINED
struct MinimapHUD {
    static constexpr int RADAR_X = 8;
    static constexpr int RADAR_Y = 8;
    static constexpr int RADAR_SIZE = 72;
    static constexpr int MAP_WORLD_SIZE = 1024;

    static QPointF worldToRadar(double worldX, double worldZ) {
        double wrappedX = std::fmod(std::fmod(worldX, (double)MAP_WORLD_SIZE) + (double)MAP_WORLD_SIZE, (double)MAP_WORLD_SIZE);
        double wrappedZ = std::fmod(std::fmod(worldZ, (double)MAP_WORLD_SIZE) + (double)MAP_WORLD_SIZE, (double)MAP_WORLD_SIZE);
        double rx = RADAR_X + (wrappedX / (double)MAP_WORLD_SIZE) * (double)RADAR_SIZE;
        double ry = RADAR_Y + (wrappedZ / (double)MAP_WORLD_SIZE) * (double)RADAR_SIZE;
        return QPointF(rx, ry);
    }
};
#endif

class GameCanvas : public QWidget
{
    Q_OBJECT

public:
    explicit GameCanvas(QWidget *parent = nullptr);
    ~GameCanvas() override = default;

    void setTrackLevel(TrackLevel level);
    void resetKart();
    void triggerCelebration(const QString &title = "VICTORY!", const QString &subtitle = "COURSE CLEAR!");
    void startRace();

    GameState getGameState() const { return currentState; }
    void setGameState(GameState state);
    GameMode getGameMode() const { return selectedGameMode; }
    void setGameMode(GameMode mode) { selectedGameMode = mode; update(); }
    const std::vector<CharacterDef>& getCharacters() const { return characters; }
    const CharacterDef& getSelectedCharacterP1() const;
    const CharacterDef& getSelectedCharacterP2() const;
    int getSelectedCharIndexP1() const { return selectedCharP1; }
    int getSelectedCharIndexP2() const { return selectedCharP2; }
    void setSelectedCharIndexP1(int index) { selectedCharP1 = (index % 4 + 4) % 4; update(); }
    void setSelectedCharIndexP2(int index) { selectedCharP2 = (index % 4 + 4) % 4; update(); }

    const std::vector<QPointF>& getTrackWaypoints() const { return currentTrackWaypoints; }
    const std::vector<QPointF>& getTrackWaypoints(int trackIdx) const {
        return (trackIdx >= 0 && trackIdx < 3) ? trackWaypoints[trackIdx] : currentTrackWaypoints;
    }
    const std::vector<KartEntity>& getAIOpponents() const { return aiOpponents; }
    const std::vector<Projectile>& getActiveProjectiles() const { return activeProjectiles; }
    const std::vector<TrackHazard>& getTrackHazards() const { return trackHazards; }

signals:
    void statsUpdated(double speed, int lap, double x, double z, double angle, QString surface);

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private slots:
    void updateGameLoop();

private:
    // Screen buffer (Retro low-resolution buffer)
    static constexpr int BUFFER_WIDTH = 400;
    static constexpr int BUFFER_HEIGHT = 300;
    static constexpr int HORIZON_Y = 150;
    static constexpr int TRACK_MAP_SIZE = 1024;

    QImage screenBuffer;
    QTimer gameTimer;
    QElapsedTimer frameTimer;
    std::set<int> pressedKeys;

    // Game State Flow & Mode
    GameState currentState = GameState::TITLE_MENU;
    GameMode selectedGameMode = GameMode::PLAY_VS_AI;
    int menuIndex = 0;                  // 0: Play vs AI, 1: Multiplayer, 2: Practice
    int selectedCharP1 = 0;             // 0: Mario, 1: Bowser, 2: Peach, 3: Toad
    int selectedCharP2 = 1;             // P2 selection
    int charSelectPhase = 0;            // 0: P1 selecting, 1: P2 selecting in 2P mode
    std::vector<CharacterDef> characters;
    void initCharacters();

    // Multi-kart and downstream entities
    KartEntity player1;
    KartEntity player2;
    std::vector<KartEntity> aiOpponents;
    std::vector<Projectile> activeProjectiles;
    std::vector<TrackHazard> trackHazards;

    // Global animation time
    double globalTime = 0.0;

    // Track state
    TrackLevel currentLevel = TrackLevel::DONUT_PLAINS;
    QImage trackVisualMap;
    QImage trackMaskMap;
    std::vector<WorldSprite> worldSprites;

    // Kart state
    double kartX = 470.0;
    double kartZ = 200.0;
    double kartAngle = 0.0;     // radians: 0 points East along starting straight
    double kartSpeed = 0.0;
    double maxSpeed = 160.0;
    double accel = 110.0;
    double brakeDecel = 240.0;
    double friction = 45.0;
    double turnRate = 2.8;
    double driftFactor = 1.0;
    double weight = 1.0;
    double steeringVisualAngle = 0.0; // For cockpit steering wheel

    // Milestone 3: Drift & Mini-Turbo System
    bool isDrifting = false;
    double driftTimer = 0.0;
    double miniTurboTimer = 0.0;
    double driftHopTimer = 0.0;
    double driftHopHeight = 0.0;
    double lateralSlipAngle = 0.0;
    int driftDirection = 0;

    struct SparkParticle {
        double x, y;
        double vx, vy;
        double life;
        double maxLife;
        QRgb color;
    };
    std::vector<SparkParticle> driftSparks;
    void updateDriftSparks(double dt);

    // Camera parameters
    double cameraHeight = 14.0;
    double focalLength = 180.0;
    double cameraNearPlane = 2.0;

    // Game stats
    int currentLap = 1;
    int nextCheckpoint = 1;
    QString currentSurfaceName = "Tarmac";

    // Player Item System
    PlayerItem heldItem = PlayerItem::NONE;
    bool isRouletteActive = false;
    double rouletteTimer = 0.0;
    double rouletteDuration = 2.0;
    int rouletteIndex = 0;
    int targetItemIndex = 0;
    double rouletteSpeed = 0.0;

    // Item effect timers
    double mushroomBoostTimer = 0.0;
    bool isSpinning = false;
    double spinTimer = 0.0;
    double inkTimer = 0.0;

    // Celebration & Finish Line Animation
    struct ConfettiParticle {
        double x, y;
        double vx, vy;
        double w, h;
        double angle;
        double vRot;
        QColor color;
    };

    bool isCelebrationActive = false;
    bool isRaceFinished = false;
    double celebrationTimer = 0.0;
    double celebrationDuration = 3.6;
    QString celebrationTitle = "LAP COMPLETE!";
    QString celebrationSubtitle = "NICE DRIVING!";
    std::vector<ConfettiParticle> confetti;
    double flashIntensity = 0.0;
    bool hasPassedHalfway = false;
    bool firstStartCrossed = false;

    // Persistent waypoint collections per track (0: Donut, 1: Choco, 2: Bowser)
    std::vector<QPointF> trackWaypoints[3];
    std::vector<QPointF> currentTrackWaypoints;

    void initTrackWaypoints();
    void extractDonutPlainsWaypoints();
    void extractChocoValleyWaypoints();
    void extractBowserCastleWaypoints();

    // Engine rendering steps
    void initTrackMaps();
    void generateDonutPlains();
    void generateChocoValley();
    void generateBowserCastle();

    void updatePhysics(double dt);
    void updateAIOpponents(double dt);
    void updateItemSystem(double dt);
    void updateTrackHazards(double dt);
    SpriteType getKartSpriteType(int characterId, bool isAlt = false) const;
    void renderSky();
    void renderGroundMode7();
    void renderSprites();
    void renderCockpit();
    void renderItemHUD();
    void renderMinimap();
    void renderInkOverlay();

    // Minimap track cache (F12)
    QImage minimapTrackCache;
    void updateMinimapTrackCache();

    void updateCelebration(double dt);
    void renderCelebration();

    // Menu and overlay rendering
    void renderTitleMenu();
    void renderCharacterSelect();
    void renderRaceFinishedOverlay();

    // Input handlers
    void handleTitleMenuInput(int key);
    void handleCharacterSelectInput(int key);
    void handleRacingInput(int key);
    void confirmTitleMenuSelection();

    // Texture-mapped sprite drawing
    void drawTexturedBillboard(int screenX, int screenY, int drawW, int drawH, const QImage& texture);

    // Item roulette helpers
    void startItemRoulette();
    void useHeldItem();
    SpriteType rouletteItemAt(int index) const;
    PlayerItem spriteToPlayerItem(SpriteType type) const;
};

#endif // GAME_CANVAS_H
