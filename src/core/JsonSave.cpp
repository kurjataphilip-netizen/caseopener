#include "JsonSave.hpp"
#include "../items/ItemRegistry.hpp"

#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <chrono>

using json = nlohmann::json;

// ── Constructor ───────────────────────────────────────────────────────────────
JsonSave::JsonSave(std::string savePath)
    : m_path(std::move(savePath))
{}

// ── exists ────────────────────────────────────────────────────────────────────
bool JsonSave::exists() const
{
    return std::filesystem::exists(m_path);
}

// ── deleteSave ────────────────────────────────────────────────────────────────
void JsonSave::deleteSave() const
{
    std::error_code ec;
    std::filesystem::remove(m_path, ec);
}

// ── save ──────────────────────────────────────────────────────────────────────
bool JsonSave::save(const SaveData& data, const Inventory& inventory) const
{
    json j;
    j["version"]                 = SAVE_VERSION;
    j["balance"]                 = data.balance;
    j["free_case_cooldown_end"]  = data.freeCaseCooldownEnd;

    json itemsArr = json::array();
    for (const Item* item : inventory.sortedView())
    {
        json entry;
        entry["id"]   = item->id();
        entry["wear"] = item->wear();
        itemsArr.push_back(std::move(entry));
    }
    j["inventory"] = std::move(itemsArr);

    // Atomic write: write to .tmp then rename
    const std::string tmpPath = m_path + ".tmp";
    {
        std::ofstream f(tmpPath);
        if (!f)
        {
            std::cerr << "[JsonSave] Cannot open tmp file: " << tmpPath << "\n";
            return false;
        }
        f << j.dump(2); // pretty-print with 2-space indent
        if (!f)
        {
            std::cerr << "[JsonSave] Write error.\n";
            return false;
        }
    }

    std::error_code ec;
    std::filesystem::rename(tmpPath, m_path, ec);
    if (ec)
    {
        std::cerr << "[JsonSave] Rename failed: " << ec.message() << "\n";
        // Fall back: copy manually
        std::filesystem::copy_file(tmpPath, m_path,
            std::filesystem::copy_options::overwrite_existing, ec);
        std::filesystem::remove(tmpPath, ec);
        if (ec) return false;
    }

    std::cout << "[JsonSave] Saved " << inventory.count()
              << " items, balance=$" << data.balance << "\n";
    return true;
}

// ── load ──────────────────────────────────────────────────────────────────────
bool JsonSave::load(SaveData& outData, Inventory& outInventory) const
{
    outData = SaveData{}; // reset to defaults
    outInventory.clear();

    if (!exists())
    {
        std::cout << "[JsonSave] No save file found — using defaults.\n";
        return false;
    }

    std::ifstream f(m_path);
    if (!f)
    {
        std::cerr << "[JsonSave] Cannot open: " << m_path << "\n";
        return false;
    }

    json j;
    try { j = json::parse(f); }
    catch (const json::parse_error& e)
    {
        std::cerr << "[JsonSave] Parse error: " << e.what() << "\n";
        return false;
    }

    // Version check
    const int version = j.value("version", 0);
    if (version < 1)
    {
        std::cerr << "[JsonSave] Unsupported save version: " << version << "\n";
        return false;
    }

    outData.balance              = j.value("balance",                (float)SaveData{}.balance);
    outData.freeCaseCooldownEnd  = j.value("free_case_cooldown_end", SaveData{}.freeCaseCooldownEnd);

    int loaded = 0, skipped = 0;
    if (j.contains("inventory") && j["inventory"].is_array())
    {
        for (const auto& entry : j["inventory"])
        {
            if (!entry.contains("id") || !entry.contains("wear")) continue;

            const std::string id   = entry["id"].get<std::string>();
            const float       wear = entry["wear"].get<float>();

            const ItemDef* def = ItemRegistry::instance().find(id);
            if (!def)
            {
                std::cerr << "[JsonSave] Unknown item '" << id << "' — skipped.\n";
                ++skipped;
                continue;
            }
            outInventory.addItem(Item(*def, wear));
            ++loaded;
        }
    }

    std::cout << "[JsonSave] Loaded balance=$" << outData.balance
              << ", items=" << loaded;
    if (skipped) std::cout << " (" << skipped << " skipped)";
    std::cout << "\n";
    return true;
}
