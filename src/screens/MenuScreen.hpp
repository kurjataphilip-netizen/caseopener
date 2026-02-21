#pragma once

#include "Screen.hpp"
#include "../ui/Button.hpp"
#include <SFML/Graphics.hpp>
#include <vector>
#include <random>

// ── MenuScreen ────────────────────────────────────────────────────────────────
// Main menu with animated background, title, and buttons.
// ─────────────────────────────────────────────────────────────────────────────
class MenuScreen : public Screen
{
public:
    explicit MenuScreen(Game& game);

    void handleEvent(const sf::Event& event, Game& game) override;
    void update     (float dt,               Game& game) override;
    void render     (sf::RenderWindow& window)           override;

private:
    sf::Text           m_title;
    sf::Text           m_subtitle;
    sf::Text           m_hintText;
    sf::RectangleShape m_titleUnderline;

    Button m_playButton;
    Button m_quitButton;

    sf::RectangleShape m_bgOverlay;
    float              m_animTime { 0.f };

    // Decorative corner brackets
    sf::RectangleShape m_cornerTL[2];
    sf::RectangleShape m_cornerBR[2];

    // Procedural background dots
    struct BgDot { float x, y, size, phase; };
    std::vector<BgDot> m_bgDots;
};
