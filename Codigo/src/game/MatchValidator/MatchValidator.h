#pragma once
// RNF-001: antes de guardar, la partida debe ser posible.
// Si falla, se marca como sospechosa y no entra al ranking.
struct MatchValidator {
    static bool valid(long long score, float seconds, long long maxTheoretical, float minSeconds) {
        return score >= 0 && score <= maxTheoretical && seconds >= minSeconds;
    }
};
