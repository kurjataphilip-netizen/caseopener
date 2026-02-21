#include "MultiReelManager.hpp"
#include <algorithm>
#include <cmath>

// ── Constructor ───────────────────────────────────────────────────────────────
MultiReelManager::MultiReelManager(const sf::Font& font)
    : m_font(&font)
{}

// ── prepare ───────────────────────────────────────────────────────────────────
void MultiReelManager::prepare(const Case& sourceCase, OpenCount count,
                                sf::Vector2u windowSize)
{
    const int n = static_cast<int>(count);

    m_reels.clear();
    m_startDelays.clear();
    m_startTimers.clear();
    m_started.clear();
    m_results.clear();
    m_doneCount = 0;

    buildLayout(n, windowSize);

    // Roll all winning items up-front
    std::vector<Item> wins;
    wins.reserve(n);
    for (int i = 0; i < n; ++i)
    {
        auto opt = sourceCase.open();
        if (!opt) opt = sourceCase.open();
        if (opt) wins.push_back(std::move(*opt));
    }
    while (static_cast<int>(wins.size()) < n)
    {
        auto opt = sourceCase.open();
        if (opt) wins.push_back(std::move(*opt));
        else     break;
    }

    const float baseDuration = (n <= 1) ? 4.5f
                             : (n <= 3) ? 4.0f
                             : (n <= 5) ? 3.5f
                             :            3.0f;

    for (int i = 0; i < static_cast<int>(m_reels.size())
                 && i < static_cast<int>(wins.size()); ++i)
    {
        m_reels[i]->prepare(sourceCase, wins[i], baseDuration);

        m_reels[i]->setOnComplete([this](const Item& item)
        {
            m_results.push_back(item);
            ++m_doneCount;
            if (m_doneCount == static_cast<int>(m_reels.size()) && m_onAllComplete)
                m_onAllComplete(m_results);
        });

        m_startDelays.push_back(static_cast<float>(i) * 0.10f);
        m_startTimers.push_back(0.f);
        m_started.push_back(false);
    }
}

// ── computeScale ─────────────────────────────────────────────────────────────
// FIX: return type and parameter type are plain LayoutScale, not ReelAnimation::LayoutScale
LayoutScale MultiReelManager::computeScale(int n, sf::Vector2u winSize,
                                            int cols, int rows,
                                            float availW, float availH) const
{
    LayoutScale s;

    const float hGap = (cols > 1) ? 16.f : 0.f;
    const float vGap = (rows > 1) ? 12.f : 0.f;

    const float maxReelW = (availW - hGap * (cols - 1)) / cols;
    const float maxReelH = (availH - vGap * (rows - 1)) / rows;

    const float visibleCards = static_cast<float>(ReelAnimation::VISIBLE_COLS);
    const float cardGap  = std::max(4.f, ReelAnimation::CARD_GAP * (maxReelW / ReelAnimation::REEL_W));
    const float cardW    = std::clamp((maxReelW - cardGap * (visibleCards - 1)) / visibleCards,
                                       60.f, ReelAnimation::CARD_W);
    const float cardH    = std::clamp(maxReelH - 20.f, 45.f, ReelAnimation::CARD_H);
    const float stride   = cardW + cardGap;
    const float reelW    = stride * visibleCards - cardGap;
    const float reelH    = cardH + 20.f;

    s.cardW      = cardW;
    s.cardH      = cardH;
    s.cardGap    = cardGap;
    s.cardStride = stride;
    s.reelW      = reelW;
    s.reelH      = reelH;

    return s;
}

// ── buildLayout ───────────────────────────────────────────────────────────────
void MultiReelManager::buildLayout(int n, sf::Vector2u winSize)
{
    const float W = static_cast<float>(winSize.x);
    const float H = static_cast<float>(winSize.y);

    const float topPad    = 150.f;
    const float bottomPad = 100.f;
    const float availH    = H - topPad - bottomPad;
    const float availW    = W - 32.f;

    int cols = 1, rows = n;
    if (n == 3)  { cols = 3; rows = 1; }
    if (n == 5)  { cols = 5; rows = 1; }
    if (n == 10) { cols = 5; rows = 2; }

    // FIX: plain LayoutScale, not ReelAnimation::LayoutScale
    const LayoutScale scale =
        computeScale(n, winSize, cols, rows, availW, availH);

    const float rW = scale.reelW;
    const float rH = scale.reelH;

    const float hGap = (cols > 1) ? std::max(8.f, (availW - cols * rW) / (cols - 1)) : 0.f;
    const float vGap = (rows > 1) ? std::max(8.f, (availH - rows * rH) / (rows - 1)) : 0.f;

    const float totalW = cols * rW + (cols - 1) * hGap;
    const float totalH = rows * rH + (rows - 1) * vGap;

    const float startX = (W - totalW) / 2.f;
    const float startY = topPad + (availH - totalH) / 2.f;

    m_layoutHeight = totalH;

    for (int r = 0; r < rows; ++r)
    {
        for (int c = 0; c < cols; ++c)
        {
            const int idx = r * cols + c;
            if (idx >= n) break;

            const float x = startX + c * (rW + hGap);
            const float y = startY + r * (rH + vGap);

            m_reels.push_back(
                std::make_unique<ReelAnimation>(
                    sf::Vector2f(x, y), *m_font, scale));
        }
    }
}

// ── winnerCentre ─────────────────────────────────────────────────────────────
sf::Vector2f MultiReelManager::winnerCentre(std::size_t i) const
{
    if (i >= m_reels.size()) return { -1.f, -1.f };

    const auto& reel  = *m_reels[i];
    const auto& scale = reel.scale();
    const sf::Vector2f tl = reel.topLeft();

    return { tl.x + scale.reelW / 2.f,
             tl.y + scale.reelH / 2.f };
}

// ── startAll ──────────────────────────────────────────────────────────────────
void MultiReelManager::startAll()
{
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
