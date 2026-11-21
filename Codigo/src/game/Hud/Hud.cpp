#include "game/Hud/Hud.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include "core/Config/Config.h"
#include "game/Player/Player.h"

void Hud::update(float dt, int combo) {
    if (combo > lastCombo_) comboPop_ = 1.0f;
    lastCombo_ = combo;
    comboPop_ = std::max(0.0f, comboPop_ - dt * 5.0f);
    scoreShown_ += (scoreTarget_ - scoreShown_) * std::min(1.0f, dt * 10.0f);
}

static const char* comboWord(int c) {
    if (c >= 20) return "¡IMPARABLE!";
    if (c >= 10) return "¡BRUTAL!";
    if (c >= 5) return "¡GENIAL!";
    if (c >= 3) return "¡BIEN!";
    return "";
}

void Hud::draw(const Assets& a, const HudData& d) const {
    scoreTarget_ = static_cast<float>(d.score);

    // Panel del jugador (arriba a la izquierda)
    skewRect(14, 14, 266, 64, 10, pal::Ink);
    text(a.bold, upper(d.player), 34, 20, 22, pal::Cream);
    char buf[32];
    snprintf(buf, sizeof buf, "VIDA %d", static_cast<int>(std::ceil(d.hp)));
    text(a.bold, buf, 262, 20, 18, pal::Cream, Align::Right);
    float hpk = std::clamp(d.hp / d.maxHp, 0.0f, 1.0f);
    DrawRectangle(34, 46, 226, 9, Color{70, 66, 62, 255});
    DrawRectangle(34, 46, static_cast<int>(226 * hpk), 9, hpk < 0.3f ? pal::Red : pal::Cream);
    // barra de furia en 4 segmentos (tiempo bala)
    for (int i = 0; i < 4; ++i) {
        float seg = std::clamp(d.fury * 4 - i, 0.0f, 1.0f);
        float x = 34 + i * 57.0f;
        DrawRectangle(static_cast<int>(x), 61, 52, 6, Color{70, 66, 62, 255});
        DrawRectangle(static_cast<int>(x), 61, static_cast<int>(52 * seg), 6, d.fury >= 0.999f ? pal::Yellow : pal::Red);
    }

    const Color fg = d.dark ? pal::Cream : pal::Ink;
    const Color sub = d.dark ? Color{220, 200, 190, 255} : Color{90, 84, 78, 255};
    // Título del nivel
    text(a.title, d.levelTitle, cfg::W / 2.0f, 18, 32, fg, Align::Center);
    if (!d.boss) {
        snprintf(buf, sizeof buf, "OLEADA %d / %d", std::min(d.wave, d.waves), d.waves);
        text(a.bold, buf, cfg::W / 2.0f, 54, 20, sub, Align::Center);
    }

    // Puntaje
    text(a.bold, "PUNTAJE", cfg::W - 24.0f, 16, 16, sub, Align::Right);
    text(a.title, fmtThousands(static_cast<long long>(scoreShown_ + 0.5f)), cfg::W - 24.0f, 32, 40, fg, Align::Right);

    // Combo grande en rojo (MK-03)
    if (d.combo >= 2) {
        float s = 1.0f + comboPop_ * 0.35f;
        snprintf(buf, sizeof buf, "X%d", d.combo);
        textShadow(a.title, buf, cfg::W - 34.0f, 100, 88 * s, pal::Red, pal::Ink, {4, 4}, Align::Right);
        const char* w = comboWord(d.combo);
        if (*w) text(a.title, w, cfg::W - 34.0f, 100 + 84 * s, 34, fg, Align::Right);
        DrawRectangle(cfg::W - 34 - 120, static_cast<int>(100 + 84 * s + 42), static_cast<int>(120 * d.comboLeft), 4, pal::Red);
        snprintf(buf, sizeof buf, "×%.1f", d.multiplier);
        text(a.bold, buf, cfg::W - 34.0f, 100 + 84 * s + 50, 20, sub, Align::Right);
    }

    // Arma (abajo a la izquierda)
    if (!d.boss) {
        skewRect(14, cfg::H - 62.0f, 210, 44, 8, Color{20, 20, 20, 210});
        std::string wtxt = weaponName(d.weapon);
        if (d.weapon == WeaponKind::Pistola) wtxt += "  " + std::to_string(d.ammo) + "/12";
        text(a.bold, wtxt, 34, cfg::H - 54.0f, 26, d.weapon == WeaponKind::None ? pal::Cream : pal::Yellow);
    }

    // Barra del jefe (MK-05)
    if (d.boss) {
        float y = cfg::H - 74.0f;
        text(a.title, "KAGE · MAESTRO DE LA SOMBRA", 110, y - 34, 30, pal::Cream);
        snprintf(buf, sizeof buf, "FASE %d / 3", d.bossPhase);
        text(a.bold, buf, cfg::W - 110.0f, y - 26, 20, pal::Cream, Align::Right);
        DrawRectangle(108, static_cast<int>(y), cfg::W - 216, 18, pal::Cream);
        DrawRectangle(110, static_cast<int>(y + 2), cfg::W - 220, 14, Color{60, 14, 20, 255});
        DrawRectangle(110, static_cast<int>(y + 2), static_cast<int>((cfg::W - 220) * std::clamp(d.bossHp / d.bossMax, 0.0f, 1.0f)), 14, pal::Red);
        for (float k : {0.33f, 0.66f}) DrawRectangle(static_cast<int>(110 + (cfg::W - 220) * k), static_cast<int>(y + 2), 3, 14, pal::Cream);
    }
}
