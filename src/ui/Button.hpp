#pragma once

#include <SFML/Graphics.hpp>
#include <functional>
#include <string>
#include "HoverEffect.hpp"

// ── Button ────────────────────────────────────────────────────────────────────
// Self-contained clickable button with smooth hover animation,
// press state, optional pulse effect, and callback on click-release.
// ─────────────────────────────────────────────────────────────────────────────
class Button : public sf::Drawable
{
public:
    enum class State { Idle, Hovered, Pressed };

    Button() = default;
    Button(const sf::Font& font,
           const std::string& label,
           sf::Vector2f position,
           sf::Vector2f size,
           unsigned int charSize = 22);

    // ── Configuration ─────────────────────────────────────────────────────────
    void setLabel   (const std::string& label);
    void setPosition(sf::Vector2f pos);
    void setSize    (sf::Vector2f size);
    void setOnClick (std::function<void()> callback);

    void setIdleColor   (sf::Color c);
    void setHoverColor  (sf::Color c);
    void setPressColor  (sf::Color c);
    void setTextColor   (sf::Color c);
    void setOutlineColor(sf::Color c, float thickness = 2.f);

    // Enable a subtle idle "breathing" pulse effect (useful for CTA buttons)
    void setPulse(bool enabled, sf::Color pulseColour = sf::Color(220,180,60,60))
    {
        m_pulseEnabled = enabled;
        m_pulseColour  = pulseColour;
    }

    // ── Per-frame ─────────────────────────────────────────────────────────────
    void update(sf::Vector2i mousePos, float dt = 0.016f);
    void handleEvent(const sf::Event& event, const sf::RenderWindow& window);

    // ── Accessors ─────────────────────────────────────────────────────────────
    State         state()    const { return m_state; }
    sf::FloatRect bounds()   const { return m_rect.getGlobalBounds(); }
    bool          isHovered()const { return m_state == State::Hovered; }

    // ── Hover animation value (0–1) for external use ──────────────────────────
    float hoverT() const { return m_hover.value(); }

private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
    void applyVisuals(float dt);
    void centreText();

    sf::RectangleShape m_rect;
    sf::Text           m_text;
    State              m_state { State::Idle };

    std::function<void()> m_onClick;

    sf::Color m_idleColor  { 35,  35,  50 };
    sf::Color m_hoverColor { 60,  60,  90 };
    sf::Color m_pressColor { 20,  20,  35 };
    sf::Color m_textColor  { 220, 220, 220 };
    sf::Color m_outlineColor { 100, 100, 150 };
    float     m_outlineThickness { 2.f };

    // Hover animation
    HoverEffect m_hover { 7.f };

    // Pulse
    bool      m_pulseEnabled { false };
    sf::Color m_pulseColour  { 220, 180, 60, 60 };
    float     m_pulseTimer   { 0.f };
};
