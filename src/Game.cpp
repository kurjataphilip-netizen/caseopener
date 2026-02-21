#include "Game.hpp"
#include "screens/MenuScreen.hpp"
#include "screens/GameScreen.hpp"
#include "items/ItemRegistry.hpp"
#include "cases/CaseRegistry.hpp"
#include "core/AudioManager.hpp"
#include "gameplay/Inventory.hpp"

#include <iostream>
#include <stdexcept>

// ── Constructor ───────────────────────────────────────────────────────────────
Game::Game()
    : m_window(sf::VideoMode(WIDTH, HEIGHT),
               "Case Opener",
               sf::Style::Titlebar | sf::Style::Close)
    , m_playerData("save.dat")
{
    m_window.setFramerateLimit(144);
    m_window.setVerticalSyncEnabled(false);

    // Font — try bundled asset first, fall back to system font
    if (!m_font.loadFromFile("assets/fonts/font.ttf"))
    {
        const char* fallbacks[] = {
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/System/Library/Fonts/Helvetica.ttc",
            "C:/Windows/Fonts/arial.ttf",
        };
        bool loaded = false;
        for (const char* fb : fallbacks)
            if (m_font.loadFromFile(fb)) { loaded = true; break; }
        if (!loaded)
            std::cerr << "[Game] Warning: no font loaded — text will be invisible.\n";
    }

    initSystems();

    // Load player save (sets m_balance; inventory not owned here — GameScreen owns it)
    Inventory dummy;
    m_playerData.load(m_balance, dummy);
    // Inventory from save will be re-applied inside GameScreen on construction.

    loadScreen(ScreenID::Menu);
}

// ── Destructor ────────────────────────────────────────────────────────────────
Game::~Game()
{
    // GameScreen owns the inventory; we can't autosave inventory from here
    // without a deeper coupling.  Balance-only save as a safety net.
    std::cout << "[Game] Shutting down. Final balance: $" << m_balance << "\n";
}

// ── initSystems ───────────────────────────────────────────────────────────────
void Game::initSystems()
{
    // Registries must be loaded before any screen that uses items/cases.
    ItemRegistry::instance().load();
    CaseRegistry::instance().load();

    // Audio — missing files are silently ignored by AudioManager.
    AudioManager::instance().load();
}

// ── run ───────────────────────────────────────────────────────────────────────
void Game::run()
{
    m_clock.restart();

    while (m_window.isOpen())
    {
        const float rawDt = m_clock.restart().asSeconds();
        // Cap dt to avoid physics explosion after alt-tab / debugger pause
        const float dt    = std::min(rawDt, 0.05f);

        processEvents();
        update(dt);
        render();

        if (m_screenSwitch)
        {
            m_screenSwitch = false;
            loadScreen(m_pendingScreen);
        }
    }
}

// ── switchScreen ──────────────────────────────────────────────────────────────
void Game::switchScreen(ScreenID id)
{
    m_pendingScreen = id;
    m_screenSwitch  = true;
}

// ── processEvents ─────────────────────────────────────────────────────────────
void Game::processEvents()
{
    sf::Event event{};
    while (m_window.pollEvent(event))
    {
        if (event.type == sf::Event::Closed)
            m_window.close();

        // Global mute toggle: M key
        if (event.type == sf::Event::KeyPressed &&
            event.key.code == sf::Keyboard::M)
            AudioManager::instance().toggleMute();

        if (m_currentScreen)
            m_currentScreen->handleEvent(event, *this);
    }
}

// ── update ────────────────────────────────────────────────────────────────────
void Game::update(float dt)
{
    if (m_currentScreen)
        m_currentScreen->update(dt, *this);

    // Periodic autosave (balance only — full save happens inside GameScreen)
    m_saveTimer += dt;
    if (m_saveTimer >= AUTOSAVE_INTERVAL)
    {
        m_saveTimer = 0.f;
        autosave();
    }
}

// ── render ────────────────────────────────────────────────────────────────────
void Game::render()
{
    m_window.clear(sf::Color(14, 14, 22));

    if (m_currentScreen)
        m_currentScreen->render(m_window);

    m_window.display();
}

// ── loadScreen ────────────────────────────────────────────────────────────────
void Game::loadScreen(ScreenID id)
{
    switch (id)
    {
        case ScreenID::Menu:
            m_currentScreen = std::make_unique<MenuScreen>(*this);
            break;
        case ScreenID::Game:
            m_currentScreen = std::make_unique<GameScreen>(*this);
            break;
        default:
            throw std::runtime_error("[Game] Unknown ScreenID.");
    }
}

// ── autosave ──────────────────────────────────────────────────────────────────
void Game::autosave()
{
    // Balance-only periodic save; full inventory save is triggered by GameScreen.
    // We write a minimal stub so the balance isn't lost on crash.
    Inventory empty;
    m_playerData.save(m_balance, empty);
}
