#include "core/Game/Game.h"
#include <algorithm>
#include <filesystem>
#include "core/Config/Config.h"
#include "scenes/Scenes/Scenes.h"

Game::Game(const LaunchOptions& opt) : opt_(opt) {
    assets_.load();
    profiles_.load();
    ranking_.load();
    if (!opt_.profile.empty()) {
        player = opt_.profile; guest = false;
        profiles_.create(player);
    }
}

Game::~Game() { assets_.unload(); }

void Game::run() {
    const float STEP = cfg::STEP;
    float acc = 0.0f;
    RenderTexture2D target = LoadRenderTexture(cfg::W, cfg::H);
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);

    if (opt_.scene == "menu") scenes_.change(std::make_unique<MenuScene>(*this));
    else if (opt_.scene == "ranking") scenes_.change(std::make_unique<RankingScene>(*this));
    else if (opt_.scene == "controls") scenes_.change(std::make_unique<ControlsScene>(*this));
    else if (opt_.scene == "level") scenes_.change(std::make_unique<LevelScene>(*this, std::clamp(opt_.level, 1, LEVEL_COUNT)));
    else scenes_.change(std::make_unique<LoginScene>(*this));
    scenes_.applyChanges();

    float shotT = opt_.shotEvery;
    int shotN = 0;
    if (!opt_.shotsDir.empty()) std::filesystem::create_directories(opt_.shotsDir);

    while (!WindowShouldClose() && !quit_) {
        input_.poll();                                   // lee teclado y mando UNA vez por fotograma
        if (IsKeyPressed(KEY_F3)) debug_ = !debug_;
        if (IsKeyPressed(KEY_F11)) ToggleBorderlessWindowed();
        float frame = std::min(GetFrameTime(), 0.25f);   // evita la "espiral" tras un tirón
        if (!opt_.shotsDir.empty()) frame = STEP;        // en capturas, tiempo constante
        acc += frame;
        time_ += frame;
        while (acc >= STEP) {
            if (Scene* s = scenes_.top()) s->update(STEP);
            input_.consumePressed();                     // un "presionado" se usa en un solo paso
            scenes_.applyChanges();
            acc -= STEP;
        }

        BeginTextureMode(target);
        ClearBackground(RAYWHITE);
        scenes_.drawAll();
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);
        float k = std::min(GetScreenWidth() / static_cast<float>(cfg::W), GetScreenHeight() / static_cast<float>(cfg::H));
        Rectangle src{0, 0, static_cast<float>(cfg::W), -static_cast<float>(cfg::H)};   // alto negativo: la textura viene invertida
        Rectangle dst{(GetScreenWidth() - cfg::W * k) / 2, (GetScreenHeight() - cfg::H * k) / 2, cfg::W * k, cfg::H * k};
        DrawTexturePro(target.texture, src, dst, {0, 0}, 0.0f, WHITE);
        if (debug_) DrawFPS(10, 10);                     // RNF-002
        EndDrawing();

        if (!opt_.shotsDir.empty() && (shotT -= frame) <= 0) {
            shotT = opt_.shotEvery;
            Image img = LoadImageFromTexture(target.texture);
            ImageFlipVertical(&img);
            ExportImage(img, TextFormat("%s/shot_%03d.png", opt_.shotsDir.c_str(), shotN++));
            UnloadImage(img);
        }
        if (opt_.quitAfter > 0 && time_ >= opt_.quitAfter) quit_ = true;
    }
    UnloadRenderTexture(target);
}
