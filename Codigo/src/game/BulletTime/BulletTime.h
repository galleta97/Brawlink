#pragma once
#include <algorithm>
// RF-007: el juego va al 25 % y la barra llena dura 4 s de uso
class BulletTime {
public:
    static constexpr float SCALE = 0.25f, DURATION = 4.0f;
    void addFury(float v) { fury_ = std::min(1.0f, fury_ + v); } // p. ej. 0.06 por golpe acertado
    void update(float realDt, bool held) {
        active_ = held && fury_ > 0.0f;
        if (active_) fury_ = std::max(0.0f, fury_ - realDt / DURATION);
        blend_ += ((active_ ? 1.0f : 0.0f) - blend_) * std::min(1.0f, realDt * 10.0f);
    }
    float scale() const { return active_ ? SCALE : 1.0f; }
    float fury() const { return fury_; }
    bool active() const { return active_; }
    float blend() const { return blend_; }   // 0..1 para el efecto visual
private:
    float fury_ = 0.0f, blend_ = 0.0f;
    bool active_ = false;
};
