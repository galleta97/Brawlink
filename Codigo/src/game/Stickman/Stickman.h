#pragma once
#include <raylib.h>

// Esqueleto de 11 puntos, relativo a la cadera, en px (guía 5.1).
struct Pose { Vector2 neck, head, le, lh, re, rh, hip, lk, lf, rk, rf; };

// Ángulos de las extremidades: 0 = hacia abajo, +π/2 = hacia adelante, π = hacia arriba.
// Las animaciones se generan con estos ángulos y se interpolan suavemente.
struct Rig {
    float lean = 0.06f, tilt = 0.0f;            // inclinación del torso y de la cabeza
    float lu = 0.25f, ll = 2.2f;                // brazo de atrás: brazo / antebrazo
    float ru = 0.55f, rl = 2.5f;                // brazo de adelante
    float lt = -0.2f, ls = -0.32f;              // pierna de atrás: muslo / canilla
    float rt = 0.22f, rs = 0.05f;               // pierna de adelante
};

Rig lerpRig(const Rig& a, const Rig& b, float t);
Pose buildPose(const Rig& r, float scale);
float footDepth(const Pose& p);   // cuánto bajan los pies desde la cadera

// Pose en coordenadas del mundo (cadera en `hip`, volteada si mira a la izquierda)
Pose worldPose(const Pose& p, Vector2 hip, bool flip);
void drawPose(const Pose& wp, Color c, float width, float headR);
void drawScarf(const Pose& wp, int facing, float t, float speed, float scale);

enum class WeaponKind { None, Bate, Pistola, Katana };
void drawWeapon(WeaponKind w, Vector2 elbow, Vector2 hand, float scale);

// Biblioteca de animaciones
namespace rigs {
Rig idle(float t);
Rig walk(float phase);
Rig run(float phase);
Rig jump(float vy);
Rig crouch();
Rig punch(float p, int side);
Rig kick(float p);
Rig airKick(float p);
Rig block();
Rig hit(float p);
Rig swing(float p);
Rig aim(float recoil);
Rig windup(float p);
Rig dash();
Rig victory(float t);
Rig menuKick(float t);
Rig loginKick(float t);
}
