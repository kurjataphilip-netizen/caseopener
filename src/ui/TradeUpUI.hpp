#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <optional>
#include <functional>

#include "../gameplay/TradeUp.hpp"
#include "../gameplay/Inventory.hpp"
#include "../ui/Button.hpp"

// ── TradeUpUI ─────────────────────────────────────────────────────────────────
// Screen panel for the trade-up mechanic.
//
// Layout:
//
//  ┌────────────────────────────────────────────────────────────┐
//  │  TRADE UP                                                  │
//  │  Select 3 items of the same rarity from your inventory     │
//  │                                                            │
//  │  [slot 0]  [slot 1]  [slot 2]   →   [result slot]         │
//  │   (item)    (item)   (+ add)         (? unknown)           │
//  │                                                            │
//  │  ← Scroll inventory below to pick items →                 │
//  │  [item][item][item][item][item][item][item]...             │
//  │                                                            │
//  │  Rarity: Mil-Spec → Restricted           [TRADE UP]        │
//  └────────────────────────────────────────────────────────────┘
//
// Interaction flow:
//   1. Player clicks inventory items → added to trade slots.
//   2. [TRADE UP] button enabled once 3 valid items selected.
//   3. Brief flash animation → result item revealed.
//   4. Result automatically goes to inventory. Callback fires.
// ─────────────────────────────────────────────────────────────────────────────
class TradeUpUI
{
public:
    explicit TradeUpUI(const sf::Font& font, Inventory& inventory,
                       sf::FloatRect bounds);

    // ── Per-frame ─────────────────────────────────────────────────────────────
    void handleEvent(const sf::Event& event, const sf::RenderWindow& window);
    void update(float dt);
    void render(sf::RenderWindow& window);

    // Fired after a successful trade-up, passing the resulting item.
    void setOnTradeComplete(std::function<void(Item)> cb) { m_onComplete = cb; }

    // Resize / reposition.
    void setBounds(sf::FloatRect bounds);

private:
    // ── Slots ─────────────────────────────────────────────────────────────────
    static constexpr int   SLOTS          = TradeUp::ITEMS_REQUIRED;
    static constexpr float SLOT_W         = 110.f;
    static constexpr float SLOT_H         = 95.f;
    static constexpr float RESULT_SLOT_W  = 130.f;
    static constexpr float RESULT_SLOT_H  = 110.f;
    static constexpr float MINI_CARD_W    = 90.f;
    static constexpr float MINI_CARD_H    = 76.f;
    static constexpr float MINI_GAP       = 8.f;

    // ── State ─────────────────────────────────────────────────────────────────
    enum class State { Selecting, Animating, ShowingResult };
    State m_state { State::Selecting };

    std::vector<std::optional<std::size_t>> m_slots;  // view-space inventory indices
    std::optional<Item>                     m_result;
    std::string                             m_validationMsg;
    bool                                    m_validationOk { false };

    // Flash animation
    float m_flashTimer  { 0.f };
    float m_resultTimer { 0.f };
    static constexpr float FLASH_DURATION  = 0.6f;
    static constexpr float RESULT_HOLD     = 3.0f;

    // ── Scroll inventory picker ───────────────────────────────────────────────
    float m_scrollX     { 0.f };
    float m_scrollVel   { 0.f };

    // ── Helpers ───────────────────────────────────────────────────────────────
    void doTradeUp();
    void revalidate();
    bool isSlotFilled(int slot) const;
    void removeFromSlot(int slot);
    void addToNextSlot(std::size_t viewIdx);
    bool isInSlots(std::size_t viewIdx) const;

    void drawInputSlot  (sf::RenderWindow& w, int slot, sf::Vector2f pos);
    void drawResultSlot (sf::RenderWindow& w, sf::Vector2f pos);
    void drawPickerArea (sf::RenderWindow& w);
    void drawMiniCard   (sf::RenderWindow& w, const Item& item,
                         sf::FloatRect rect, bool selected);
    void drawArrow      (sf::RenderWindow& w, sf::Vector2f from, sf::Vector2f to);

    // ── Widgets ───────────────────────────────────────────────────────────────
    const sf::Font*  m_font;
    Inventory&       m_inventory;
    sf::FloatRect    m_bounds;

    Button           m_tradeButton;
    Button           m_clearButton;

    // Shared shapes
    sf::RectangleShape m_slotShape;
    sf::RectangleShape m_bgShape;

    std::function<void(Item)> m_onComplete;

    // Pixel areas
    sf::FloatRect m_slotArea;
    sf::FloatRect m_pickerArea;
};
