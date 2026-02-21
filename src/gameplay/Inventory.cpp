#include "Inventory.hpp"
#include <algorithm>
#include <numeric>

// ── addItem ───────────────────────────────────────────────────────────────────
void Inventory::addItem(Item item)
{
    m_items.push_back(std::move(item));
    notifyChanged();
}

// ── addItems ──────────────────────────────────────────────────────────────────
void Inventory::addItems(std::vector<Item> items)
{
    for (auto& item : items)
        m_items.push_back(std::move(item));
    notifyChanged();
}

// ── sellItem ──────────────────────────────────────────────────────────────────
// inventoryIndex refers to the sorted view index, so UI indices stay correct.
float Inventory::sellItem(std::size_t viewIndex)
{
    // Map sorted view index → raw index
    auto view = sortedView();
    if (viewIndex >= view.size()) return 0.f;

    const Item* ptr = view[viewIndex];
    float val = ptr->value();

    // Find raw index
    for (std::size_t i = 0; i < m_items.size(); ++i)
    {
        if (&m_items[i] == ptr)
        {
            m_items.erase(m_items.begin() + static_cast<std::ptrdiff_t>(i));
            break;
        }
    }

    notifyChanged();
    return val;
}

// ── clear ─────────────────────────────────────────────────────────────────────
void Inventory::clear()
{
    m_items.clear();
    notifyChanged();
}

// ── setSort ───────────────────────────────────────────────────────────────────
void Inventory::setSort(SortMode mode)
{
    m_sortMode = mode;
    notifyChanged();
}

// ── cycleSort ─────────────────────────────────────────────────────────────────
void Inventory::cycleSort()
{
    const int next = (static_cast<int>(m_sortMode) + 1) % 5;
    setSort(static_cast<SortMode>(next));
}

// ── sortedView ────────────────────────────────────────────────────────────────
std::vector<const Item*> Inventory::sortedView() const
{
    std::vector<const Item*> view;
    view.reserve(m_items.size());
    for (const auto& item : m_items) view.push_back(&item);

    switch (m_sortMode)
    {
        case SortMode::None: break;

        case SortMode::ByRarityDesc:
            std::stable_sort(view.begin(), view.end(),
                [](const Item* a, const Item* b){
                    return static_cast<int>(a->rarity()) > static_cast<int>(b->rarity());
                });
            break;

        case SortMode::ByRarityAsc:
            std::stable_sort(view.begin(), view.end(),
                [](const Item* a, const Item* b){
                    return static_cast<int>(a->rarity()) < static_cast<int>(b->rarity());
                });
            break;

        case SortMode::ByValueDesc:
            std::stable_sort(view.begin(), view.end(),
                [](const Item* a, const Item* b){ return a->value() > b->value(); });
            break;

        case SortMode::ByValueAsc:
            std::stable_sort(view.begin(), view.end(),
                [](const Item* a, const Item* b){ return a->value() < b->value(); });
            break;
    }
    return view;
}

// ── totalValue ────────────────────────────────────────────────────────────────
float Inventory::totalValue() const
{
    float sum = 0.f;
    for (const auto& item : m_items) sum += item.value();
    return sum;
}

// ── itemAt ────────────────────────────────────────────────────────────────────
const Item* Inventory::itemAt(std::size_t rawIndex) const
{
    if (rawIndex >= m_items.size()) return nullptr;
    return &m_items[rawIndex];
}

// ── sortModeName ──────────────────────────────────────────────────────────────
const char* Inventory::sortModeName(SortMode m)
{
    switch (m)
    {
        case SortMode::None:          return "Default";
        case SortMode::ByRarityDesc:  return "Rarity ↓";
        case SortMode::ByRarityAsc:   return "Rarity ↑";
        case SortMode::ByValueDesc:   return "Value ↓";
        case SortMode::ByValueAsc:    return "Value ↑";
    }
    return "?";
}

// ── notifyChanged ─────────────────────────────────────────────────────────────
void Inventory::notifyChanged()
{
    if (m_onChanged) m_onChanged();
}
