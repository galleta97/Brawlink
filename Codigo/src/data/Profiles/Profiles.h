#pragma once
#include <map>
#include <string>

// Perfiles y progreso (RF-001, RF-009).
// DEMO: se guardan en un archivo de texto en la carpeta de datos.
// Sprint 4: se reemplaza por PerfilRepository / ProgresoRepository sobre SQLite (tablas usuario y progreso).
std::string dataDir();   // %APPDATA%\Brawlink en Windows, ./data en otros sistemas

class Profiles {
public:
    void load();
    void save() const;
    bool exists(const std::string& name) const { return levels_.count(name) > 0; }
    void create(const std::string& name) { if (!exists(name)) { levels_[name] = 1; save(); } }
    int levelMax(const std::string& name) const { auto it = levels_.find(name); return it == levels_.end() ? 1 : it->second; }
    void unlock(const std::string& name, int level) { if (level > levelMax(name)) { levels_[name] = level; save(); } }
private:
    std::map<std::string, int> levels_;
};
