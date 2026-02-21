#pragma once

#include <string>
#include <vector>
#include "../gameplay/Inventory.hpp"

// ── JsonSave ──────────────────────────────────────────────────────────────────
// Saves / loads the complete game state to a JSON file.
//
// Format:
// {
//   "version": 2,
//   "balance": 1234.56,
//   "free_case_cooldown_end": 1700000000.0,   ← Unix timestamp (seconds)
//   "inventory": [
//     { "id": "ak47_redline", "wear": 0.043 },
//     ...
//   ]
// }
//
// Design:
//   - Uses nlohmann/json (fetched automatically via CMake FetchContent).
//   - All fields are optional on load — missing fields use safe defaults.
//   - Unknown item IDs are skipped gracefully.
//   - File is written atomically (temp file + rename) to prevent corruption.
// ─────────────────────────────────────────────────────────────────────────────
class JsonSave
{
public:
    static constexpr int    SAVE_VERSION    = 2;
    static constexpr float  DEFAULT_BALANCE = 1000.f;
    static constexpr double NO_COOLDOWN     = 0.0;

    explicit JsonSave(std::string savePath = "save.json");

    // ── Persistence ───────────────────────────────────────────────────────────
    struct SaveData
    {
        float  balance               { DEFAULT_BALANCE };
        double freeCaseCooldownEnd   { NO_COOLDOWN };   // Unix time_t as double
        // Inventory is read/written directly via Inventory& to avoid copies
    };

    // Returns true on success. Uses atomic write (temp + rename).
    bool save(const SaveData& data, const Inventory& inventory) const;

    // Returns true if file existed and was parsed. Populates outData + outInventory.
    // On failure, outData is set to safe defaults.
    bool load(SaveData& outData, Inventory& outInventory) const;

    bool   exists()     const;
    void   deleteSave() const;
    const  std::string& path() const { return m_path; }

private:
    std::string m_path;
};
