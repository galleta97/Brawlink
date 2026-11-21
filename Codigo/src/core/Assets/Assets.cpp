#include "core/Assets/Assets.h"
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <vector>

namespace fs = std::filesystem;

static std::string findAssets() {
    const std::string app = GetApplicationDirectory();
    for (const std::string& c : {app + "assets", std::string("assets"), std::string("../assets"), std::string("../../assets"), app + "../assets"}) {
        std::error_code ec;
        if (fs::exists(c + "/fonts", ec)) return c + "/";
    }
    return "assets/";
}

static Font loadFont(const std::string& path, int size) {
    // ASCII + letras del español
    std::string chars;
    for (int c = 32; c < 127; ++c) chars += static_cast<char>(c);
    chars += "áéíóúÁÉÍÓÚñÑ¡¿·×üÜ—";
    int count = 0;
    int* cps = LoadCodepoints(chars.c_str(), &count);
    Font f{};
    if (FileExists(path.c_str())) f = LoadFontEx(path.c_str(), size, cps, count);
    UnloadCodepoints(cps);
    if (f.texture.id == 0) return GetFontDefault();
    GenTextureMipmaps(&f.texture);
    SetTextureFilter(f.texture, TEXTURE_FILTER_TRILINEAR);
    return f;
}

// --- Síntesis de sonidos ---------------------------------------------------
static uint32_t rngState = 12345u;
static float noise() { rngState = rngState * 1664525u + 1013904223u; return ((rngState >> 9) & 0x7FFF) / 16383.5f - 1.0f; }

static Sound synth(float dur, const std::function<float(float t, float u)>& f) {
    const int rate = 44100;
    const int n = static_cast<int>(dur * rate);
    auto* data = static_cast<short*>(MemAlloc(n * sizeof(short)));
    for (int i = 0; i < n; ++i) {
        float t = static_cast<float>(i) / rate;
        float v = f(t, t / dur);
        if (v > 1) v = 1; if (v < -1) v = -1;
        data[i] = static_cast<short>(v * 30000);
    }
    Wave w{static_cast<unsigned>(n), rate, 16, 1, data};
    Sound s = LoadSoundFromWave(w);
    UnloadWave(w);
    return s;
}

void Assets::load() {
    const std::string base = findAssets();
    title = loadFont(base + "fonts/BarlowCondensed-BlackItalic.ttf", 128);
    bold = loadFont(base + "fonts/BarlowCondensed-BoldItalic.ttf", 64);
    ui = loadFont(base + "fonts/BarlowCondensed-SemiBold.ttf", 40);

    audio = IsAudioDeviceReady();
    if (!audio) return;
    const float PI2 = 6.2831853f;
    punch = synth(0.11f, [&](float t, float u) { float e = std::exp(-u * 6); return (0.7f * std::sin(PI2 * (140 - 80 * u) * t) + 0.5f * noise() * std::exp(-u * 18)) * e; });
    kick = synth(0.16f, [&](float t, float u) { float e = std::exp(-u * 5); return (0.8f * std::sin(PI2 * (95 - 50 * u) * t) + 0.4f * noise() * std::exp(-u * 14)) * e; });
    slash = synth(0.20f, [&](float t, float u) { float s = std::sin(PI2 * (2400 - 1800 * u) * t) * 0.15f; return (noise() * 0.6f + s) * std::sin(3.14159f * u) * (1 - u); });
    shot = synth(0.22f, [&](float t, float u) { return (noise() * std::exp(-u * 9) + 0.6f * std::sin(PI2 * 80 * t) * std::exp(-u * 6)) * 0.9f; });
    enemyShot = synth(0.18f, [&](float t, float u) { return (noise() * std::exp(-u * 11) + 0.4f * std::sin(PI2 * 130 * t) * std::exp(-u * 8)) * 0.6f; });
    ko = synth(0.45f, [&](float t, float u) { return (0.6f * std::sin(PI2 * (220 - 170 * u) * t) + 0.3f * noise() * std::exp(-u * 4)) * (1 - u); });
    blip = synth(0.05f, [&](float t, float u) { return 0.35f * std::sin(PI2 * 880 * t) * (1 - u); });
    select = synth(0.14f, [&](float t, float u) { float f = u < 0.5f ? 660 : 990; return 0.35f * std::sin(PI2 * f * t) * (1 - u); });
    slowmo = synth(0.6f, [&](float t, float u) { return (0.5f * std::sin(PI2 * (300 - 230 * u) * t) + 0.15f * noise()) * std::sin(3.14159f * u); });
    block = synth(0.09f, [&](float t, float u) { return (0.5f * std::sin(PI2 * 1300 * t) + 0.3f * noise()) * std::exp(-u * 7); });
    hurt = synth(0.18f, [&](float t, float u) { return (0.6f * std::sin(PI2 * (330 - 200 * u) * t) + 0.3f * noise()) * (1 - u); });
    pickup = synth(0.18f, [&](float t, float u) { float f = u < 0.33f ? 520 : (u < 0.66f ? 780 : 1040); return 0.3f * std::sin(PI2 * f * t) * (1 - u * 0.5f); });
    phase = synth(0.9f, [&](float t, float u) { return (0.5f * std::sin(PI2 * (55 + 20 * u) * t) + 0.35f * noise() * (1 - u)) * (1 - u); });
    whoosh = synth(0.16f, [&](float t, float u) { (void)t; return noise() * 0.35f * std::sin(3.14159f * u); });
}

void Assets::unload() {
    for (Font* f : {&title, &bold, &ui})
        if (f->texture.id != GetFontDefault().texture.id) UnloadFont(*f);
    if (!audio) return;
    for (Sound* s : {&punch, &kick, &slash, &shot, &enemyShot, &ko, &blip, &select, &slowmo, &block, &hurt, &pickup, &phase, &whoosh})
        UnloadSound(*s);
}

void Assets::play(const Sound& s, float pitchVar, float vol) const {
    if (!audio || s.frameCount == 0) return;
    SetSoundPitch(s, 1.0f + pitchVar * (GetRandomValue(-100, 100) / 100.0f));
    SetSoundVolume(s, vol);
    PlaySound(s);
}

// --- Texto -----------------------------------------------------------------
float textWidth(const Font& f, const std::string& s, float size, float spacing) {
    return MeasureTextEx(f, s.c_str(), size, spacing).x;
}

void text(const Font& f, const std::string& s, float x, float y, float size, Color c, Align a, float spacing) {
    float w = (a == Align::Left) ? 0 : textWidth(f, s, size, spacing);
    if (a == Align::Center) x -= w / 2;
    if (a == Align::Right) x -= w;
    DrawTextEx(f, s.c_str(), {std::round(x), std::round(y)}, size, spacing, c);
}

void textShadow(const Font& f, const std::string& s, float x, float y, float size, Color c, Color shadow, Vector2 off, Align a) {
    text(f, s, x + off.x, y + off.y, size, shadow, a);
    text(f, s, x, y, size, c, a);
}

// --- Formas ----------------------------------------------------------------
void fillQuad(Vector2 a, Vector2 b, Vector2 c, Vector2 d, Color col) {
    // se dibuja en ambos sentidos para no depender del orden de los vértices
    DrawTriangle(a, b, c, col); DrawTriangle(a, c, b, col);
    DrawTriangle(a, c, d, col); DrawTriangle(a, d, c, col);
}

void skewRect(float x, float y, float w, float h, float skew, Color col) {
    fillQuad({x + skew, y}, {x + w + skew, y}, {x + w, y + h}, {x, y + h}, col);
}

void starburst(Vector2 c, float rOut, float rIn, int points, float rot, Color col) {
    const int n = points * 2;
    for (int i = 0; i < n; ++i) {
        float a0 = rot + 6.2831853f * i / n, a1 = rot + 6.2831853f * (i + 1) / n;
        float r0 = (i % 2 == 0) ? rOut : rIn, r1 = (i % 2 == 0) ? rIn : rOut;
        Vector2 p0{c.x + std::cos(a0) * r0, c.y + std::sin(a0) * r0};
        Vector2 p1{c.x + std::cos(a1) * r1, c.y + std::sin(a1) * r1};
        DrawTriangle(c, p0, p1, col); DrawTriangle(c, p1, p0, col);
    }
}

std::string fmtThousands(long long v) {
    std::string s = std::to_string(v < 0 ? -v : v), out;
    int cnt = 0;
    for (int i = static_cast<int>(s.size()) - 1; i >= 0; --i) {
        out.insert(out.begin(), s[i]);
        if (++cnt % 3 == 0 && i > 0) out.insert(out.begin(), ',');
    }
    return v < 0 ? "-" + out : out;
}

std::string fmtTime(float seconds) {
    int s = static_cast<int>(seconds);
    char buf[16];
    snprintf(buf, sizeof buf, "%d:%02d", s / 60, s % 60);
    return buf;
}

std::string upper(const std::string& s) {
    std::string r;
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = s[i];
        if (c == 0xC3 && i + 1 < s.size()) {        // á é í ó ú ñ ü en UTF-8
            unsigned char d = s[i + 1];
            if (d >= 0xA0 && d <= 0xBF) d -= 0x20;
            r += static_cast<char>(c); r += static_cast<char>(d); ++i;
        } else r += static_cast<char>(std::toupper(c));
    }
    return r;
}
