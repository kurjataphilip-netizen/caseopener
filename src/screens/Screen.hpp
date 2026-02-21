#pragma once

#include <SFML/Graphics.hpp>

class Game; // forward declaration — avoids circular includes

// ── Screen ────────────────────────────────────────────────────────────────────
// Pure-virtual interface every screen must implement.
// Game owns exactly one Screen at a time via std::unique_ptr<Screen>.
// ─────────────────────────────────────────────────────────────────────────────
class Screen
{
public:
    virtual ~Screen() = default;

    // Called for every polled sf::Event (keyboard, mouse, window resize, …)
    virtual void handleEvent(const sf::Event& event, Game& game) = 0;

    // Called once per frame with the delta time in seconds.
    virtual void update(float dt, Game& game) = 0;

    // Called once per frame — draw everything to the window.
    virtual void render(sf::RenderWindow& window) = 0;
};
