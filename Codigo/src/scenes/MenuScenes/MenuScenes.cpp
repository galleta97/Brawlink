// Pantallas de menú: perfiles (MK-01), menú (MK-02), ranking (MK-06), controles, pausa y resultado.
#include <algorithm>
#include <cctype>
#include <cmath>
#include "core/Config/Config.h"
#include "core/Game/Game.h"
#include "scenes/Scenes/Scenes.h"

namespace {
Color alpha(Color c, float a) { c.a = static_cast<unsigned char>(255 * std::clamp(a, 0.0f, 1.0f)); return c; }
const Color GREY_TXT{150, 144, 136, 255};
}

void drawBigStickman(const Rig& r, Vector2 hip, float s, Color c, float width, int ghosts, Color ghost) {
    Pose p = buildPose(r, s);
    for (int i = ghosts; i >= 1; --i) {
        Pose g = worldPose(p, {hip.x - i * 26.0f, hip.y + i * 4.0f}, false);
        drawPose(g, alpha(ghost, 0.22f - i * 0.05f), width, 11 * s);
    }
    drawPose(worldPose(p, hip, false), c, width, 11 * s);
}

void drawLogo(const Assets& a, float x, float y, float size, float angle) {
    const char* t = "BRAWLINK";
    DrawTextPro(a.title, t, {x + size * 0.05f, y + size * 0.05f}, {0, 0}, angle, size, 0, pal::Ink);
    DrawTextPro(a.title, t, {x + size * 0.025f, y + size * 0.025f}, {0, 0}, angle, size, 0, pal::Red);
    DrawTextPro(a.title, t, {x, y}, {0, 0}, angle, size, 0, pal::White);
}

// =============================================================================
// MK-01 · Perfiles
// =============================================================================
void LoginScene::enter(bool guest) {
    const Assets& a = g_.assets();
    if (guest) {
        g_.player = "Invitado"; g_.guest = true;
    } else {
        if (name_.size() < 3) { err_ = 0.4f; msg_ = "El perfil necesita al menos 3 caracteres"; a.play(a.hurt, 0, 0.6f); return; }
        g_.player = name_; g_.guest = false;
        g_.profiles().create(name_);   // si no existe, se crea (nombre único)
    }
    a.play(a.select, 0, 0.8f);
    g_.scenes().change(std::make_unique<MenuScene>(g_));
}

void LoginScene::update(float dt) {
    t_ += dt; err_ = std::max(0.0f, err_ - dt);
    Input& in = g_.input();
    const Assets& a = g_.assets();
    if (in.keyPressed(KEY_TAB) || in.keyPressed(KEY_DOWN)) { focus_ = (focus_ + 1) % 3; a.play(a.blip); }
    if (in.keyPressed(KEY_UP)) { focus_ = (focus_ + 2) % 3; a.play(a.blip); }
    if (focus_ == 0) {
        for (char c : in.typed())
            if ((std::isalnum(static_cast<unsigned char>(c)) || c == '_') && name_.size() < 14) name_ += c;
        if (in.keyPressed(KEY_BACKSPACE) && !name_.empty()) name_.pop_back();
        if (in.keyPressed(KEY_ENTER)) enter(false);
    } else {
        if (in.keyPressed(KEY_W)) { focus_ = (focus_ + 2) % 3; a.play(a.blip); }
        if (in.keyPressed(KEY_S)) { focus_ = (focus_ + 1) % 3; a.play(a.blip); }
        if (in.keyPressed(KEY_ENTER) || in.keyPressed(KEY_SPACE) || in.pressed(Act::Punch)) enter(focus_ == 2);
    }
    if (in.keyPressed(KEY_ESCAPE)) g_.quit();
}

void LoginScene::draw() {
    const Assets& a = g_.assets();
    ClearBackground(Color{24, 23, 22, 255});
    fillQuad({60, 0}, {585, 0}, {465, 720}, {0, 720}, pal::Red);
    drawBigStickman(rigs::loginKick(t_), {250, 360}, 2.7f, pal::Cream, 14, 0, pal::Cream);

    drawLogo(a, 620, 52, 124, 0);
    const float bx = 620, by = 190, bw = 520, bh = 440;
    DrawRectangle(static_cast<int>(bx), static_cast<int>(by), static_cast<int>(bw), static_cast<int>(bh), Color{36, 34, 31, 255});
    DrawRectangleLinesEx({bx, by, bw, bh}, 2, Color{62, 58, 54, 255});
    text(a.title, "INICIAR SESIÓN", bx + 30, by + 22, 46, pal::Cream);
    text(a.bold, "PERFIL", bx + 30, by + 100, 22, GREY_TXT);

    float shake = err_ > 0 ? std::sin(err_ * 80) * 8 * err_ : 0;
    Rectangle field{bx + 30 + shake, by + 128, bw - 60, 58};
    DrawRectangleRec(field, Color{16, 16, 15, 255});
    DrawRectangleLinesEx(field, 2, focus_ == 0 ? pal::Cream : Color{90, 86, 80, 255});
    std::string shown = name_.empty() && focus_ != 0 ? "tu_nombre" : name_;
    text(a.ui, shown, field.x + 16, field.y + 12, 32, name_.empty() ? Color{90, 86, 80, 255} : pal::Cream);
    if (focus_ == 0 && std::fmod(t_, 1.0f) < 0.55f)
        DrawRectangle(static_cast<int>(field.x + 18 + textWidth(a.ui, name_, 32)), static_cast<int>(field.y + 14), 3, 30, pal::Cream);

    text(a.ui, g_.profiles().exists(name_) ? "Perfil encontrado: continúa tu progreso" : "Si el perfil no existe, se crea al ingresar",
         bx + 30, by + 194, 22, err_ > 0 ? pal::Red : GREY_TXT);
    if (err_ > 0) text(a.ui, msg_, bx + 30, by + 194, 22, pal::Red);

    Rectangle btn{bx + 30, by + 240, bw - 60, 60};
    DrawRectangleRec(btn, focus_ == 1 ? pal::Red : Color{170, 30, 48, 255});
    if (focus_ == 1) DrawRectangleLinesEx(btn, 3, pal::Cream);
    text(a.title, "INGRESAR", btn.x + btn.width / 2, btn.y + 8, 40, pal::Cream, Align::Center);

    text(a.bold, "Jugar como invitado (sin ranking)", bx + 30, by + 330, 26, focus_ == 2 ? pal::Cream : GREY_TXT);
    if (focus_ == 2) DrawRectangle(static_cast<int>(bx + 30), static_cast<int>(by + 362), static_cast<int>(textWidth(a.bold, "Jugar como invitado (sin ranking)", 26)), 3, pal::Red);
    text(a.ui, "TAB o flechas cambiar · ENTER aceptar · ESC salir", bx + 30, by + 398, 20, Color{110, 104, 98, 255});
    text(a.ui, "Contraseña con Argon2id: sprint 4", cfg::W - 20.0f, cfg::H - 30.0f, 18, Color{90, 86, 80, 255}, Align::Right);
}

// =============================================================================
// MK-02 · Menú principal
// =============================================================================
static const char* MENU_ITEMS[] = {"NUEVA PARTIDA", "CONTINUAR", "RANKING", "CONTROLES", "CERRAR SESIÓN", "SALIR"};
constexpr int MENU_N = 6;

void MenuScene::update(float dt) {
    t_ += dt;
    Input& in = g_.input();
    const Assets& a = g_.assets();
    if (in.pressed(Act::Up)) { sel_ = (sel_ + MENU_N - 1) % MENU_N; a.play(a.blip); }
    if (in.pressed(Act::Down)) { sel_ = (sel_ + 1) % MENU_N; a.play(a.blip); }
    if (in.pressed(Act::Back)) { sel_ = MENU_N - 1; }
    if (in.pressed(Act::Confirm) || in.pressed(Act::Punch)) {
        a.play(a.select);
        switch (sel_) {
        case 0: g_.scenes().change(std::make_unique<LevelScene>(g_, 1)); break;
        case 1: g_.scenes().change(std::make_unique<LevelScene>(g_, std::min(g_.levelMax(), LEVEL_COUNT))); break;
        case 2: g_.scenes().change(std::make_unique<RankingScene>(g_)); break;
        case 3: g_.scenes().change(std::make_unique<ControlsScene>(g_)); break;
        case 4: g_.scenes().change(std::make_unique<LoginScene>(g_)); break;
        default: g_.quit();
        }
    }
}

void MenuScene::draw() {
    const Assets& a = g_.assets();
    ClearBackground(pal::Cream);
    // banda roja diagonal y línea negra
    fillQuad({0, 220}, {1280, 40}, {1280, 190}, {0, 372}, pal::Red);
    DrawLineEx({0, 380}, {1280, 196}, 7, pal::Ink);
    drawLogo(a, 70, 230, 180, -9.5f);
    drawBigStickman(rigs::menuKick(t_), {1010, 330}, 2.75f, pal::Ink, 15, 3, pal::Ink);

    float y0 = 384, step = 42;
    for (int i = 0; i < MENU_N; ++i) {
        float y = y0 + i * step;
        bool on = i == sel_;
        if (on) skewRect(56, y - 4, 380, 42, 10, pal::Ink);
        text(a.title, MENU_ITEMS[i], 92, y - 2, 34, on ? pal::Cream : pal::Ink);
        if (i == 1) {
            int lv = std::min(g_.levelMax(), LEVEL_COUNT);
            text(a.bold, "Nivel " + std::to_string(lv) + " · " + levelName(lv), 460, y + 6, 22, Color{90, 84, 78, 255});
        }
    }
    std::string foot = "Jugador: " + g_.player + (g_.guest ? " (sin ranking)" : "") + " · " + cfg::VERSION;
    text(a.bold, foot, cfg::W - 20.0f, cfg::H - 36.0f, 20, pal::Ink, Align::Right);
    text(a.ui, "W/S o flechas elegir · ENTER aceptar", 92, cfg::H - 36.0f, 20, Color{90, 84, 78, 255});
}

// =============================================================================
// MK-06 · Ranking
// =============================================================================
void RankingScene::update(float dt) {
    t_ += dt;
    Input& in = g_.input();
    if (in.pressed(Act::Left) || in.pressed(Act::Right)) { weekly_ = !weekly_; g_.assets().play(g_.assets().blip); }
    if (in.pressed(Act::Back) || in.pressed(Act::Confirm)) g_.scenes().change(std::make_unique<MenuScene>(g_));
}

void RankingScene::draw() {
    const Assets& a = g_.assets();
    ClearBackground(Color{24, 23, 22, 255});
    fillQuad({960, 0}, {1280, 0}, {1280, 720}, {840, 720}, pal::Red);
    drawBigStickman(rigs::victory(t_), {1110, 360}, 2.4f, pal::Cream, 13, 0, pal::Cream);
    text(a.title, "RANKING", 58, 32, 86, pal::Cream);

    const char* tabs[] = {"GLOBAL", "SEMANAL"};
    for (int i = 0; i < 2; ++i) {
        bool on = (i == 1) == weekly_;
        Rectangle r{58.0f + i * 150, 132, 136, 40};
        DrawRectangleRec(r, on ? pal::Cream : Color{48, 46, 44, 255});
        text(a.bold, tabs[i], r.x + r.width / 2, r.y + 8, 22, on ? pal::Ink : pal::Cream, Align::Center);
    }
    const float cols[] = {70, 120, 380, 470, 600, 790};
    const char* heads[] = {"#", "JUGADOR", "NIVEL", "COMBO MÁX", "TIEMPO", "PUNTAJE"};
    for (int i = 0; i < 6; ++i) text(a.bold, heads[i], cols[i], 196, 18, GREY_TXT, i == 5 ? Align::Right : Align::Left);

    auto rows = g_.ranking().top(8, weekly_);
    if (rows.empty()) {
        text(a.bold, "Aún no hay partidas ganadas" + std::string(weekly_ ? " esta semana." : "."), 70, 250, 30, pal::Cream);
        text(a.ui, "Supera un nivel con un perfil registrado para aparecer aquí (los invitados no entran).", 70, 292, 22, GREY_TXT);
    }
    for (size_t i = 0; i < rows.size(); ++i) {
        const auto& e = rows[i];
        float y = 226 + i * 54.0f;
        bool me = !g_.guest && e.player == g_.player;
        DrawRectangle(56, static_cast<int>(y), 750, 48, me ? pal::Red : (i % 2 ? Color{30, 29, 28, 255} : Color{36, 35, 33, 255}));
        text(a.title, std::to_string(i + 1), cols[0], y + 6, 32, me ? pal::Cream : pal::Yellow);
        text(a.ui, e.player + (me ? " (tú)" : ""), cols[1], y + 10, 26, pal::Cream);
        text(a.ui, std::to_string(e.level), cols[2], y + 10, 26, pal::Cream);
        text(a.ui, "x" + std::to_string(e.comboMax), cols[3], y + 10, 26, pal::Cream);
        text(a.ui, fmtTime(static_cast<float>(e.seconds)), cols[4], y + 10, 26, pal::Cream);
        text(a.title, fmtThousands(e.score), cols[5], y + 6, 32, pal::Cream, Align::Right);
    }
    text(a.ui, "A/D cambiar pestaña · ESC volver", 58, cfg::H - 40.0f, 20, GREY_TXT);
}

// =============================================================================
// Controles
// =============================================================================
void ControlsScene::update(float dt) {
    t_ += dt;
    Input& in = g_.input();
    if (in.pressed(Act::Back) || in.pressed(Act::Confirm)) g_.scenes().change(std::make_unique<MenuScene>(g_));
}

void ControlsScene::draw() {
    const Assets& a = g_.assets();
    ClearBackground(pal::Cream);
    fillQuad({0, 0}, {1280, 0}, {1280, 90}, {0, 150}, pal::Red);
    text(a.title, "CONTROLES", 58, 22, 80, pal::Cream);
    const char* rows[][3] = {
        {"Moverse", "A / D", "Stick izquierdo"}, {"Correr", "Doble toque A / D", "Stick a fondo"},
        {"Saltar", "Espacio", "A"}, {"Agacharse (esquiva balas)", "S", "Stick abajo"},
        {"Puño / atacar con arma", "J", "X"}, {"Patada", "K", "Y"},
        {"Bloquear", "L (mantener)", "B"}, {"Recoger / soltar arma", "E", "RB"},
        {"Tiempo bala", "Shift (mantener)", "LT"}, {"Pausa", "Esc", "Start"},
    };
    text(a.bold, "ACCIÓN", 90, 180, 22, Color{90, 84, 78, 255});
    text(a.bold, "TECLADO", 560, 180, 22, Color{90, 84, 78, 255});
    text(a.bold, "MANDO XBOX", 860, 180, 22, Color{90, 84, 78, 255});
    for (int i = 0; i < 10; ++i) {
        float y = 214 + i * 44.0f;
        if (i % 2 == 0) DrawRectangle(70, static_cast<int>(y - 4), 1060, 42, Color{226, 216, 200, 255});
        text(a.bold, rows[i][0], 90, y, 28, pal::Ink);
        text(a.title, rows[i][1], 560, y - 2, 30, pal::Red);
        text(a.bold, rows[i][2], 860, y, 28, pal::Ink);
    }
    text(a.ui, "Combo: golpea sin parar; se reinicia tras 1,5 s sin acertar. Multiplicador = 1 + combo / 10.   ESC volver",
         90, cfg::H - 46.0f, 22, Color{90, 84, 78, 255});
}

// =============================================================================
// Pausa
// =============================================================================
void PauseScene::update(float dt) {
    (void)dt;
    Input& in = g_.input();
    const Assets& a = g_.assets();
    if (in.pressed(Act::Up)) { sel_ = (sel_ + 2) % 3; a.play(a.blip); }
    if (in.pressed(Act::Down)) { sel_ = (sel_ + 1) % 3; a.play(a.blip); }
    if (in.pressed(Act::Pause)) { g_.scenes().pop(); return; }
    if (in.pressed(Act::Confirm) || in.pressed(Act::Punch)) {
        a.play(a.select);
        if (sel_ == 0) g_.scenes().pop();
        else if (sel_ == 1) g_.scenes().change(std::make_unique<LevelScene>(g_, level_));
        else g_.scenes().change(std::make_unique<MenuScene>(g_));
    }
}

void PauseScene::draw() {
    const Assets& a = g_.assets();
    DrawRectangle(0, 0, cfg::W, cfg::H, Color{15, 15, 15, 190});
    skewRect(380, 190, 520, 330, 24, pal::Ink);
    text(a.title, "PAUSA", 640, 210, 80, pal::Red, Align::Center);
    const char* opts[] = {"CONTINUAR", "REINICIAR NIVEL", "MENÚ PRINCIPAL"};
    for (int i = 0; i < 3; ++i) {
        float y = 320 + i * 56.0f;
        if (i == sel_) DrawRectangle(450, static_cast<int>(y - 4), 380, 48, pal::Red);
        text(a.title, opts[i], 640, y, 36, pal::Cream, Align::Center);
    }
    text(a.ui, "ESC continuar", 640, 540, 20, GREY_TXT, Align::Center);
}

// =============================================================================
// Resultado del nivel (RF-009)
// =============================================================================
ResultScene::ResultScene(Game& g, MatchStats s) : g_(g), s_(s) {
    if (s_.win && s_.level < LEVEL_COUNT) opts_ = {"SIGUIENTE NIVEL", "REPETIR NIVEL", "MENÚ PRINCIPAL"};
    else if (s_.win) opts_ = {"VER RANKING", "REPETIR NIVEL", "MENÚ PRINCIPAL"};
    else opts_ = {"REINTENTAR", "MENÚ PRINCIPAL"};
}

void ResultScene::update(float dt) {
    t_ += dt;
    Input& in = g_.input();
    const Assets& a = g_.assets();
    const int n = static_cast<int>(opts_.size());
    if (in.pressed(Act::Up)) { sel_ = (sel_ + n - 1) % n; a.play(a.blip); }
    if (in.pressed(Act::Down)) { sel_ = (sel_ + 1) % n; a.play(a.blip); }
    if (t_ > 0.5f && (in.pressed(Act::Confirm) || in.pressed(Act::Punch))) {
        a.play(a.select);
        const std::string& o = opts_[sel_];
        if (o == "SIGUIENTE NIVEL") g_.scenes().change(std::make_unique<LevelScene>(g_, s_.level + 1));
        else if (o == "REPETIR NIVEL" || o == "REINTENTAR") g_.scenes().change(std::make_unique<LevelScene>(g_, s_.level));
        else if (o == "VER RANKING") g_.scenes().change(std::make_unique<RankingScene>(g_));
        else g_.scenes().change(std::make_unique<MenuScene>(g_));
    }
}

void ResultScene::draw() {
    const Assets& a = g_.assets();
    const bool win = s_.win;
    ClearBackground(win ? pal::Cream : Color{24, 23, 22, 255});
    fillQuad({0, 70}, {1280, 0}, {1280, 150}, {0, 220}, win ? pal::Red : pal::RedDark);
    std::string title = win ? (s_.level == 4 ? "¡KAGE HA CAÍDO!" : "¡NIVEL SUPERADO!") : "DERROTA";
    DrawTextPro(a.title, title.c_str(), {64, 132}, {0, 0}, -3.2f, 92, 0, pal::Cream);
    text(a.bold, "NIVEL " + std::to_string(s_.level) + " · " + upper(levelName(s_.level)), 70, 240, 28, win ? pal::Ink : pal::Cream);
    if (win) drawBigStickman(rigs::victory(t_), {1040, 420}, 2.3f, pal::Ink, 13, 0, pal::Ink);
    else drawBigStickman(rigs::hit(0.2f), {1040, 440}, 2.3f, pal::Cream, 13, 0, pal::Cream);

    Color fg = win ? pal::Ink : pal::Cream;
    Color sub = win ? Color{100, 94, 88, 255} : GREY_TXT;
    const std::string labels[] = {"PUNTAJE", "COMBO MÁX", "ENEMIGOS", "TIEMPO"};
    const std::string vals[] = {fmtThousands(s_.score), "x" + std::to_string(s_.comboMax), std::to_string(s_.kills), fmtTime(s_.seconds)};
    for (int i = 0; i < 4; ++i) {
        float x = 70 + i * 190.0f;
        float k = std::clamp((t_ - 0.15f * i) * 4, 0.0f, 1.0f);
        text(a.bold, labels[i], x, 300, 20, sub);
        text(a.title, vals[i], x, 324 + (1 - k) * 20, 54, alpha(fg, k));
    }
    std::string note;
    if (!s_.win) note = "Ánimo: el combo se reinicia, tu progreso no.";
    else if (g_.guest) note = "Modo invitado: la partida no entra al ranking.";
    else if (!s_.valid) note = "Partida sospechosa (RNF-001): no entra al ranking.";
    else note = "Partida validada y guardada en el ranking local.";
    text(a.ui, note, 70, 400, 24, s_.valid ? sub : pal::Red);
    if (s_.win) {
        LevelDef d = makeLevel(s_.level);
        if (d.unlocks != WeaponKind::None) text(a.bold, std::string("Arma desbloqueada: ") + weaponName(d.unlocks), 70, 432, 26, pal::Red);
    }
    for (size_t i = 0; i < opts_.size(); ++i) {
        float y = 500 + i * 56.0f;
        bool on = static_cast<int>(i) == sel_;
        if (on) skewRect(56, y - 4, 400, 46, 10, win ? pal::Ink : pal::Red);
        text(a.title, opts_[i], 90, y, 34, on ? pal::Cream : fg);
    }
}
