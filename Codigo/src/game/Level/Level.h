#pragma once
#include <string>
#include <vector>
#include "game/Enemy/Enemy.h"
#include "game/World/World.h"

enum class Theme { Callejon, Azotea, Dojo, Kage };

struct WaveDef { std::vector<std::pair<EType, int>> enemies; };

// Guía 5.6. DEMO: los niveles están definidos en código.
// Sprint 3: WaveSpawner los lee de assets/levels/nivel1.json … nivel4.json.
struct LevelDef {
    int id;
    std::string name;
    Theme theme;
    float width;
    std::vector<Plat> plats;
    std::vector<WaveDef> waves;
    std::vector<Pickup> pickups;
    long long maxScore;     // nivel.puntaje_max_teorico (RNF-001)
    float minSeconds;       // duración mínima posible
    WeaponKind unlocks;
};

LevelDef makeLevel(int id);
constexpr int LEVEL_COUNT = 1;   // demo: solo el nivel 1 (El callejón)
std::string levelName(int id);

// Fondo con paralaje por tema
class Backdrop {
public:
    void build(Theme t, float worldW);
    void drawFar(float camX, float time) const;       // espacio de pantalla
    void drawWorld(const std::vector<Plat>& plats, float camX) const;   // dentro de la cámara
private:
    struct Block { float x, w, h; Color c; bool sign; };
    Theme theme_ = Theme::Azotea;
    float worldW_ = 2560;
    std::vector<Block> far_, mid_;
};
