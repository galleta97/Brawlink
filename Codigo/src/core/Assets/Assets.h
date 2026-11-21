#pragma once
#include <raylib.h>
#include <string>

enum class Align { Left, Center, Right };

// Fuentes y sonidos. Los sonidos se sintetizan al iniciar (no hace falta ningún .wav).
struct Assets {
    Font title{};   // Barlow Condensed Black Italic: títulos
    Font bold{};    // Barlow Condensed Bold Italic: menús y HUD
    Font ui{};      // Barlow Condensed SemiBold: textos chicos
    Sound punch{}, kick{}, slash{}, shot{}, enemyShot{}, ko{}, blip{}, select{}, slowmo{}, block{}, hurt{}, pickup{}, phase{}, whoosh{};
    bool audio = false;

    void load();
    void unload();
    void play(const Sound& s, float pitchVar = 0.08f, float vol = 1.0f) const;
};

void text(const Font& f, const std::string& s, float x, float y, float size, Color c, Align a = Align::Left, float spacing = 0.0f);
void textShadow(const Font& f, const std::string& s, float x, float y, float size, Color c, Color shadow, Vector2 off, Align a = Align::Left);
float textWidth(const Font& f, const std::string& s, float size, float spacing = 0.0f);

// Formas auxiliares
void fillQuad(Vector2 a, Vector2 b, Vector2 c, Vector2 d, Color col);
void skewRect(float x, float y, float w, float h, float skew, Color col);   // paralelogramo inclinado (paneles del HUD)
void starburst(Vector2 c, float rOut, float rIn, int points, float rot, Color col);
std::string fmtThousands(long long v);
std::string fmtTime(float seconds);
std::string upper(const std::string& s);
