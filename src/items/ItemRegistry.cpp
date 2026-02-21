#include "ItemRegistry.hpp"

#include <random>
#include <stdexcept>
#include <iostream>

// ── instance ──────────────────────────────────────────────────────────────────
ItemRegistry& ItemRegistry::instance()
{
    static ItemRegistry s_instance;
    return s_instance;
}

// ── load ──────────────────────────────────────────────────────────────────────
// Define every item in the game here.
// imagePath is relative to the binary (e.g. assets/images/<file>.png).
// baseValue is a USD-equivalent price at Field-Tested condition; the rarity and
// wear multipliers in Rarity.hpp / Wear.hpp scale it up or down.
//
// To add a new item: append a registerDef({...}) call.
// ─────────────────────────────────────────────────────────────────────────────
void ItemRegistry::load()
{
    if (m_loaded) return;
    m_loaded = true;

    // ── Consumer Grade ────────────────────────────────────────────────────────
    registerDef({ "glock_sand_dune",
                  "Glock-18 | Sand Dune",
                  Rarity::Consumer, 0.03f,
                  "assets/images/glock_sand_dune.png" });

    registerDef({ "p2000_silver",
                  "P2000 | Silver",
                  Rarity::Consumer, 0.04f,
                  "assets/images/p2000_silver.png" });

    // ── Industrial Grade ──────────────────────────────────────────────────────
    registerDef({ "mp9_storm",
                  "MP9 | Storm",
                  Rarity::Industrial, 0.15f,
                  "assets/images/mp9_storm.png" });

    registerDef({ "nova_gradan",
                  "Nova | Gra•dan",
                  Rarity::Industrial, 0.18f,
                  "assets/images/nova_gradan.png" });

    // ── Mil-Spec ──────────────────────────────────────────────────────────────
    registerDef({ "m4a4_radiation_hazard",
                  "M4A4 | Radiation Hazard",
                  Rarity::MilSpec, 0.60f,
                  "assets/images/m4a4_radiation_hazard.png" });

    registerDef({ "famas_mecha_industries",
                  "FAMAS | Mecha Industries",
                  Rarity::MilSpec, 0.55f,
                  "assets/images/famas_mecha_industries.png" });

    registerDef({ "hkp2000_corticera",
                  "HKP2000 | Corticera",
                  Rarity::MilSpec, 0.50f,
                  "assets/images/hkp2000_corticera.png" });

    // ── Restricted ────────────────────────────────────────────────────────────
    registerDef({ "ak47_uncharted",
                  "AK-47 | Uncharted",
                  Rarity::Restricted, 2.00f,
                  "assets/images/ak47_uncharted.png" });

    registerDef({ "mp5sd_phosphor",
                  "MP5-SD | Phosphor",
                  Rarity::Restricted, 1.80f,
                  "assets/images/mp5sd_phosphor.png" });

    // ── Classified ────────────────────────────────────────────────────────────
    registerDef({ "awp_containment_breach",
                  "AWP | Containment Breach",
                  Rarity::Classified, 8.00f,
                  "assets/images/awp_containment_breach.png" });

    registerDef({ "m4a1s_welcome_to_the_jungle",
                  "M4A1-S | Welcome to the Jungle",
                  Rarity::Classified, 7.50f,
                  "assets/images/m4a1s_welcome_to_the_jungle.png" });

    // ── Covert ────────────────────────────────────────────────────────────────
    registerDef({ "ak47_redline",
                  "AK-47 | Redline",
                  Rarity::Covert, 15.00f,
                  "assets/images/ak47_redline.png" });

    registerDef({ "m4a4_howl",
                  "M4A4 | Howl",
                  Rarity::Covert, 20.00f,
                  "assets/images/m4a4_howl.png" });

    // ── Contraband ────────────────────────────────────────────────────────────
    registerDef({ "karambit_fade",
                  "Karambit | Fade",
                  Rarity::Contraband, 60.00f,
                  "assets/images/karambit_fade.png" });

    std::cout << "[ItemRegistry] Loaded " << m_defs.size() << " item definitions.\n";
}

// ── find ──────────────────────────────────────────────────────────────────────
const ItemDef* ItemRegistry::find(const std::string& id) const
{
    auto it = m_index.find(id);
    if (it == m_index.end()) return nullptr;
    return &m_defs[it->second];
}

// ── byRarity ──────────────────────────────────────────────────────────────────
std::vector<const ItemDef*> ItemRegistry::byRarity(Rarity r) const
{
    std::vector<const ItemDef*> result;
    for (const auto& def : m_defs)
        if (def.rarity == r)
            result.push_back(&def);
    return result;
}

// ── makeItem ──────────────────────────────────────────────────────────────────
std::optional<Item> ItemRegistry::makeItem(const std::string& id, float wear) const
{
    const ItemDef* def = find(id);
    if (!def) return std::nullopt;
    return Item(*def, wear);
}

// ── makeRandomItem ────────────────────────────────────────────────────────────
std::optional<Item> ItemRegistry::makeRandomItem(const std::string& id) const
{
    const ItemDef* def = find(id);
    if (!def) return std::nullopt;

    static std::mt19937 rng{ std::random_device{}() };
    std::uniform_real_distribution<float> dist(0.f, 1.f);

    return Item(*def, dist(rng));
}

// ── makeItemInTier ────────────────────────────────────────────────────────────
std::optional<Item> ItemRegistry::makeItemInTier(const std::string& id, WearTier tier) const
{
    const ItemDef* def = find(id);
    if (!def) return std::nullopt;
    return Item::makeWithRandomWear(*def, tier);
}

// ── registerDef ───────────────────────────────────────────────────────────────
void ItemRegistry::registerDef(ItemDef def)
{
    if (m_index.count(def.id))
    {
        std::cerr << "[ItemRegistry] Duplicate item id: " << def.id << " — skipped.\n";
        return;
    }
    m_index[def.id] = m_defs.size();
    m_defs.push_back(std::move(def));
}
