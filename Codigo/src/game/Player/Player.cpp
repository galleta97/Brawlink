#include "game/Player/Player.h"
#include <algorithm>
#include "core/Assets/Assets.h"
#include "core/Input/Input.h"

const char* weaponName(WeaponKind w) {
    switch (w) {
    case WeaponKind::Bate: return "BATE";
    case WeaponKind::Pistola: return "PISTOLA";
    case WeaponKind::Katana: return "KATANA";
    default: return "PUÑOS";
    }
}

int weaponDamage(WeaponKind w) {
    switch (w) {
    case WeaponKind::Bate: return 25;      // RF-005
    case WeaponKind::Pistola: return 20;
    case WeaponKind::Katana: return 35;
    default: return 0;
    }
}

Player::Player() { hp = maxHp = 100; }

Rectangle Player::hurtbox() const {
    float h = crouching ? 62.0f : 100.0f;
    return {pos.x - 16, pos.y - h, 32, h};
}

Rectangle Player::hitbox() const {
    float x0 = pos.x + facing * 8.0f, x1 = pos.x + facing * (8.0f + atk.range);
    return {std::min(x0, x1), pos.y + atk.top, std::fabs(x1 - x0), atk.bottom - atk.top};
}

void Player::startAttack(AtkType t, World& w) {
    Attack a;
    a.type = t;
    switch (t) {
    case AtkType::Punch:   a.dur = 0.26f; a.a0 = 0.07f; a.a1 = 0.15f; a.dmg = 8;  a.range = 52; a.kb = 170; a.top = -88; a.bottom = -45; a.stun = 0.3f;
        a.side = punchSide; punchSide ^= 1; vel.x += facing * 90; break;
    case AtkType::Kick:    a.dur = 0.40f; a.a0 = 0.13f; a.a1 = 0.27f; a.dmg = 12; a.range = 74; a.kb = 440; a.top = -80; a.bottom = -25; a.stun = 0.5f; a.heavy = true; break;
    case AtkType::AirKick: a.dur = 0.36f; a.a0 = 0.07f; a.a1 = 0.28f; a.dmg = 12; a.range = 62; a.kb = 380; a.top = -70; a.bottom = 5; a.stun = 0.45f; a.heavy = true; break;
    case AtkType::Swing: {
        bool katana = weapon == WeaponKind::Katana;
        a.dur = katana ? 0.34f : 0.42f; a.a0 = katana ? 0.12f : 0.15f; a.a1 = katana ? 0.24f : 0.30f;
        a.dmg = weaponDamage(weapon); a.range = katana ? 98 : 84; a.kb = katana ? 380 : 540;
        a.top = -120; a.bottom = -20; a.stun = 0.55f; a.heavy = true;
        w.assets->play(w.assets->whoosh, 0.15f, 0.8f);
        break;
    }
    case AtkType::Shoot:   a.dur = 0.22f; a.a0 = 0; a.a1 = 0; break;
    default: break;
    }
    atk = a;
    if (t == AtkType::Shoot) fire(w);
}

void Player::fire(World& w) {
    if (ammo <= 0) return;
    Vector2 muzzle{pos.x + facing * 46.0f, pos.y - 74.0f};
    w.bullets.push_back({muzzle, {facing * 1500.0f, 0}, weaponDamage(WeaponKind::Pistola), true});
    w.fx.spark({muzzle.x + facing * 10, muzzle.y}, 0.35f, false);
    w.assets->play(w.assets->shot, 0.05f, 0.7f);
    w.shake = std::max(w.shake, 3.0f);
    vel.x -= facing * 60;
    if (--ammo <= 0) {   // RF-005: sin balas se descarta
        weapon = WeaponKind::None;
        w.fx.text({pos.x, pos.y - 150}, "¡SIN BALAS!", pal::Red, 30);
    }
}

void Player::update(float dt, Input& in, World& w) {
    animT += dt;
    hurtT = std::max(0.0f, hurtT - dt);
    invuln = std::max(0.0f, invuln - dt);
    blockBroken = std::max(0.0f, blockBroken - dt);
    dmgFlash = std::max(0.0f, dmgFlash - dt);
    jumpBuf = std::max(0.0f, jumpBuf - dt);
    coyote = onGround ? 0.1f : std::max(0.0f, coyote - dt);

    // estelas (ghosts) detrás del jugador
    for (auto& g : ghosts) g.life -= dt;
    while (!ghosts.empty() && ghosts.front().life <= 0) ghosts.pop_front();
    ghostT -= dt;
    bool fast = std::fabs(vel.x) > 300 || (atk.type != AtkType::None && atk.type != AtkType::Shoot) || w.bulletTime;
    if (fast && ghostT <= 0 && alive()) { ghosts.push_back({wpose, 0.18f}); ghostT = 0.035f; if (ghosts.size() > 5) ghosts.pop_front(); }

    if (!alive()) {
        physics(dt, w, 0.85f);
        animate(rigs::hit(0), dt);
        return;
    }

    if (hurtT > 0 || blockBroken > 0) {
        atk.type = AtkType::None; blocking = false; crouching = false;
        physics(dt, w, 0.88f);
        animate(rigs::hit(1.0f - std::max(hurtT, blockBroken) / 0.45f), dt, 30);
        return;
    }

    if (in.pressed(Act::Jump)) jumpBuf = 0.12f;
    const float axis = in.moveAxis();
    running = in.running() && axis != 0;

    // --- ataques (guarda la siguiente orden para encadenar combos) ---
    auto wanted = AtkType::None;
    if (in.pressed(Act::Punch)) {
        if (weapon == WeaponKind::Pistola) wanted = AtkType::Shoot;
        else if (weapon != WeaponKind::None) wanted = AtkType::Swing;
        else wanted = AtkType::Punch;
    }
    if (in.pressed(Act::Kick)) wanted = onGround ? AtkType::Kick : AtkType::AirKick;
    if (wanted != AtkType::None) {
        if (atk.type == AtkType::None) startAttack(wanted, w);
        else queued = wanted;
    }
    if (atk.type != AtkType::None) {
        atk.t += dt;
        bool canChain = atk.t >= (atk.a1 > 0 ? atk.a1 : atk.dur * 0.6f);
        if (queued != AtkType::None && canChain) {
            AtkType q = queued; queued = AtkType::None;
            if (q == AtkType::Kick && !onGround) q = AtkType::AirKick;
            startAttack(q, w);
        } else if (atk.t >= atk.dur) {
            atk.type = AtkType::None;
        }
    }
    if (atk.type == AtkType::AirKick && onGround && atk.t > 0.1f) atk.type = AtkType::None;

    blocking = in.held(Act::Block) && onGround && atk.type == AtkType::None;
    crouching = in.held(Act::Down) && onGround && atk.type == AtkType::None && !blocking;

    // --- movimiento (RF-003) ---
    bool attacking = atk.type != AtkType::None;
    if (!attacking && !blocking && !crouching) {
        float target = axis * (running ? 440.0f : 270.0f);
        float accel = onGround ? 18.0f : 9.0f;
        vel.x += (target - vel.x) * std::min(1.0f, accel * dt);
        if (axis != 0) facing = axis > 0 ? 1 : -1;
    } else if (!onGround && axis != 0) {
        vel.x += (axis * 270.0f - vel.x) * std::min(1.0f, 4.0f * dt);
    }

    if (jumpBuf > 0 && coyote > 0 && !blocking && (atk.type == AtkType::None || atk.type == AtkType::Shoot)) {
        bool onThin = false;
        for (auto& p : w.plats) if (p.oneWay && std::fabs(pos.y - p.r.y) < 2 && pos.x >= p.r.x && pos.x <= p.r.x + p.r.width) onThin = true;
        if (in.held(Act::Down) && onThin) { dropT = 0.25f; pos.y += 2; }
        else { vel.y = -850; w.fx.dust(pos, 5); w.assets->play(w.assets->whoosh, 0.1f, 0.4f); }
        jumpBuf = 0; coyote = 0; onGround = false; crouching = false;
    }
    if (vel.y < 0 && !in.held(Act::Jump)) vel.y += cfg::GRAVITY * 1.3f * dt;   // salto corto si se suelta

    // --- armas (RF-005) ---
    if (in.pressed(Act::Pickup)) {
        int best = -1; float bd = 70;
        for (size_t i = 0; i < w.pickups.size(); ++i) {
            float d = std::fabs(w.pickups[i].p.x - pos.x);
            if (d < bd && std::fabs(w.pickups[i].p.y - pos.y) < 60) { bd = d; best = static_cast<int>(i); }
        }
        if (best >= 0) {
            Pickup got = w.pickups[best];
            w.pickups.erase(w.pickups.begin() + best);
            if (weapon != WeaponKind::None) w.pickups.push_back({{pos.x, pos.y}, weapon, ammo});
            weapon = got.w; ammo = got.ammo;
            w.fx.text({pos.x, pos.y - 150}, weaponName(weapon), pal::Ink, 32);
            w.assets->play(w.assets->pickup, 0.0f, 0.8f);
        } else if (weapon != WeaponKind::None) {
            w.pickups.push_back({{pos.x + facing * 30.0f, pos.y}, weapon, ammo});
            weapon = WeaponKind::None; ammo = 0;
        }
    }

    bool wasAir = !onGround;
    physics(dt, w, (attacking || blocking || crouching) ? 0.82f : 0.75f);
    if (wasAir && onGround) w.fx.dust(pos, 6);

    // --- animación ---
    Rig target;
    float spd = std::fabs(vel.x);
    switch (atk.type) {
    case AtkType::Punch:   target = rigs::punch(atk.t / atk.dur, atk.side); break;
    case AtkType::Kick:    target = rigs::kick(atk.t / atk.dur); break;
    case AtkType::AirKick: target = rigs::airKick(atk.t / atk.dur); break;
    case AtkType::Swing:   target = rigs::swing(atk.t / atk.dur); break;
    case AtkType::Shoot:   target = rigs::aim(1.0f - atk.t / atk.dur); break;
    default:
        if (!onGround) target = rigs::jump(vel.y);
        else if (blocking) target = rigs::block();
        else if (crouching) target = rigs::crouch();
        else if (spd > 330) { phase += dt * spd / 34.0f; target = rigs::run(phase); }
        else if (spd > 30) { phase += dt * spd / 30.0f; target = rigs::walk(phase); }
        else target = weapon == WeaponKind::Pistola ? rigs::aim(0) : rigs::idle(animT);
    }
    animate(target, dt, attacking ? 34.0f : 20.0f);
}

int Player::takeHit(int dmg, float fromX, bool breaksBlock, float kb, World& w) {
    if (invuln > 0 || !alive()) return 0;
    int dir = fromX < pos.x ? 1 : -1;            // hacia dónde sale empujado
    bool front = (fromX - pos.x) * facing > 0;
    Vector2 at{pos.x, pos.y - 70};
    if (blocking && front && !breaksBlock) {
        int d = std::max(1, dmg / 5);            // bloquear reduce el daño
        hp -= d;
        vel.x = dir * kb * 0.3f;
        w.fx.spark({at.x - dir * 18.0f, at.y}, 0.35f, false);
        w.assets->play(w.assets->block, 0.1f, 0.8f);
        return d;
    }
    if (blocking && breaksBlock) {
        blockBroken = 0.6f;                      // el pesado rompe el bloqueo
        w.fx.text({pos.x, pos.y - 150}, "¡GUARDIA ROTA!", pal::Red, 30);
    }
    hp -= dmg;
    hurtT = 0.32f; invuln = 0.45f; dmgFlash = 0.15f;
    vel.x = dir * kb; vel.y = -180;
    blocking = false;
    w.fx.spark(at, 0.6f, false);
    w.fx.ink(at, 5, static_cast<float>(dir));
    w.assets->play(w.assets->hurt, 0.1f, 0.9f);
    w.shake = std::max(w.shake, 7.0f);
    if (hp <= 0) { hp = 0; w.assets->play(w.assets->ko, 0.0f, 1.0f); }
    return dmg;
}

void Player::draw(float time, bool bt) const {
    for (const auto& g : ghosts) {
        float a = g.life / 0.18f;
        drawPose(g.p, Color{150, 140, 128, static_cast<unsigned char>((bt ? 120 : 80) * a)}, 6, 11);
    }
    if (!alive()) return;
    bool blink = invuln > 0 && std::fmod(time, 0.1f) < 0.05f;
    Color body = dmgFlash > 0 ? pal::Red : pal::Ink;
    if (blink) body.a = 120;
    drawScarf(wpose, facing, time, vel.x, 1.0f);
    drawPose(wpose, body, 6, 11);
    drawWeapon(weapon, wpose.re, wpose.rh, 1.0f);
}
