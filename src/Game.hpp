#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include "screens/Screen.hpp"
#include "core/PlayerData.hpp"

// ── Screen identifiers ────────────────────────────────────────────────────────
enum class ScreenID { Menu, Game };

// ── ScreenTransition ──────────────────────────────────────────────────────────
// Handles fade-to-black transitions between screens.
struct ScreenTransition
{
    enum class Phase { None, FadingOut, FadingIn };

    Phase  phase     { Phase::None };
    float  timer     { 0.f };
    float  duration  { 0.25f };   // half-transition (out or in), seconds
    ScreenID pending { ScreenID::Menu };

    sf::RectangleShape overlay;

    bool isActive() const { return phase != Phase::None; }

    void begin(ScreenID next)
    {
        pending = next;
        phase   = Phase::FadingOut;
        timer   = 0.f;
        overlay.setFillColor(sf::Color(0, 0, 0, 0));
    }

    // Returns true when the screen should actually switch (mid-transition)
    bool update(float dt)
    {
        if (phase == Phase::None) return false;
        timer += dt;

        if (phase == Phase::FadingOut)
        {
            const float t = std::min(timer / duration, 1.f);
            const sf::Uint8 a = static_cast<sf::Uint8>(t * 255.f);
            overlay.setFillColor(sf::Color(0, 0, 0, a));
            if (timer >= duration) { phase = Phase::FadingIn; timer = 0.f; return true; }
        }
        else if (phase == Phase::FadingIn)
        {
            const float t = std::min(timer / duration, 1.f);
            const sf::Uint8 a = static_cast<sf::Uint8>((1.f - t) * 255.f);
            overlay.setFillColor(sf::Color(0, 0, 0, a));
            if (timer >= duration) { phase = Phase::None; }
        }
        return false;
    }

    void render(sf::RenderWindow& w)
    {
        if (phase == Phase::None) return;
        overlay.setSize({ static_cast<float>(w.getSize().x),
                          static_cast<float>(w.getSize().y) });
        w.draw(overlay);
    }
};

// ── Game ──────────────────────────────────────────────────────────────────────
// Owns the window, main loop, shared font, player data, and the active screen.
// Supports fullscreen toggle via F11, and smooth screen transitions.
class Game
{
public:
    Game();
    ~Game();

    void run();

    // Trigger a fade-transition to a new screen (safe to call from screen code)
    void switchScreen(ScreenID id);

    sf::RenderWindow&       window()       { return m_window; }
    const sf::RenderWindow& window() const { return m_window; }
    const sf::Font&         font()   const { return m_font;   }
    PlayerData&             playerData()   { return m_playerData; }

    // Dynamic dimensions — always query these, never use compile-time constants
    float width()  const { return static_cast<float>(m_window.getSize().x); }
    float height() const { return static_cast<float>(m_window.getSize().y); }

    // Legacy helpers (kept for compatibility)
    static constexpr unsigned int WIDTH  = 1280;
    static constexpr unsigned int HEIGHT = 720;

    float  balance()          const { return m_balance; }
    void   setBalance(float b)      { m_balance = b; }
    void   addBalance(float delta)  { m_balance += delta; }

    void toggleFullscreen();
    bool isFullscreen() const { return m_fullscreen; }

private:
    void processEvents();
    void update(float dt);
    void render();
    void loadScreen(ScreenID id);
    void initSystems();
    void autosave();
    void loadFont();

    sf::RenderWindow        m_window;
    sf::Font                m_font;
    std::unique_ptr<Screen> m_currentScreen;
    ScreenTransition        m_transition;
    sf::Clock               m_clock;

    bool                    m_fullscreen { false };

    PlayerData              m_playerData;
    float                   m_balance    { PlayerData::DEFAULT_BALANCE };

    float                   m_saveTimer  { 0.f };
    static constexpr float  AUTOSAVE_INTERVAL = 60.f;
};
