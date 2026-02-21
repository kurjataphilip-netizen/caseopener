#pragma once

#include <vector>
#include <functional>
#include <optional>
#include <string>

#include "../items/Item.hpp"

// ── SortMode ──────────────────────────────────────────────────────────────────
enum class SortMode { None, ByRarityAsc, ByRarityDesc, ByValueAsc, ByValueDesc };

// ── Inventory ─────────────────────────────────────────────────────────────────
// Stores a player's collected items.
// Items have stable IDs (slot indices), sort creates a view not a reorder.
// ─────────────────────────────────────────────────────────────────────────────
class Inventory
{
public:
    Inventory() = default;

    // ── Mutation ──────────────────────────────────────────────────────────────
    void addItem  (Item item);
    void addItems (std::vector<Item> items);

    // Returns value of sold item, or 0 if index invalid.
    float sellItem(std::size_t inventoryIndex);

    void clear();

    // ── Sorting ───────────────────────────────────────────────────────────────
    void        setSort(SortMode mode);
    SortMode    sortMode() const { return m_sortMode; }
    void        cycleSort(); // advance through all sort modes

    // ── Views ─────────────────────────────────────────────────────────────────
    // Returns items in current sort order. Indices match sorted view.
    std::vector<const Item*> sortedView() const;

    // Raw item count
    std::size_t count() const { return m_items.size(); }
    bool        empty() const { return m_items.empty(); }

    // Total market value of all items
    float totalValue() const;

    // Item at raw storage index
    const Item* itemAt(std::size_t rawIndex) const;

    // Callback: fired when any mutation occurs (add/sell/sort)
    void setOnChanged(std::function<void()> cb) { m_onChanged = cb; }

    static const char* sortModeName(SortMode m);

private:
    void notifyChanged();

    std::vector<Item>          m_items;     // raw storage (never reordered)
    SortMode                   m_sortMode { SortMode::ByRarityDesc };
    std::function<void()>      m_onChanged;
};
