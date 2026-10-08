#ifndef SPRITE_ATLAS_H
#define SPRITE_ATLAS_H

#include <QImage>
#include <QColor>
#include <map>

// All sprite images are generated programmatically using pixel manipulation.
// No external assets needed - every pixel is hand-placed in code.

enum class SpriteType {
    // Powerup pickups (bob up and down, glowing)
    QUESTION_BOX,       // Contains a random item
    MUSHROOM,           // Speed boost
    ROCKET,             // Bullet Bill super-boost
    BANANA,             // Drop behind you
    GREEN_SHELL,        // Fire forward, bounces off walls
    INK_BLOOPER,        // Obscures opponents' vision

    // Obstacles (grounded, static, dark/threatening)
    GREEN_PIPE,         // Roadside obstacle
    THWOMP,             // Crushing stone block
    OIL_SLICK,          // Flat ground hazard

    // Collectibles
    COIN,               // Score/currency

    // Dropped items on track
    BANANA_DROPPED,     // Banana sitting on the road
    SHELL_PROJECTILE    // Moving shell
};

class SpriteAtlas
{
public:
    static SpriteAtlas& instance();

    const QImage& getSprite(SpriteType type) const;
    bool isPickup(SpriteType type) const;
    bool isObstacle(SpriteType type) const;

private:
    SpriteAtlas();
    void generateAllSprites();

    // 32x32 ARGB sprites
    static constexpr int SPRITE_SIZE = 32;

    void generateQuestionBox();
    void generateMushroom();
    void generateRocket();
    void generateBanana();
    void generateGreenShell();
    void generateInkBlooper();
    void generateGreenPipe();
    void generateThwomp();
    void generateOilSlick();
    void generateCoin();
    void generateBananaDropped();
    void generateShellProjectile();

    // Helper: set pixel with bounds check
    void setPixel(QImage& img, int x, int y, QRgb color);
    void fillCircle(QImage& img, int cx, int cy, int r, QRgb fill, QRgb outline);
    void fillRect(QImage& img, int x, int y, int w, int h, QRgb color);

    std::map<SpriteType, QImage> sprites;
};

#endif // SPRITE_ATLAS_H
