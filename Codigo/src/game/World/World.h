#pragma once
#include <array>
#include <deque>
#include <string>
#include <vector>
#include <raylib.h>
#include "game/Stickman/Stickman.h"

struct Assets;

struct Plat { Rectangle r; bool oneWay; };

struct Bullet {
    Vector2 p, v;
    int dmg;
    bool fromPlayer;
    bool alive = true;
    bool nearMissDone = false;
    float life = 2.5f;
};

struct Pickup { Vector2 p; WeaponKind w; int ammo; float bob = 0; };

// --- Efectos visuales ------------------------------------------------------
struct Particle { Vector2 p, v; float life, max, size, grav; Color c; };
struct Spark { Vector2 p; float life, max, scale, rot; bool big; };
struct FloatText { Vector2 p; std::string s; float life, max, size; Color c; };
struct Ring { Vector2 p; float life, max, r; Color c; };

class Effects {
public:
    void spark(Vector2 p, float scale, bool big);
    void ink(Vector2 p, int n, float dir);            // gotas negras al golpear (como en el mockup)
    void dust(Vector2 p, int n);
    void smoke(Vector2 p, int n, Color c);
    void text(Vector2 p, const std::string& s, Color c, float size);
    void ring(Vector2 p, float r, Color c);
    void update(float dt);
    void draw(const Assets& a) const;
    void clear() { parts_.clear(); sparks_.clear(); texts_.clear(); rings_.clear(); }
private:
    std::vector<Particle> parts_;
    std::vector<Spark> sparks_;
    std::vector<FloatText> texts_;
    std::vector<Ring> rings_;
};

// --- Ragdoll (RF-006) --------------------------------------------------------
// Física Verlet propia: 11 puntos unidos por distancias (sin motor externo).
class Ragdoll {
public:
    void init(const Pose& world, Vector2 vel, Vector2 impulse, Color c, float width, float headR);
    void update(float dt, const std::vector<Plat>& plats);
    void draw() const;
    bool dead() const { return t_ > 9.0f; }
    float age() const { return t_; }
private:
    std::array<Vector2, 11> p_{}, o_{};
    std::vector<std::array<float, 3>> links_;   // a, b, largo
    Color c_{};
    float w_ = 6, headR_ = 11, t_ = 0, prevDt_ = 1.0f / 60.0f;
    bool frozen_ = false;
};

// Todo lo que comparten jugador, enemigos y escena
struct World {
    std::vector<Plat> plats;
    float width = 2560;
    std::vector<Bullet> bullets;
    std::vector<Pickup> pickups;
    std::deque<Ragdoll> ragdolls;
    Effects fx;
    const Assets* assets = nullptr;
    float shake = 0;            // temblor de cámara
    float hitStop = 0;          // congelado breve al pegar fuerte
    int attackTokens = 2;       // cuántos enemigos más pueden atacar ahora
    bool bulletTime = false;
    struct SpawnReq { int type; float x; };
    std::vector<SpawnReq> spawnRequests;   // el jefe pide clones
    std::vector<std::pair<std::string, Color>> banners;   // avisos grandes (cambio de fase…)

    void addRagdoll(const Pose& wp, Vector2 vel, Vector2 impulse, Color c, float width, float headR);
    float groundAt(float x, float feetY) const;   // la plataforma más alta por debajo de feetY
};
