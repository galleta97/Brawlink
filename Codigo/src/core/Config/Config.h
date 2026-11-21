#pragma once
#include <raylib.h>

// Constantes globales del juego (valores de la guía; los marcados "inicial" se balancean jugando)
namespace cfg {
constexpr int   W = 1280;                 // resolución interna (RNF-006: se escala a la ventana)
constexpr int   H = 720;
constexpr float STEP = 1.0f / 60.0f;      // paso fijo de lógica
constexpr float GROUND_Y = 620.0f;        // altura del suelo en píxeles
constexpr float GRAVITY = 2100.0f;        // px/s²
constexpr float PLAYER_BT_SCALE = 0.6f;   // en tiempo bala el jugador va al 60 % y el mundo al 25 % (inicial)
constexpr int   MAX_ATTACKERS = 2;        // enemigos que pueden atacar a la vez (justicia del combate)
constexpr int   MAX_RAGDOLLS = 6;         // RF-006: límite de cuerpos en pantalla
constexpr float RAGDOLL_FREEZE = 3.0f;    // RF-006: se congelan a los 3 s
const char* const VERSION = "v0.1 demo";
}

// Paleta tomada de los mockups
namespace pal {
constexpr Color Red{215, 38, 61, 255};
constexpr Color RedDark{150, 24, 40, 255};
constexpr Color Ink{20, 20, 20, 255};
constexpr Color Char{38, 38, 40, 255};
constexpr Color Cream{238, 230, 217, 255};
constexpr Color Sand{205, 193, 174, 255};
constexpr Color SandDark{180, 168, 150, 255};
constexpr Color Yellow{246, 196, 66, 255};
constexpr Color Panel{34, 32, 30, 255};
constexpr Color Grey{120, 116, 110, 255};
constexpr Color White{250, 247, 240, 255};
}
