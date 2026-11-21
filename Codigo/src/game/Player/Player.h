#pragma once
#include <deque>
#include <vector>
#include "game/Fighter/Fighter.h"

class Input;

enum class AtkType { None, Punch, Kick, AirKick, Swing, Shoot };

struct Attack {
    AtkType type = AtkType::None;
    float t = 0, dur = 0, a0 = 0, a1 = 0;   // ventana activa [a0, a1] (RF-004)
    int dmg = 0;
    float range = 0, kb = 0, top = 0, bottom = 0, stun = 0.3f;
    bool heavy = false;
    int side = 0;
    std::vector<int> hitIds;                // un golpe por enemigo por ataque
};

const char* weaponName(WeaponKind w);
int weaponDamage(WeaponKind w);             // debe coincidir con la tabla arma (002_catalogos.sql)
constexpr int PISTOL_AMMO = 12;

class Player : public Fighter {
public:
    WeaponKind weapon = WeaponKind::None;
    int ammo = 0;
    Attack atk;
    bool blocking = false, crouching = false, running = false;
    float hurtT = 0, invuln = 0, coyote = 0, jumpBuf = 0, blockBroken = 0;
    AtkType queued = AtkType::None;
    int punchSide = 0;
    float dmgFlash = 0;

    struct Ghost { Pose p; float life; };
    std::deque<Ghost> ghosts;
    float ghostT = 0;

    Player();
    void update(float dt, Input& in, World& w);
    bool attackActive() const { return atk.type != AtkType::None && atk.type != AtkType::Shoot && atk.t >= atk.a0 && atk.t <= atk.a1; }
    Rectangle hitbox() const;
    Rectangle hurtbox() const;
    int takeHit(int dmg, float fromX, bool breaksBlock, float kb, World& w);
    void draw(float time, bool bulletTime) const;

private:
    void startAttack(AtkType t, World& w);
    void fire(World& w);
};
