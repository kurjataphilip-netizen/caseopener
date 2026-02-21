#pragma once

#include <string>
#include "../gameplay/Inventory.hpp"

// ── PlayerData ────────────────────────────────────────────────────────────────
// Serialises and deserialises the player's balance + inventory to a plain-text
// save file so progress persists between sessions.
//
// Save format (human-readable, easy to debug):
//
//   version 1
//   balance 1234.56
//   item ak47_redline 0.043
//   item karambit_fade 0.002
//   ...
//
// Design notes:
//   - Items are stored by (id, wear) pair.  On load they are re-constructed
//     via ItemRegistry so the full ItemDef is always fresh from code.
//   - If the save file is missing or corrupt, defaults are used silently.
//   - ItemDef ids that no longer exist in ItemRegistry are skipped (forward
//     compatibility when items are removed).
// ─────────────────────────────────────────────────────────────────────────────
class PlayerData
{
public:
    static constexpr float DEFAULT_BALANCE = 1000.f;
    static constexpr int   SAVE_VERSION    = 1;

    explicit PlayerData(std::string savePath = "save.dat");

    // ── Persistence ───────────────────────────────────────────────────────────
    // Returns true on success.  Failures are logged to stderr.
    bool save(float balance, const Inventory& inventory) const;
    bool load(float& outBalance, Inventory& outInventory) const;

    // True if a save file exists on disk.
    bool saveExists() const;

    // Delete the save file (e.g. "New Game").
    void deleteSave() const;

    const std::string& path() const { return m_savePath; }

private:
    std::string m_savePath;
};
