#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <functional>
#include <optional>

#include "../items/Item.hpp"
#include "../cases/Case.hpp"

// ── ReelItem ──────────────────────────────────────────────────────────────────
struct ReelItem
{
    Item item;
    bool textureLoaded { false };
};

// ── LayoutScale ───────────────────────────────────────────────────────────────
// Moved OUTSIDE ReelAnimation so default member initialisers can reference
// ReelAnimation constants without triggering the "incomplete class" error.
struct LayoutScale
{
    float cardW      { 152.f };
    float cardH      { 114.f };
    float cardGap    {   6.f };
    float reelW      { 1190.f };  // CARD_STRIDE * VISIBLE_COLS - CARD_GAP
    float reelH      { 140.f };   // CARD_H + 26
    float cardStride { 158.f };   // CARD_W + CARD_GAP
};

// ── ReelAnimation ─────────────────────────────────────────────────────────────
class ReelAnimation
{
public:
    // ── Layout constants ──────────────────────────────────────────────────────
    static constexpr float CARD_W        = 152.f;
    static constexpr float CARD_H        = 114.f;
    static constexpr float CARD_GAP      =   6.f;
    static constexpr float CARD_STRIDE   = CARD_W + CARD_GAP;
    static constexpr int   VISIBLE_COLS  = 7;
    static constexpr int   PADDING_CARDS = 22;
    static constexpr float REEL_W = CARD_STRIDE * VISIBLE_COLS - CARD_GAP;
    static constexpr float REEL_H = CARD_H + 26.f;

    enum class Phase { Idle, Spinning, Revealing, Done };

    // ── Construction ──────────────────────────────────────────────────────────
    // Default scale = LayoutScale{} is now safe because LayoutScale is a
    // complete type at this point (defined above, outside the class).
    explicit ReelAnimation(sf::Vector2f topLeft, const sf::Font& font,
                           LayoutScale scale = LayoutScale{});

    // ── Setup ─────────────────────────────────────────────────────────────────
    void prepare(const Case& sourceCase, Item winningItem, float durationSecs = 4.5f);

    // ── Control ───────────────────────────────────────────────────────────────
    void start();
    void skip();

    void setOnComplete(std::function<void(const Item&)> cb) { m_onComplete = cb; }

    // ── Per-frame ─────────────────────────────────────────────────────────────
    void update(float dt);
    void render(sf::RenderWindow& window);

    // ── Accessors ─────────────────────────────────────────────────────────────
    Phase        phase()   const { return m_phase; }
    bool         isDone()  const { return m_phase == Phase::Done; }
    const Item*  winner()  const
    {
        return m_winnerIndex < m_strip.size()
               ? &m_strip[m_winnerIndex].item : nullptr;
    }
    sf::Vector2f topLeft() const { return m_topLeft; }
    LayoutScale  scale()   const { return m_scale; }

private:
    void buildStrip(const Case& sourceCase, Item winningItem);
    void drawCard(sf::RenderWindow& window, const ReelItem& ri,
                  sf::Vector2f centre, bool isWinner, float revealAlpha);
    void transitionToDone();
    static float easeOutQuint(float t);

    // ── State ─────────────────────────────────────────────────────────────────
    Phase           m_phase  { Phase::Idle };
    sf::Vector2f    m_topLeft;
    const sf::Font* m_font   { nullptr };
    LayoutScale     m_scale;

    std::vector<ReelItem> m_strip;
    std::size_t           m_winnerIndex { 0 };

    float m_scrollOffset { 0.f };
    float m_targetOffset { 0.f };
    float m_startOffset  { 0.f };
    float m_elapsed      { 0.f };
    float m_duration     { 4.5f };

    float m_revealTimer  { 0.f };
    static constexpr float REVEAL_DURATION = 1.0f;

    float m_glowTimer    { 0.f };

    std::function<void(const Item&)> m_onComplete;

    sf::RectangleShape m_cardShape;
    sf::RectangleShape m_markerLine;
    sf::RectangleShape m_reelBg;
};
