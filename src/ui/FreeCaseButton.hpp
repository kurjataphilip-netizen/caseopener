#pragma once

#include <SFML/Graphics.hpp>
#include <functional>
#include <string>
#include "../core/FreeCaseReward.hpp"
#include "HoverEffect.hpp"

// ── FreeCaseButton ────────────────────────────────────────────────────────────
// A special button that shows a circular countdown arc and glows gold
// when the free case reward becomes available.
//
// Usage:
//   FreeCaseButton btn(font, reward, {x, y});
//   btn.setOnClaim([](){ ... });
//   btn.update(dt);
//   btn.handleEvent(event, window);
//   btn.render(window);
// ─────────────────────────────────────────────────────────────────────────────
class FreeCaseButton
{
public:
    static constexpr float SIZE = 180.f;  // square bounding box
    static constexpr float ARC_RADIUS = 70.f;
    static constexpr int   ARC_POINTS = 60;

    FreeCaseButton() = default;
    FreeCaseButton(const sf::Font& font,
                   FreeCaseReward& reward,
                   sf::Vector2f position);

    void setPosition(sf::Vector2f pos);
    void setOnClaim(std::function<void()> cb) { m_onClaim = cb; }

    void handleEvent(const sf::Event& event, const sf::RenderWindow& window);
    void update(float dt);
    void render(sf::RenderWindow& window);

    sf::FloatRect bounds() const;

private:
    void rebuildArc(float progress);
    void drawArc   (sf::RenderWindow& window, float progress);

    const sf::Font*   m_font   { nullptr };
    FreeCaseReward*   m_reward { nullptr };
    sf::Vector2f      m_pos;

    HoverEffect       m_hover { 5.f };
    float             m_glowTimer { 0.f };

    sf::RectangleShape  m_bg;
    sf::CircleShape     m_arcBg;   // grey background circle
    sf::VertexArray     m_arcFill; // progress arc (triangle fan)

    std::function<void()> m_onClaim;
};
