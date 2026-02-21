#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include "screens/Screen.hpp"
#include "core/PlayerData.hpp"

// ── Screen identifiers ────────────────────────────────────────────────────────
enum class ScreenID { Menu, Game };

// ── Game ──────────────────────────────────────────────────────────────────────
// Owns the window, main loop, delta time, shared font, player data,
// and the active screen.  All singletons (registries, audio) are
// initialised here before the first screen is pushed.
class Game
{
public:
    Game();
    ~Game();   // autosave on destruction

    void run();
    void switchScreen(ScreenID id);

    sf::RenderWindow&       window()       { return m_window; }
    const sf::RenderWindow& window() const { return m_window; }
    const sf::Font&         font()   const { return m_font;   }
    PlayerData&             playerData()   { return m_playerData; }

    // Shared balance — owned here so screens can read/write it.
    float  balance()          const { return m_balance; }
    void   setBalance(float b)      { m_balance = b;    }
    void   addBalance(float delta)  { m_balance += delta; }

    static constexpr unsigned int WIDTH  = 1280;
    static constexpr unsigned int HEIGHT = 720;

private:
    void processEvents();
    void update(float dt);
    void render();
    void loadScreen(ScreenID id);
    void initSystems();   // registries + audio
    void autosave();

    sf::RenderWindow        m_window;
    sf::Font                m_font;
    std::unique_ptr<Screen> m_currentScreen;
    ScreenID                m_pendingScreen  { ScreenID::Menu };
    bool                    m_screenSwitch   { false };
    sf::Clock               m_clock;

    PlayerData              m_playerData;
    float                   m_balance        { PlayerData::DEFAULT_BALANCE };

    // Autosave timer (every 60 seconds)
    float                   m_saveTimer      { 0.f };
    static constexpr float  AUTOSAVE_INTERVAL = 60.f;
};
