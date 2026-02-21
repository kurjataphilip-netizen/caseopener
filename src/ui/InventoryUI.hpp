#pragma once

#include <SFML/Graphics.hpp>
#include <optional>
#include <functional>

#include "../gameplay/Inventory.hpp"
#include "Button.hpp"

// ── InventoryUI ───────────────────────────────────────────────────────────────
// Renders the player's inventory as a scrollable grid of item cards.
// Clicking a card opens an inspect panel with item details and a Sell button.
//
// Layout (within a given bounds rectangle):
//
//   ┌─────────────────────────────────────────────────┐
//   │  Sort: [Rarity ↓]    Total value: $1,234.56      │  ← header
//   ├─────────────────────────────────────────────────┤
//   │  [card][card][card][card][card][card]            │
//   │  [card][card][card][card][card][card]  ← grid    │
//   │  [card][card]                                   │
//   ├─────────────────────────────────────────────────┤
//   │  ↑↓ scroll hint                                 │  ← footer
//   └─────────────────────────────────────────────────┘
//
//   When a card is selected, an inspect panel slides in from the right:
//   ┌──────────────────┐
//   │  [item image]    │
//   │  Name            │
//   │  Rarity / Wear   │
//   │  Value: $xx.xx   │
//   │  [Sell]  [Close] │
//   └──────────────────┘
// ─────────────────────────────────────────────────────────────────────────────
class InventoryUI
{
public:
    static constexpr float CARD_W    = 110.f;
    static constexpr float CARD_H    = 90.f;
    static constexpr float CARD_GAP  = 10.f;
    static constexpr float HEADER_H  = 44.f;
    static constexpr float FOOTER_H  = 28.f;

    // bounds: the screen rectangle this widget occupies.
    InventoryUI(sf::FloatRect bounds, const sf::Font& font, Inventory& inventory);

    // ── Per-frame ─────────────────────────────────────────────────────────────
    void handleEvent(const sf::Event& event, const sf::RenderWindow& window);
    void update(float dt);
    void render(sf::RenderWindow& window);

    // Callback: fired when the player sells an item; argument is sell value.
    void setOnSell(std::function<void(float)> cb) { m_onSell = cb; }

    // Recompute grid layout (call after window resize or bounds change).
    void setBounds(sf::FloatRect bounds);

    // Force-close any open inspect panel.
    void closeInspect();

    bool isInspecting() const { return m_inspectIndex.has_value(); }

private:
    void rebuildGrid();
    void openInspect(std::size_t viewIndex);
    void drawHeader(sf::RenderWindow& window);
    void drawGrid  (sf::RenderWindow& window);
    void drawCard  (sf::RenderWindow& window, const Item& item,
                    sf::FloatRect cardRect, bool selected);
    void drawInspectPanel(sf::RenderWindow& window);
    void drawEmptyState  (sf::RenderWindow& window);

    // Scroll helpers
    void    clampScroll();
    float   maxScrollY() const;
    int     colCount()   const;
    sf::FloatRect cardRect(int col, int row) const; // in screen space

    // ── Data ──────────────────────────────────────────────────────────────────
    sf::FloatRect        m_bounds;
    const sf::Font*      m_font;
    Inventory&           m_inventory;

    float                m_scrollY    { 0.f };
    float                m_scrollVel  { 0.f }; // momentum

    std::optional<std::size_t> m_inspectIndex; // view-space index

    // ── Inspect panel ─────────────────────────────────────────────────────────
    sf::RectangleShape   m_inspectBg;
    Button               m_sellButton;
    Button               m_closeButton;
    sf::RectangleShape   m_inspectImageBox;

    // ── Header widgets ────────────────────────────────────────────────────────
    Button               m_sortButton;

    // ── Shared shapes ─────────────────────────────────────────────────────────
    sf::RectangleShape   m_gridBg;
    sf::RectangleShape   m_headerBg;
    sf::RectangleShape   m_cardShape;

    std::function<void(float)> m_onSell;

    // Grid area (bounds minus header/footer)
    sf::FloatRect        m_gridArea;

    // Inspect panel dimensions
    static constexpr float PANEL_W   = 260.f;
    static constexpr float PANEL_H   = 340.f;
};
