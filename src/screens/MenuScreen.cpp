#include "MenuScreen.hpp"
#include "../Game.hpp"
#include <cmath>
#include <algorithm>

static constexpr float PI = 3.14159265f;

// ── Constructor ───────────────────────────────────────────────────────────────
MenuScreen::MenuScreen(Game& game)
{
    const float W  = game.width();
    const float H  = game.height();
    const float cx = W / 2.f;
    const float cy = H / 2.f;
    const sf::Font& font = game.font();

    // ── Background: subtle animated particles are drawn procedurally in render()
    m_bgOverlay.setSize({ W, H });
    m_bgOverlay.setFillColor(sf::Color(0, 0, 0, 0));

    // ── Logo / Title ──────────────────────────────────────────────────────────
    m_title.setFont(font);
    m_title.setString("CASE OPENER");
    m_title.setCharacterSize(80);
    m_title.setFillColor(sf::Color(220, 180, 60));
    m_title.setStyle(sf::Text::Bold);
    {
        sf::FloatRect tb = m_title.getLocalBounds();
        m_title.setOrigin(tb.left + tb.width / 2.f, tb.top + tb.height / 2.f);
        m_title.setPosition(cx, cy - 200.f);
    }

    // Title underline accent
    m_titleUnderline.setSize({ 300.f, 3.f });
    m_titleUnderline.setFillColor(sf::Color(220, 180, 60, 160));
    m_titleUnderline.setOrigin(150.f, 0.f);
    m_titleUnderline.setPosition(cx, cy - 152.f);

    m_subtitle.setFont(font);
    m_subtitle.setString("open cases  \xe2\x80\xa2  collect items  \xe2\x80\xa2  trade up");
    m_subtitle.setCharacterSize(18);
    m_subtitle.setFillColor(sf::Color(120, 120, 155));
    {
        sf::FloatRect tb = m_subtitle.getLocalBounds();
        m_subtitle.setOrigin(tb.left + tb.width / 2.f, tb.top + tb.height / 2.f);
        m_subtitle.setPosition(cx, cy - 122.f);
    }

    // ── Buttons ───────────────────────────────────────────────────────────────
    const sf::Vector2f btnSize { 260.f, 60.f };
    const float btnX = cx - btnSize.x / 2.f;

    m_playButton = Button(font, "PLAY", { btnX, cy - 20.f }, btnSize, 26);
    m_playButton.setIdleColor ({ 30,  80,  50 });
    m_playButton.setHoverColor({ 45, 120,  75 });
    m_playButton.setPressColor({ 20,  55,  35 });
    m_playButton.setOutlineColor(sf::Color(70, 200, 110), 2.f);
    m_playButton.setPulse(true, sf::Color(80, 200, 110, 45));
    m_playButton.setOnClick([&game]() { game.switchScreen(ScreenID::Game); });

    m_quitButton = Button(font, "QUIT", { btnX, cy + 60.f }, btnSize, 26);
    m_quitButton.setIdleColor ({ 80,  30,  30 });
    m_quitButton.setHoverColor({120,  45,  45 });
    m_quitButton.setPressColor({ 55,  20,  20 });
    m_quitButton.setOutlineColor(sf::Color(200, 70, 70), 2.f);
    m_quitButton.setOnClick([&game]() { game.window().close(); });

    // ── Version / hint text ───────────────────────────────────────────────────
    m_hintText.setFont(font);
    m_hintText.setString("F11 \xe2\x80\x93 toggle fullscreen    \xe2\x80\xa2    M \xe2\x80\x93 mute");
    m_hintText.setCharacterSize(12);
    m_hintText.setFillColor(sf::Color(55, 55, 80));
    {
        sf::FloatRect tb = m_hintText.getLocalBounds();
        m_hintText.setOrigin(tb.left + tb.width / 2.f, tb.top + tb.height);
        m_hintText.setPosition(cx, H - 12.f);
    }

    // ── Decorative corner lines ───────────────────────────────────────────────
    // Top-left
    m_cornerTL[0].setSize({ 80.f, 2.f });  m_cornerTL[0].setPosition(20.f, 20.f);
    m_cornerTL[1].setSize({ 2.f, 80.f });  m_cornerTL[1].setPosition(20.f, 20.f);
    // Bottom-right
    m_cornerBR[0].setSize({ 80.f, 2.f });  m_cornerBR[0].setPosition(W - 100.f, H - 22.f);
    m_cornerBR[1].setSize({ 2.f, 80.f });  m_cornerBR[1].setPosition(W - 22.f,  H - 100.f);
    for (auto& s : m_cornerTL) s.setFillColor(sf::Color(220, 180, 60, 80));
    for (auto& s : m_cornerBR) s.setFillColor(sf::Color(220, 180, 60, 80));

    // Seed procedural background dots
    static std::mt19937 rng{ 42u };
    std::uniform_real_distribution<float> xd(0.f, W);
    std::uniform_real_distribution<float> yd(0.f, H);
    std::uniform_real_distribution<float> sd(1.f, 3.f);
    std::uniform_real_distribution<float> pd(0.f, 6.28f);
    m_bgDots.resize(60);
    for (auto& d : m_bgDots)
    {
        d.x     = xd(rng);
        d.y     = yd(rng);
        d.size  = sd(rng);
        d.phase = pd(rng);
    }
}

// ── handleEvent ───────────────────────────────────────────────────────────────
void MenuScreen::handleEvent(const sf::Event& event, Game& game)
{
    m_playButton.handleEvent(event, game.window());
    m_quitButton.handleEvent(event, game.window());
}

// ── update ────────────────────────────────────────────────────────────────────
void MenuScreen::update(float dt, Game& /*game*/)
{
    m_animTime += dt;

    // Pulsing overlay
    const float pulse = (std::sin(m_animTime * 0.6f) + 1.f) / 2.f;
    const sf::Uint8 alpha = static_cast<sf::Uint8>(pulse * 14.f);
    m_bgOverlay.setFillColor(sf::Color(40, 20, 70, alpha));

    const sf::Vector2i mouse = sf::Mouse::getPosition();
    m_playButton.update(mouse);
    m_quitButton.update(mouse);

    // Animate underline width gently
    const float uw = 260.f + std::sin(m_animTime * 1.1f) * 30.f;
    m_titleUnderline.setSize({ uw, 3.f });
    m_titleUnderline.setOrigin(uw / 2.f, 0.f);
}

// ── render ────────────────────────────────────────────────────────────────────
void MenuScreen::render(sf::RenderWindow& window)
{
    // Animated background dots (very subtle)
    sf::CircleShape dot;
    for (const auto& d : m_bgDots)
    {
        const float alpha = (std::sin(m_animTime * 0.7f + d.phase) + 1.f) / 2.f;
        dot.setRadius(d.size);
        dot.setOrigin(d.size, d.size);
        dot.setPosition(d.x, d.y);
        dot.setFillColor(sf::Color(60, 45, 90, static_cast<sf::Uint8>(alpha * 55.f)));
        window.draw(dot);
    }

    window.draw(m_bgOverlay);

    // Corners
    for (const auto& s : m_cornerTL) window.draw(s);
    for (const auto& s : m_cornerBR) window.draw(s);

    window.draw(m_titleUnderline);
    window.draw(m_title);
    window.draw(m_subtitle);

    window.draw(m_playButton);
    window.draw(m_quitButton);

    window.draw(m_hintText);
}
