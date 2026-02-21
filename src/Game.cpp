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

    loadFont();
    initSystems();

    Inventory dummy;
    m_playerData.load(m_balance, dummy);

    loadScreen(ScreenID::Menu);
}

// ── Destructor ────────────────────────────────────────────────────────────────
Game::~Game()
{
    std::cout << "[Game] Shutting down. Final balance: $" << m_balance << "\n";
}

// ── loadFont ──────────────────────────────────────────────────────────────────
void Game::loadFont()
{
    // Prefer a clean, modern sans-serif. Bundle one in assets/ for best results.
    const char* candidates[] = {
        "assets/fonts/font.ttf",
        "assets/fonts/Roboto-Regular.ttf",
        "assets/fonts/OpenSans-Regular.ttf",
        // Linux
        "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
        // macOS
        "/System/Library/Fonts/SFNS.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        "/Library/Fonts/Arial.ttf",
        // Windows
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf",
    };

    for (const char* path : candidates)
    {
        if (m_font.loadFromFile(path))
        {
            std::cout << "[Game] Font loaded: " << path << "\n";
            return;
        }
    }
    std::cerr << "[Game] WARNING: No font found — text will be invisible. "
                 "Place a .ttf font at assets/fonts/font.ttf\n";
}

// ── initSystems ───────────────────────────────────────────────────────────────
void Game::initSystems()
{
    ItemRegistry::instance().load();
    CaseRegistry::instance().load();
    AudioManager::instance().load();
}

// ── toggleFullscreen ──────────────────────────────────────────────────────────
void Game::toggleFullscreen()
{
    m_fullscreen = !m_fullscreen;

    if (m_fullscreen)
    {
        const sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
        m_window.create(desktop, "Case Opener", sf::Style::Fullscreen);
    }
    else
    {
        m_window.create(sf::VideoMode(WIDTH, HEIGHT), "Case Opener",
                        sf::Style::Titlebar | sf::Style::Close);
    }

    m_window.setFramerateLimit(144);
    m_window.setVerticalSyncEnabled(false);

    // Notify current screen so it can re-layout
    // (screens should query game.width() / game.height() each frame)
    if (m_currentScreen)
        loadScreen(m_transition.pending != ScreenID::Menu
                   ? m_transition.pending : ScreenID::Menu);
}

// ── run ───────────────────────────────────────────────────────────────────────
void Game::run()
{
    m_clock.restart();

    while (m_window.isOpen())
    {
        const float rawDt = m_clock.restart().asSeconds();
        const float dt    = std::min(rawDt, 0.05f);

        processEvents();
        update(dt);
        render();
    }
}

// ── switchScreen ──────────────────────────────────────────────────────────────
void Game::switchScreen(ScreenID id)
{
    if (m_transition.isActive()) return; // already transitioning
    m_transition.begin(id);
}

// ── processEvents ─────────────────────────────────────────────────────────────
void Game::processEvents()
{
    sf::Event event{};
    while (m_window.pollEvent(event))
    {
        if (event.type == sf::Event::Closed)
            m_window.close();

        if (event.type == sf::Event::KeyPressed)
        {
            if (event.key.code == sf::Keyboard::M)
                AudioManager::instance().toggleMute();

            if (event.key.code == sf::Keyboard::F11)
                toggleFullscreen();

            if (event.key.code == sf::Keyboard::Escape && m_fullscreen)
                toggleFullscreen();
        }

        if (m_currentScreen && !m_transition.isActive())
            m_currentScreen->handleEvent(event, *this);
    }
}

// ── update ────────────────────────────────────────────────────────────────────
void Game::update(float dt)
{
    // Handle screen transition
    if (m_transition.isActive())
    {
        const bool doSwitch = m_transition.update(dt);
        if (doSwitch)
            loadScreen(m_transition.pending);
    }

    if (m_currentScreen && !m_transition.isActive())
        m_currentScreen->update(dt, *this);

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
    m_window.clear(sf::Color(12, 12, 20));

    if (m_currentScreen)
        m_currentScreen->render(m_window);

    // Transition overlay drawn on top of everything
    m_transition.render(m_window);

    m_window.display();
}

// ── loadScreen ────────────────────────────────────────────────────────────────
void Game::loadScreen(ScreenID id)
{
    // Destroy old screen before constructing new one
    m_currentScreen.reset();

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
    Inventory empty;
    m_playerData.save(m_balance, empty);
}
