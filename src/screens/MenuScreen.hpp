#pragma once

#include "Screen.hpp"
#include "../ui/Button.hpp"
#include <SFML/Graphics.hpp>

// ── MenuScreen ────────────────────────────────────────────────────────────────
// The main menu: title text + "Play" and "Quit" buttons.
// ─────────────────────────────────────────────────────────────────────────────
class MenuScreen : public Screen
{
public:
    explicit MenuScreen(Game& game);

    void handleEvent(const sf::Event& event, Game& game) override;
    void update     (float dt,               Game& game) override;
    void render     (sf::RenderWindow& window)           override;

private:
    sf::Text   m_title;
    sf::Text   m_subtitle;

    Button     m_playButton;
    Button     m_quitButton;

    // Subtle animated background gradient overlay
    sf::RectangleShape m_bgOverlay;
    float              m_animTime { 0.f };
};
