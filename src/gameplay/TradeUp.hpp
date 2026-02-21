#pragma once

#include <vector>
#include <optional>
#include <string>
#include "../items/Item.hpp"
#include "../gameplay/Inventory.hpp"

// ── TradeUpResult ─────────────────────────────────────────────────────────────
struct TradeUpResult
{
    Item        item;          // the resulting item
    bool        success;       // false = error (shouldn't happen if validated first)
    std::string errorMsg;      // populated on failure
};

// ── TradeUp ───────────────────────────────────────────────────────────────────
// Combines 3 items of the same rarity into 1 item of the next rarity tier.
//
// Rules:
//   - Requires exactly ITEMS_REQUIRED items (all same rarity).
//   - Contraband items cannot be traded up (already max tier).
//   - Result rarity  = input rarity + 1.
//   - Result wear    = weighted average of input wears, ± small random variance.
//   - Result item    = random item def from ItemRegistry at the result rarity.
//
// Wear formula (CS-inspired):
//   averageWear = mean(input wears)
//   resultWear  = clamp(averageWear + uniform(-0.10, +0.05), 0, 1)
//
// The small positive bias means trade-ups trend very slightly worse than their
// inputs, which is authentic to CS mechanics.
// ─────────────────────────────────────────────────────────────────────────────
class TradeUp
{
public:
    static constexpr int ITEMS_REQUIRED = 3;

    // ── Validation ────────────────────────────────────────────────────────────
    struct ValidationResult
    {
        bool        valid     { false };
        std::string reason;           // human-readable error if !valid
        Rarity      inputRarity  {};
        Rarity      outputRarity {};
    };

    // Validate a proposed trade-up (does NOT consume items).
    // slotIndices: inventory view-space indices of the selected items.
    static ValidationResult validate(const Inventory& inv,
                                     const std::vector<std::size_t>& slotIndices);

    // ── Execution ─────────────────────────────────────────────────────────────
    // Removes items from inventory (by view-space indices) and returns result.
    // Always call validate() first.
    static TradeUpResult execute(Inventory& inv,
                                 const std::vector<std::size_t>& slotIndices);

private:
    // Compute result wear from input items
    static float computeResultWear(const std::vector<const Item*>& items);

    // Pick a random ItemDef at the given rarity from the registry
    static const ItemDef* pickResultDef(Rarity rarity);
};
