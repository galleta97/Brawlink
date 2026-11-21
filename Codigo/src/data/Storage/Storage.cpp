#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include "data/Profiles/Profiles.h"
#include "data/Ranking/Ranking.h"

namespace fs = std::filesystem;

std::string dataDir() {
    fs::path dir;
    if (const char* app = std::getenv("APPDATA")) dir = fs::path(app) / "Brawlink";
    else dir = fs::path("data");
    std::error_code ec;
    fs::create_directories(dir, ec);
    return dir.string();
}

static std::string profilesPath() { return (fs::path(dataDir()) / "perfiles.txt").string(); }
static std::string rankingPath() { return (fs::path(dataDir()) / "ranking.txt").string(); }

void Profiles::load() {
    levels_.clear();
    std::ifstream in(profilesPath());
    std::string line;
    while (std::getline(in, line)) {
        auto bar = line.find('|');
        if (bar == std::string::npos) continue;
        levels_[line.substr(0, bar)] = std::max(1, std::atoi(line.c_str() + bar + 1));
    }
}

void Profiles::save() const {
    std::ofstream out(profilesPath(), std::ios::trunc);
    for (auto& [name, lvl] : levels_) out << name << '|' << lvl << '\n';
}

void Ranking::load() {
    entries_.clear();
    std::ifstream in(rankingPath());
    std::string line;
    while (std::getline(in, line)) {
        std::stringstream ss(line);
        RankEntry e;
        std::string f;
        std::getline(ss, e.player, '|');
        if (!std::getline(ss, f, '|')) continue; e.level = std::atoi(f.c_str());
        if (!std::getline(ss, f, '|')) continue; e.comboMax = std::atoi(f.c_str());
        if (!std::getline(ss, f, '|')) continue; e.seconds = std::atoi(f.c_str());
        if (!std::getline(ss, f, '|')) continue; e.score = std::atoll(f.c_str());
        if (!std::getline(ss, f, '|')) continue; e.when = static_cast<std::time_t>(std::atoll(f.c_str()));
        entries_.push_back(e);
    }
}

void Ranking::save() const {
    std::ofstream out(rankingPath(), std::ios::trunc);
    for (const auto& e : entries_)
        out << e.player << '|' << e.level << '|' << e.comboMax << '|' << e.seconds << '|' << e.score << '|' << static_cast<long long>(e.when) << '\n';
}

void Ranking::add(const RankEntry& e) {
    entries_.push_back(e);
    save();
}

std::vector<RankEntry> Ranking::top(size_t n, bool weekly) const {
    std::time_t now = std::time(nullptr);
    std::tm t = *std::localtime(&now);
    int sinceMonday = (t.tm_wday + 6) % 7;
    t.tm_hour = 0; t.tm_min = 0; t.tm_sec = 0; t.tm_mday -= sinceMonday;
    std::time_t monday = std::mktime(&t);

    std::vector<RankEntry> v;
    for (const auto& e : entries_)
        if (!weekly || e.when >= monday) v.push_back(e);
    // mayor nivel primero, luego mayor puntaje
    std::sort(v.begin(), v.end(), [](const RankEntry& a, const RankEntry& b) {
        return a.level != b.level ? a.level > b.level : a.score > b.score;
    });
    if (v.size() > n) v.resize(n);
    return v;
}
