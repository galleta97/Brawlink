// El juego: niveles por oleadas, combate, tiempo bala, jefe (MK-03, MK-04, MK-05)
#include <algorithm>
#include <cmath>
#include "core/Config/Config.h"
#include "core/Game/Game.h"
#include "game/MatchValidator/MatchValidator.h"
#include "scenes/Scenes/Scenes.h"

namespace {
Color alphaC(Color c, float a) { c.a = static_cast<unsigned char>(c.a * std::clamp(a, 0.0f, 1.0f)); return c; }
float frand(float a, float b) { return a + (b - a) * (GetRandomValue(0, 10000) / 10000.0f); }
}

LevelScene::LevelScene(Game& g, int level) : g_(g), level_(level), def_(makeLevel(level)) {}

void LevelScene::onEnter() {
    world_.plats = def_.plats;
    world_.width = def_.width;
    world_.pickups = def_.pickups;
    world_.assets = &g_.assets();
    back_.build(def_.theme, def_.width);
    player_.pos = {220, cfg::GROUND_Y};
    player_.animate(rigs::idle(0), 1.0f);
    bt_.addFury(0.5f);   // empieza con media barra para poder mostrar el tiempo bala
    camX_ = 0;
    bannerText_ = "NIVEL " + std::to_string(level_) + " · " + upper(def_.name);
    bannerColor_ = pal::Ink;
    banner_ = 1.6f;
    g_.input().setVirtual(g_.options().autoplay);
}

void LevelScene::spawn(EType t, float x) {
    if (x < 0) {
        bool left = (nextId_ % 2) == 0;
        x = left ? camX_ - 70 : camX_ + cfg::W + 70;
        x = std::clamp(x, 40.0f, def_.width - 40);
        if (std::fabs(x - player_.pos.x) < 260) x = std::clamp(left ? camX_ + cfg::W + 70 : camX_ - 70, 40.0f, def_.width - 40);
    }
    Enemy e(t, x, nextId_++);
    e.facing = player_.pos.x >= x ? 1 : -1;
    e.animate(rigs::idle(0), 1.0f);
    if (t == EType::Clon || t == EType::Kage) world_.fx.smoke({x, cfg::GROUND_Y}, 10, Color{40, 20, 40, 150});
    enemies_.push_back(e);
}

void LevelScene::startWave() {
    ++wave_;
    const auto& w = def_.waves[wave_ - 1];
    queue_.clear();
    float delay = 0.2f;
    // intercala los tipos para que no lleguen todos iguales juntos
    std::vector<EType> list;
    size_t maxN = 0;
    for (auto& [t, n] : w.enemies) maxN = std::max(maxN, static_cast<size_t>(n));
    for (size_t i = 0; i < maxN; ++i)
        for (auto& [t, n] : w.enemies) if (static_cast<int>(i) < n) list.push_back(t);
    for (EType t : list) { queue_.push_back({t, delay}); delay = 0.55f; }
    if (def_.theme == Theme::Kage) bannerText_ = "KAGE · MAESTRO DE LA SOMBRA";
    else bannerText_ = "OLEADA " + std::to_string(wave_) + " / " + std::to_string(def_.waves.size());
    bannerColor_ = pal::Red;
    banner_ = 1.4f;
}

void LevelScene::killEnemy(Enemy& e, float dir, float force) {
    const Assets& a = g_.assets();
    ++kills_;
    long long pts = static_cast<long long>(e.killScore() * combo_.multiplier());
    score_ += pts;
    world_.fx.text({e.pos.x, e.pos.y - 160 * e.scale}, "+" + std::to_string(pts), pal::Red, 34);
    if (e.type == EType::Clon) {
        world_.fx.smoke(e.pos, 12, Color{50, 30, 60, 160});
    } else {
        float w = e.type == EType::Pesado ? 8.5f : (e.type == EType::Kage ? 7.0f : 5.5f);
        float hr = e.type == EType::Pesado ? 13.0f : (e.type == EType::Kage ? 12.5f : 10.5f);
        world_.addRagdoll(e.wpose, e.vel, {dir * force, -force * 0.55f}, e.color(), w, hr);
    }
    a.play(a.ko, 0.1f, 0.9f);
    if (e.type == EType::Pistolero && GetRandomValue(0, 1) == 0)   // a veces suelta la pistola
        world_.pickups.push_back({{e.pos.x, e.pos.y}, WeaponKind::Pistola, 6});
    if (e.type == EType::Kage) {
        world_.hitStop = 0.35f;
        world_.shake = 20;
        world_.fx.ring(e.center(), 600, pal::Red);
        for (auto& o : enemies_) if (o.type == EType::Clon && o.alive()) { o.hp = 0; world_.fx.smoke(o.pos, 10, Color{50, 30, 60, 160}); }
    }
}

void LevelScene::resolvePlayerAttacks() {
    if (!player_.attackActive()) return;
    const Rectangle hb = player_.hitbox();
    const Assets& a = g_.assets();
    for (auto& e : enemies_) {
        if (!e.alive()) continue;
        auto& ids = player_.atk.hitIds;
        if (std::find(ids.begin(), ids.end(), e.id) != ids.end()) continue;
        Rectangle eh = e.hurtbox();
        if (!CheckCollisionRecs(hb, eh)) continue;
        ids.push_back(e.id);
        float before = e.hp;
        bool died = e.takeHit(player_.atk.dmg, player_.pos.x, player_.atk.kb, player_.atk.stun, player_.atk.heavy, world_);
        if (e.hp >= before) continue;   // inmune
        combo_.registerHit();
        bt_.addFury(0.06f);
        long long pts = static_cast<long long>(player_.atk.dmg * 10 * combo_.multiplier());
        score_ += pts;
        Vector2 at{(std::max(hb.x, eh.x) + std::min(hb.x + hb.width, eh.x + eh.width)) / 2, std::max(hb.y, eh.y) + 18};
        bool heavy = player_.atk.heavy;
        world_.fx.spark(at, heavy ? 1.0f : 0.7f, heavy);
        world_.fx.ink(at, heavy ? 7 : 4, static_cast<float>(player_.facing));
        if (heavy) world_.fx.text({at.x, at.y - 70}, "+" + std::to_string(pts), pal::Red, 28);
        switch (player_.atk.type) {
        case AtkType::Kick: case AtkType::AirKick: a.play(a.kick); break;
        case AtkType::Swing: a.play(player_.weapon == WeaponKind::Katana ? a.slash : a.kick, 0.1f, 1.0f); break;
        default: a.play(a.punch);
        }
        world_.hitStop = std::max(world_.hitStop, heavy ? 0.07f : 0.035f);
        world_.shake = std::max(world_.shake, heavy ? 8.0f : 4.0f);
        if (died) killEnemy(e, static_cast<float>(player_.facing), heavy ? 520.0f : 360.0f);
    }
}

void LevelScene::updateBullets(float dt) {
    const Assets& a = g_.assets();
    for (auto& b : world_.bullets) {
        if (!b.alive) continue;
        b.p.x += b.v.x * dt; b.p.y += b.v.y * dt; b.life -= dt;
        if (b.life <= 0 || b.p.x < -50 || b.p.x > def_.width + 50) { b.alive = false; continue; }
        if (b.p.y >= cfg::GROUND_Y) { b.alive = false; world_.fx.dust({b.p.x, cfg::GROUND_Y}, 3); continue; }
        if (b.fromPlayer) {
            for (auto& e : enemies_) {
                if (!e.alive() || !CheckCollisionPointRec(b.p, e.hurtbox())) continue;
                b.alive = false;
                float before = e.hp;
                bool died = e.takeHit(b.dmg, b.p.x - b.v.x * 0.05f, 260, 0.3f, false, world_);
                if (e.hp < before) {
                    combo_.registerHit(); bt_.addFury(0.04f);
                    score_ += static_cast<long long>(b.dmg * 10 * combo_.multiplier());
                    world_.fx.spark(b.p, 0.6f, false); world_.fx.ink(b.p, 4, b.v.x > 0 ? 1.0f : -1.0f);
                    a.play(a.punch);
                    if (died) killEnemy(e, b.v.x > 0 ? 1.0f : -1.0f, 420);
                }
                break;
            }
        } else if (player_.alive()) {
            Rectangle ph = player_.hurtbox();
            if (CheckCollisionPointRec(b.p, ph)) {
                b.alive = false;
                player_.takeHit(b.dmg, b.p.x - b.v.x * 0.05f, false, 220, world_);
            } else if (!b.nearMissDone && std::fabs(b.p.x - player_.pos.x) < 26 && std::fabs(b.p.y - (player_.pos.y - 70)) < 60) {
                b.nearMissDone = true;
                if (bt_.active() || player_.crouching || !player_.onGround) {   // la esquivó a propósito
                    dodgeT_ = 1.2f;
                    score_ += 100;
                    bt_.addFury(0.05f);
                }
            }
        }
    }
    world_.bullets.erase(std::remove_if(world_.bullets.begin(), world_.bullets.end(), [](const Bullet& b) { return !b.alive; }), world_.bullets.end());
}

void LevelScene::autopilot(float dt) {
    Input& in = g_.input();
    in.virtualClearHeld();
    if (phase_ != Phase::Play || !player_.alive()) return;
    apT_ -= dt; apJump_ -= dt;
    // recoger un arma cercana si no tiene
    if (player_.weapon == WeaponKind::None) {
        for (auto& p : world_.pickups) {
            float d = p.p.x - player_.pos.x;
            if (std::fabs(d) < 350 && std::fabs(p.p.y - player_.pos.y) < 10) {
                if (std::fabs(d) < 40) in.virtualSet(Act::Pickup, true, true);
                else in.virtualSet(d > 0 ? Act::Right : Act::Left, true, false);
                return;
            }
        }
    }
    const Enemy* t = nullptr;
    for (auto& e : enemies_) if (e.alive() && (!t || std::fabs(e.pos.x - player_.pos.x) < std::fabs(t->pos.x - player_.pos.x))) t = &e;
    for (auto& b : world_.bullets)
        if (!b.fromPlayer && std::fabs(b.p.x - player_.pos.x) < 260 && (b.v.x > 0) == (b.p.x < player_.pos.x)) {
            if (bt_.fury() > 0.15f) in.virtualSet(Act::Bullet, true, false);
            if (std::fabs(b.p.x - player_.pos.x) < 140) { in.virtualSet(Act::Down, true, false); return; }
        }
    if (!t) return;
    float dx = t->pos.x - player_.pos.x;
    float reach = player_.weapon == WeaponKind::Pistola ? 380.0f : (player_.weapon != WeaponKind::None ? 85.0f : 62.0f);
    if (t->pos.y < player_.pos.y - 60 && apJump_ <= 0 && std::fabs(dx) < 160) { in.virtualSet(Act::Jump, true, true); apJump_ = 1.0f; }
    if (std::fabs(dx) > reach || (dx > 0) != (player_.facing > 0)) {
        in.virtualSet(dx > 0 ? Act::Right : Act::Left, true, false);
        in.virtualRun(std::fabs(dx) > 320);
        return;
    }
    if (t->type == EType::Kage) {   // contra el jefe: bloquear sus ataques y castigar cuando se recupera
        bool open = t->st == EState::Recover || t->st == EState::Stunned;
        if (!open) { if (std::fabs(dx) < 160) in.virtualSet(Act::Block, true, false); return; }
    } else if (t->st == EState::Windup && t->type != EType::Pistolero && GetRandomValue(0, 3) == 0) { in.virtualSet(Act::Block, true, false); return; }
    if (apT_ <= 0) {
        apT_ = 0.17f;
        ++apToggle_;
        in.virtualSet(apToggle_ % 4 == 3 ? Act::Kick : Act::Punch, true, true);
    }
}

void LevelScene::finish(bool win) {
    MatchStats s;
    s.level = level_; s.win = win; s.score = score_;
    s.comboMax = combo_.maxCombo(); s.kills = kills_; s.seconds = playTime_;
    s.valid = !win || MatchValidator::valid(score_, playTime_, def_.maxScore, def_.minSeconds);
    if (!s.valid) TraceLog(LOG_WARNING, "BRAWLINK: partida sospechosa (nivel %d, puntaje %lld, %.1f s)", level_, score_, playTime_);
    if (win && !g_.guest) {
        g_.profiles().unlock(g_.player, std::min(level_ + 1, LEVEL_COUNT));   // RF-009
        if (s.valid) {
            RankEntry e{g_.player, level_, s.comboMax, static_cast<int>(playTime_), score_, std::time(nullptr)};
            g_.ranking().add(e);
            s.saved = true;
        }
    }
    g_.input().setVirtual(false);
    g_.scenes().change(std::make_unique<ResultScene>(g_, s));
}

void LevelScene::update(float dt) {
    time_ += dt;
    Input& in = g_.input();
    if (g_.options().autoplay) autopilot(dt);
    if (in.pressed(Act::Pause) && phase_ == Phase::Play) { g_.scenes().push(std::make_unique<PauseScene>(g_, level_)); return; }

    banner_ = std::max(0.0f, banner_ - dt);
    btTitle_ = std::max(0.0f, btTitle_ - dt);
    dodgeT_ = std::max(0.0f, dodgeT_ - dt);
    world_.shake = std::max(0.0f, world_.shake - dt * 40);
    hud_.update(dt, combo_.count());
    if (world_.hitStop > 0) { world_.hitStop -= dt; return; }   // congelado breve al pegar

    bt_.update(dt, in.held(Act::Bullet) && phase_ == Phase::Play);
    if (bt_.active() && !btWas_) { g_.assets().play(g_.assets().slowmo, 0, 0.8f); btTitle_ = 1.0f; }
    btWas_ = bt_.active();
    world_.bulletTime = bt_.active();

    float gdt = dt * bt_.scale();
    float pdt = bt_.active() ? dt * cfg::PLAYER_BT_SCALE : dt;
    if (phase_ == Phase::Dead) gdt = pdt = dt * 0.35f;

    int attacking = 0;
    for (auto& e : enemies_) if (e.type != EType::Kage && e.alive() && e.attacking()) ++attacking;
    world_.attackTokens = cfg::MAX_ATTACKERS - attacking;

    float hpBefore = player_.hp;
    player_.update(pdt, in, world_);
    for (auto& e : enemies_) if (e.alive()) e.update(gdt, world_, player_);
    // separación entre enemigos para que no se amontonen
    for (size_t i = 0; i < enemies_.size(); ++i)
        for (size_t j = i + 1; j < enemies_.size(); ++j) {
            auto& a = enemies_[i]; auto& b = enemies_[j];
            float d = b.pos.x - a.pos.x;
            if (std::fabs(d) < 34 && std::fabs(a.pos.y - b.pos.y) < 40) {
                float push = (34 - std::fabs(d)) * 0.5f * (d >= 0 ? 1 : -1);
                a.pos.x -= push; b.pos.x += push;
            }
        }
    resolvePlayerAttacks();
    updateBullets(gdt);
    if (player_.hp < hpBefore) combo_.breakCombo();   // al recibir daño se pierde el combo

    for (auto& req : world_.spawnRequests) {
        int clones = 0;
        for (auto& e : enemies_) if (e.type == EType::Clon && e.alive()) ++clones;
        if (clones < 4) spawn(static_cast<EType>(req.type), std::clamp(req.x, 40.0f, def_.width - 40));
    }
    world_.spawnRequests.clear();
    enemies_.erase(std::remove_if(enemies_.begin(), enemies_.end(), [](const Enemy& e) { return !e.alive(); }), enemies_.end());

    for (auto& r : world_.ragdolls) r.update(gdt, world_.plats);
    while (!world_.ragdolls.empty() && world_.ragdolls.front().dead()) world_.ragdolls.pop_front();
    for (auto& p : world_.pickups) p.bob += dt;
    world_.fx.update(gdt);
    combo_.update(gdt);
    for (auto& [s, c] : world_.banners) { bannerText_ = s; bannerColor_ = c; banner_ = 1.8f; }
    world_.banners.clear();

    // cámara
    float target = player_.pos.x - cfg::W / 2.0f + player_.facing * 90.0f;
    camX_ += (target - camX_) * std::min(1.0f, dt * 5.0f);
    camX_ = std::clamp(camX_, 0.0f, std::max(0.0f, def_.width - cfg::W));

    // fases del nivel y oleadas (RF-008)
    phaseT_ += dt;
    switch (phase_) {
    case Phase::Intro:
        if (phaseT_ > 1.3f) { phase_ = Phase::Play; phaseT_ = 0; startWave(); }
        break;
    case Phase::Play:
        playTime_ += dt;
        if (!queue_.empty()) {
            queue_.front().delay -= gdt;
            if (queue_.front().delay <= 0) {
                EType t = queue_.front().t;
                queue_.erase(queue_.begin());
                spawn(t, t == EType::Kage ? def_.width * 0.72f : -1.0f);
            }
        } else if (enemies_.empty()) {
            waveGap_ += dt;
            if (static_cast<size_t>(wave_) < def_.waves.size()) {
                if (waveGap_ > 1.2f) { waveGap_ = 0; startWave(); }
            } else {
                phase_ = Phase::Clear; phaseT_ = 0;
                bannerText_ = def_.theme == Theme::Kage ? "¡KAGE HA CAÍDO!" : "¡NIVEL SUPERADO!";
                bannerColor_ = pal::Red; banner_ = 2.4f;
                g_.assets().play(g_.assets().select, 0, 1.0f);
            }
        }
        if (!player_.alive()) {
            phase_ = Phase::Dead; phaseT_ = 0;
            bannerText_ = "K.O."; bannerColor_ = pal::Red; banner_ = 2.0f;
        }
        break;
    case Phase::Clear:
        if (phaseT_ > 2.4f) finish(true);
        break;
    case Phase::Dead:
        if (phaseT_ > 2.0f) finish(false);
        break;
    }
}

void LevelScene::drawTutorial() const {
    if (level_ != 1 || wave_ > 2 || phase_ != Phase::Play) return;
    static const char* tips[] = {
        "A / D moverse  ·  doble toque para correr  ·  ESPACIO saltar",
        "J puño  ·  K patada  —  ¡encadena golpes para subir el combo!",
        "L (mantener) bloquear  ·  S agacharse",
        "E recoger el bate  ·  J para golpear con el arma",
        "SHIFT (mantener) TIEMPO BALA: gasta la barra de furia",
    };
    int i = static_cast<int>(playTime_ / 5.0f) % 5;
    const Assets& a = g_.assets();
    float w = textWidth(a.bold, tips[i], 26) + 60;
    skewRect(cfg::W / 2.0f - w / 2, cfg::H - 66.0f, w, 44, 10, Color{20, 20, 20, 225});
    text(a.bold, tips[i], cfg::W / 2.0f + 5, cfg::H - 58.0f, 26, pal::Cream, Align::Center);
}

void LevelScene::draw() {
    const Assets& a = g_.assets();
    back_.drawFar(camX_, time_);

    Camera2D cam{};
    cam.zoom = 1.0f;
    float sx = world_.shake > 0 ? frand(-world_.shake, world_.shake) : 0;
    float sy = world_.shake > 0 ? frand(-world_.shake, world_.shake) * 0.6f : 0;
    cam.target = {std::round(camX_ + sx), std::round(sy)};
    BeginMode2D(cam);
    back_.drawWorld(world_.plats, camX_);

    for (const auto& p : world_.pickups) {
        float y = p.p.y - 14 - std::sin(p.bob * 4) * 4;
        DrawEllipse(static_cast<int>(p.p.x), static_cast<int>(p.p.y - 2), 34, 6, Color{0, 0, 0, 50});
        DrawCircleV({p.p.x, y}, 30, Color{246, 196, 66, static_cast<unsigned char>(60 + 30 * std::sin(p.bob * 5))});
        float half = p.w == WeaponKind::Pistola ? 10.0f : (p.w == WeaponKind::Katana ? 40.0f : 30.0f);
        drawWeapon(p.w, {p.p.x - half - 10, y}, {p.p.x - half, y}, 1.0f);
        if (std::fabs(p.p.x - player_.pos.x) < 70 && std::fabs(p.p.y - player_.pos.y) < 60) {
            DrawRectangle(static_cast<int>(p.p.x - 14), static_cast<int>(y - 66), 28, 28, pal::Ink);
            text(a.title, "E", p.p.x, y - 66, 26, pal::Yellow, Align::Center);
        }
    }
    for (const auto& r : world_.ragdolls) r.draw();
    for (const auto& e : enemies_) e.draw(time_);
    player_.draw(time_, bt_.active());
    for (const auto& b : world_.bullets) {
        float trail = bt_.active() ? 0.12f : 0.03f;
        Vector2 tail{b.p.x - b.v.x * trail, b.p.y - b.v.y * trail};
        DrawLineEx(tail, b.p, bt_.active() ? 3.0f : 4.0f, alphaC(pal::Yellow, 0.8f));
        DrawCircleV(b.p, 4, b.fromPlayer ? pal::Yellow : Color{255, 230, 150, 255});
        if (bt_.active()) DrawRing(b.p, 9, 11, 0, 360, 20, alphaC(pal::Yellow, 0.5f));
    }
    world_.fx.draw(a);
    EndMode2D();

    // efecto de tiempo bala (MK-04)
    float k = bt_.blend();
    if (k > 0.01f) {
        DrawRectangle(0, 0, cfg::W, cfg::H, Color{40, 54, 88, static_cast<unsigned char>(120 * k)});
        DrawRectangleGradientV(0, 0, cfg::W, 160, Color{10, 14, 30, static_cast<unsigned char>(130 * k)}, Color{10, 14, 30, 0});
        DrawRectangleGradientV(0, cfg::H - 160, cfg::W, 160, Color{10, 14, 30, 0}, Color{10, 14, 30, static_cast<unsigned char>(150 * k)});
        text(a.title, "TIEMPO BALA", cfg::W / 2.0f + 80, 96, 76, alphaC(pal::White, k), Align::Center);
        text(a.title, "X0.25", cfg::W / 2.0f + 80, 172, 34, alphaC(pal::Yellow, k), Align::Center);
    }
    if (dodgeT_ > 0) {
        Vector2 pp{player_.pos.x - camX_, player_.pos.y - 190};
        DrawTextPro(a.title, "¡ESQUIVE PERFECTO!", pp, {0, 0}, -8, 38, 0, alphaC(pal::Yellow, dodgeT_));
    }

    HudData d;
    d.player = g_.player; d.hp = player_.hp; d.maxHp = player_.maxHp; d.fury = bt_.fury();
    d.levelTitle = def_.theme == Theme::Kage ? "NIVEL 4 · JEFE FINAL" : "NIVEL " + std::to_string(level_) + " · " + upper(def_.name);
    d.score = score_; d.combo = combo_.count(); d.comboLeft = combo_.timeLeft(); d.multiplier = combo_.multiplier();
    d.weapon = player_.weapon; d.ammo = player_.ammo;
    d.wave = wave_; d.waves = static_cast<int>(def_.waves.size());
    d.boss = def_.theme == Theme::Kage; d.bossHp = 0; d.bossMax = 1; d.bossPhase = 1; d.time = time_;
    d.dark = def_.theme == Theme::Kage || k > 0.5f;
    for (const auto& e : enemies_)
        if (e.type == EType::Kage) { d.boss = true; d.bossHp = e.hp; d.bossMax = e.maxHp; d.bossPhase = e.phase; }
    hud_.draw(a, d);
    drawTutorial();

    if (banner_ > 0) {
        float in = std::min(1.0f, (banner_) * 4.0f);
        float w = textWidth(a.title, bannerText_, 64) + 120;
        skewRect(cfg::W / 2.0f - w / 2, 300, w, 92, 22, alphaC(pal::Ink, in * 0.92f));
        text(a.title, bannerText_, cfg::W / 2.0f + 10, 310, 64, alphaC(bannerColor_.r == pal::Ink.r ? pal::Cream : bannerColor_, in), Align::Center);
    }
    if (phase_ == Phase::Dead) DrawRectangle(0, 0, cfg::W, cfg::H, Color{120, 0, 10, static_cast<unsigned char>(std::min(90.0f, phaseT_ * 80))});
    if (g_.options().autoplay) text(a.ui, "DEMO AUTOMÁTICA", 20, cfg::H - 30.0f, 18, pal::Red);
}
