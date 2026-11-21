#pragma once
#include <ctime>
#include <string>
#include <vector>

// Ranking local (RF-010). Solo entran victorias válidas de perfiles registrados (como vw_ranking).
// DEMO: archivo de texto. Sprint 4: tabla partida + vista vw_ranking en SQLite.
struct RankEntry {
    std::string player;
    int level = 1, comboMax = 0, seconds = 0;
    long long score = 0;
    std::time_t when = 0;
};

class Ranking {
public:
    void load();
    void add(const RankEntry& e);
    std::vector<RankEntry> top(size_t n, bool weekly) const;   // semanal: desde el lunes de esta semana
private:
    void save() const;
    std::vector<RankEntry> entries_;
};
