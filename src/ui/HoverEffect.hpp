#pragma once

#include <SFML/Graphics.hpp>
#include <cmath>
#include <algorithm>
#include <random>
#include <vector>

// ── HoverEffect ───────────────────────────────────────────────────────────────
// Manages a smoothly animated float value that eases to 1.0 when hovered
// and back to 0.0 when not. Uses exponential smoothing for a snappy feel.
// ─────────────────────────────────────────────────────────────────────────────
class HoverEffect
{
public:
    explicit HoverEffect(float speed = 7.f) : m_speed(speed) {}

    void update(float dt, bool hovered)
    {
        const float target = hovered ? 1.f : 0.f;
        m_value += (target - m_value) * std::min(1.f, m_speed * dt);
        m_value  = std::clamp(m_value, 0.f, 1.f);
    }

    float value()     const { return m_value; }
    bool  isHovered() const { return m_value > 0.5f; }

    void  setSpeed(float s)    { m_speed = s; }
    void  reset()              { m_value = 0.f; }
    void  snapTo(bool hovered) { m_value = hovered ? 1.f : 0.f; }

    // ── Colour helpers ────────────────────────────────────────────────────────
    static sf::Color lerp(sf::Color a, sf::Color b, float t)
    {
        t = std::clamp(t, 0.f, 1.f);
        return sf::Color(
            static_cast<sf::Uint8>(a.r + (b.r - a.r) * t),
            static_cast<sf::Uint8>(a.g + (b.g - a.g) * t),
            static_cast<sf::Uint8>(a.b + (b.b - a.b) * t),
            static_cast<sf::Uint8>(a.a + (b.a - a.a) * t));
    }

    static sf::Color brighten(sf::Color c, float factor)
    {
        return sf::Color(
            static_cast<sf::Uint8>(std::min(255.f, c.r * factor)),
            static_cast<sf::Uint8>(std::min(255.f, c.g * factor)),
            static_cast<sf::Uint8>(std::min(255.f, c.b * factor)),
            c.a);
    }

    static sf::Color applyToShape(sf::RectangleShape& shape,
                                   sf::Color idleColor,
                                   sf::Color hoverColor,
                                   float t,
                                   float maxGrow = 0.f)
    {
        const sf::Color fill = lerp(idleColor, hoverColor, t);
        shape.setFillColor(fill);

        if (maxGrow > 0.f)
        {
            const float s = 1.f + maxGrow * t;
            const sf::Vector2f sz = shape.getSize();
            shape.setOrigin(sz.x / 2.f, sz.y / 2.f);
            shape.setScale(s, s);
        }
        return fill;
    }

private:
    float m_value { 0.f };
    float m_speed { 7.f };
};

// ── Sparkle / SparkleEmitter ──────────────────────────────────────────────────
// Tiny particle system for rare drop reveals.
// BUG FIX: removed illegal `#include <random>` inside the class body.
// ─────────────────────────────────────────────────────────────────────────────
struct Sparkle
{
    sf::Vector2f pos;
    sf::Vector2f vel;
    float        life    { 0.f };
    float        maxLife { 1.f };
    float        size    { 3.f };
    sf::Color    colour;
};

class SparkleEmitter
{
public:
    // origin: where to spawn. baseColour: particle tint. count: how many.
    // spread: max launch speed (pixels/sec).
    explicit SparkleEmitter(sf::Vector2f origin,
                             sf::Color    baseColour,
                             int          count  = 40,
                             float        spread = 180.f)
    {
        static std::mt19937 rng{ std::random_device{}() };
        std::uniform_real_distribution<float> angleDist(0.f, 6.2832f);
        std::uniform_real_distribution<float> speedDist(30.f, spread);
        std::uniform_real_distribution<float> lifeDist(0.5f, 1.8f);
        std::uniform_real_distribution<float> sizeDist(2.f, 5.f);

        m_particles.reserve(count);
        for (int i = 0; i < count; ++i)
        {
            const float angle = angleDist(rng);
            const float speed = speedDist(rng);
            Sparkle s;
            s.pos     = origin;
            s.vel     = { std::cos(angle) * speed, std::sin(angle) * speed };
            s.life    = lifeDist(rng);
            s.maxLife = s.life;
            s.size    = sizeDist(rng);
            s.colour  = baseColour;
            m_particles.push_back(s);
        }
    }

    bool isDead() const { return m_particles.empty(); }

    void update(float dt)
    {
        for (auto& s : m_particles)
        {
            s.pos  += s.vel * dt;
            s.vel  *= std::pow(0.90f, dt * 60.f); // drag
            s.vel.y += 80.f * dt;                  // gravity
            s.life  -= dt;
        }
        m_particles.erase(
            std::remove_if(m_particles.begin(), m_particles.end(),
                           [](const Sparkle& s){ return s.life <= 0.f; }),
            m_particles.end());
    }

    void render(sf::RenderWindow& window)
    {
        sf::RectangleShape dot;
        for (const auto& s : m_particles)
        {
            const float t  = std::max(0.f, s.life / s.maxLife);
            const float sz = s.size * t;
            dot.setSize({ sz, sz });
            dot.setOrigin(sz / 2.f, sz / 2.f);
            dot.setPosition(s.pos);
            sf::Color c = s.colour;
            c.a = static_cast<sf::Uint8>(t * 220.f);
            dot.setFillColor(c);
            window.draw(dot);
        }
    }

private:
    std::vector<Sparkle> m_particles;
};
