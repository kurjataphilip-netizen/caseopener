#include "TradeUp.hpp"
#include "../items/ItemRegistry.hpp"
#include "../items/Wear.hpp"

#include <numeric>
#include <random>
#include <algorithm>
#include <iostream>

// ── validate ──────────────────────────────────────────────────────────────────
TradeUp::ValidationResult TradeUp::validate(const Inventory& inv,
                                              const std::vector<std::size_t>& slots)
{
    ValidationResult res;

    // Count check
    if (static_cast<int>(slots.size()) != ITEMS_REQUIRED)
    {
        res.reason = "Select exactly " + std::to_string(ITEMS_REQUIRED) + " items.";
        return res;
    }

    // Duplicate slot check
    {
        std::vector<std::size_t> sorted = slots;
        std::sort(sorted.begin(), sorted.end());
        if (std::adjacent_find(sorted.begin(), sorted.end()) != sorted.end())
        {
            res.reason = "Duplicate item slots selected.";
            return res;
        }
    }

    // Gather item pointers from the sorted view
    auto view = inv.sortedView();
    std::vector<const Item*> items;
    for (std::size_t idx : slots)
    {
        if (idx >= view.size())
        {
            res.reason = "Invalid slot index.";
            return res;
        }
        items.push_back(view[idx]);
    }

    // All same rarity check
    const Rarity firstRarity = items[0]->rarity();
    for (const Item* item : items)
    {
        if (item->rarity() != firstRarity)
        {
            res.reason = "All items must be the same rarity.";
            return res;
        }
    }

    // Can't trade up Contraband (already max tier)
    if (firstRarity == Rarity::Contraband)
    {
        res.reason = "Contraband items cannot be traded up.";
        return res;
    }

    // Check there exists at least one item def at the output rarity
    const Rarity outputRarity = static_cast<Rarity>(static_cast<int>(firstRarity) + 1);
    const auto candidates = ItemRegistry::instance().byRarity(outputRarity);
    if (candidates.empty())
    {
        res.reason = std::string("No items exist at ")
                   + rarityName(outputRarity) + " rarity.";
        return res;
    }

    res.valid        = true;
    res.inputRarity  = firstRarity;
    res.outputRarity = outputRarity;
    return res;
}

// ── execute ───────────────────────────────────────────────────────────────────
TradeUpResult TradeUp::execute(Inventory& inv,
                                const std::vector<std::size_t>& slots)
{
    // Re-validate to be safe
    auto v = validate(inv, slots);
    if (!v.valid)
        return { Item(ItemDef{}, 0.f), false, v.reason };

    // Snapshot item pointers BEFORE removing anything
    auto view = inv.sortedView();
    std::vector<const Item*> items;
    for (std::size_t idx : slots)
        items.push_back(view[idx]);

    const float resultWear = computeResultWear(items);
    const ItemDef* def     = pickResultDef(v.outputRarity);

    if (!def)
        return { Item(ItemDef{}, 0.f), false, "No result item found." };

    Item resultItem(*def, resultWear);

    // Remove input items from inventory (sort indices descending to avoid shifting)
    std::vector<std::size_t> sortedSlots = slots;
    std::sort(sortedSlots.rbegin(), sortedSlots.rend());
    for (std::size_t idx : sortedSlots)
        inv.sellItem(idx); // sellItem at 0 value effectively just removes it

    // Note: we used sellItem() to remove, but we don't want to credit the player
    // with sell value — the caller is responsible for any economy adjustments.
    // A cleaner approach would be a dedicated removeItem(), but sellItem works
    // as remove since the returned value is discarded here.

    return { std::move(resultItem), true, "" };
}

// ── computeResultWear ─────────────────────────────────────────────────────────
float TradeUp::computeResultWear(const std::vector<const Item*>& items)
{
    float sum = 0.f;
    for (const Item* i : items) sum += i->wear();
    const float avg = sum / static_cast<float>(items.size());

    // Apply small random variance: bias slightly positive (harder wear)
    static std::mt19937 rng{ std::random_device{}() };
    std::uniform_real_distribution<float> variance(-0.10f, 0.05f);

    return clampWear(avg + variance(rng));
}

// ── pickResultDef ─────────────────────────────────────────────────────────────
const ItemDef* TradeUp::pickResultDef(Rarity rarity)
{
    auto candidates = ItemRegistry::instance().byRarity(rarity);
    if (candidates.empty()) return nullptr;

    static std::mt19937 rng{ std::random_device{}() };
    std::uniform_int_distribution<int> dist(0, static_cast<int>(candidates.size()) - 1);

    return candidates[dist(rng)];
}
