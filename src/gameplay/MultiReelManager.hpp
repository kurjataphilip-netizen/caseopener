#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <functional>

#include "ReelAnimation.hpp"
#include "../cases/Case.hpp"
#include "../items/Item.hpp"

enum class OpenCount : int { One = 1, Three = 3, Five = 5, Ten = 10 };

class MultiReelManager
{
public:
    explicit MultiReelManager(const sf::Font& font);

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

    sf::Vector2f winnerCentre(std::size_t i) const;

private:
    // FIX: LayoutScale is now a free struct, not ReelAnimation::LayoutScale
    LayoutScale computeScale(int n, sf::Vector2u winSize,
                             int cols, int rows,
                             float availW, float availH) const;

    void buildLayout(int n, sf::Vector2u winSize);

    const sf::Font* m_font { nullptr };

    std::vector<std::unique_ptr<ReelAnimation>> m_reels;

    std::vector<float> m_startDelays;
    std::vector<float> m_startTimers;
    std::vector<bool>  m_started;

    std::vector<Item>  m_results;
    int                m_doneCount { 0 };

    std::function<void(std::vector<Item>)> m_onAllComplete;

    float m_layoutHeight { 0.f };
};
