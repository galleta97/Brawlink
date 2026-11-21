#pragma once
#include <string>
#include "core/Assets/Assets.h"
#include "core/Input/Input.h"
#include "core/Scene/Scene.h"
#include "data/Profiles/Profiles.h"
#include "data/Ranking/Ranking.h"

// Opciones de línea de comandos (para la demo y las pruebas automáticas)
struct LaunchOptions {
    std::string scene = "login";   // login | menu | ranking | controls | level
    int level = 1;
    std::string profile;           // entra directo con este perfil
    bool autoplay = false;         // el jugador se maneja solo (modo demostración)
    float quitAfter = 0;           // cierra solo tras N segundos (0 = nunca)
    std::string shotsDir;          // guarda capturas aquí
    float shotEvery = 2.0f;
};

class Game {
public:
    explicit Game(const LaunchOptions& opt);
    ~Game();
    void run();

    Input& input() { return input_; }
    SceneManager& scenes() { return scenes_; }
    const Assets& assets() const { return assets_; }
    Profiles& profiles() { return profiles_; }
    Ranking& ranking() { return ranking_; }
    const LaunchOptions& options() const { return opt_; }

    // Sesión actual
    std::string player = "Invitado";
    bool guest = true;
    int levelMax() { return guest ? 1 : profiles_.levelMax(player); }

    void quit() { quit_ = true; }
    float time() const { return time_; }

private:
    LaunchOptions opt_;
    Input input_;
    SceneManager scenes_;
    Assets assets_;
    Profiles profiles_;
    Ranking ranking_;
    bool quit_ = false, debug_ = false;
    float time_ = 0;
};
