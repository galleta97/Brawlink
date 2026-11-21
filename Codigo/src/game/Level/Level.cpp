#include "game/Level/Level.h"
#include <cmath>
#include "core/Assets/Assets.h"
#include "core/Config/Config.h"

namespace {
Plat ground(float w) { return {{-200, cfg::GROUND_Y, w + 400, 400}, false}; }
Plat ledge(float x, float y, float w) { return {{x, y, w, 18}, true}; }
}

std::string levelName(int id) {
    switch (id) {
    case 1: return "El callejón";
    case 2: return "La azotea";
    case 3: return "El dojo";
    default: return "Jefe final";
    }
}

LevelDef makeLevel(int id) {
    LevelDef d;
    d.id = id;
    d.name = levelName(id);
    switch (id) {
    default:   // la demo solo tiene el nivel 1
        d.theme = Theme::Callejon; d.width = 2400;
        d.plats = {ground(2400), ledge(700, 480, 220), ledge(1500, 470, 260)};
        d.waves = {{{{EType::Maton, 3}}}, {{{EType::Maton, 4}}}, {{{EType::Maton, 3}, {EType::Pesado, 1}}}};
        d.pickups = {{{1150, cfg::GROUND_Y}, WeaponKind::Bate, 0}};
        d.maxScore = 60000; d.minSeconds = 20; d.unlocks = WeaponKind::Bate;
        break;
    }
    return d;
}

// --- Fondo -------------------------------------------------------------------
void Backdrop::build(Theme t, float worldW) {
    theme_ = t; worldW_ = worldW;
    far_.clear(); mid_.clear();
    SetRandomSeed(static_cast<unsigned>(static_cast<int>(t) * 77 + 5));
    auto R = [](int a, int b) { return static_cast<float>(GetRandomValue(a, b)); };
    for (float x = -200; x < worldW * 0.4f + 1600; x += R(110, 220)) far_.push_back({x, R(90, 160), R(220, 430), {}, false});
    for (float x = -200; x < worldW * 0.6f + 1600; x += R(150, 280)) mid_.push_back({x, R(120, 220), R(150, 330), {}, GetRandomValue(0, 4) == 0});
    SetRandomSeed(static_cast<unsigned>(GetTime() * 1000));
}

void Backdrop::drawFar(float camX, float time) const {
    const float G = cfg::GROUND_Y;
    switch (theme_) {
    case Theme::Callejon: {
        DrawRectangleGradientV(0, 0, cfg::W, static_cast<int>(G), Color{206, 186, 164, 255}, Color{232, 216, 194, 255});
        DrawCircle(980, 210, 70, Color{240, 226, 205, 255});
        for (auto& b : far_) { float x = b.x - camX * 0.22f; if (x > -300 && x < cfg::W + 50) DrawRectangle(static_cast<int>(x), static_cast<int>(G - b.h - 40), static_cast<int>(b.w), static_cast<int>(b.h + 40), Color{184, 166, 146, 255}); }
        for (auto& b : mid_) {
            float x = b.x - camX * 0.55f;
            if (x < -300 || x > cfg::W + 50) continue;
            DrawRectangle(static_cast<int>(x), static_cast<int>(G - b.h), static_cast<int>(b.w), static_cast<int>(b.h), Color{150, 118, 98, 255});
            for (float wy = G - b.h + 24; wy < G - 50; wy += 46)
                for (float wx = x + 16; wx < x + b.w - 30; wx += 38) DrawRectangle(static_cast<int>(wx), static_cast<int>(wy), 18, 24, Color{120, 92, 76, 255});
            // escalera de incendios
            DrawLineEx({x + 10, G - b.h + 60}, {x + b.w - 10, G - b.h + 60}, 3, Color{60, 50, 46, 255});
            DrawLineEx({x + 20, G - b.h + 60}, {x + b.w - 20, G - b.h + 130}, 3, Color{60, 50, 46, 255});
            if (b.sign) {
                bool on = std::fmod(time + b.x, 3.0f) > 0.2f;
                DrawRectangle(static_cast<int>(x + 20), static_cast<int>(G - b.h - 30), 46, 28, on ? pal::Red : pal::RedDark);
                DrawText("24H", static_cast<int>(x + 25), static_cast<int>(G - b.h - 26), 20, pal::Cream);
            }
        }
        break;
    }
    case Theme::Azotea: {
        DrawRectangleGradientV(0, 0, cfg::W, static_cast<int>(G), Color{240, 233, 221, 255}, Color{228, 218, 202, 255});
        DrawCircle(990, 150, 64, Color{245, 239, 230, 255});
        DrawCircleLines(990, 150, 64, Color{222, 210, 194, 255});
        for (auto& b : far_) { float x = b.x - camX * 0.18f; if (x > -300 && x < cfg::W + 50) DrawRectangle(static_cast<int>(x), static_cast<int>(G - b.h - 60), static_cast<int>(b.w), static_cast<int>(b.h + 60), Color{214, 204, 188, 255}); }
        for (auto& b : mid_) {
            float x = b.x - camX * 0.42f;
            if (x < -300 || x > cfg::W + 50) continue;
            DrawRectangle(static_cast<int>(x), static_cast<int>(G - b.h), static_cast<int>(b.w), static_cast<int>(b.h), Color{198, 186, 168, 255});
            for (float wy = G - b.h + 30; wy < G - 30; wy += 54) DrawRectangle(static_cast<int>(x + 12), static_cast<int>(wy), static_cast<int>(b.w - 24), 4, Color{186, 174, 156, 255});
            if (b.sign) {
                DrawRectangle(static_cast<int>(x + 18), static_cast<int>(G - b.h + 24), 46, 28, pal::Red);
                DrawText("24H", static_cast<int>(x + 23), static_cast<int>(G - b.h + 28), 20, pal::Cream);
            }
        }
        break;
    }
    case Theme::Dojo:
    case Theme::Kage: {
        bool night = theme_ == Theme::Kage;
        if (night) {
            DrawRectangleGradientV(0, 0, cfg::W, static_cast<int>(G), Color{92, 54, 50, 255}, Color{150, 98, 80, 255});
            DrawCircle(static_cast<int>(640 - camX * 0.08f + 300), 280, 230, Color{215, 38, 61, 150});
        } else {
            DrawRectangle(0, 0, cfg::W, static_cast<int>(G), Color{212, 198, 172, 255});
        }
        Color grid = night ? Color{48, 28, 24, 200} : Color{96, 56, 34, 255};
        float par = 0.85f, cell = 92;
        float off = std::fmod(camX * par, cell);
        for (float x = -off; x < cfg::W + cell; x += cell) DrawLineEx({x, 0}, {x, G}, 5, grid);
        for (float y = 30; y < G; y += cell) DrawLineEx({0, y}, {static_cast<float>(cfg::W), y}, 5, grid);
        // estandartes rojos con el círculo ensō
        for (float wx = 300; wx < worldW_ * par + 800; wx += 900) {
            float x = wx - camX * par;
            if (x < -200 || x > cfg::W + 100) continue;
            DrawRectangle(static_cast<int>(x), 120, 120, 250, night ? Color{150, 20, 36, 255} : pal::Red);
            DrawRectangle(static_cast<int>(x - 8), 112, 136, 10, night ? Color{40, 22, 18, 255} : Color{96, 56, 34, 255});
            DrawRing({x + 60, 250}, 30, 40, 20, 340, 40, pal::Ink);
        }
        break;
    }
    }
}

void Backdrop::drawWorld(const std::vector<Plat>& plats, float camX) const {
    (void)camX;
    const float G = cfg::GROUND_Y;
    Color floor, line;
    switch (theme_) {
    case Theme::Callejon: floor = {50, 46, 44, 255}; line = {28, 26, 26, 255}; break;
    case Theme::Azotea:   floor = {34, 34, 36, 255}; line = {20, 20, 22, 255}; break;
    case Theme::Dojo:     floor = {139, 90, 50, 255}; line = {104, 66, 38, 255}; break;
    default:              floor = {96, 58, 38, 255}; line = {66, 38, 26, 255}; break;
    }
    DrawRectangle(-200, static_cast<int>(G), static_cast<int>(worldW_ + 400), 200, floor);
    DrawRectangle(-200, static_cast<int>(G), static_cast<int>(worldW_ + 400), 6, line);
    if (theme_ == Theme::Dojo || theme_ == Theme::Kage)
        for (float x = 0; x < worldW_; x += 160) DrawLineEx({x, G}, {x - 40, G + 100}, 3, line);
    // decorado sobre el suelo
    if (theme_ == Theme::Azotea)
        for (float x = 300; x < worldW_; x += 820) { DrawRectangle(static_cast<int>(x), static_cast<int>(G - 70), 110, 70, Color{38, 38, 42, 255}); DrawRectangle(static_cast<int>(x + 10), static_cast<int>(G - 60), 40, 40, Color{60, 60, 64, 255}); }
    if (theme_ == Theme::Callejon)
        for (float x = 420; x < worldW_; x += 900) { DrawRectangle(static_cast<int>(x), static_cast<int>(G - 62), 130, 62, Color{58, 70, 58, 255}); DrawRectangle(static_cast<int>(x - 6), static_cast<int>(G - 70), 142, 10, Color{40, 50, 40, 255}); }

    for (const auto& p : plats) {
        if (!p.oneWay) continue;
        Color c = (theme_ == Theme::Dojo || theme_ == Theme::Kage) ? Color{110, 70, 40, 255} : Color{36, 36, 38, 255};
        float legH = G - p.r.y;
        DrawRectangle(static_cast<int>(p.r.x + 14), static_cast<int>(p.r.y), 8, static_cast<int>(legH), Color{c.r, c.g, c.b, 200});
        DrawRectangle(static_cast<int>(p.r.x + p.r.width - 22), static_cast<int>(p.r.y), 8, static_cast<int>(legH), Color{c.r, c.g, c.b, 200});
        DrawRectangleRec(p.r, c);
        DrawRectangle(static_cast<int>(p.r.x), static_cast<int>(p.r.y), static_cast<int>(p.r.width), 4, Color{static_cast<unsigned char>(c.r + 30), static_cast<unsigned char>(c.g + 30), static_cast<unsigned char>(c.b + 30), 255});
    }
}
