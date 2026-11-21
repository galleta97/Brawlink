#pragma once
#include <memory>
#include <string>
#include <vector>
#include "core/Scene/Scene.h"
#include "game/BulletTime/BulletTime.h"
#include "game/ComboSystem/ComboSystem.h"
#include "game/Enemy/Enemy.h"
#include "game/Hud/Hud.h"
#include "game/Level/Level.h"
#include "game/Player/Player.h"

class Game;

// MK-01 · Perfiles (RF-001)
class LoginScene : public Scene {
public:
    explicit LoginScene(Game& g) : g_(g) {}
    void update(float dt) override;
    void draw() override;
private:
    void enter(bool guest);
    Game& g_;
    std::string name_;
    int focus_ = 0;       // 0 campo, 1 ingresar, 2 invitado
    float t_ = 0, err_ = 0;
    std::string msg_;
};

// MK-02 · Menú principal (RF-002)
class MenuScene : public Scene {
public:
    explicit MenuScene(Game& g) : g_(g) {}
    void update(float dt) override;
    void draw() override;
private:
    Game& g_;
    int sel_ = 0;
    float t_ = 0, idle_ = 0;
};

struct MatchStats {
    int level = 1;
    bool win = false;
    long long score = 0;
    int comboMax = 0, kills = 0;
    float seconds = 0;
    bool valid = true;
    bool saved = false;
};

// MK-03/04/05 · El juego
class LevelScene : public Scene {
public:
    LevelScene(Game& g, int level);
    void onEnter() override;
    void update(float dt) override;
    void draw() override;
private:
    enum class Phase { Intro, Play, Clear, Dead };
    void spawn(EType t, float x);
    void startWave();
    void resolvePlayerAttacks();
    void updateBullets(float dt);
    void killEnemy(Enemy& e, float dirX, float force);
    void autopilot(float dt);
    void finish(bool win);
    void drawTutorial() const;

    Game& g_;
    int level_;
    LevelDef def_;
    World world_;
    Backdrop back_;
    Player player_;
    std::vector<Enemy> enemies_;
    ComboSystem combo_;
    BulletTime bt_;
    Hud hud_;
    Phase phase_ = Phase::Intro;
    float phaseT_ = 0, time_ = 0, playTime_ = 0, camX_ = 0, banner_ = 0, btTitle_ = 0, dodgeT_ = 0;
    std::string bannerText_;
    Color bannerColor_ = pal::Ink;
    long long score_ = 0;
    int kills_ = 0, wave_ = 0, nextId_ = 1;
    struct Pending { EType t; float delay; };
    std::vector<Pending> queue_;
    float waveGap_ = 0;
    bool btWas_ = false;
    // autopiloto
    float apT_ = 0, apJump_ = 0;
    int apToggle_ = 0;
};

class PauseScene : public Scene {
public:
    PauseScene(Game& g, int level) : g_(g), level_(level) {}
    void update(float dt) override;
    void draw() override;
private:
    Game& g_;
    int level_, sel_ = 0;
};

class ResultScene : public Scene {
public:
    ResultScene(Game& g, MatchStats s);
    void update(float dt) override;
    void draw() override;
private:
    Game& g_;
    MatchStats s_;
    int sel_ = 0;
    float t_ = 0;
    std::vector<std::string> opts_;
};

// MK-06 · Ranking (RF-010)
class RankingScene : public Scene {
public:
    explicit RankingScene(Game& g) : g_(g) {}
    void update(float dt) override;
    void draw() override;
private:
    Game& g_;
    bool weekly_ = false;
    float t_ = 0;
};

class ControlsScene : public Scene {
public:
    explicit ControlsScene(Game& g) : g_(g) {}
    void update(float dt) override;
    void draw() override;
private:
    Game& g_;
    float t_ = 0;
};

// Arte compartido de los menús
void drawBigStickman(const Rig& r, Vector2 hip, float scale, Color c, float width, int ghosts, Color ghost);
void drawLogo(const Assets& a, float x, float y, float size, float angle);
