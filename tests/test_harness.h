#ifndef TEST_HARNESS_H
#define TEST_HARNESS_H

#include "test_types.h"
#include <vector>
#include <set>
#include <cmath>

class MenuStateController {
public:
    MenuStateController();

    GameState currentState() const { return state; }
    GameMode currentMode() const { return mode; }
    int menuIndex() const { return mIndex; }
    int p1CharIndex() const { return p1Char; }
    int p2CharIndex() const { return p2Char; }
    bool isP2AlternateColor() const { return p2AltColor; }

    void navigateMenu(int dir);
    void confirmMenu();
    void navigateCharSelect(int playerNum, int dir);
    void confirmCharSelect();
    void backAction();
    void finishRace();
    void resetToTitle();

private:
    GameState state = GameState::TITLE_MENU;
    GameMode mode = GameMode::PLAY_VS_AI;
    int mIndex = 0; // 0: Play vs AI, 1: Multiplayer, 2: Practice
    int p1Char = 0;
    int p2Char = 1;
    bool p2AltColor = false;
};

class CharacterRoster {
public:
    static CharacterRoster& instance();

    const std::vector<CharacterDef>& allCharacters() const { return roster; }
    CharacterDef getCharacter(int id) const;
    QColor getP2AlternateColor(int id) const;

private:
    CharacterRoster();
    std::vector<CharacterDef> roster;
};

class TrackWaypointManager {
public:
    static std::vector<QPointF> generateWaypoints(TrackLevel level);
    static double trackLength(const std::vector<QPointF>& waypoints);
    static double distanceAlongTrack(const std::vector<QPointF>& waypoints, int waypointIndex, double x, double z);
};

class KartPhysicsEngine {
public:
    static void updateKart(KartEntity &kart, bool throttle, bool brake, int steerDir, bool driftKey, double dt);
    static void applyHazardHit(KartEntity &kart, double pushDirX, double pushDirZ, double penaltySpeedMult = 0.25);
    static void applyItemSpinout(KartEntity &kart);
    static void applyInkedDebuff(KartEntity &kart);
};

class AISteeringEngine {
public:
    static void updateAI(KartEntity &ai, const std::vector<QPointF> &waypoints, double leadPlayerDist, double dt);
};

class ItemTargetingEngine {
public:
    ItemTargetingEngine() = default;

    std::vector<ShellProjectile>& projectiles() { return m_projectiles; }
    std::vector<DroppedBanana>& bananas() { return m_bananas; }

    void fireGreenShell(const KartEntity &firingKart);
    void dropBanana(const KartEntity &droppingKart);
    void useInkBlooper(int firingKartId, std::vector<KartEntity> &karts);

    void update(std::vector<KartEntity> &karts, double dt);
    void clear();

private:
    std::vector<ShellProjectile> m_projectiles;
    std::vector<DroppedBanana> m_bananas;
};

class HazardEngine {
public:
    static std::vector<TrackHazard> createTrackHazards(TrackLevel level);
    static void updateHazards(std::vector<TrackHazard> &hazards, std::vector<KartEntity> &karts, double dt);
};

class SplitScreenEngine {
public:
    static bool isPointInP1Viewport(int x, int y);
    static bool isPointInP2Viewport(int x, int y);
    static bool isPointInDivider(int x, int y);

    static void handleInputMultiplexing(
        const std::set<int>& pressedKeys,
        bool &p1Throttle, bool &p1Brake, int &p1Steer, bool &p1Item, bool &p1Drift,
        bool &p2Throttle, bool &p2Brake, int &p2Steer, bool &p2Item, bool &p2Drift
    );
};

#endif // TEST_HARNESS_H
