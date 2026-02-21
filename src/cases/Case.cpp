#include "Case.hpp"

#include <numeric>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <cassert>
#include <cmath>

// ── Constructor ───────────────────────────────────────────────────────────────
Case::Case(std::string id,
           std::string displayName,
           std::string imagePath,
           float       price)
    : m_id         (std::move(id))
    , m_displayName(std::move(displayName))
    , m_imagePath  (std::move(imagePath))
    , m_price      (price)
{}

// ── addItem ───────────────────────────────────────────────────────────────────
Case& Case::addItem(const std::string& itemDefId, float weightOverride)
{
    m_pool.push_back({ itemDefId, weightOverride });
    return *this;
}

// ── setRarityOdds ─────────────────────────────────────────────────────────────
Case& Case::setRarityOdds(const RarityOddsOverride& odds)
{
    m_oddsOverride = odds;
    return *this;
}

// ── effectiveRarityWeight ─────────────────────────────────────────────────────
// Price bonus: every $1 above a $1 base adds a tiny multiplier to rarer tiers.
//
//   priceBonus = log2(price / 1.0 + 1)   so $1 → 1.0×, $3 → ~2×, $10 → ~3.5×
//
// The bonus scales the rare-tier weights up while keeping common tiers untouched,
// so an expensive case genuinely shifts odds without breaking the math.
float Case::effectiveRarityWeight(Rarity r) const
{
    // Base weight: override > global default
    const float base = (m_oddsOverride.weights[static_cast<int>(r)] > 0.f)
                     ? m_oddsOverride.weights[static_cast<int>(r)]
                     : rarityDropWeight(r);

    // Price multiplier only boosts tiers >= Restricted (index 3)
    const int tier = static_cast<int>(r);
    if (tier < 3) return base;

    const float priceBonus = std::log2(m_price + 1.f); // $1→1.0, $5→2.58, $25→4.7
    const float tierScale  = 1.f + (tier - 2) * 0.15f; // scales up further for rarer tiers
    return base * priceBonus * tierScale;
}

// ── totalEffectiveWeight ──────────────────────────────────────────────────────
float Case::totalEffectiveWeight() const
{
    float sum = 0.f;
    for (int i = 0; i < 7; ++i)
        sum += effectiveRarityWeight(static_cast<Rarity>(i));
    return sum;
}

// ── oddsString ────────────────────────────────────────────────────────────────
std::string Case::oddsString() const
{
    const float total = totalEffectiveWeight();
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3);

    for (int i = 0; i < 7; ++i)
    {
        const Rarity r    = static_cast<Rarity>(i);
        const float  pct  = (effectiveRarityWeight(r) / total) * 100.f;
        oss << rarityName(r) << ": " << pct << "%\n";
    }
    return oss.str();
}

// ── open ──────────────────────────────────────────────────────────────────────
std::optional<Item> Case::open() const
{
    if (m_pool.empty())
    {
        std::cerr << "[Case:" << m_id << "] Pool is empty — cannot open.\n";
        return std::nullopt;
    }

    // Step 1: roll rarity
    const Rarity rarity = rollRarity();

    // Step 2+3: pick item def
    const ItemDef* def = rollItemDef(rarity);
    if (!def)
    {
        // No pool entry matched the rolled rarity — walk down until we find one.
        for (int fallback = static_cast<int>(rarity); fallback >= 0; --fallback)
        {
            def = rollItemDef(static_cast<Rarity>(fallback));
            if (def) break;
        }
    }
    if (!def)
    {
        std::cerr << "[Case:" << m_id << "] Could not find any matching item def.\n";
        return std::nullopt;
    }

    // Step 4: roll wear
    const float wear = rollWear(rarity);

    return Item(*def, wear);
}

// ── openAndLoad ───────────────────────────────────────────────────────────────
std::optional<Item> Case::openAndLoad() const
{
    auto item = open();
    if (item) item->ensureTextureLoaded();
    return item;
}

// ── rollRarity ────────────────────────────────────────────────────────────────
Rarity Case::rollRarity() const
{
    const float total = totalEffectiveWeight();

    std::uniform_real_distribution<float> dist(0.f, total);
    float roll = dist(m_rng);

    for (int i = 0; i < 7; ++i)
    {
        const float w = effectiveRarityWeight(static_cast<Rarity>(i));
        if (roll < w) return static_cast<Rarity>(i);
        roll -= w;
    }
    return Rarity::Consumer; // fallback (floating-point edge case)
}

// ── rollItemDef ───────────────────────────────────────────────────────────────
// Collects all pool entries whose ItemDef has the target rarity, then does a
// weighted random draw. Per-entry weights default to 1.0 (uniform) unless
// a weightOverride > 0 was set in addItem().
const ItemDef* Case::rollItemDef(Rarity rarity) const
{
    const ItemRegistry& reg = ItemRegistry::instance();

    // Build candidates list
    struct Candidate { const ItemDef* def; float weight; };
    std::vector<Candidate> candidates;
    float totalWeight = 0.f;

    for (const CaseEntry& entry : m_pool)
    {
        const ItemDef* def = reg.find(entry.itemDefId);
        if (!def)
        {
            std::cerr << "[Case:" << m_id << "] Unknown item id: "
                      << entry.itemDefId << "\n";
            continue;
        }
        if (def->rarity != rarity) continue;

        const float w = (entry.weightOverride > 0.f) ? entry.weightOverride : 1.f;
        candidates.push_back({ def, w });
        totalWeight += w;
    }

    if (candidates.empty()) return nullptr;

    std::uniform_real_distribution<float> dist(0.f, totalWeight);
    float roll = dist(m_rng);

    for (const auto& c : candidates)
    {
        if (roll < c.weight) return c.def;
        roll -= c.weight;
    }
    return candidates.back().def; // floating-point safety
}

// ── rollWear ──────────────────────────────────────────────────────────────────
// Wear is sampled from a Beta-like distribution:
//   - Consumer/Industrial: flat uniform [0, 1]
//   - MilSpec/Restricted:  biased toward middle (FT range)
//   - Classified/Covert:   biased toward low wear (FN/MW more likely)
//   - Contraband:          strongly biased toward FN
//
// Implementation: use the minimum of N uniform samples to create right-skew.
float Case::rollWear(Rarity rarity) const
{
    std::uniform_real_distribution<float> dist(0.f, 1.f);

    switch (rarity)
    {
        case Rarity::Consumer:
        case Rarity::Industrial:
            // Flat — any wear equally likely
            return dist(m_rng);

        case Rarity::MilSpec:
        case Rarity::Restricted:
        {
            // Average of 2 rolls → bell curve centered on 0.5 (FT range)
            const float a = dist(m_rng);
            const float b = dist(m_rng);
            return (a + b) / 2.f;
        }

        case Rarity::Classified:
        case Rarity::Covert:
        {
            // Min of 2 rolls → skew toward low wear (FN/MW premium)
            const float a = dist(m_rng);
            const float b = dist(m_rng);
            return std::min(a, b);
        }

        case Rarity::Contraband:
        {
            // Min of 3 rolls → strongly skewed toward FN
            const float a = dist(m_rng);
            const float b = dist(m_rng);
            const float c = dist(m_rng);
            return std::min({ a, b, c });
        }
    }
    return dist(m_rng); // unreachable
}
