#include "core/Input/Input.h"
#include <algorithm>
#include <cmath>
#include <raylib.h>

namespace {
// Tabla de controles por defecto (guía, sección 1). RF-012 la cargaría desde la tabla configuracion.
struct Bind { Act act; int k1, k2; int pad; };
const Bind BINDS[] = {
    {Act::Left,    KEY_A,          KEY_LEFT,      GAMEPAD_BUTTON_LEFT_FACE_LEFT},
    {Act::Right,   KEY_D,          KEY_RIGHT,     GAMEPAD_BUTTON_LEFT_FACE_RIGHT},
    {Act::Up,      KEY_W,          KEY_UP,        GAMEPAD_BUTTON_LEFT_FACE_UP},
    {Act::Down,    KEY_S,          KEY_DOWN,      GAMEPAD_BUTTON_LEFT_FACE_DOWN},
    {Act::Jump,    KEY_SPACE,      KEY_NULL,      GAMEPAD_BUTTON_RIGHT_FACE_DOWN},   // A
    {Act::Punch,   KEY_J,          KEY_NULL,      GAMEPAD_BUTTON_RIGHT_FACE_LEFT},   // X
    {Act::Kick,    KEY_K,          KEY_NULL,      GAMEPAD_BUTTON_RIGHT_FACE_UP},     // Y
    {Act::Block,   KEY_L,          KEY_NULL,      GAMEPAD_BUTTON_RIGHT_FACE_RIGHT},  // B
    {Act::Pickup,  KEY_E,          KEY_NULL,      GAMEPAD_BUTTON_RIGHT_TRIGGER_1},   // RB
    {Act::Bullet,  KEY_LEFT_SHIFT, KEY_RIGHT_SHIFT, GAMEPAD_BUTTON_LEFT_TRIGGER_2},  // LT
    {Act::Pause,   KEY_ESCAPE,     KEY_P,         GAMEPAD_BUTTON_MIDDLE_RIGHT},      // Start
    {Act::Confirm, KEY_ENTER,      KEY_SPACE,     GAMEPAD_BUTTON_RIGHT_FACE_DOWN},
    {Act::Back,    KEY_ESCAPE,     KEY_BACKSPACE, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT},
};
}

void Input::poll() {
    const bool pad = IsGamepadAvailable(0);
    float ax = pad ? GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X) : 0.0f;
    float ay = pad ? GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y) : 0.0f;
    float lt = pad ? GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_TRIGGER) : -1.0f;

    anyPressed_ = false;
    for (const Bind& b : BINDS) {
        size_t i = idx(b.act);
        bool h = IsKeyDown(b.k1) || (b.k2 != KEY_NULL && IsKeyDown(b.k2)) || (pad && IsGamepadButtonDown(0, b.pad));
        bool p = IsKeyPressed(b.k1) || (b.k2 != KEY_NULL && IsKeyPressed(b.k2)) || (pad && IsGamepadButtonPressed(0, b.pad));
        if (b.act == Act::Left)  { h = h || ax < -0.3f; }
        if (b.act == Act::Right) { h = h || ax > 0.3f; }
        if (b.act == Act::Up)    { h = h || ay < -0.5f; }
        if (b.act == Act::Down)  { h = h || ay > 0.5f; }
        if (b.act == Act::Bullet){ h = h || lt > 0.3f; }
        // flanco del stick para navegar menús
        if (!held_[i] && h && !p && (b.act == Act::Left || b.act == Act::Right || b.act == Act::Up || b.act == Act::Down)) p = true;
        held_[i] = h;
        if (p) { pressed_[i] = true; anyPressed_ = true; }
    }

    // Correr: doble toque A / D, o stick a fondo
    const double now = GetTime();
    if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT))  { if (now - lastTapL_ < 0.28) run_ = true; lastTapL_ = now; }
    if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT)) { if (now - lastTapR_ < 0.28) run_ = true; lastTapR_ = now; }
    if (!held_[idx(Act::Left)] && !held_[idx(Act::Right)]) run_ = false;
    if (std::abs(ax) > 0.9f) run_ = true;

    for (int c = GetCharPressed(); c > 0; c = GetCharPressed())
        if (c < 128) typed_ += static_cast<char>(c);
    for (int k = GetKeyPressed(); k > 0; k = GetKeyPressed()) keys_.push_back(k);
    if (!keys_.empty()) anyPressed_ = true;
}

void Input::consumePressed() {
    pressed_.fill(false);
    vPressed_.fill(false);
    typed_.clear();
    keys_.clear();
}

bool Input::keyPressed(int key) const {
    return std::find(keys_.begin(), keys_.end(), key) != keys_.end() || IsKeyPressedRepeat(key);
}
