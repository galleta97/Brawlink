#pragma once
#include <string>
#include "core/Assets/Assets.h"
#include "game/Stickman/Stickman.h"

// HUD según los mockups MK-03 a MK-05. El HUD no se ralentiza en tiempo bala.
struct HudData {
    std::string player;
    float hp, maxHp, fury;
    std::string levelTitle;
    long long score;
    int combo;
    float comboLeft, multiplier;
    WeaponKind weapon;
    int ammo;
    int wave, waves;
    bool boss;
    float bossHp, bossMax;
    int bossPhase;
    float time;
    bool dark = false;   // fondo oscuro (nivel del jefe)
};

class Hud {
public:
    void update(float realDt, int combo);
    void draw(const Assets& a, const HudData& d) const;
private:
    float comboPop_ = 0, scoreShown_ = 0;
    int lastCombo_ = 0;
    mutable float scoreTarget_ = 0;
};
