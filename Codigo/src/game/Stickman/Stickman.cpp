#include "game/Stickman/Stickman.h"
#include <algorithm>
#include <cmath>
#include "core/Config/Config.h"

namespace {
constexpr float TORSO = 46, HEAD = 19, UPPER = 25, FORE = 23, THIGH = 28, SHIN = 28;
constexpr float kPI = 3.14159265f;
Vector2 polar(float a, float len) { return {std::sin(a) * len, std::cos(a) * len}; }
Vector2 add(Vector2 a, Vector2 b) { return {a.x + b.x, a.y + b.y}; }
float lerp(float a, float b, float t) { return a + (b - a) * t; }
float smooth(float t) { t = std::clamp(t, 0.0f, 1.0f); return t * t * (3 - 2 * t); }
// curva de golpe: sube rápido hasta `peak` y vuelve
float strike(float p, float peak) { return p < peak ? smooth(p / peak) : 1.0f - smooth((p - peak) / (1.0f - peak)); }
}

Rig lerpRig(const Rig& a, const Rig& b, float t) {
    Rig r;
    r.lean = lerp(a.lean, b.lean, t); r.tilt = lerp(a.tilt, b.tilt, t);
    r.lu = lerp(a.lu, b.lu, t); r.ll = lerp(a.ll, b.ll, t);
    r.ru = lerp(a.ru, b.ru, t); r.rl = lerp(a.rl, b.rl, t);
    r.lt = lerp(a.lt, b.lt, t); r.ls = lerp(a.ls, b.ls, t);
    r.rt = lerp(a.rt, b.rt, t); r.rs = lerp(a.rs, b.rs, t);
    return r;
}

Pose buildPose(const Rig& r, float s) {
    Pose p{};
    p.hip = {0, 0};
    p.neck = {std::sin(r.lean) * TORSO * s, -std::cos(r.lean) * TORSO * s};
    p.head = add(p.neck, {std::sin(r.lean + r.tilt) * HEAD * s, -std::cos(r.lean + r.tilt) * HEAD * s});
    p.le = add(p.neck, polar(r.lu, UPPER * s)); p.lh = add(p.le, polar(r.ll, FORE * s));
    p.re = add(p.neck, polar(r.ru, UPPER * s)); p.rh = add(p.re, polar(r.rl, FORE * s));
    p.lk = add(p.hip, polar(r.lt, THIGH * s));  p.lf = add(p.lk, polar(r.ls, SHIN * s));
    p.rk = add(p.hip, polar(r.rt, THIGH * s));  p.rf = add(p.rk, polar(r.rs, SHIN * s));
    return p;
}

float footDepth(const Pose& p) { return std::max(p.lf.y, p.rf.y); }

Pose worldPose(const Pose& p, Vector2 at, bool flip) {
    auto P = [&](Vector2 v) { return Vector2{at.x + (flip ? -v.x : v.x), at.y + v.y}; };
    return {P(p.neck), P(p.head), P(p.le), P(p.lh), P(p.re), P(p.rh), P(p.hip), P(p.lk), P(p.lf), P(p.rk), P(p.rf)};
}

void drawPose(const Pose& p, Color c, float w, float headR) {
    auto seg = [&](Vector2 a, Vector2 b) { DrawLineEx(a, b, w, c); DrawCircleV(b, w * 0.5f, c); };
    DrawCircleV(p.neck, w * 0.5f, c);
    DrawCircleV(p.hip, w * 0.5f, c);
    // brazo y pierna de atrás primero
    seg(p.neck, p.le); seg(p.le, p.lh);
    seg(p.hip, p.lk);  seg(p.lk, p.lf);
    seg(p.neck, p.hip);
    seg(p.hip, p.rk);  seg(p.rk, p.rf);
    seg(p.neck, p.re); seg(p.re, p.rh);
    DrawLineEx(p.neck, p.head, w, c);
    DrawCircleV(p.head, headR, c);
}

void drawScarf(const Pose& p, int facing, float t, float speed, float s) {
    Vector2 knot{p.neck.x + (p.head.x - p.neck.x) * 0.25f, p.neck.y + (p.head.y - p.neck.y) * 0.25f};
    float flow = std::min(1.0f, 0.35f + std::fabs(speed) / 420.0f);
    Vector2 prev = knot;
    for (int i = 1; i <= 6; ++i) {
        float wave = std::sin(t * 14.0f - i * 0.9f) * (2.0f + i * 1.2f) * flow;
        Vector2 q{knot.x - facing * i * 11.0f * s * flow, knot.y + (1.0f - flow) * i * 7.0f * s + wave * s};
        DrawLineEx(prev, q, (7.5f - i * 0.8f) * s, pal::Red);
        DrawCircleV(q, (7.5f - i * 0.8f) * s * 0.5f, pal::Red);
        prev = q;
    }
}

void drawWeapon(WeaponKind w, Vector2 elbow, Vector2 hand, float s) {
    if (w == WeaponKind::None) return;
    Vector2 d{hand.x - elbow.x, hand.y - elbow.y};
    float len = std::sqrt(d.x * d.x + d.y * d.y);
    if (len < 0.001f) d = {1, 0}; else d = {d.x / len, d.y / len};
    auto at = [&](float k) { return Vector2{hand.x + d.x * k * s, hand.y + d.y * k * s}; };
    switch (w) {
    case WeaponKind::Bate:
        DrawLineEx(at(-6), at(26), 5 * s, Color{120, 78, 42, 255});
        DrawLineEx(at(26), at(60), 9 * s, Color{150, 100, 55, 255});
        DrawCircleV(at(60), 4.5f * s, Color{150, 100, 55, 255});
        break;
    case WeaponKind::Katana:
        DrawLineEx(at(-12), at(4), 5 * s, pal::RedDark);
        DrawCircleV(at(4), 4 * s, pal::Ink);
        DrawLineEx(at(4), at(84), 3 * s, Color{228, 228, 232, 255});
        DrawLineEx(at(60), at(84), 1.5f * s, WHITE);
        break;
    case WeaponKind::Pistola: {
        Vector2 n{-d.y, d.x};
        DrawLineEx(at(-2), at(22), 7 * s, pal::Ink);
        DrawLineEx(at(2), Vector2{at(2).x + n.x * 10 * s, at(2).y + n.y * 10 * s}, 5 * s, pal::Ink);
        break;
    }
    default: break;
    }
}

namespace rigs {
Rig idle(float t) {
    Rig r;
    float b = std::sin(t * 3.0f);
    r.lean = 0.06f + 0.015f * b;
    r.ru = 0.55f + 0.04f * b; r.rl = 2.5f + 0.05f * b;
    r.lu = 0.25f + 0.04f * b; r.ll = 2.2f + 0.05f * b;
    return r;
}
Rig walk(float ph) {
    Rig r = idle(0);
    r.lean = 0.12f;
    r.rt = 0.5f * std::sin(ph);       r.rs = r.rt - 0.8f * std::max(0.0f, std::sin(ph + 1.3f));
    r.lt = 0.5f * std::sin(ph + kPI);  r.ls = r.lt - 0.8f * std::max(0.0f, std::sin(ph + kPI + 1.3f));
    r.ru += 0.1f * std::sin(ph);      r.lu += 0.1f * std::sin(ph + kPI);
    return r;
}
Rig run(float ph) {
    Rig r;
    r.lean = 0.32f; r.tilt = -0.1f;
    r.rt = 0.95f * std::sin(ph);      r.rs = r.rt - 1.5f * std::max(0.0f, std::sin(ph + 1.2f)) - 0.2f;
    r.lt = 0.95f * std::sin(ph + kPI); r.ls = r.lt - 1.5f * std::max(0.0f, std::sin(ph + kPI + 1.2f)) - 0.2f;
    r.ru = -0.9f * std::sin(ph);      r.rl = r.ru + 1.7f;
    r.lu = -0.9f * std::sin(ph + kPI); r.ll = r.lu + 1.7f;
    return r;
}
Rig jump(float vy) {
    Rig r;
    float up = std::clamp(-vy / 800.0f, 0.0f, 1.0f);
    r.lean = 0.15f;
    r.rt = lerp(0.5f, 1.1f, up);  r.rs = lerp(0.1f, -0.3f, up);
    r.lt = lerp(-0.2f, 0.3f, up); r.ls = lerp(-0.3f, -1.0f, up);
    r.ru = lerp(1.2f, 2.3f, up);  r.rl = lerp(1.6f, 2.8f, up);
    r.lu = lerp(-0.7f, -1.2f, up); r.ll = lerp(-0.3f, -0.6f, up);
    return r;
}
Rig crouch() {
    Rig r;
    r.lean = 0.3f;
    r.rt = 1.35f; r.rs = -0.05f;
    r.lt = 0.55f; r.ls = -1.25f;
    r.ru = 0.9f; r.rl = 2.6f; r.lu = 0.6f; r.ll = 2.4f;
    return r;
}
Rig punch(float p, int side) {
    Rig r = idle(0);
    float e = strike(p, 0.35f);
    r.lean = lerp(0.06f, 0.32f, e);
    if (side == 0) { r.ru = lerp(0.55f, 1.62f, e); r.rl = lerp(2.5f, 1.58f, e); r.lu = lerp(0.25f, -0.3f, e); }
    else           { r.lu = lerp(0.25f, 1.6f, e);  r.ll = lerp(2.2f, 1.56f, e); r.ru = lerp(0.55f, 0.1f, e); r.rl = lerp(2.5f, 2.8f, e); }
    r.rt = lerp(0.22f, 0.45f, e); r.lt = lerp(-0.2f, -0.45f, e); r.ls = lerp(-0.32f, -0.4f, e);
    return r;
}
Rig kick(float p) {
    Rig r = idle(0);
    float e = strike(p, 0.45f);
    r.lean = lerp(0.06f, -0.38f, e); r.tilt = lerp(0.0f, 0.15f, e);
    r.rt = lerp(0.22f, 1.78f, e); r.rs = lerp(0.05f, 1.62f, e);
    r.lt = lerp(-0.2f, -0.08f, e); r.ls = lerp(-0.32f, -0.1f, e);
    r.ru = lerp(0.55f, -0.7f, e); r.rl = lerp(2.5f, -0.2f, e);
    r.lu = lerp(0.25f, 1.3f, e);  r.ll = lerp(2.2f, 2.3f, e);
    return r;
}
Rig airKick(float p) {
    Rig r = kick(p);
    float e = strike(p, 0.3f);
    r.lt = lerp(0.3f, 0.6f, e); r.ls = lerp(-0.9f, -1.6f, e);
    r.rt = lerp(1.0f, 1.45f, e); r.rs = lerp(0.2f, 1.3f, e);
    r.lean = lerp(0.1f, -0.2f, e);
    return r;
}
Rig block() {
    Rig r;
    r.lean = 0.12f; r.tilt = 0.15f;
    r.ru = 1.25f; r.rl = 2.95f; r.lu = 1.05f; r.ll = 2.8f;
    r.rt = 0.32f; r.rs = 0.12f; r.lt = -0.32f; r.ls = -0.38f;
    return r;
}
Rig hit(float p) {
    Rig r;
    float e = 1.0f - smooth(p);
    r.lean = -0.4f * e + 0.06f * (1 - e); r.tilt = -0.45f * e;
    r.ru = lerp(0.55f, -0.6f, e); r.rl = lerp(2.5f, 0.3f, e);
    r.lu = lerp(0.25f, 0.9f, e);  r.ll = lerp(2.2f, 1.6f, e);
    r.rt = lerp(0.22f, 0.45f, e); r.lt = lerp(-0.2f, -0.1f, e);
    return r;
}
Rig swing(float p) {
    Rig r = idle(0);
    float a = p < 0.3f ? smooth(p / 0.3f) : 1.0f;
    float b = p < 0.3f ? 0.0f : smooth((p - 0.3f) / 0.3f);
    float arm = lerp(lerp(0.55f, 3.7f, a), 0.95f, b);
    r.ru = arm; r.rl = arm + lerp(0.0f, -0.25f, b);
    r.lean = lerp(lerp(0.06f, -0.12f, a), 0.38f, b);
    r.rt = lerp(0.22f, 0.55f, b); r.lt = lerp(-0.2f, -0.5f, b);
    r.lu = lerp(0.25f, -0.4f, b);
    return r;
}
Rig aim(float recoil) {
    Rig r = idle(0);
    r.lean = 0.02f - 0.08f * recoil;
    r.ru = 1.57f - 0.25f * recoil; r.rl = 1.57f - 0.5f * recoil;
    r.lu = 0.2f; r.ll = 1.9f;
    return r;
}
Rig windup(float p) {
    Rig r = idle(0);
    float e = smooth(p);
    r.ru = lerp(0.55f, -0.5f, e); r.rl = lerp(2.5f, 2.9f, e);
    r.lean = lerp(0.06f, -0.12f, e);
    r.rt = lerp(0.22f, 0.4f, e); r.lt = lerp(-0.2f, -0.35f, e);
    return r;
}
Rig dash() {
    Rig r;
    r.lean = 0.55f; r.tilt = -0.2f;
    r.rt = 1.2f; r.rs = 0.25f; r.lt = -0.7f; r.ls = -0.9f;
    r.ru = 1.45f; r.rl = 1.5f; r.lu = -0.6f; r.ll = -0.3f;
    return r;
}
Rig victory(float t) {
    Rig r;
    float b = std::sin(t * 4.0f) * 0.08f;
    r.lean = 0.0f;
    r.ru = 2.72f + b; r.rl = 2.78f + b;
    r.lu = -2.72f - b; r.ll = -2.78f - b;
    r.rt = 0.3f; r.rs = 0.3f; r.lt = -0.3f; r.ls = -0.3f;
    return r;
}
Rig menuKick(float t) {
    Rig r = kick(0.45f);
    float b = std::sin(t * 2.0f) * 0.05f;
    r.rt = 1.62f + b; r.rs = 1.58f + b; r.lean = -0.5f; r.tilt = 0.2f;
    r.ru = 1.2f; r.rl = 0.7f; r.lu = 1.9f; r.ll = 2.5f;
    r.lt = -0.25f; r.ls = 0.05f;
    return r;
}
Rig loginKick(float t) {
    Rig r = menuKick(t);
    r.rt = 1.72f; r.rs = 1.62f; r.ru = 1.75f; r.rl = 2.35f; r.lu = -1.55f; r.ll = -2.1f; r.lean = -0.35f; r.tilt = 0.25f;
    r.lt = -0.15f; r.ls = 0.2f;
    return r;
}
}  // namespace rigs
