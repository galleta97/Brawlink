#pragma once
#include "game/Fighter/Fighter.h"

class Player;

// RF-006: 3 tipos mínimos + el jefe Kage y sus clones de sombra
enum class EType { Maton = 0, Pistolero = 1, Pesado = 2, Clon = 3, Kage = 4 };
// Máquina de estados: Patrullar → Perseguir → (Preparar) → Atacar → Recuperarse / Aturdido → Muerto
enum class EState { Patrol, Chase, Windup, Attack, Recover, Stunned, Teleport };

class Enemy : public Fighter {
public:
    EType type = EType::Maton;
    EState st = EState::Patrol;
    float stT = 0, cd = 0, stunT = 0, invuln = 0, flash = 0;
    int id = 0;
    float speed = 170, range = 55, spawnX = 0, alpha = 1;
    int atkDmg = 8;
    bool hitDone = false;
    int phase = 1, comboHits = 0;          // jefe
    float cloneT = 6, teleT = 5;
    bool dashAttack = false;
    float aimAngle = 0;

    Enemy(EType t, float x, int id);
    void update(float dt, World& w, Player& p);
    // devuelve true si murió con este golpe
    bool takeHit(int dmg, float fromX, float kb, float stun, bool heavy, World& w);
    void draw(float time) const;
    bool attacking() const { return st == EState::Windup || st == EState::Attack; }
    int killScore() const;
    Color color() const;
    WeaponKind weapon() const;
    const char* name() const;

private:
    void meleeAI(float dt, World& w, Player& p, float dist, int dir);
    void gunnerAI(float dt, World& w, Player& p, float dist, int dir);
    void bossAI(float dt, World& w, Player& p, float dist, int dir);
    void tryHit(World& w, Player& p, float reach, int dmg, bool breaks, float kb);
    Rig pickRig(float dt) const;
};
