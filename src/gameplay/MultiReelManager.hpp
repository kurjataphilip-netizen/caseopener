#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <functional>

#include "ReelAnimation.hpp"
#include "../cases/Case.hpp"
#include "../items/Item.hpp"

// ── OpenCount ─────────────────────────────────────────────────────────────────
enum class OpenCount : int { One = 1, Three = 3, Five = 5, Ten = 10 };

// ── MultiReelManager ─────────────────────────────────────────────────────────
// Owns N ReelAnimations, lays them out on screen, and stagger-starts them.
//
// BUG FIX: buildLayout() was receiving default winSize={0,0}.
//          Now takes the actual window size as a required parameter.
//
// IMPROVEMENT: Layout now properly scales card dimensions for 3/5/10 reels
//              so they always fit without overlapping.
// ─────────────────────────────────────────────────────────────────────────────
class MultiReelManager
{
public:
    explicit MultiReelManager(const sf::Font& font);

    // Call with actual window dimensions so layout is correct.
    void prepare(const Case& sourceCase, OpenCount count,
                 sf::Vector2u windowSize = { 1280, 720 });

    void startAll();
    void skipAll();

    void update(float dt);
    void render(sf::RenderWindow& window);

    bool isRunning() const;
    bool allDone()   const;

    void setOnAllComplete(std::function<void(std::vector<Item>)> cb)
    {
        m_onAllComplete = cb;
    }

    const std::vector<Item>& results() const { return m_results; }

    // Returns the screen-space centre of the winning card for reel at index i.
    // Useful for placing sparkle emitters. Returns {-1,-1} if out of range.
    sf::Vector2f winnerCentre(std::size_t i) const;

private:
    // Computes an appropriate LayoutScale so all reels fit in the window.
    ReelAnimation::LayoutScale computeScale(int n, sf::Vector2u winSize,
                                             int cols, int rows,
                                             float availW, float availH) const;

    void buildLayout(int n, sf::Vector2u winSize);

    const sf::Font* m_font { nullptr };

    std::vector<std::unique_ptr<ReelAnimation>> m_reels;

    // Stagger timing
    std::vector<float> m_startDelays;
    std::vector<float> m_startTimers;
    std::vector<bool>  m_started;

    std::vector<Item>  m_results;
    int                m_doneCount { 0 };

    std::function<void(std::vector<Item>)> m_onAllComplete;

    float m_layoutHeight { 0.f };
};
