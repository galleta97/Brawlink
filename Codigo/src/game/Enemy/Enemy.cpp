#include "game/Enemy/Enemy.h"
#include <algorithm>
#include "core/Assets/Assets.h"
#include "game/Player/Player.h"

namespace {
float frand(float a, float b) { return a + (b - a) * (GetRandomValue(0, 10000) / 10000.0f); }
}

Enemy::Enemy(EType t, float x, int id_) : type(t), id(id_) {
    pos = {x, cfg::GROUND_Y};
    spawnX = x;
    switch (t) {   // valores iniciales (guía 5.4), para balancear en el sprint 2
    case EType::Maton:     hp = 40;  speed = 175; range = 55;  atkDmg = 8;  scale = 1.0f; break;
    case EType::Pistolero: hp = 30;  speed = 150; range = 420; atkDmg = 10; scale = 0.97f; break;
    case EType::Pesado:    hp = 100; speed = 95;  range = 75;  atkDmg = 20; scale = 1.3f; break;
    case EType::Clon:      hp = 25;  speed = 215; range = 55;  atkDmg = 7;  scale = 1.0f; alpha = 0.7f; break;
    case EType::Kage:      hp = 720; speed = 235; range = 62;  atkDmg = 9;  scale = 1.15f; break;
    }
    maxHp = hp;
    cd = frand(0.5f, 1.2f);
    rig = rigs::idle(0);
}

const char* Enemy::name() const {
    switch (type) {
    case EType::Maton: return "MATÓN";
    case EType::Pistolero: return "PISTOLERO";
    case EType::Pesado: return "PESADO";
    case EType::Clon: return "SOMBRA";
    default: return "KAGE";
    }
}

int Enemy::killScore() const {
    switch (type) {
    case EType::Maton: return 150;
    case EType::Pistolero: return 200;
    case EType::Pesado: return 400;
    case EType::Clon: return 100;
    default: return 3000;
    }
}

Color Enemy::color() const {
    Color c;
    switch (type) {
    case EType::Maton: c = {66, 60, 56, 255}; break;
    case EType::Pistolero: c = {46, 50, 64, 255}; break;
    case EType::Pesado: c = {28, 26, 26, 255}; break;
    case EType::Clon: c = {58, 36, 80, 255}; break;
    default: c = pal::Ink;
    }
    c.a = static_cast<unsigned char>(255 * alpha);
    return c;
}

WeaponKind Enemy::weapon() const {
    if (type == EType::Pistolero) return WeaponKind::Pistola;
    if (type == EType::Kage && phase >= 2) return WeaponKind::Katana;
    return WeaponKind::None;
}

void Enemy::tryHit(World& w, Player& p, float reach, int dmg, bool breaks, float kb) {
    if (hitDone) return;
    float x0 = pos.x + facing * 6.0f, x1 = pos.x + facing * (6.0f + reach);
    Rectangle hb{std::min(x0, x1), pos.y - 95 * scale, std::fabs(x1 - x0), 70 * scale};
    if (CheckCollisionRecs(hb, p.hurtbox())) {
        hitDone = true;
        if (p.takeHit(dmg, pos.x, breaks, kb, w) > 0 && !p.blocking) w.assets->play(w.assets->punch, 0.1f, 0.8f);
    }
}

void Enemy::update(float dt, World& w, Player& p) {
    animT += dt;
    invuln = std::max(0.0f, invuln - dt);
    flash = std::max(0.0f, flash - dt);
    const float dx = p.pos.x - pos.x;
    const int dir = dx >= 0 ? 1 : -1;
    const float dist = std::fabs(dx);

    if (st == EState::Stunned) {
        stunT -= dt;
        if (stunT <= 0) { st = EState::Chase; cd = std::max(cd, 0.35f); }
    } else if (!p.alive()) {
        vel.x *= 0.8f;
        st = EState::Patrol;
    } else if (type == EType::Pistolero) {
        gunnerAI(dt, w, p, dist, dir);
    } else if (type == EType::Kage) {
        bossAI(dt, w, p, dist, dir);
    } else {
        meleeAI(dt, w, p, dist, dir);
    }

    physics(dt, w, st == EState::Stunned ? 0.86f : 0.8f);
    animate(pickRig(dt), dt, attacking() ? 30.0f : 18.0f);
}

void Enemy::meleeAI(float dt, World& w, Player& p, float dist, int dir) {
    const bool heavy = type == EType::Pesado;
    switch (st) {
    case EState::Patrol:
    case EState::Chase:
        st = EState::Chase;
        facing = dir;
        cd -= dt;
        if (dist > range) vel.x += (dir * speed - vel.x) * std::min(1.0f, 10.0f * dt);
        else vel.x *= 0.8f;
        // sube a las plataformas si el jugador está arriba
        if (!heavy && onGround && p.onGround && p.pos.y < pos.y - 60 && dist < 160) vel.y = -880;
        if (dist <= range + (heavy ? 160 : 0) && cd <= 0 && w.attackTokens > 0 && std::fabs(p.pos.y - pos.y) < 60) {
            st = EState::Windup; stT = 0; hitDone = false; w.attackTokens--;
        }
        break;
    case EState::Windup:
        stT += dt; vel.x *= 0.8f;
        if (stT >= (heavy ? 0.55f : 0.32f)) {
            st = EState::Attack; stT = 0;
            if (heavy) { vel.x = facing * 620.0f; w.assets->play(w.assets->whoosh, 0.0f, 0.9f); }
        }
        break;
    case EState::Attack:
        stT += dt;
        if (heavy) {
            vel.x = facing * 620.0f;    // embestida
            tryHit(w, p, 45, atkDmg, true, 650);
            if (stT > 0.5f) { st = EState::Recover; stT = 0; }
        } else {
            if (stT > 0.05f && stT < 0.16f) tryHit(w, p, range + 5, atkDmg, false, 260);
            if (stT > 0.26f) { st = EState::Recover; stT = 0; }
        }
        break;
    case EState::Recover:
        stT += dt; vel.x *= 0.85f;
        if (stT > (heavy ? 0.7f : 0.4f)) { st = EState::Chase; cd = heavy ? frand(1.8f, 2.8f) : frand(0.7f, 1.5f); }
        break;
    default: st = EState::Chase;
    }
}

void Enemy::gunnerAI(float dt, World& w, Player& p, float dist, int dir) {
    facing = dir;
    switch (st) {
    case EState::Patrol:
    case EState::Chase: {
        st = EState::Chase;
        cd -= dt;
        float target = 0;
        bool nearWall = pos.x < 80 || pos.x > w.width - 80;
        if (dist < 260 && !nearWall) target = -dir * speed;        // mantiene distancia
        else if (dist > 480) target = dir * speed;
        vel.x += (target - vel.x) * std::min(1.0f, 8.0f * dt);
        if (cd <= 0 && dist < 700 && w.attackTokens > 0) { st = EState::Windup; stT = 0; w.attackTokens--; }
        break;
    }
    case EState::Windup: {   // apunta (se ve la mira láser)
        stT += dt; vel.x *= 0.8f;
        Vector2 hand = wpose.rh;
        Vector2 tgt{p.pos.x, p.pos.y - 74};
        aimAngle = std::atan2(tgt.y - hand.y, tgt.x - hand.x);
        if (stT >= 0.5f) {
            float spd = 720;
            w.bullets.push_back({hand, {std::cos(aimAngle) * spd, std::sin(aimAngle) * spd}, atkDmg, false});
            w.fx.spark({hand.x + facing * 12.0f, hand.y}, 0.3f, false);
            w.assets->play(w.assets->enemyShot, 0.1f, 0.6f);
            st = EState::Attack; stT = 0;
        }
        break;
    }
    case EState::Attack:
        stT += dt;
        if (stT > 0.35f) { st = EState::Chase; cd = 1.2f; }   // dispara cada 1,2 s
        break;
    default: st = EState::Chase;
    }
}

void Enemy::bossAI(float dt, World& w, Player& p, float dist, int dir) {
    // Cambio de fase al 66 % y 33 % de vida (RF-008)
    float ratio = hp / maxHp;
    int want = ratio > 0.66f ? 1 : (ratio > 0.33f ? 2 : 3);
    if (want > phase) {
        phase = want;
        invuln = 1.0f;                       // invulnerable 1 s
        st = EState::Recover; stT = 0;
        speed = phase == 3 ? 285.0f : 250.0f;
        w.banners.push_back({phase == 2 ? "FASE 2 / 3 · LA KATANA" : "FASE 3 / 3 · CLONES DE SOMBRA", pal::Red});
        w.fx.ring(center(), 420, pal::Red);
        w.fx.smoke(pos, 14, Color{40, 20, 40, 160});
        w.assets->play(w.assets->phase, 0.0f, 1.0f);
        w.shake = 14;
        p.vel.x = (p.pos.x >= pos.x ? 1 : -1) * 750.0f; p.vel.y = -250;
        if (phase == 3) cloneT = 0.5f;
        return;
    }
    if (phase == 3 && st != EState::Teleport) {
        cloneT -= dt;
        if (cloneT <= 0) {
            cloneT = 8.0f;
            w.spawnRequests.push_back({static_cast<int>(EType::Clon), pos.x - 160});
            w.spawnRequests.push_back({static_cast<int>(EType::Clon), pos.x + 160});
        }
    }
    const bool katana = phase >= 2;
    switch (st) {
    case EState::Patrol:
    case EState::Chase:
        st = EState::Chase;
        facing = dir; cd -= dt;
        if (phase == 3) teleT -= dt;
        if (phase == 3 && teleT <= 0 && onGround) { st = EState::Teleport; stT = 0; teleT = frand(5, 7); break; }
        if (dist > range + (katana ? 30 : 0)) vel.x += (dir * speed - vel.x) * std::min(1.0f, 10.0f * dt);
        else vel.x *= 0.8f;
        if (onGround && p.onGround && p.pos.y < pos.y - 60 && dist < 200) vel.y = -900;
        if (cd <= 0 && std::fabs(p.pos.y - pos.y) < 70) {
            if (katana && dist > 230 && dist < 520) { dashAttack = true; st = EState::Windup; stT = 0; hitDone = false; }
            else if (dist <= range + (katana ? 40 : 10)) { dashAttack = false; st = EState::Windup; stT = 0; hitDone = false; comboHits = 0; }
        }
        break;
    case EState::Windup:
        stT += dt; vel.x *= 0.8f;
        if (stT >= (dashAttack ? 0.42f : (katana ? 0.3f : 0.2f))) {
            st = EState::Attack; stT = 0;
            if (dashAttack) { vel.x = facing * 950.0f; w.assets->play(w.assets->slash, 0.1f, 1.0f); }
            else if (katana) w.assets->play(w.assets->slash, 0.1f, 0.8f);
        }
        break;
    case EState::Attack:
        stT += dt;
        if (dashAttack) {
            vel.x = facing * 950.0f;
            tryHit(w, p, 70, 20, false, 520);
            if (stT > 0.3f) { st = EState::Recover; stT = 0; vel.x *= 0.3f; }
        } else if (katana) {
            if (stT > 0.06f && stT < 0.2f) tryHit(w, p, 98, 18, false, 460);
            if (stT > 0.32f) { st = EState::Recover; stT = 0; }
        } else {   // fase 1: ráfaga de 3 puñetazos
            if (stT > 0.04f && stT < 0.12f) tryHit(w, p, range + 8, atkDmg, false, 200);
            if (stT > 0.18f) {
                if (++comboHits < 3) { stT = 0; hitDone = false; vel.x = facing * 120.0f; }
                else { st = EState::Recover; stT = 0; }
            }
        }
        break;
    case EState::Recover:
        stT += dt; vel.x *= 0.85f;
        if (stT > (phase == 1 ? 0.55f : 0.45f) + (invuln > 0 ? 0.6f : 0)) { st = EState::Chase; cd = frand(0.4f, 1.0f); }
        break;
    case EState::Teleport:
        stT += dt;
        if (stT < 0.25f) alpha = 1.0f - stT / 0.25f;
        else if (stT < 0.27f) {
            float nx = p.pos.x - p.facing * 110.0f;
            nx = std::clamp(nx, 60.0f, w.width - 60);
            pos.x = nx; pos.y = p.pos.y; vel = {0, 0};
            facing = p.pos.x >= pos.x ? 1 : -1;
            w.fx.smoke(pos, 10, Color{40, 20, 40, 150});
        } else if (stT < 0.5f) alpha = (stT - 0.27f) / 0.23f;
        else { alpha = 1; st = EState::Windup; stT = 0; hitDone = false; dashAttack = false; }
        break;
    default: st = EState::Chase;
    }
}

bool Enemy::takeHit(int dmg, float fromX, float kb, float stun, bool heavy, World& w) {
    if (invuln > 0 || !alive() || st == EState::Teleport) {
        if (invuln > 0.2f) w.fx.text({pos.x, pos.y - 170}, "¡INMUNE!", pal::Grey, 26);
        return false;
    }
    int dir = fromX < pos.x ? 1 : -1;
    // Kage bloquea golpes de frente mientras camina y contraataca: hay que castigarlo tras sus ataques
    if (type == EType::Kage && (st == EState::Chase || st == EState::Patrol) && dir == -facing && GetRandomValue(0, 99) < 40) {
        invuln = 0.12f;
        w.fx.spark({pos.x - dir * 20.0f, pos.y - 80}, 0.4f, false);
        w.fx.text({pos.x, pos.y - 175}, "¡BLOQUEA!", pal::Cream, 28);
        w.assets->play(w.assets->block, 0.1f, 0.9f);
        st = EState::Windup; stT = 0; hitDone = false; dashAttack = false; comboHits = 0;
        vel.x = dir * 120.0f;
        return false;
    }
    hp -= dmg;
    flash = 0.12f;
    float resist = type == EType::Pesado ? 0.35f : (type == EType::Kage ? 0.4f : 1.0f);
    vel.x = dir * kb * resist;
    if (heavy && type != EType::Kage) vel.y = -220;
    bool armor = (type == EType::Pesado && !heavy) || (type == EType::Kage && st == EState::Attack);
    if (!armor && hp > 0) {
        st = EState::Stunned;
        stunT = type == EType::Kage ? 0.15f : stun;
        facing = -dir;
    }
    return hp <= 0;
}

Rig Enemy::pickRig(float dt) const {
    (void)dt;
    float spd = std::fabs(vel.x);
    const float ph = animT * (spd / 30.0f + 0.01f);
    switch (st) {
    case EState::Stunned: return rigs::hit(1.0f - stunT / 0.5f);
    case EState::Windup:
        if (type == EType::Pistolero) return rigs::aim(0);
        if (type == EType::Kage && phase >= 2) return rigs::swing(std::min(0.3f, stT));
        if (type == EType::Pesado) { Rig r = rigs::windup(stT / 0.55f); r.lean = -0.25f; return r; }
        return rigs::windup(stT / 0.32f);
    case EState::Attack:
        if (type == EType::Pistolero) return rigs::aim(1.0f - stT / 0.35f);
        if (type == EType::Pesado || dashAttack) return rigs::dash();
        if (type == EType::Kage && phase >= 2) return rigs::swing(0.3f + stT / 0.32f * 0.7f);
        return rigs::punch(std::min(1.0f, stT / 0.18f), comboHits % 2);
    default: break;
    }
    if (!onGround) return rigs::jump(vel.y);
    if (spd > 260) return rigs::run(ph * 1.2f);
    if (spd > 25) { Rig r = rigs::walk(animT * spd / 26.0f); if (type == EType::Pistolero) { r.ru = 1.2f; r.rl = 1.5f; } return r; }
    if (type == EType::Pistolero) return rigs::aim(0);
    return rigs::idle(animT + id);
}

void Enemy::draw(float time) const {
    (void)time;
    Color c = color();
    if (flash > 0) c = {pal::Red.r, pal::Red.g, pal::Red.b, c.a};
    float w = type == EType::Pesado ? 8.5f : (type == EType::Kage ? 7.0f : 5.5f);
    float hr = type == EType::Pesado ? 13.0f : (type == EType::Kage ? 12.5f : 10.5f);
    // mira láser del pistolero
    if (type == EType::Pistolero && st == EState::Windup) {
        Vector2 h = wpose.rh;
        float k = std::min(1.0f, stT / 0.5f);
        DrawLineEx(h, {h.x + std::cos(aimAngle) * 900, h.y + std::sin(aimAngle) * 900}, 1.5f + k, Color{215, 38, 61, static_cast<unsigned char>(60 + 140 * k)});
    }
    if (type == EType::Kage && invuln > 0) DrawCircleV(center(), 70, Color{215, 38, 61, static_cast<unsigned char>(50 + 30 * std::sin(time * 30))});
    drawPose(wpose, c, w, hr);
    if (type == EType::Kage || type == EType::Clon) {   // ojos rojos
        Vector2 hd = wpose.head;
        Color eye{230, 40, 50, static_cast<unsigned char>(255 * alpha)};
        DrawCircleV({hd.x + facing * 3.0f, hd.y - 2}, 2.6f, eye);
        DrawCircleV({hd.x + facing * 8.5f, hd.y - 2}, 2.6f, eye);
    }
    if (type == EType::Pesado) DrawLineEx({wpose.head.x - 13, wpose.head.y - 4}, {wpose.head.x + 13, wpose.head.y - 4}, 4, pal::RedDark);   // vincha
    drawWeapon(weapon(), wpose.re, wpose.rh, scale);
    // barra de vida pequeña (no para el jefe, que tiene la grande)
    if (type != EType::Kage && hp < maxHp && alive()) {
        float bw = 40 * scale;
        Vector2 at{pos.x - bw / 2, wpose.head.y - hr - 14};
        DrawRectangleV(at, {bw, 5}, Color{0, 0, 0, 90});
        DrawRectangleV(at, {bw * std::max(0.0f, hp / maxHp), 5}, pal::Red);
    }
}
