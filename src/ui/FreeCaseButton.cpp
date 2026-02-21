#include "FreeCaseButton.hpp"
#include "../core/AudioManager.hpp"
#include <cmath>
#include <algorithm>

static constexpr float PI = 3.14159265358979f;

// ── Constructor ───────────────────────────────────────────────────────────────
FreeCaseButton::FreeCaseButton(const sf::Font& font,
                                FreeCaseReward& reward,
                                sf::Vector2f position)
    : m_font(&font)
    , m_reward(&reward)
{
    m_arcFill.setPrimitiveType(sf::TriangleFan);

    m_arcBg.setRadius(ARC_RADIUS);
    m_arcBg.setOrigin(ARC_RADIUS, ARC_RADIUS);
    m_arcBg.setFillColor(sf::Color(0,0,0,0));
    m_arcBg.setOutlineThickness(6.f);
    m_arcBg.setOutlineColor(sf::Color(35, 35, 55));

    m_bg.setSize({ SIZE, SIZE });
    m_bg.setFillColor(sf::Color(18, 18, 30));
    m_bg.setOutlineThickness(2.f);
    m_bg.setOutlineColor(sf::Color(55, 55, 85));

    setPosition(position);
}

// ── setPosition ───────────────────────────────────────────────────────────────
void FreeCaseButton::setPosition(sf::Vector2f pos)
{
    m_pos = pos;
    m_bg.setPosition(pos);
    m_arcBg.setPosition(pos.x + SIZE / 2.f, pos.y + SIZE / 2.f - 14.f);
}

// ── bounds ────────────────────────────────────────────────────────────────────
sf::FloatRect FreeCaseButton::bounds() const
{
    return m_bg.getGlobalBounds();
}

// ── handleEvent ───────────────────────────────────────────────────────────────
void FreeCaseButton::handleEvent(const sf::Event& event,
                                  const sf::RenderWindow& window)
{
    if (!m_reward) return;

    if (event.type == sf::Event::MouseButtonReleased &&
        event.mouseButton.button == sf::Mouse::Left)
    {
        const sf::Vector2f mf {
            static_cast<float>(event.mouseButton.x),
            static_cast<float>(event.mouseButton.y)
        };
        if (bounds().contains(mf) && m_reward->isReady())
        {
            if (m_onClaim) m_onClaim();
            AudioManager::instance().play(SoundID::ReelStart);
        }
    }
}

// ── update ────────────────────────────────────────────────────────────────────
void FreeCaseButton::update(float dt)
{
    if (!m_reward) return;

    m_glowTimer += dt;

    const sf::Vector2i mouse = sf::Mouse::getPosition();
    const bool over = bounds().contains(static_cast<float>(mouse.x),
                                         static_cast<float>(mouse.y));
    m_hover.update(dt, over);

    // Update bg colour
    const bool ready = m_reward->isReady();
    const float t = m_hover.value();

    if (ready)
    {
        const float pulse = (std::sin(m_glowTimer * 3.f) + 1.f) / 2.f;
        const sf::Uint8 glowA = static_cast<sf::Uint8>(40.f + 60.f * pulse + 40.f * t);
        m_bg.setFillColor(sf::Color(40 + static_cast<sf::Uint8>(20*t),
                                     30 + static_cast<sf::Uint8>(15*t), 10, 255));
        m_bg.setOutlineColor(sf::Color(220, 180, 60, 120 + glowA));
        m_bg.setOutlineThickness(2.f + t);
    }
    else
    {
        m_bg.setFillColor(sf::Color(18, 18, 30));
        m_bg.setOutlineColor(
            HoverEffect::lerp(sf::Color(55,55,85), sf::Color(90,90,130), t));
    }
}

// ── render ────────────────────────────────────────────────────────────────────
void FreeCaseButton::render(sf::RenderWindow& window)
{
    if (!m_reward) return;

    window.draw(m_bg);

    const bool  ready    = m_reward->isReady();
    const float progress = m_reward->cooldownProgress();
    const sf::Vector2f centre { m_pos.x + SIZE / 2.f, m_pos.y + SIZE / 2.f - 14.f };

    // ── Arc progress ──────────────────────────────────────────────────────────
    window.draw(m_arcBg);
    drawArc(window, progress);

    // ── Centre icon / percent ─────────────────────────────────────────────────
    sf::Text centreText;
    centreText.setFont(*m_font);
    centreText.setCharacterSize(ready ? 22 : 16);
    centreText.setStyle(ready ? sf::Text::Bold : sf::Text::Regular);

    if (ready)
    {
        const float pulse = (std::sin(m_glowTimer * 3.f) + 1.f) / 2.f;
        const sf::Uint8 a = static_cast<sf::Uint8>(200 + 55 * pulse);
        centreText.setFillColor(sf::Color(220, 180, 60, a));
        centreText.setString("FREE");
    }
    else
    {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "%d%%",
                      static_cast<int>(progress * 100.f));
        centreText.setFillColor(sf::Color(140, 140, 170));
        centreText.setString(buf);
    }

    sf::FloatRect ctb = centreText.getLocalBounds();
    centreText.setOrigin(ctb.left + ctb.width / 2.f,
                         ctb.top  + ctb.height / 2.f);
    centreText.setPosition(centre);
    window.draw(centreText);

    // ── Label text beneath arc ────────────────────────────────────────────────
    sf::Text label;
    label.setFont(*m_font);
    label.setCharacterSize(11);
    label.setFillColor(ready ? sf::Color(180, 160, 80) : sf::Color(90, 90, 120));
    label.setString(m_reward->cooldownLabel());
    sf::FloatRect lb = label.getLocalBounds();
    label.setOrigin(lb.left + lb.width / 2.f, lb.top);
    label.setPosition(m_pos.x + SIZE / 2.f, m_pos.y + SIZE - 22.f);
    window.draw(label);

    // ── "Click to claim" hint when ready ─────────────────────────────────────
    if (ready && m_hover.value() > 0.1f)
    {
        sf::Text hint;
        hint.setFont(*m_font);
        hint.setCharacterSize(10);
        hint.setFillColor(sf::Color(200, 170, 60,
                          static_cast<sf::Uint8>(m_hover.value() * 200.f)));
        hint.setString("click to claim!");
        sf::FloatRect hb = hint.getLocalBounds();
        hint.setOrigin(hb.left + hb.width / 2.f, hb.top);
        hint.setPosition(m_pos.x + SIZE / 2.f, m_pos.y + SIZE - 10.f);
        window.draw(hint);
    }
}

// ── drawArc ───────────────────────────────────────────────────────────────────
void FreeCaseButton::drawArc(sf::RenderWindow& window, float progress)
{
    if (progress <= 0.f) return;

    const sf::Vector2f centre { m_pos.x + SIZE / 2.f, m_pos.y + SIZE / 2.f - 14.f };
    const float R = ARC_RADIUS;

    // Start at top (-PI/2) and go clockwise
    const float startAngle = -PI / 2.f;
    const float endAngle   = startAngle + 2.f * PI * std::clamp(progress, 0.f, 1.f);
    const int   segments   = std::max(2, static_cast<int>(ARC_POINTS * progress));

    // Draw as thick arc using a ring of quads (inner + outer radius)
    const float innerR = R - 6.f;
    const float outerR = R + 0.f;

    const bool ready = m_reward && m_reward->isReady();
    const float pulse = (std::sin(m_glowTimer * 3.f) + 1.f) / 2.f;
    const sf::Uint8 alpha = ready
        ? static_cast<sf::Uint8>(180.f + 75.f * pulse)
        : 180;
    const sf::Color arcColour = ready
        ? sf::Color(220, 180, 60, alpha)
        : sf::Color(80, 140, 220, alpha);

    sf::VertexArray arc(sf::TriangleStrip, (segments + 1) * 2);
    for (int i = 0; i <= segments; ++i)
    {
        const float angle = startAngle + (endAngle - startAngle)
                          * static_cast<float>(i) / segments;
        const float cx = std::cos(angle);
        const float cy = std::sin(angle);

        arc[i * 2 + 0].position = { centre.x + cx * innerR,
                                     centre.y + cy * innerR };
        arc[i * 2 + 1].position = { centre.x + cx * outerR,
                                     centre.y + cy * outerR };
        arc[i * 2 + 0].color = arcColour;
        arc[i * 2 + 1].color = arcColour;
    }
    window.draw(arc);
}
