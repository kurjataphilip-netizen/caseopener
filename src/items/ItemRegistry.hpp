#pragma once

#include "Item.hpp"
#include <vector>
#include <string>
#include <optional>
#include <unordered_map>

// ── ItemRegistry ──────────────────────────────────────────────────────────────
// Singleton-style store of every ItemDef in the game.
//
// Usage:
//   ItemRegistry& reg = ItemRegistry::instance();
//   reg.load();                              // call once at startup
//
//   const ItemDef* def = reg.find("ak47_redline");
//   Item item = reg.makeItem("ak47_redline", 0.043f);
//   Item random = reg.makeRandomItem("ak47_redline");  // random wear in def tier
//
//   // Get all defs at a given rarity for a case pool:
//   auto coverts = reg.byRarity(Rarity::Covert);
// ─────────────────────────────────────────────────────────────────────────────
class ItemRegistry
{
public:
    static ItemRegistry& instance();

    // Registers all built-in item definitions.
    // Safe to call multiple times — subsequent calls are no-ops.
    void load();

    // ── Lookup ────────────────────────────────────────────────────────────────
    // Returns nullptr if the id is not found.
    const ItemDef* find(const std::string& id) const;

    // All definitions, in registration order.
    const std::vector<ItemDef>& all() const { return m_defs; }

    // All definitions with a specific rarity.
    std::vector<const ItemDef*> byRarity(Rarity r) const;

    // ── Factory ───────────────────────────────────────────────────────────────
    // Returns std::nullopt if id is not found.
    std::optional<Item> makeItem(const std::string& id, float wear) const;

    // Picks a random wear value inside [0.00, 1.00] for the created item.
    std::optional<Item> makeRandomItem(const std::string& id) const;

    // Picks a random wear value within a specific tier.
    std::optional<Item> makeItemInTier(const std::string& id, WearTier tier) const;

private:
    ItemRegistry() = default;

    void registerDef(ItemDef def);

    std::vector<ItemDef>                       m_defs;
    std::unordered_map<std::string, std::size_t> m_index; // id → index in m_defs
    bool m_loaded { false };
};
