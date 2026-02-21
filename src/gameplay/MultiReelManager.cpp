#include "MultiReelManager.hpp"
#include <algorithm>

// ── Constructor ───────────────────────────────────────────────────────────────
MultiReelManager::MultiReelManager(const sf::Font& font)
    : m_font(&font)
{}

// ── prepare ───────────────────────────────────────────────────────────────────
void MultiReelManager::prepare(const Case& sourceCase, OpenCount count)
{
    const int n = static_cast<int>(count);

    m_reels.clear();
    m_startDelays.clear();
    m_startTimers.clear();
    m_started.clear();
    m_results.clear();
    m_doneCount = 0;

    buildLayout(count);

    // Roll all winning items up-front (independent RNG calls)
    std::vector<Item> wins;
    wins.reserve(n);
    for (int i = 0; i < n; ++i)
    {
        auto opt = sourceCase.open();
        if (!opt) opt = sourceCase.open(); // retry once
        if (opt) wins.push_back(std::move(*opt));
    }
    // If any rolls failed, pad with re-opens
    while (static_cast<int>(wins.size()) < n)
    {
        auto opt = sourceCase.open();
        if (opt) wins.push_back(std::move(*opt));
        else break;
    }

    // Animation duration scales with number of reels (10 reels get slightly shorter)
    const float baseDuration = (n <= 3) ? 4.5f
                             : (n <= 5) ? 3.8f
                             :            3.2f;

    for (int i = 0; i < static_cast<int>(m_reels.size()) && i < static_cast<int>(wins.size()); ++i)
    {
        m_reels[i]->prepare(sourceCase, wins[i], baseDuration);

        // Wire up per-reel callback
        m_reels[i]->setOnComplete([this](const Item& item)
        {
            m_results.push_back(item);
            ++m_doneCount;
            if (m_doneCount == static_cast<int>(m_reels.size()) && m_onAllComplete)
                m_onAllComplete(m_results);
        });

        // Stagger: each reel starts 0.12 s after the previous
        m_startDelays.push_back(static_cast<float>(i) * 0.12f);
        m_startTimers.push_back(0.f);
        m_started.push_back(false);
    }
}

// ── buildLayout ───────────────────────────────────────────────────────────────
// Calculates top-left position for each reel and creates ReelAnimation objects.
void MultiReelManager::buildLayout(OpenCount count, sf::Vector2f winSize)
{
    const int n  = static_cast<int>(count);
    const float W = winSize.x;
    const float H = winSize.y;

    // Header bar is 64px + we want some padding
    const float topPad    = 80.f;
    const float bottomPad = 120.f; // space for buttons
    const float availH    = H - topPad - bottomPad;

    float reelW = ReelAnimation::REEL_W;
    float reelH = ReelAnimation::REEL_H;

    int cols = 1;
    int rows = n;

    if (n == 10) { cols = 2; rows = 5; }

    // Vertical spacing
    const float totalReelH = rows * reelH;
    const float vGap = (rows > 1) ? (availH - totalReelH) / (rows - 1) : 0.f;
    const float safeVGap = std::max(4.f, std::min(vGap, 24.f));

    // Horizontal spacing for 2-column layout
    const float hGap = (cols == 2) ? 24.f : 0.f;
    const float totalW = cols * reelW + (cols - 1) * hGap;
    const float startX = (W - totalW) / 2.f;

    m_layoutHeight = rows * reelH + (rows - 1) * safeVGap;

    for (int r = 0; r < rows; ++r)
    {
        for (int c = 0; c < cols; ++c)
        {
            const int idx = r * cols + c;
            if (idx >= n) break;

            const float x = startX + c * (reelW + hGap);
            const float y = topPad + r * (reelH + safeVGap);

            m_reels.push_back(
                std::make_unique<ReelAnimation>(sf::Vector2f(x, y), *m_font));
        }
    }
}

// ── startAll ──────────────────────────────────────────────────────────────────
void MultiReelManager::startAll()
{
    // Reset stagger timers — update() handles the actual start calls
    for (std::size_t i = 0; i < m_startTimers.size(); ++i)
    {
        m_startTimers[i] = 0.f;
        m_started[i]     = false;
    }
}

// ── skipAll ───────────────────────────────────────────────────────────────────
void MultiReelManager::skipAll()
{
    for (std::size_t i = 0; i < m_reels.size(); ++i)
    {
        if (!m_started[i])
        {
            // Force-start then immediately skip
            m_reels[i]->start();
            m_started[i] = true;
        }
        m_reels[i]->skip();
    }
}

// ── update ────────────────────────────────────────────────────────────────────
void MultiReelManager::update(float dt)
{
    for (std::size_t i = 0; i < m_reels.size(); ++i)
    {
        if (!m_started[i])
        {
            m_startTimers[i] += dt;
            if (m_startTimers[i] >= m_startDelays[i])
            {
                m_reels[i]->start();
                m_started[i] = true;
            }
        }
        m_reels[i]->update(dt);
    }
}

// ── render ────────────────────────────────────────────────────────────────────
void MultiReelManager::render(sf::RenderWindow& window)
{
    for (auto& reel : m_reels)
        reel->render(window);
}

// ── isRunning ─────────────────────────────────────────────────────────────────
bool MultiReelManager::isRunning() const
{
    for (const auto& reel : m_reels)
        if (!reel->isDone()) return true;
    return false;
}

// ── allDone ───────────────────────────────────────────────────────────────────
bool MultiReelManager::allDone() const
{
    if (m_reels.empty()) return false;
    for (const auto& reel : m_reels)
        if (!reel->isDone()) return false;
    return true;
}
