#include "PlayerData.hpp"
#include "../items/ItemRegistry.hpp"

#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

// ── Constructor ───────────────────────────────────────────────────────────────
PlayerData::PlayerData(std::string savePath)
    : m_savePath(std::move(savePath))
{}

// ── saveExists ────────────────────────────────────────────────────────────────
bool PlayerData::saveExists() const
{
    return std::filesystem::exists(m_savePath);
}

// ── deleteSave ────────────────────────────────────────────────────────────────
void PlayerData::deleteSave() const
{
    std::error_code ec;
    std::filesystem::remove(m_savePath, ec);
    if (ec)
        std::cerr << "[PlayerData] Could not delete save: " << ec.message() << "\n";
}

// ── save ──────────────────────────────────────────────────────────────────────
bool PlayerData::save(float balance, const Inventory& inventory) const
{
    std::ofstream f(m_savePath, std::ios::trunc);
    if (!f)
    {
        std::cerr << "[PlayerData] Cannot open save file for writing: "
                  << m_savePath << "\n";
        return false;
    }

    f << "version " << SAVE_VERSION << "\n";
    f << "balance " << std::fixed << std::setprecision(2) << balance << "\n";

    // Write items in current sort order so the file is readable by humans too.
    for (const Item* item : inventory.sortedView())
    {
        f << "item " << item->id()
          << " " << std::fixed << std::setprecision(6) << item->wear()
          << "\n";
    }

    f.flush();
    if (!f)
    {
        std::cerr << "[PlayerData] Write error on save file.\n";
        return false;
    }

    std::cout << "[PlayerData] Saved " << inventory.count()
              << " items to " << m_savePath << "\n";
    return true;
}

// ── load ──────────────────────────────────────────────────────────────────────
bool PlayerData::load(float& outBalance, Inventory& outInventory) const
{
    std::ifstream f(m_savePath);
    if (!f)
    {
        // No save file — use defaults silently.
        outBalance = PlayerData::DEFAULT_BALANCE;
        return false;
    }

    outBalance = PlayerData::DEFAULT_BALANCE;
    outInventory.clear();

    std::string line;
    int lineNum = 0;
    int version = 0;
    int itemsLoaded = 0;
    int itemsSkipped = 0;

    while (std::getline(f, line))
    {
        ++lineNum;
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string token;
        iss >> token;

        if (token == "version")
        {
            iss >> version;
            if (version != SAVE_VERSION)
            {
                std::cerr << "[PlayerData] Warning: save version " << version
                          << " != expected " << SAVE_VERSION << "\n";
            }
        }
        else if (token == "balance")
        {
            if (!(iss >> outBalance))
            {
                std::cerr << "[PlayerData] Corrupt balance on line " << lineNum << "\n";
                outBalance = DEFAULT_BALANCE;
            }
        }
        else if (token == "item")
        {
            std::string id;
            float wear = 0.f;
            if (!(iss >> id >> wear))
            {
                std::cerr << "[PlayerData] Corrupt item line " << lineNum << "\n";
                continue;
            }

            const ItemDef* def = ItemRegistry::instance().find(id);
            if (!def)
            {
                std::cerr << "[PlayerData] Unknown item id '" << id
                          << "' on line " << lineNum << " — skipped.\n";
                ++itemsSkipped;
                continue;
            }

            outInventory.addItem(Item(*def, wear));
            ++itemsLoaded;
        }
        else
        {
            std::cerr << "[PlayerData] Unknown token '" << token
                      << "' on line " << lineNum << " — ignored.\n";
        }
    }

    std::cout << "[PlayerData] Loaded balance=$" << outBalance
              << ", items=" << itemsLoaded;
    if (itemsSkipped > 0)
        std::cout << " (skipped " << itemsSkipped << " unknown)";
    std::cout << "\n";

    return true;
}
