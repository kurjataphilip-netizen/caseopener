#include "MenuScreen.hpp"
#include "../Game.hpp"
#include <cmath>

// ── Constructor ───────────────────────────────────────────────────────────────
MenuScreen::MenuScreen(Game& game)
{
    const float cx = Game::WIDTH  / 2.f;
    const float cy = Game::HEIGHT / 2.f;
    const sf::Font& font = game.font();

    // ── Title ─────────────────────────────────────────────────────────────────
    m_title.setFont(font);
    m_title.setString("CASE OPENER");
    m_title.setCharacterSize(72);
    m_title.setFillColor(sf::Color(220, 180, 60));   // gold
    m_title.setStyle(sf::Text::Bold);
    {
        sf::FloatRect tb = m_title.getLocalBounds();
        m_title.setOrigin(tb.left + tb.width / 2.f, tb.top + tb.height / 2.f);
        m_title.setPosition(cx, cy - 180.f);
    }

    // ── Subtitle ──────────────────────────────────────────────────────────────
    m_subtitle.setFont(font);
    m_subtitle.setString("Open cases. Collect items. Show off.");
    m_subtitle.setCharacterSize(22);
    m_subtitle.setFillColor(sf::Color(160, 160, 180));
    {
        sf::FloatRect tb = m_subtitle.getLocalBounds();
        m_subtitle.setOrigin(tb.left + tb.width / 2.f, tb.top + tb.height / 2.f);
        m_subtitle.setPosition(cx, cy - 100.f);
    }

    // ── Buttons ───────────────────────────────────────────────────────────────
    const sf::Vector2f btnSize { 240.f, 58.f };

    m_playButton = Button(font, "PLAY", {cx - btnSize.x / 2.f, cy}, btnSize, 24);
    m_playButton.setIdleColor ({ 40,  90,  60 });
    m_playButton.setHoverColor({ 55, 130,  85 });
    m_playButton.setPressColor({ 25,  60,  40 });
    m_playButton.setOutlineColor(sf::Color(80, 200, 120), 2.f);
    m_playButton.setOnClick([&game]()
    {
        game.switchScreen(ScreenID::Game);
    });

    m_quitButton = Button(font, "QUIT", {cx - btnSize.x / 2.f, cy + 80.f}, btnSize, 24);
    m_quitButton.setIdleColor ({ 90,  35,  35 });
    m_quitButton.setHoverColor({130,  50,  50 });
    m_quitButton.setPressColor({ 60,  20,  20 });
    m_quitButton.setOutlineColor(sf::Color(200, 80, 80), 2.f);
    m_quitButton.setOnClick([&game]()
    {
        game.window().close();
    });

    // ── Background overlay ────────────────────────────────────────────────────
    m_bgOverlay.setSize({static_cast<float>(Game::WIDTH),
                         static_cast<float>(Game::HEIGHT)});
    m_bgOverlay.setFillColor(sf::Color(0, 0, 0, 0)); // will be animated
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

    // Gentle pulsing overlay — gives the dark background some life.
    const float pulse = (std::sin(m_animTime * 0.8f) + 1.f) / 2.f; // 0..1
    const sf::Uint8 alpha = static_cast<sf::Uint8>(pulse * 18.f);
    m_bgOverlay.setFillColor(sf::Color(60, 30, 90, alpha));

    const sf::Vector2i mouse = sf::Mouse::getPosition(); // screen-space is fine here
    m_playButton.update(mouse);
    m_quitButton.update(mouse);
}

// ── render ────────────────────────────────────────────────────────────────────
void MenuScreen::render(sf::RenderWindow& window)
{
    window.draw(m_bgOverlay);
    window.draw(m_title);
    window.draw(m_subtitle);
    window.draw(m_playButton);
    window.draw(m_quitButton);
}
