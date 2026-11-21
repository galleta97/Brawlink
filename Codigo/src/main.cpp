#include <cstdlib>
#include <cstring>
#include <string>
#include <raylib.h>
#include "core/Config/Config.h"
#include "core/Game/Game.h"

// Uso:  BRAWLINK.exe                     juego normal
//       BRAWLINK.exe --level 4           entra directo a un nivel (como invitado)
//       BRAWLINK.exe --level 2 --demo    el personaje juega solo (para presentar)
//       BRAWLINK.exe --profile kai       entra con un perfil sin pasar por el login
int main(int argc, char** argv) {
    LaunchOptions opt;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&]() { return i + 1 < argc ? std::string(argv[++i]) : std::string(); };
        if (a == "--level") { opt.scene = "level"; opt.level = std::atoi(next().c_str()); }
        else if (a == "--scene") opt.scene = next();
        else if (a == "--profile") opt.profile = next();
        else if (a == "--demo" || a == "--autoplay") opt.autoplay = true;
        else if (a == "--quit-after") opt.quitAfter = static_cast<float>(std::atof(next().c_str()));
        else if (a == "--shots") opt.shotsDir = next();
        else if (a == "--shot-every") opt.shotEvery = static_cast<float>(std::atof(next().c_str()));
    }

    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    if (opt.shotsDir.empty()) SetTraceLogLevel(LOG_WARNING);
    InitWindow(cfg::W, cfg::H, "BRAWLINK");
    SetWindowMinSize(640, 360);
    InitAudioDevice();
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);          // Esc abre la pausa, no cierra el juego

    {
        Game game(opt);            // carga fuentes, sonidos, perfiles y ranking
        game.run();
    }

    if (IsAudioDeviceReady()) CloseAudioDevice();
    CloseWindow();
    return 0;
}
