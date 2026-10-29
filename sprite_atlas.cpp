#include "sprite_atlas.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

SpriteAtlas& SpriteAtlas::instance()
{
    static SpriteAtlas atlas;
    return atlas;
}

SpriteAtlas::SpriteAtlas()
{
    generateAllSprites();
}

void SpriteAtlas::generateAllSprites()
{
    generateQuestionBox();
    generateMushroom();
    generateRocket();
    generateBanana();
    generateGreenShell();
    generateInkBlooper();
    generateGreenPipe();
    generateThwomp();
    generateOilSlick();
    generateCoin();
    generateBananaDropped();
    generateShellProjectile();
    generateKartMario();
    generateKartBowser();
    generateKartPeach();
    generateKartToad();
    generateKartAlts();
    generateLavaGeyserBubbling();
    generateLavaGeyserErupting();
    generateChocolateFountain();
    generateChocolateCandy();
}

const QImage& SpriteAtlas::getSprite(SpriteType type) const
{
    static QImage fallback(SPRITE_SIZE, SPRITE_SIZE, QImage::Format_ARGB32);
    auto it = sprites.find(type);
    if (it != sprites.end()) return it->second;
    return fallback;
}

bool SpriteAtlas::isPickup(SpriteType type) const
{
    return type == SpriteType::QUESTION_BOX || type == SpriteType::MUSHROOM ||
           type == SpriteType::ROCKET || type == SpriteType::BANANA ||
           type == SpriteType::GREEN_SHELL || type == SpriteType::INK_BLOOPER ||
           type == SpriteType::COIN;
}

bool SpriteAtlas::isObstacle(SpriteType type) const
{
    return type == SpriteType::GREEN_PIPE || type == SpriteType::THWOMP ||
           type == SpriteType::OIL_SLICK || type == SpriteType::BANANA_DROPPED ||
           type == SpriteType::LAVA_GEYSER_ERUPTING ||
           type == SpriteType::CHOCOLATE_FOUNTAIN ||
           type == SpriteType::CHOCOLATE_CANDY;
}

void SpriteAtlas::setPixel(QImage& img, int x, int y, QRgb color)
{
    if (x >= 0 && x < img.width() && y >= 0 && y < img.height())
        img.setPixel(x, y, color);
}

void SpriteAtlas::fillCircle(QImage& img, int cx, int cy, int r, QRgb fill, QRgb outline)
{
    for (int y = cy - r; y <= cy + r; ++y) {
        for (int x = cx - r; x <= cx + r; ++x) {
            int d2 = (x - cx) * (x - cx) + (y - cy) * (y - cy);
            if (d2 <= r * r) {
                if (d2 >= (r - 1) * (r - 1))
                    setPixel(img, x, y, outline);
                else
                    setPixel(img, x, y, fill);
            }
        }
    }
}

void SpriteAtlas::fillRect(QImage& img, int x, int y, int w, int h, QRgb color)
{
    for (int py = y; py < y + h; ++py)
        for (int px = x; px < x + w; ++px)
            setPixel(img, px, py, color);
}

// ========================= POWERUP SPRITES =========================

void SpriteAtlas::generateQuestionBox()
{
    QImage img(SPRITE_SIZE, SPRITE_SIZE, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    QRgb gold     = qRgba(255, 200, 0, 255);
    QRgb darkGold = qRgba(200, 150, 0, 255);
    QRgb outline  = qRgba(80, 50, 0, 255);
    QRgb white    = qRgba(255, 255, 240, 255);
    QRgb shadow   = qRgba(160, 100, 0, 255);

    // Rounded box body
    for (int y = 4; y < 28; ++y) {
        for (int x = 4; x < 28; ++x) {
            bool isCorner = (x < 6 && y < 6) || (x > 25 && y < 6) ||
                            (x < 6 && y > 25) || (x > 25 && y > 25);
            if (isCorner) continue;

            bool isBorder = (x == 4 || x == 27 || y == 4 || y == 27);
            if (isBorder) setPixel(img, x, y, outline);
            else if (y < 8) setPixel(img, x, y, gold);      // top highlight
            else if (y > 24) setPixel(img, x, y, shadow);   // bottom shadow
            else setPixel(img, x, y, darkGold);
        }
    }

    // "?" character in center (pixel font)
    int qx = 12, qy = 8;
    // Top arc of ?
    fillRect(img, qx+2, qy, 6, 2, white);
    fillRect(img, qx, qy+2, 3, 2, white);
    fillRect(img, qx+7, qy+2, 3, 2, white);
    fillRect(img, qx+6, qy+4, 3, 2, white);
    fillRect(img, qx+4, qy+6, 3, 2, white);
    fillRect(img, qx+4, qy+8, 3, 2, white);
    // Dot
    fillRect(img, qx+4, qy+11, 3, 2, white);

    // Corner rivets (small bright dots)
    fillCircle(img, 8, 8, 1, white, white);
    fillCircle(img, 23, 8, 1, white, white);
    fillCircle(img, 8, 23, 1, white, white);
    fillCircle(img, 23, 23, 1, white, white);

    sprites[SpriteType::QUESTION_BOX] = img;
}

void SpriteAtlas::generateMushroom()
{
    QImage img(SPRITE_SIZE, SPRITE_SIZE, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    QRgb red      = qRgba(220, 30, 30, 255);
    QRgb darkRed  = qRgba(170, 20, 20, 255);
    QRgb white    = qRgba(255, 255, 245, 255);
    QRgb cream    = qRgba(255, 230, 200, 255);
    QRgb outline  = qRgba(60, 15, 15, 255);
    QRgb eyes     = qRgba(20, 20, 20, 255);

    // Cap (half dome)
    for (int y = 4; y < 18; ++y) {
        for (int x = 4; x < 28; ++x) {
            int dx = x - 16, dy = y - 17;
            if (dx * dx + dy * dy * 2 < 13 * 13) {
                bool edge = (dx * dx + dy * dy * 2 >= 12 * 12);
                setPixel(img, x, y, edge ? outline : (y >= 15 ? darkRed : red));
            }
        }
    }
    // White dots on cap
    fillCircle(img, 11, 10, 2, white, white);
    fillCircle(img, 21, 10, 2, white, white);
    fillCircle(img, 16, 7, 2, white, white);

    // Stem (beige rectangle)
    fillRect(img, 11, 17, 10, 9, cream);
    fillRect(img, 11, 17, 1, 9, outline);
    fillRect(img, 20, 17, 1, 9, outline);
    fillRect(img, 11, 25, 10, 1, outline);

    // Eyes on stem
    fillRect(img, 13, 19, 2, 3, eyes);
    fillRect(img, 18, 19, 2, 3, eyes);

    sprites[SpriteType::MUSHROOM] = img;
}

void SpriteAtlas::generateRocket()
{
    QImage img(SPRITE_SIZE, SPRITE_SIZE, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    QRgb black   = qRgba(25, 25, 35, 255);
    QRgb dark    = qRgba(50, 50, 60, 255);
    QRgb white   = qRgba(255, 255, 255, 255);
    QRgb red     = qRgba(220, 40, 40, 255);
    QRgb orange  = qRgba(255, 160, 30, 255);
    QRgb yellow  = qRgba(255, 240, 60, 255);

    // Bullet body (elongated capsule)
    for (int y = 6; y < 26; ++y) {
        for (int x = 6; x < 26; ++x) {
            int dx = x - 16;
            int r = (y < 12) ? (y - 4) : 10;
            if (std::abs(dx) <= r) {
                if (y < 10)
                    setPixel(img, x, y, dark);
                else
                    setPixel(img, x, y, black);
            }
        }
    }
    // White eyes
    fillCircle(img, 12, 14, 2, white, white);
    fillCircle(img, 20, 14, 2, white, white);
    setPixel(img, 12, 14, black);
    setPixel(img, 20, 14, black);

    // Red arm bands
    fillRect(img, 6, 20, 4, 3, red);
    fillRect(img, 22, 20, 4, 3, red);

    // Exhaust flame at bottom
    fillRect(img, 10, 25, 12, 2, orange);
    fillRect(img, 12, 27, 8, 2, yellow);
    fillRect(img, 14, 29, 4, 2, orange);

    sprites[SpriteType::ROCKET] = img;
}

void SpriteAtlas::generateBanana()
{
    QImage img(SPRITE_SIZE, SPRITE_SIZE, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    QRgb yellow  = qRgba(255, 220, 30, 255);
    QRgb darkY   = qRgba(200, 170, 10, 255);
    QRgb outline = qRgba(120, 90, 0, 255);
    QRgb brown   = qRgba(100, 60, 20, 255);

    // Crescent shape (banana curve)
    for (int y = 4; y < 28; ++y) {
        for (int x = 6; x < 26; ++x) {
            int dx = x - 16, dy = y - 16;
            double dist = std::sqrt(dx * dx + dy * dy);
            double angle = std::atan2(dy, dx);
            // Crescent: outer circle minus inner circle offset
            if (dist < 11 && dist > 5 && angle > -2.5 && angle < 1.0) {
                bool edge = (dist > 9.5 || dist < 6);
                setPixel(img, x, y, edge ? outline : yellow);
            }
        }
    }
    // Brown tip
    setPixel(img, 22, 9, brown);
    setPixel(img, 23, 10, brown);
    // Darker spots
    setPixel(img, 14, 10, darkY);
    setPixel(img, 18, 13, darkY);

    sprites[SpriteType::BANANA] = img;
}

void SpriteAtlas::generateGreenShell()
{
    QImage img(SPRITE_SIZE, SPRITE_SIZE, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    QRgb green   = qRgba(30, 180, 30, 255);
    QRgb dark    = qRgba(15, 110, 15, 255);
    QRgb light   = qRgba(80, 230, 80, 255);
    QRgb outline = qRgba(10, 60, 10, 255);
    QRgb white   = qRgba(255, 255, 240, 255);

    // Shell dome
    fillCircle(img, 16, 14, 11, green, outline);
    // Highlight arc
    fillCircle(img, 14, 11, 4, light, light);

    // Hexagonal pattern on shell
    fillCircle(img, 10, 12, 2, dark, dark);
    fillCircle(img, 22, 12, 2, dark, dark);
    fillCircle(img, 16, 8, 2, dark, dark);
    fillCircle(img, 13, 18, 2, dark, dark);
    fillCircle(img, 19, 18, 2, dark, dark);

    // White underbelly
    fillRect(img, 8, 22, 16, 4, white);
    fillRect(img, 8, 22, 16, 1, outline);

    sprites[SpriteType::GREEN_SHELL] = img;
}

void SpriteAtlas::generateInkBlooper()
{
    QImage img(SPRITE_SIZE, SPRITE_SIZE, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    QRgb white   = qRgba(245, 245, 255, 255);
    QRgb cream   = qRgba(220, 220, 235, 255);
    QRgb outline = qRgba(60, 60, 80, 255);
    QRgb eyes    = qRgba(20, 20, 30, 255);
    QRgb pupil   = qRgba(0, 180, 255, 255);

    // Squid body (teardrop shape)
    for (int y = 3; y < 22; ++y) {
        int halfW = (y < 10) ? (y - 2) : 9;
        for (int x = 16 - halfW; x <= 16 + halfW; ++x) {
            bool edge = (x == 16 - halfW || x == 16 + halfW || y == 3);
            setPixel(img, x, y, edge ? outline : (y < 8 ? cream : white));
        }
    }

    // Eyes
    fillCircle(img, 12, 12, 3, white, outline);
    fillCircle(img, 20, 12, 3, white, outline);
    fillCircle(img, 13, 13, 1, pupil, eyes);
    fillCircle(img, 21, 13, 1, pupil, eyes);

    // Tentacles
    for (int t = 0; t < 4; ++t) {
        int tx = 9 + t * 5;
        for (int y = 22; y < 29; ++y) {
            int wobble = static_cast<int>(std::sin((y + t) * 0.8) * 1.5);
            setPixel(img, tx + wobble, y, white);
            setPixel(img, tx + wobble + 1, y, outline);
        }
    }

    sprites[SpriteType::INK_BLOOPER] = img;
}

// ========================= OBSTACLE SPRITES =========================

void SpriteAtlas::generateGreenPipe()
{
    QImage img(SPRITE_SIZE, SPRITE_SIZE, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    QRgb green    = qRgba(20, 140, 30, 255);
    QRgb darkG    = qRgba(10, 90, 15, 255);
    QRgb lightG   = qRgba(60, 200, 70, 255);
    QRgb outline  = qRgba(5, 50, 5, 255);
    QRgb shadow   = qRgba(0, 0, 0, 60);

    // Pipe lip (wider top)
    fillRect(img, 4, 6, 24, 6, green);
    fillRect(img, 4, 6, 24, 1, outline);
    fillRect(img, 4, 11, 24, 1, outline);
    fillRect(img, 4, 6, 1, 6, outline);
    fillRect(img, 27, 6, 1, 6, outline);
    // Highlight stripe on lip
    fillRect(img, 6, 7, 3, 4, lightG);

    // Pipe body (narrower)
    fillRect(img, 7, 12, 18, 16, green);
    fillRect(img, 7, 12, 1, 16, outline);
    fillRect(img, 24, 12, 1, 16, outline);
    fillRect(img, 7, 27, 18, 1, outline);
    // Highlight stripe on body
    fillRect(img, 9, 12, 2, 16, lightG);
    // Dark right edge
    fillRect(img, 22, 12, 2, 16, darkG);

    // Ground shadow
    for (int x = 5; x < 27; ++x) {
        setPixel(img, x, 28, shadow);
        setPixel(img, x, 29, shadow);
    }

    sprites[SpriteType::GREEN_PIPE] = img;
}

void SpriteAtlas::generateThwomp()
{
    QImage img(SPRITE_SIZE, SPRITE_SIZE, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    QRgb stone   = qRgba(140, 140, 155, 255);
    QRgb dark    = qRgba(90, 90, 100, 255);
    QRgb light   = qRgba(180, 180, 195, 255);
    QRgb outline = qRgba(40, 40, 50, 255);
    QRgb eyes    = qRgba(255, 255, 255, 255);
    QRgb pupil   = qRgba(200, 30, 30, 255);

    // Rough stone block
    fillRect(img, 3, 3, 26, 26, stone);
    fillRect(img, 3, 3, 26, 1, outline);
    fillRect(img, 3, 28, 26, 1, outline);
    fillRect(img, 3, 3, 1, 26, outline);
    fillRect(img, 28, 3, 1, 26, outline);

    // Jagged edges (spikes)
    setPixel(img, 2, 8, outline); setPixel(img, 1, 9, outline);
    setPixel(img, 29, 12, outline); setPixel(img, 30, 13, outline);
    setPixel(img, 2, 20, outline); setPixel(img, 1, 21, outline);
    setPixel(img, 29, 22, outline); setPixel(img, 30, 23, outline);

    // Cracks and texture
    fillRect(img, 8, 6, 6, 1, dark);
    fillRect(img, 18, 8, 5, 1, dark);
    fillRect(img, 6, 22, 8, 1, dark);
    fillRect(img, 20, 24, 5, 1, dark);
    fillRect(img, 5, 5, 3, 3, light);

    // Angry eyes
    fillRect(img, 8, 11, 6, 5, eyes);
    fillRect(img, 18, 11, 6, 5, eyes);
    fillRect(img, 10, 12, 3, 3, pupil);
    fillRect(img, 20, 12, 3, 3, pupil);

    // Angry mouth
    fillRect(img, 10, 20, 12, 2, outline);
    // Teeth
    fillRect(img, 12, 19, 2, 1, eyes);
    fillRect(img, 16, 19, 2, 1, eyes);
    fillRect(img, 20, 19, 2, 1, eyes);

    sprites[SpriteType::THWOMP] = img;
}

void SpriteAtlas::generateOilSlick()
{
    QImage img(SPRITE_SIZE, SPRITE_SIZE, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    QRgb black   = qRgba(20, 18, 25, 200);
    QRgb sheen   = qRgba(80, 60, 120, 180);
    QRgb rainbow = qRgba(100, 80, 160, 150);

    // Flat irregular splat shape
    for (int y = 8; y < 26; ++y) {
        for (int x = 4; x < 28; ++x) {
            int dx = x - 16, dy = y - 17;
            double dist = std::sqrt(dx * dx * 0.8 + dy * dy * 1.2);
            if (dist < 9 + std::sin(dx * 0.7) * 2) {
                if (dist < 5) setPixel(img, x, y, sheen);
                else if ((x + y) % 5 == 0) setPixel(img, x, y, rainbow);
                else setPixel(img, x, y, black);
            }
        }
    }

    sprites[SpriteType::OIL_SLICK] = img;
}

void SpriteAtlas::generateCoin()
{
    QImage img(SPRITE_SIZE, SPRITE_SIZE, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    QRgb gold    = qRgba(255, 210, 40, 255);
    QRgb light   = qRgba(255, 240, 120, 255);
    QRgb dark    = qRgba(180, 140, 10, 255);
    QRgb outline = qRgba(120, 80, 0, 255);

    fillCircle(img, 16, 16, 10, gold, outline);
    fillCircle(img, 16, 16, 7, dark, dark);
    fillCircle(img, 16, 16, 6, gold, gold);

    // Star emblem in center
    for (int i = 0; i < 5; ++i) {
        double a = (i * 72 - 90) * M_PI / 180.0;
        int sx = 16 + static_cast<int>(4 * std::cos(a));
        int sy = 16 + static_cast<int>(4 * std::sin(a));
        setPixel(img, sx, sy, light);
        setPixel(img, sx + 1, sy, light);
        setPixel(img, sx, sy + 1, light);
    }

    // Shine highlight
    setPixel(img, 12, 11, light);
    setPixel(img, 13, 10, light);
    setPixel(img, 11, 12, light);

    sprites[SpriteType::COIN] = img;
}

void SpriteAtlas::generateBananaDropped()
{
    // Reuse banana sprite but slightly different tint (more brownish)
    QImage img(SPRITE_SIZE, SPRITE_SIZE, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    QRgb yellow  = qRgba(240, 210, 20, 255);
    QRgb darkY   = qRgba(190, 160, 5, 255);
    QRgb outline = qRgba(100, 70, 0, 255);
    QRgb brown   = qRgba(80, 50, 15, 255);

    for (int y = 4; y < 28; ++y) {
        for (int x = 6; x < 26; ++x) {
            int dx = x - 16, dy = y - 16;
            double dist = std::sqrt(dx * dx + dy * dy);
            double angle = std::atan2(dy, dx);
            if (dist < 11 && dist > 5 && angle > -2.5 && angle < 1.0) {
                bool edge = (dist > 9.5 || dist < 6);
                setPixel(img, x, y, edge ? outline : yellow);
            }
        }
    }
    setPixel(img, 22, 9, brown);
    setPixel(img, 23, 10, brown);
    setPixel(img, 14, 10, darkY);
    setPixel(img, 18, 13, darkY);

    sprites[SpriteType::BANANA_DROPPED] = img;
}

void SpriteAtlas::generateShellProjectile()
{
    // Same as green shell but with a speed trail indicator
    QImage img = sprites.count(SpriteType::GREEN_SHELL)
        ? sprites.at(SpriteType::GREEN_SHELL).copy()
        : QImage(SPRITE_SIZE, SPRITE_SIZE, QImage::Format_ARGB32);

    // Add motion lines behind it
    QRgb trail = qRgba(200, 255, 200, 150);
    fillRect(img, 2, 14, 4, 1, trail);
    fillRect(img, 1, 16, 5, 1, trail);
    fillRect(img, 3, 18, 3, 1, trail);

    sprites[SpriteType::SHELL_PROJECTILE] = img;
}

void SpriteAtlas::generateKartMario()
{
    QImage img(SPRITE_SIZE, SPRITE_SIZE, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    QRgb red       = qRgba(220, 25, 25, 255);
    QRgb darkRed   = qRgba(150, 15, 15, 255);
    QRgb blue      = qRgba(30, 80, 200, 255);
    QRgb white     = qRgba(250, 250, 250, 255);
    QRgb skin      = qRgba(255, 200, 150, 255);
    QRgb brown     = qRgba(90, 50, 20, 255);
    QRgb tire      = qRgba(25, 25, 28, 255);
    QRgb rim       = qRgba(165, 170, 180, 255);
    QRgb chrome    = qRgba(210, 215, 225, 255);
    QRgb shadow    = qRgba(15, 15, 15, 160);

    // Ground Shadow
    fillCircle(img, 16, 29, 8, shadow, shadow);

    // Tires (Left & Right)
    fillRect(img, 3, 19, 5, 9, tire);
    fillRect(img, 24, 19, 5, 9, tire);
    fillRect(img, 4, 22, 2, 4, rim);
    fillRect(img, 25, 22, 2, 4, rim);

    // Kart Chassis
    fillRect(img, 8, 20, 16, 8, red);
    fillRect(img, 7, 23, 18, 5, darkRed);
    fillRect(img, 15, 20, 2, 8, white); // Center racing stripe

    // Twin Exhausts
    fillRect(img, 9, 25, 3, 4, chrome);
    fillRect(img, 20, 25, 3, 4, chrome);

    // Overalls & Shoulders
    fillRect(img, 11, 14, 10, 6, blue);
    fillRect(img, 13, 15, 6, 5, red); // Shirt back

    // Head, Hair & Red Cap
    fillRect(img, 12, 10, 8, 4, skin);
    fillRect(img, 11, 11, 2, 3, brown); // Hair
    fillRect(img, 19, 11, 2, 3, brown);
    fillCircle(img, 16, 7, 4, red, darkRed); // Cap dome
    fillRect(img, 12, 8, 8, 2, red);         // Visor
    fillCircle(img, 16, 6, 2, white, white); // White 'M' badge
    setPixel(img, 16, 6, red);               // Red M pixel

    sprites[SpriteType::KART_MARIO] = img;
}

void SpriteAtlas::generateKartBowser()
{
    QImage img(SPRITE_SIZE, SPRITE_SIZE, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    QRgb darkGreen = qRgba(30, 105, 40, 255);
    QRgb deepGreen = qRgba(20, 75, 25, 255);
    QRgb orange    = qRgba(240, 130, 15, 255);
    QRgb hornBone  = qRgba(245, 235, 180, 255);
    QRgb rimWhite  = qRgba(245, 245, 235, 255);
    QRgb brassGold = qRgba(220, 175, 30, 255);
    QRgb tire      = qRgba(20, 20, 22, 255);
    QRgb chrome    = qRgba(180, 185, 195, 255);
    QRgb shadow    = qRgba(15, 15, 15, 160);

    // Ground Shadow
    fillCircle(img, 16, 29, 9, shadow, shadow);

    // Massive Heavy Tires
    fillRect(img, 2, 18, 6, 11, tire);
    fillRect(img, 24, 18, 6, 11, tire);
    fillRect(img, 3, 22, 2, 4, brassGold); // Studded brass hub
    fillRect(img, 25, 22, 2, 4, brassGold);

    // Spiked Kart Chassis
    fillRect(img, 8, 20, 16, 9, darkGreen);
    fillRect(img, 7, 24, 18, 5, deepGreen);
    fillRect(img, 15, 20, 2, 9, orange); // Fiery orange stripe

    // Oversized Chrome Exhausts
    fillRect(img, 9, 26, 3, 5, chrome);
    fillRect(img, 20, 26, 3, 5, chrome);

    // Spiked Shell Back
    fillCircle(img, 16, 14, 6, darkGreen, rimWhite);
    setPixel(img, 13, 13, rimWhite); // Shell bone spikes
    setPixel(img, 19, 13, rimWhite);
    setPixel(img, 16, 11, rimWhite);

    // Bowser Horns & Fiery Red-Orange Hair
    fillRect(img, 13, 4, 6, 4, orange); // Fiery hair crest
    fillRect(img, 14, 3, 4, 2, qRgba(255, 60, 20, 255));
    // Curved Horns
    fillRect(img, 9, 5, 3, 2, hornBone);
    setPixel(img, 8, 4, hornBone);
    fillRect(img, 20, 5, 3, 2, hornBone);
    setPixel(img, 23, 4, hornBone);

    sprites[SpriteType::KART_BOWSER] = img;
}

void SpriteAtlas::generateKartPeach()
{
    QImage img(SPRITE_SIZE, SPRITE_SIZE, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    QRgb pink      = qRgba(245, 130, 175, 255);
    QRgb darkPink  = qRgba(200, 80, 130, 255);
    QRgb gold      = qRgba(255, 215, 0, 255);
    QRgb blonde    = qRgba(255, 220, 60, 255);
    QRgb darkBlond = qRgba(215, 180, 30, 255);
    QRgb white     = qRgba(255, 255, 255, 255);
    QRgb gemBlue   = qRgba(40, 170, 255, 255);
    QRgb tire      = qRgba(30, 30, 35, 255);
    QRgb shadow    = qRgba(15, 15, 15, 160);

    // Ground Shadow
    fillCircle(img, 16, 29, 8, shadow, shadow);

    // Slim Agility Tires
    fillRect(img, 4, 20, 4, 9, tire);
    fillRect(img, 24, 20, 4, 9, tire);
    fillRect(img, 5, 23, 2, 3, gold); // Gold wheel nuts
    fillRect(img, 25, 23, 2, 3, gold);

    // Sleek Pink Chassis
    fillRect(img, 8, 20, 16, 8, pink);
    fillRect(img, 7, 23, 18, 5, darkPink);
    fillRect(img, 15, 20, 2, 8, white); // White racing stripe

    // Flowing Blonde Hair & Dress
    fillRect(img, 10, 14, 12, 6, pink); // Pink dress shoulders
    fillRect(img, 9, 10, 14, 7, blonde);
    fillRect(img, 10, 12, 12, 4, darkBlond);

    // Royal Golden Crown
    fillRect(img, 13, 5, 6, 3, gold);
    setPixel(img, 12, 4, gold); // Crown tips
    setPixel(img, 15, 3, gold);
    setPixel(img, 18, 4, gold);
    setPixel(img, 15, 5, gemBlue); // Center sapphire jewel

    sprites[SpriteType::KART_PEACH] = img;
}

void SpriteAtlas::generateKartToad()
{
    QImage img(SPRITE_SIZE, SPRITE_SIZE, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    QRgb blue      = qRgba(25, 95, 220, 255);
    QRgb darkBlue  = qRgba(15, 65, 160, 255);
    QRgb white     = qRgba(255, 255, 255, 255);
    QRgb redSpot   = qRgba(225, 30, 30, 255);
    QRgb skin      = qRgba(255, 210, 170, 255);
    QRgb goldVest  = qRgba(255, 200, 30, 255);
    QRgb tire      = qRgba(28, 28, 30, 255);
    QRgb chrome    = qRgba(215, 220, 230, 255);
    QRgb shadow    = qRgba(15, 15, 15, 160);

    // Ground Shadow
    fillCircle(img, 16, 29, 8, shadow, shadow);

    // Lightweight Tires
    fillRect(img, 4, 20, 4, 9, tire);
    fillRect(img, 24, 20, 4, 9, tire);
    fillRect(img, 5, 23, 2, 3, chrome);
    fillRect(img, 25, 23, 2, 3, chrome);

    // Compact Blue Chassis
    fillRect(img, 8, 20, 16, 8, blue);
    fillRect(img, 7, 23, 18, 5, darkBlue);
    fillRect(img, 15, 20, 2, 8, white); // White stripe

    // Blue Vest with Gold Trim
    fillRect(img, 11, 14, 10, 5, blue);
    fillRect(img, 12, 14, 1, 5, goldVest);
    fillRect(img, 18, 14, 1, 5, goldVest);
    fillRect(img, 13, 11, 6, 4, skin); // Neck/head

    // Mushroom Cap (White dome with Red Spots)
    fillCircle(img, 16, 7, 6, white, qRgba(220, 220, 220, 255));
    // Red Spots
    fillCircle(img, 16, 5, 2, redSpot, redSpot); // Top spot
    fillRect(img, 11, 7, 2, 3, redSpot);         // Left spot
    fillRect(img, 19, 7, 2, 3, redSpot);         // Right spot

    sprites[SpriteType::KART_TOAD] = img;
}

void SpriteAtlas::generateKartAlts()
{
    // Fire Mario ALT
    if (sprites.count(SpriteType::KART_MARIO)) {
        QImage alt = sprites.at(SpriteType::KART_MARIO).copy();
        for (int y = 0; y < SPRITE_SIZE; ++y) {
            for (int x = 0; x < SPRITE_SIZE; ++x) {
                QRgb p = alt.pixel(x, y);
                // Swap red body for crisp white
                if (qRed(p) > 180 && qGreen(p) < 60 && qBlue(p) < 60 && y >= 20) {
                    alt.setPixel(x, y, qRgba(240, 240, 255, qAlpha(p)));
                }
            }
        }
        sprites[SpriteType::KART_MARIO_ALT] = alt;
    }

    // Dry Bowser ALT (Charcoal gray)
    if (sprites.count(SpriteType::KART_BOWSER)) {
        QImage alt = sprites.at(SpriteType::KART_BOWSER).copy();
        for (int y = 0; y < SPRITE_SIZE; ++y) {
            for (int x = 0; x < SPRITE_SIZE; ++x) {
                QRgb p = alt.pixel(x, y);
                if (qGreen(p) > 80 && qRed(p) < 50) {
                    alt.setPixel(x, y, qRgba(65, 65, 70, qAlpha(p)));
                }
            }
        }
        sprites[SpriteType::KART_BOWSER_ALT] = alt;
    }

    // Daisy ALT (Orange)
    if (sprites.count(SpriteType::KART_PEACH)) {
        QImage alt = sprites.at(SpriteType::KART_PEACH).copy();
        for (int y = 0; y < SPRITE_SIZE; ++y) {
            for (int x = 0; x < SPRITE_SIZE; ++x) {
                QRgb p = alt.pixel(x, y);
                if (qRed(p) > 200 && qBlue(p) > 140) {
                    alt.setPixel(x, y, qRgba(255, 160, 30, qAlpha(p)));
                }
            }
        }
        sprites[SpriteType::KART_PEACH_ALT] = alt;
    }

    // Yellow Toad ALT
    if (sprites.count(SpriteType::KART_TOAD)) {
        QImage alt = sprites.at(SpriteType::KART_TOAD).copy();
        for (int y = 0; y < SPRITE_SIZE; ++y) {
            for (int x = 0; x < SPRITE_SIZE; ++x) {
                QRgb p = alt.pixel(x, y);
                if (qBlue(p) > 180 && qRed(p) < 50) {
                    alt.setPixel(x, y, qRgba(245, 210, 30, qAlpha(p)));
                }
            }
        }
        sprites[SpriteType::KART_TOAD_ALT] = alt;
    }
}

// -------------------------------------------------------------
// TRACK HAZARDS GENERATION (Milestone 3 F10, F11)
// -------------------------------------------------------------

void SpriteAtlas::generateLavaGeyserBubbling()
{
    QImage img(SPRITE_SIZE, SPRITE_SIZE, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    // 1. Base caldera magma pool (y: 20 to 30)
    for (int y = 20; y < 31; ++y) {
        int halfW = static_cast<int>(14.0 * std::sin((y - 19) / 12.0 * M_PI));
        for (int x = 16 - halfW; x <= 16 + halfW; ++x) {
            int d = std::abs(x - 16);
            if (d > halfW - 2)
                setPixel(img, x, y, qRgba(90, 15, 10, 240));    // Dark crust outline
            else if (d > halfW - 5)
                setPixel(img, x, y, qRgba(190, 45, 10, 255));   // Crimson magma
            else
                setPixel(img, x, y, qRgba(255, 130, 20, 255));  // Molten orange
        }
    }

    // 2. Glowing bubbling magma domes
    fillCircle(img, 12, 23, 3, qRgba(255, 210, 30, 255), qRgba(220, 70, 10, 255));
    fillCircle(img, 19, 22, 4, qRgba(255, 235, 80, 255), qRgba(240, 90, 15, 255));
    fillCircle(img, 15, 25, 2, qRgba(255, 255, 160, 255), qRgba(255, 140, 20, 255));

    // 3. Smoke / heat ripples rising
    setPixel(img, 14, 17, qRgba(160, 60, 20, 180));
    setPixel(img, 15, 16, qRgba(180, 70, 25, 190));
    setPixel(img, 18, 15, qRgba(150, 50, 15, 170));
    setPixel(img, 17, 13, qRgba(120, 40, 15, 140));

    sprites[SpriteType::LAVA_GEYSER_BUBBLING] = img;
}

void SpriteAtlas::generateLavaGeyserErupting()
{
    QImage img(SPRITE_SIZE, SPRITE_SIZE, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    // 1. Magma pool base
    for (int y = 24; y < 32; ++y) {
        int hw = 12 - (y - 24);
        for (int x = 16 - hw; x <= 16 + hw; ++x) {
            setPixel(img, x, y, qRgba(180, 30, 10, 250));
        }
    }

    // 2. Towering flame and plasma pillar (y: 4 to 28)
    for (int y = 4; y < 28; ++y) {
        double flare = 1.0 + 0.3 * std::sin(y * 0.8);
        int halfW = static_cast<int>((5.0 + (28 - y) * 0.15) * flare);
        for (int x = 16 - halfW; x <= 16 + halfW; ++x) {
            int dist = std::abs(x - 16);
            if (dist == halfW) {
                setPixel(img, x, y, qRgba(140, 20, 10, 220));   // Dark crimson edge
            } else if (dist >= halfW - 2) {
                setPixel(img, x, y, qRgba(240, 60, 10, 255));   // Fiery red-orange
            } else if (dist >= halfW - 3) {
                setPixel(img, x, y, qRgba(255, 170, 20, 255));  // Bright orange
            } else {
                setPixel(img, x, y, qRgba(255, 255, 180, 255)); // Incandescent white-yellow core
            }
        }
    }

    // 3. Billowing smoke & fiery sparks at crest
    fillCircle(img, 12, 5, 3, qRgba(100, 25, 15, 210), qRgba(60, 15, 10, 180));
    fillCircle(img, 20, 4, 3, qRgba(110, 30, 15, 210), qRgba(60, 15, 10, 180));
    fillCircle(img, 16, 2, 2, qRgba(255, 200, 50, 255), qRgba(200, 50, 10, 255)); // Crown spark

    // Flying cinders
    setPixel(img, 6, 8, qRgba(255, 180, 30, 230));
    setPixel(img, 26, 7, qRgba(255, 160, 20, 230));
    setPixel(img, 8, 14, qRgba(255, 100, 10, 210));
    setPixel(img, 24, 15, qRgba(255, 120, 15, 210));

    sprites[SpriteType::LAVA_GEYSER_ERUPTING] = img;
}

void SpriteAtlas::generateChocolateFountain()
{
    QImage img(SPRITE_SIZE, SPRITE_SIZE, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    // 1. Lower chocolate basin (y: 22 to 30)
    for (int y = 22; y < 31; ++y) {
        int hw = static_cast<int>(13.0 * std::sin((y - 21) / 10.0 * M_PI));
        for (int x = 16 - hw; x <= 16 + hw; ++x) {
            int d = std::abs(x - 16);
            if (d == hw)
                setPixel(img, x, y, qRgba(50, 25, 10, 255));    // Dark chocolate crust
            else if (d > hw - 3)
                setPixel(img, x, y, qRgba(85, 45, 20, 255));    // Milk chocolate
            else
                setPixel(img, x, y, qRgba(115, 65, 30, 255));   // Rich brown
        }
    }

    // 2. Middle tier (y: 14 to 21)
    for (int y = 14; y < 22; ++y) {
        int hw = static_cast<int>(8.0 * std::sin((y - 13) / 9.0 * M_PI));
        for (int x = 16 - hw; x <= 16 + hw; ++x) {
            setPixel(img, x, y, qRgba(105, 55, 25, 255));
        }
    }

    // 3. Flowing dome / top plume (y: 6 to 14)
    fillCircle(img, 16, 11, 5, qRgba(135, 75, 35, 255), qRgba(60, 30, 15, 255));

    // 4. Creamy highlights & cascading syrup ribbons
    setPixel(img, 15, 8, qRgba(210, 160, 110, 240));  // Specular sheen
    setPixel(img, 16, 8, qRgba(220, 175, 125, 240));
    setPixel(img, 12, 16, qRgba(175, 115, 65, 230)); // Waterfall sheen
    setPixel(img, 20, 16, qRgba(175, 115, 65, 230));

    // Mud droplets flying
    setPixel(img, 8, 12, qRgba(90, 45, 20, 220));
    setPixel(img, 24, 11, qRgba(90, 45, 20, 220));

    sprites[SpriteType::CHOCOLATE_FOUNTAIN] = img;
}

void SpriteAtlas::generateChocolateCandy()
{
    QImage img(SPRITE_SIZE, SPRITE_SIZE, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    int cx = 16, cy = 16, r = 13;

    // 1. Shaded 3D spherical chocolate sphere
    for (int y = cy - r; y <= cy + r; ++y) {
        for (int x = cx - r; x <= cx + r; ++x) {
            double dx = x - cx;
            double dy = y - cy;
            double d2 = dx * dx + dy * dy;
            if (d2 <= r * r) {
                if (d2 >= (r - 1) * (r - 1)) {
                    setPixel(img, x, y, qRgba(40, 20, 10, 255)); // Dark outline
                } else {
                    // Spherical lighting from top-left (lx = -0.4, ly = -0.4)
                    double nx = dx / r;
                    double ny = dy / r;
                    double diff = std::clamp(-0.5 * nx - 0.5 * ny + 0.4, 0.0, 1.0);
                    int red   = static_cast<int>(60  + diff * 90);
                    int green = static_cast<int>(30  + diff * 50);
                    int blue  = static_cast<int>(12  + diff * 25);
                    setPixel(img, x, y, qRgba(red, green, blue, 255));
                }
            }
        }
    }

    // 2. White vanilla cream swirl stripes across bonbon
    for (int t = -10; t <= 10; ++t) {
        int sx = cx + t;
        int sy = cy + static_cast<int>(std::sin(t * 0.3) * 5.0);
        if ((sx - cx)*(sx - cx) + (sy - cy)*(sy - cy) < (r - 2)*(r - 2)) {
            setPixel(img, sx, sy, qRgba(250, 240, 225, 255));
            setPixel(img, sx, sy + 1, qRgba(230, 215, 195, 240));
        }
    }

    // 3. Glossy specular highlight
    fillCircle(img, cx - 4, cy - 5, 2, qRgba(255, 255, 240, 240), qRgba(200, 180, 160, 200));

    sprites[SpriteType::CHOCOLATE_CANDY] = img;
}

