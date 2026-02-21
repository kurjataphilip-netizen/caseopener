#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <functional>
#include <memory>

#include "ReelAnimation.hpp"
#include "../cases/Case.hpp"
#include "../items/Item.hpp"

// ── OpenCount ─────────────────────────────────────────────────────────────────
enum class OpenCount : int { One = 1, Three = 3, Five = 5, Ten = 10 };

// ── MultiReelManager ──────────────────────────────────────────────────────────
// Lays out N ReelAnimations on screen, handles skip-all, fires callback
// when every reel has finished.
//
// Layout strategy:
//   1 reel  → single centred reel, full-size
//   3 reels → column of 3, vertically distributed
//   5 reels → column of 5, smaller cards (scaled via sub-view)
//  10 reels → 2 columns × 5
// ─────────────────────────────────────────────────────────────────────────────
class MultiReelManager
{
public:
    explicit MultiReelManager(const sf::Font& font);

    // Prepare N reels for the given case. Wins are rolled here.
    // previousResults is cleared and replaced.
    void prepare(const Case& sourceCase, OpenCount count);

    // Start all reels (staggers start times slightly for visual appeal).
    void startAll();

    // Skip all running reels instantly.
    void skipAll();

    // Callback: fires once all reels reach Done. Provides all winning Items.
    void setOnAllComplete(std::function<void(std::vector<Item>)> cb)
    { m_onAllComplete = cb; }

    // Per-frame
    void update(float dt);
    void render(sf::RenderWindow& window);

    // True while any reel is still spinning / revealing.
    bool isRunning() const;

    // True once every reel is Done.
    bool allDone() const;

    // Results (populated as each reel finishes)
    const std::vector<Item>& results() const { return m_results; }

    int totalReels() const { return static_cast<int>(m_reels.size()); }

    // Total pixel height needed by the reel layout (used by GameScreen to position UI)
    float layoutHeight() const { return m_layoutHeight; }

private:
    void buildLayout(OpenCount count,
                     sf::Vector2f windowSize = { 1280.f, 720.f });

    const sf::Font*                          m_font;
    std::vector<std::unique_ptr<ReelAnimation>> m_reels;
    std::vector<float>                       m_startDelays; // per-reel stagger (seconds)
    std::vector<float>                       m_startTimers;
    std::vector<bool>                        m_started;

    std::vector<Item>                        m_results;
    int                                      m_doneCount { 0 };
    float                                    m_layoutHeight { 0.f };

    std::function<void(std::vector<Item>)>   m_onAllComplete;
};
