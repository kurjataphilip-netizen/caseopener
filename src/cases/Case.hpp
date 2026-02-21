#pragma once

#include <string>
#include <vector>
#include <optional>
#include <random>

#include "../items/Item.hpp"
#include "../items/ItemRegistry.hpp"
#include "../items/Rarity.hpp"

// ── CaseEntry ─────────────────────────────────────────────────────────────────
// One slot in a case's loot pool — ties an ItemDef id to an explicit
// weight override. A weight of 0 means "use the rarity's default weight".
// ─────────────────────────────────────────────────────────────────────────────
struct CaseEntry
{
    std::string itemDefId;      // must exist in ItemRegistry
    float       weightOverride; // 0 = use rarityDropWeight(rarity)
};

// ── RarityOddsOverride ────────────────────────────────────────────────────────
// Cases with a higher price tag can shift the odds toward rarer tiers.
// Supply a full 7-element array (indexed by Rarity) to override the global
// weights from Rarity.hpp for this case only.
// Any value <= 0 falls back to the global default.
// ─────────────────────────────────────────────────────────────────────────────
struct RarityOddsOverride
{
    std::array<float, 7> weights;   // indexed by static_cast<int>(Rarity)

    // Convenience: construct with all zeros (= use globals)
    RarityOddsOverride() { weights.fill(0.f); }

    // Check whether any override is actually set
    bool hasAny() const
    {
        for (float w : weights) if (w > 0.f) return true;
        return false;
    }
};

// ── Case ──────────────────────────────────────────────────────────────────────
// A named loot box with a price, a pool of items, and weighted random opening.
//
// Opening pipeline:
//   1. Roll a Rarity using effective per-rarity weights (global + price bonus).
//   2. Collect all pool entries that match that Rarity.
//   3. Roll a specific item from that sub-pool (uniform or per-entry weight).
//   4. Roll a random wear float within [0, 1] then bias it by rarity tier.
//   5. Return a fully constructed Item.
// ─────────────────────────────────────────────────────────────────────────────
class Case
{
public:
    // ── Construction ──────────────────────────────────────────────────────────
    Case(std::string id,
         std::string displayName,
         std::string imagePath,
         float       price);

    // ── Pool building (fluent API) ────────────────────────────────────────────
    Case& addItem(const std::string& itemDefId, float weightOverride = 0.f);
    Case& setRarityOdds(const RarityOddsOverride& odds);

    // ── Accessors ─────────────────────────────────────────────────────────────
    const std::string& id()          const { return m_id; }
    const std::string& displayName() const { return m_displayName; }
    const std::string& imagePath()   const { return m_imagePath; }
    float              price()       const { return m_price; }

    const std::vector<CaseEntry>& pool() const { return m_pool; }

    // Effective weight for a rarity tier (override > global, scaled by price).
    float effectiveRarityWeight(Rarity r) const;

    // Human-readable odds string, e.g. "Covert: 0.15%"
    std::string oddsString() const;

    // ── Opening ───────────────────────────────────────────────────────────────
    // Returns nullopt only if the pool is empty or no ItemRegistry match found.
    std::optional<Item> open() const;

    // Open and pre-load the item's texture immediately.
    std::optional<Item> openAndLoad() const;

private:
    // Step 1: roll a Rarity tier
    Rarity rollRarity() const;

    // Step 2+3: pick an item def from pool entries matching that rarity
    const ItemDef* rollItemDef(Rarity rarity) const;

    // Step 4: roll wear, biased so rarer items trend slightly toward FN
    float rollWear(Rarity rarity) const;

    // Effective weight sum across all 7 tiers (for normalisation)
    float totalEffectiveWeight() const;

    std::string          m_id;
    std::string          m_displayName;
    std::string          m_imagePath;
    float                m_price;

    std::vector<CaseEntry>  m_pool;
    RarityOddsOverride      m_oddsOverride;

    // Each case gets its own seeded RNG so sequences are independent.
    mutable std::mt19937 m_rng { std::random_device{}() };
};
