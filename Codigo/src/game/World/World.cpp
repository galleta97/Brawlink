#include "game/World/World.h"
#include <algorithm>
#include <cmath>
#include "core/Assets/Assets.h"
#include "core/Config/Config.h"

namespace {
float frand(float a, float b) { return a + (b - a) * (GetRandomValue(0, 10000) / 10000.0f); }
float dist(Vector2 a, Vector2 b) { return std::sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y)); }
Color fade(Color c, float a) { c.a = static_cast<unsigned char>(c.a * std::clamp(a, 0.0f, 1.0f)); return c; }
}

// --- Efectos -----------------------------------------------------------------
void Effects::spark(Vector2 p, float scale, bool big) {
    sparks_.push_back({p, 0.16f + (big ? 0.06f : 0.0f), 0.16f + (big ? 0.06f : 0.0f), scale, frand(0, 6.28f), big});
}
void Effects::ink(Vector2 p, int n, float dir) {
    for (int i = 0; i < n; ++i)
        parts_.push_back({p, {dir * frand(80, 420), frand(-380, -60)}, 0.5f, 0.5f, frand(2.5f, 5.0f), 1600, pal::Ink});
}
void Effects::dust(Vector2 p, int n) {
    for (int i = 0; i < n; ++i)
        parts_.push_back({{p.x + frand(-10, 10), p.y}, {frand(-120, 120), frand(-90, -20)}, 0.4f, 0.4f, frand(3, 7), -60, Color{150, 140, 125, 150}});
}
void Effects::smoke(Vector2 p, int n, Color c) {
    for (int i = 0; i < n; ++i)
        parts_.push_back({{p.x + frand(-20, 20), p.y + frand(-60, 0)}, {frand(-80, 80), frand(-140, -30)}, 0.7f, 0.7f, frand(8, 18), -100, c});
}
void Effects::text(Vector2 p, const std::string& s, Color c, float size) {
    texts_.push_back({p, s, 0.9f, 0.9f, size, c});
}
void Effects::ring(Vector2 p, float r, Color c) { rings_.push_back({p, 0.45f, 0.45f, r, c}); }

void Effects::update(float dt) {
    for (auto& q : parts_) { q.life -= dt; q.v.y += q.grav * dt; q.p.x += q.v.x * dt; q.p.y += q.v.y * dt; }
    for (auto& s : sparks_) s.life -= dt;
    for (auto& t : texts_) { t.life -= dt; t.p.y -= 60 * dt; }
    for (auto& r : rings_) r.life -= dt;
    parts_.erase(std::remove_if(parts_.begin(), parts_.end(), [](auto& q) { return q.life <= 0; }), parts_.end());
    sparks_.erase(std::remove_if(sparks_.begin(), sparks_.end(), [](auto& q) { return q.life <= 0; }), sparks_.end());
    texts_.erase(std::remove_if(texts_.begin(), texts_.end(), [](auto& q) { return q.life <= 0; }), texts_.end());
    rings_.erase(std::remove_if(rings_.begin(), rings_.end(), [](auto& q) { return q.life <= 0; }), rings_.end());
}

void Effects::draw(const Assets& a) const {
    for (auto& r : rings_) {
        float k = 1.0f - r.life / r.max;
        DrawRing(r.p, r.r * k * 0.9f, r.r * k + 6, 0, 360, 48, fade(r.c, 1.0f - k));
    }
    for (auto& q : parts_) DrawCircleV(q.p, q.size * (0.4f + 0.6f * q.life / q.max), fade(q.c, q.life / q.max * 1.4f));
    for (auto& s : sparks_) {
        float k = 1.0f - s.life / s.max;
        float r = (s.big ? 62 : 42) * s.scale * (0.6f + 0.6f * k);
        starburst(s.p, r, r * 0.42f, s.big ? 12 : 10, s.rot, fade(pal::Yellow, 1.2f - k));
        starburst(s.p, r * 0.62f, r * 0.26f, s.big ? 12 : 10, s.rot + 0.2f, fade(pal::Red, 1.2f - k));
    }
    for (auto& t : texts_) {
        float al = std::min(1.0f, t.life / t.max * 2.0f);
        ::text(a.title, t.s, t.p.x, t.p.y, t.size, fade(t.c, al), Align::Center);
    }
}

// --- Ragdoll -------------------------------------------------------------------
void Ragdoll::init(const Pose& wp, Vector2 vel, Vector2 impulse, Color c, float width, float headR) {
    const Vector2 pts[11] = {wp.neck, wp.head, wp.le, wp.lh, wp.re, wp.rh, wp.hip, wp.lk, wp.lf, wp.rk, wp.rf};
    const float dt = 1.0f / 60.0f;
    for (int i = 0; i < 11; ++i) {
        p_[i] = pts[i];
        // la parte de arriba recibe más impulso: el cuerpo gira al caer
        float upper = (i <= 5) ? 1.0f : 0.45f;
        Vector2 v{vel.x + impulse.x * upper + frand(-40, 40), vel.y + impulse.y * upper + frand(-40, 40)};
        o_[i] = {p_[i].x - v.x * dt, p_[i].y - v.y * dt};
    }
    const int L[][2] = {{0, 1}, {0, 2}, {2, 3}, {0, 4}, {4, 5}, {0, 6}, {6, 7}, {7, 8}, {6, 9}, {9, 10}, {1, 6}, {7, 9}, {2, 6}, {4, 6}};
    links_.clear();
    for (auto& l : L) links_.push_back({static_cast<float>(l[0]), static_cast<float>(l[1]), dist(p_[l[0]], p_[l[1]])});
    c_ = c; w_ = width; headR_ = headR; t_ = 0; frozen_ = false; prevDt_ = dt;
}

void Ragdoll::update(float dt, const std::vector<Plat>& plats) {
    t_ += dt;
    if (frozen_ || dt <= 0) return;
    if (t_ > cfg::RAGDOLL_FREEZE) { frozen_ = true; return; }   // RF-006: se congela a los 3 s
    const float ratio = dt / prevDt_;
    prevDt_ = dt;
    for (int i = 0; i < 11; ++i) {
        Vector2 v{(p_[i].x - o_[i].x) * ratio * 0.995f, (p_[i].y - o_[i].y) * ratio * 0.995f};
        o_[i] = p_[i];
        p_[i].x += v.x;
        p_[i].y += v.y + cfg::GRAVITY * dt * dt;
    }
    for (int it = 0; it < 8; ++it) {
        for (auto& l : links_) {
            int a = static_cast<int>(l[0]), b = static_cast<int>(l[1]);
            Vector2 d{p_[b].x - p_[a].x, p_[b].y - p_[a].y};
            float len = std::sqrt(d.x * d.x + d.y * d.y);
            if (len < 0.0001f) continue;
            float k = (len - l[2]) / len * 0.5f;
            p_[a].x += d.x * k; p_[a].y += d.y * k;
            p_[b].x -= d.x * k; p_[b].y -= d.y * k;
        }
        for (int i = 0; i < 11; ++i) {
            float pad = (i == 1) ? headR_ : w_ * 0.5f;
            for (const auto& pl : plats) {
                if (p_[i].x < pl.r.x || p_[i].x > pl.r.x + pl.r.width) continue;
                float top = pl.r.y - pad;
                bool crossing = o_[i].y <= top + 1.0f && p_[i].y > top;
                bool inside = !pl.oneWay && p_[i].y > top && p_[i].y < pl.r.y + pl.r.height;
                if (crossing || inside) {
                    p_[i].y = top;
                    o_[i].x = p_[i].x - (p_[i].x - o_[i].x) * 0.7f;   // fricción con el suelo
                }
            }
        }
    }
}

void Ragdoll::draw() const {
    float a = t_ > 7.0f ? std::max(0.0f, 1.0f - (t_ - 7.0f) / 2.0f) : 1.0f;
    Color c = fade(c_, a);
    Pose wp{p_[0], p_[1], p_[2], p_[3], p_[4], p_[5], p_[6], p_[7], p_[8], p_[9], p_[10]};
    drawPose(wp, c, w_, headR_);
}

void World::addRagdoll(const Pose& wp, Vector2 vel, Vector2 impulse, Color c, float width, float headR) {
    ragdolls.emplace_back();
    ragdolls.back().init(wp, vel, impulse, c, width, headR);
    while (ragdolls.size() > static_cast<size_t>(cfg::MAX_RAGDOLLS)) ragdolls.pop_front();   // RF-006: máximo 6
}

float World::groundAt(float x, float feetY) const {
    float best = 1e9f;
    for (const auto& p : plats)
        if (x >= p.r.x && x <= p.r.x + p.r.width && p.r.y >= feetY - 1 && p.r.y < best) best = p.r.y;
    return best;
}
