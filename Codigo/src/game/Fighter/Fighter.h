#pragma once
#include <cmath>
#include <raylib.h>
#include "core/Config/Config.h"
#include "game/Stickman/Stickman.h"
#include "game/World/World.h"

// Base común de jugador y enemigos: cuerpo (caja), movimiento con gravedad y esqueleto animado.
// Física propia: gravedad y colisiones contra rectángulos (sin motor externo).
struct Fighter {
    Vector2 pos{0, cfg::GROUND_Y};   // centro de los pies
    Vector2 vel{0, 0};
    float hp = 100, maxHp = 100;
    int facing = 1;
    bool onGround = true;
    float scale = 1.0f;
    float animT = 0, phase = 0;
    float dropT = 0;                 // atravesar plataformas finas hacia abajo
    Rig rig;
    Pose pose{};
    Pose wpose{};                    // pose en el mundo (para golpes, armas y ragdoll)

    float height() const { return 100.0f * scale; }
    Rectangle hurtbox() const { return {pos.x - 16 * scale, pos.y - height(), 32 * scale, height()}; }
    Vector2 center() const { return {pos.x, pos.y - height() * 0.55f}; }
    bool alive() const { return hp > 0; }

    void physics(float dt, const World& w, float friction) {
        vel.y += cfg::GRAVITY * dt;
        if (onGround) vel.x *= std::pow(friction, dt * 60.0f);
        float prevY = pos.y;
        pos.x += vel.x * dt;
        pos.y += vel.y * dt;
        dropT = std::max(0.0f, dropT - dt);
        onGround = false;
        if (vel.y >= 0) {
            for (const auto& p : w.plats) {
                if (p.oneWay && dropT > 0) continue;
                if (pos.x < p.r.x - 8 || pos.x > p.r.x + p.r.width + 8) continue;
                if (prevY <= p.r.y + 2 && pos.y >= p.r.y) { pos.y = p.r.y; vel.y = 0; onGround = true; }
            }
        }
        float m = 24 * scale;
        if (pos.x < m) { pos.x = m; vel.x = std::max(0.0f, vel.x); }
        if (pos.x > w.width - m) { pos.x = w.width - m; vel.x = std::min(0.0f, vel.x); }
    }

    // Suaviza hacia la pose objetivo y calcula la pose en el mundo con los pies apoyados
    void animate(const Rig& target, float dt, float speed = 22.0f) {
        rig = lerpRig(rig, target, 1.0f - std::exp(-dt * speed));
        pose = buildPose(rig, scale);
        float depth = onGround ? footDepth(pose) : 54.0f * scale;
        wpose = worldPose(pose, {pos.x, pos.y - depth}, facing < 0);
    }
};
