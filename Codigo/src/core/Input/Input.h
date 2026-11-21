#pragma once
#include <array>
#include <string>
#include <vector>

// Las escenas preguntan por ACCIONES, no por teclas (RF-012: reasignar solo cambia la tabla).
enum class Act { Left, Right, Up, Down, Jump, Punch, Kick, Block, Pickup, Bullet, Pause, Confirm, Back, COUNT };

class Input {
public:
    void poll();              // lee teclado y mando UNA vez por fotograma
    void consumePressed();    // un "presionado" se usa en un solo paso de lógica

    bool held(Act a) const    { return virtualOn_ ? vHeld_[idx(a)] : held_[idx(a)]; }
    bool pressed(Act a) const { return virtualOn_ ? vPressed_[idx(a)] : pressed_[idx(a)]; }
    bool running() const      { return virtualOn_ ? vRun_ : run_; }   // doble toque o stick a fondo
    float moveAxis() const    { return (held(Act::Right) ? 1.0f : 0.0f) - (held(Act::Left) ? 1.0f : 0.0f); }

    // Texto (pantalla de perfil)
    const std::string& typed() const { return typed_; }
    bool keyPressed(int key) const;   // tecla cruda presionada en este fotograma
    bool anyKeyOrButton() const { return anyPressed_; }

    // Autopiloto (modo demostración / pruebas automáticas)
    void setVirtual(bool on) { virtualOn_ = on; }
    void virtualSet(Act a, bool heldNow, bool pressedNow) { vHeld_[idx(a)] = heldNow; if (pressedNow) vPressed_[idx(a)] = true; }
    void virtualRun(bool r) { vRun_ = r; }
    void virtualClearHeld() { vHeld_.fill(false); vRun_ = false; }

private:
    static size_t idx(Act a) { return static_cast<size_t>(a); }
    std::array<bool, static_cast<size_t>(Act::COUNT)> held_{}, pressed_{}, vHeld_{}, vPressed_{};
    bool run_ = false, vRun_ = false, virtualOn_ = false, anyPressed_ = false;
    double lastTapL_ = -1, lastTapR_ = -1;
    std::string typed_;
    std::vector<int> keys_;
};
