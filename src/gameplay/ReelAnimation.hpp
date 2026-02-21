#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <functional>
#include <optional>

#include "../items/Item.hpp"
#include "../cases/Case.hpp"

// ── ReelItem ──────────────────────────────────────────────────────────────────
// One visible card on the reel strip.
struct ReelItem
{
    Item          item;
    sf::Texture   texture;        // loaded inline (placeholder if missing)
    bool          textureLoaded { false };
};

// ── ReelAnimation ─────────────────────────────────────────────────────────────
// A single horizontal scrolling reel for one case opening.
//
// Layout:
//   [card][card][card][card] ← WINNER CARD →[card][card][card]
//                                ↑ centre marker
//
// Phases:
//   Idle      → waiting to spin
//   Spinning  → accelerates then decelerates to a stop over m_duration seconds
//   Revealing → brief hold + glow on the winning card
//   Done      → fire callback, ready to inspect
// ─────────────────────────────────────────────────────────────────────────────
class ReelAnimation
{
public:
    // ── Constants ─────────────────────────────────────────────────────────────
    static constexpr float CARD_W       = 160.f;
    static constexpr float CARD_H       = 120.f;
    static constexpr float CARD_GAP     = 8.f;
    static constexpr float CARD_STRIDE  = CARD_W + CARD_GAP;
    static constexpr int   VISIBLE_COLS = 7;     // cards shown at once
    static constexpr int   PADDING_CARDS = 20;   // dummy cards on each side of winner
    static constexpr float REEL_W = CARD_STRIDE * VISIBLE_COLS - CARD_GAP;
    static constexpr float REEL_H = CARD_H + 24.f; // extra for label

    enum class Phase { Idle, Spinning, Revealing, Done };

    // ── Construction ──────────────────────────────────────────────────────────
    // bounds: the rectangle this reel occupies on screen.
    explicit ReelAnimation(sf::Vector2f topLeft, const sf::Font& font);

    // ── Setup ─────────────────────────────────────────────────────────────────
    // Call before spinning. Fills the strip with random filler items from the
    // case pool; the winning item is placed at the target scroll position.
    void prepare(const Case& sourceCase, Item winningItem, float durationSecs = 4.5f);

    // ── Control ───────────────────────────────────────────────────────────────
    void start();
    void skip();   // jump instantly to Done

    // Callback fired once when Phase transitions to Done.
    void setOnComplete(std::function<void(const Item&)> cb) { m_onComplete = cb; }

    // ── Per-frame ─────────────────────────────────────────────────────────────
    void update(float dt);
    void render(sf::RenderWindow& window);

    // ── Accessors ─────────────────────────────────────────────────────────────
    Phase              phase()      const { return m_phase; }
    bool               isDone()     const { return m_phase == Phase::Done; }
    const Item*        winner()     const { return m_winnerIndex < m_strip.size()
                                               ? &m_strip[m_winnerIndex].item
                                               : nullptr; }
    sf::Vector2f       topLeft()    const { return m_topLeft; }

private:
    void buildStrip(const Case& sourceCase, Item winningItem);
    void drawCard(sf::RenderWindow& window, const ReelItem& ri,
                  sf::Vector2f centre, bool isWinner, float revealAlpha);
    void transitionToDone();

    // Easing: smooth-stop deceleration (ease-out cubic)
    static float easeOutCubic(float t); // t in [0,1] → [0,1]

    // ── State ─────────────────────────────────────────────────────────────────
    Phase        m_phase    { Phase::Idle };
    sf::Vector2f m_topLeft;
    const sf::Font* m_font  { nullptr };

    // Strip data
    std::vector<ReelItem> m_strip;
    std::size_t           m_winnerIndex { 0 };

    // Scroll state
    float m_scrollOffset  { 0.f };  // current left-edge in strip-space (pixels)
    float m_targetOffset  { 0.f };  // where we want to end up
    float m_startOffset   { 0.f };
    float m_elapsed       { 0.f };
    float m_duration      { 4.5f };

    // Reveal
    float m_revealTimer   { 0.f };
    static constexpr float REVEAL_DURATION = 1.2f;

    // Glow pulse
    float m_glowTimer     { 0.f };

    std::function<void(const Item&)> m_onComplete;

    // Shared shapes (re-used each frame)
    sf::RectangleShape m_cardShape;
    sf::RectangleShape m_markerLine;
    sf::RectangleShape m_reelBg;

    // Placeholder texture used when item has no image
    sf::RectangleShape m_placeholder;
};
