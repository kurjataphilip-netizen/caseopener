#pragma once

#include <SFML/Graphics.hpp>
#include "../cases/Case.hpp"
#include "../ui/Button.hpp"

// ── CaseInfoPanel ─────────────────────────────────────────────────────────────
// Modal overlay that shows detailed information about a case:
//   - Case name + price
//   - Item pool list (grouped by rarity, colour-coded)
//   - Per-rarity drop odds as animated bar chart
//   - [Close] button
//
// Shown when the player clicks an info icon on the case selector.
//
// Usage:
//   CaseInfoPanel panel(font);
//   panel.open(*casePtr);      // slide-in animation starts
//   panel.handleEvent(...);
//   panel.update(dt);
//   panel.render(window);
//   if (!panel.isOpen()) { /* dismissed */ }
// ─────────────────────────────────────────────────────────────────────────────
class CaseInfoPanel
{
public:
    static constexpr float PANEL_W  = 480.f;
    static constexpr float PANEL_H  = 540.f;

    explicit CaseInfoPanel(const sf::Font& font);

    // Opens the panel for the given case with a slide-in animation.
    void open(const Case& c);

    // Dismisses the panel with a slide-out animation.
    void close();

    bool isOpen()    const { return m_open; }
    bool isVisible() const { return m_slideT > 0.f || m_open; }

    void handleEvent(const sf::Event& event, const sf::RenderWindow& window);
    void update(float dt);
    void render(sf::RenderWindow& window);

private:
    void buildContent();
    void drawOddsBar(sf::RenderWindow& window, const sf::FloatRect& row,
                     float fraction, sf::Color colour, const std::string& label,
                     const std::string& pctStr);

    const sf::Font*    m_font   { nullptr };
    const Case*        m_case   { nullptr };

    bool               m_open   { false };
    float              m_slideT { 0.f };   // 0 = hidden, 1 = fully shown
    static constexpr float SLIDE_SPEED = 4.f; // panels/second

    // Panel position: slides in from right
    sf::Vector2f panelPos() const;

    // Widgets
    sf::RectangleShape m_bg;
    sf::RectangleShape m_titleBar;
    sf::RectangleShape m_dimOverlay;
    Button             m_closeBtn;

    // Pre-built text lines (rebuilt on open())
    struct TextLine { std::string str; sf::Color colour; unsigned int size; };
    std::vector<TextLine> m_poolLines;

    // Odds (computed on open)
    struct OddsEntry { std::string name; sf::Color colour; float fraction; std::string pct; };
    std::vector<OddsEntry> m_odds;

    // Animated bar fill (0→fraction over REVEAL_TIME seconds)
    float m_barRevealT { 0.f };
    static constexpr float BAR_REVEAL_TIME = 0.8f;
};
