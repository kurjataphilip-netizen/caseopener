#include "Button.hpp"
#include <cmath>
#include <algorithm>

// ── Constructor ───────────────────────────────────────────────────────────────
Button::Button(const sf::Font& font,
               const std::string& label,
               sf::Vector2f position,
               sf::Vector2f size,
               unsigned int charSize)
{
    m_rect.setSize(size);
    m_rect.setPosition(position);
    m_rect.setOutlineThickness(m_outlineThickness);
    m_rect.setOutlineColor(m_outlineColor);

    m_text.setFont(font);
    m_text.setCharacterSize(charSize);
    m_text.setFillColor(m_textColor);
    setLabel(label);

    applyVisuals(0.f);
}

// ── Setters ───────────────────────────────────────────────────────────────────
void Button::setLabel(const std::string& label)
{
    m_text.setString(label);
    centreText();
}

void Button::setPosition(sf::Vector2f pos)
{
    m_rect.setPosition(pos);
    centreText();
}

void Button::setSize(sf::Vector2f size)
{
    m_rect.setSize(size);
    centreText();
}

void Button::setOnClick(std::function<void()> callback)
{
    m_onClick = std::move(callback);
}

void Button::setIdleColor   (sf::Color c) { m_idleColor  = c; }
void Button::setHoverColor  (sf::Color c) { m_hoverColor = c; }
void Button::setPressColor  (sf::Color c) { m_pressColor = c; }

void Button::setTextColor(sf::Color c)
{
    m_textColor = c;
    m_text.setFillColor(c);
}

void Button::setOutlineColor(sf::Color c, float thickness)
{
    m_outlineColor     = c;
    m_outlineThickness = thickness;
    m_rect.setOutlineColor(c);
    m_rect.setOutlineThickness(thickness);
}

// ── update ────────────────────────────────────────────────────────────────────
void Button::update(sf::Vector2i mousePos, float dt)
{
    const bool over = m_rect.getGlobalBounds()
                            .contains(static_cast<float>(mousePos.x),
                                      static_cast<float>(mousePos.y));

    if (m_state != State::Pressed)
        m_state = over ? State::Hovered : State::Idle;

    m_hover.update(dt, over && m_state != State::Pressed);
    m_pulseTimer += dt;

    applyVisuals(dt);
}

// ── handleEvent ───────────────────────────────────────────────────────────────
void Button::handleEvent(const sf::Event& event, const sf::RenderWindow& window)
{
    const sf::Vector2i mouse = sf::Mouse::getPosition(window);
    const bool over = m_rect.getGlobalBounds()
                            .contains(static_cast<float>(mouse.x),
                                      static_cast<float>(mouse.y));

    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Left && over)
    {
        m_state = State::Pressed;
    }

    if (event.type == sf::Event::MouseButtonReleased &&
        event.mouseButton.button == sf::Mouse::Left)
    {
        if (m_state == State::Pressed && over && m_onClick)
            m_onClick();
        m_state = over ? State::Hovered : State::Idle;
    }
}

// ── draw ──────────────────────────────────────────────────────────────────────
void Button::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    target.draw(m_rect, states);
    target.draw(m_text, states);
}

// ── applyVisuals ──────────────────────────────────────────────────────────────
void Button::applyVisuals(float /*dt*/)
{
    const float t = m_hover.value();

    sf::Color fill;
    switch (m_state)
    {
        case State::Pressed:
            fill = m_pressColor;
            break;
        default:
            // Smooth lerp between idle and hover colours
            fill = HoverEffect::lerp(m_idleColor, m_hoverColor, t);
            break;
    }

    // Pulse: additive brightness on CTA buttons when idle
    if (m_pulseEnabled && m_state == State::Idle)
    {
        const float pulse = (std::sin(m_pulseTimer * 2.2f) + 1.f) / 2.f;
        const sf::Uint8 pa = static_cast<sf::Uint8>(pulse * m_pulseColour.a);
        fill.r = static_cast<sf::Uint8>(std::min(255, fill.r + pa * m_pulseColour.r / 255));
        fill.g = static_cast<sf::Uint8>(std::min(255, fill.g + pa * m_pulseColour.g / 255));
        fill.b = static_cast<sf::Uint8>(std::min(255, fill.b + pa * m_pulseColour.b / 255));
    }

    m_rect.setFillColor(fill);

    // Slightly brighten outline on hover
    sf::Color outlineCol = m_outlineColor;
    if (t > 0.f)
        outlineCol = HoverEffect::brighten(m_outlineColor,
                                            1.f + 0.4f * t);
    m_rect.setOutlineColor(outlineCol);
    m_rect.setOutlineThickness(m_outlineThickness + t * 0.5f);

    // Text brightness
    const sf::Uint8 textA = static_cast<sf::Uint8>(190 + 65 * t);
    m_text.setFillColor(sf::Color(m_textColor.r, m_textColor.g, m_textColor.b, textA));
}

// ── centreText ────────────────────────────────────────────────────────────────
void Button::centreText()
{
    const sf::FloatRect tb = m_text.getLocalBounds();
    const sf::FloatRect rb = m_rect.getGlobalBounds();
    m_text.setOrigin(tb.left + tb.width  / 2.f,
                     tb.top  + tb.height / 2.f);
    m_text.setPosition(rb.left + rb.width  / 2.f,
                       rb.top  + rb.height / 2.f);
}
