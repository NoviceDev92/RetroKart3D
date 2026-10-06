#ifndef GAME_CANVAS_H
#define GAME_CANVAS_H

#include <QWidget>
#include <QImage>
#include <QTimer>
#include <QElapsedTimer>
#include <vector>
#include <set>
#include <cmath>

enum class TrackLevel {
    DONUT_PLAINS = 0,   // Level 1: Easy
    CHOCO_VALLEY = 1,   // Level 2: Medium
    BOWSER_CASTLE = 2   // Level 3: Hard
};

enum class SpriteType {
    QUESTION_BOX,
    GREEN_PIPE,
    COIN,
    BANANA
};

struct WorldSprite {
    double x;
    double z;
    double y;           // height offset above ground
    SpriteType type;
    bool active;
};

class GameCanvas : public QWidget
{
    Q_OBJECT

public:
    explicit GameCanvas(QWidget *parent = nullptr);
    ~GameCanvas() override = default;

    void setTrackLevel(TrackLevel level);
    void resetKart();

signals:
    void statsUpdated(double speed, int lap, double x, double z, double angle, QString surface);

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;

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
    double brakeDecel = 160.0;
    double friction = 45.0;
    double turnRate = 2.4;
    double steeringVisualAngle = 0.0; // For cockpit steering wheel

    // Camera parameters
    double cameraHeight = 14.0;
    double focalLength = 180.0;
    double cameraNearPlane = 2.0;

    // Game stats
    int currentLap = 1;
    int nextCheckpoint = 1;
    QString currentSurfaceName = "Tarmac";

    // Engine rendering steps
    void initTrackMaps();
    void generateDonutPlains();
    void generateChocoValley();
    void generateBowserCastle();

    void updatePhysics(double dt);
    void renderSky();
    void renderGroundMode7();
    void renderSprites();
    void renderCockpit();

    // Sprite drawing helpers
    void drawSpritePixelArt(int screenX, int screenY, int drawW, int drawH, SpriteType type);
};

#endif // GAME_CANVAS_H
