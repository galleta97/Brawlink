#pragma once
// RF-004: sin raylib, se prueba con pruebas unitarias
class ComboSystem {
public:
    static constexpr float RESET = 1.5f;                 // RF-004: se reinicia tras 1,5 s sin golpear
    void registerHit() { ++count_; since_ = 0.0f; if (count_ > max_) max_ = count_; }
    void update(float dt) { if (count_ > 0 && (since_ += dt) >= RESET) count_ = 0; }
    void breakCombo() { count_ = 0; }                    // al recibir daño
    int   count() const { return count_; }
    int   maxCombo() const { return max_; }             // se guarda en partida.combo_max
    float multiplier() const { return 1.0f + count_ / 10.0f; }
    float timeLeft() const { return count_ > 0 ? 1.0f - since_ / RESET : 0.0f; }
private:
    int count_ = 0, max_ = 0;
    float since_ = 0.0f;
};
