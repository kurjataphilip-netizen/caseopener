#include "FreeCaseReward.hpp"
#include <cmath>
#include <cstdio>
#include <algorithm>

// ── Constructor ───────────────────────────────────────────────────────────────
FreeCaseReward::FreeCaseReward(float cooldownSeconds)
    : m_cooldownSecs(cooldownSeconds)
{
    // Start as ready (cooldownEnd = now, so secondsRemaining = 0)
    m_cooldownEnd = Clock::now();
}

// ── isReady ───────────────────────────────────────────────────────────────────
bool FreeCaseReward::isReady() const
{
    return Clock::now() >= m_cooldownEnd;
}

// ── secondsRemaining ──────────────────────────────────────────────────────────
float FreeCaseReward::secondsRemaining() const
{
    if (isReady()) return 0.f;
    const auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(
                         m_cooldownEnd - Clock::now());
    return static_cast<float>(dur.count()) / 1000.f;
}

// ── cooldownProgress ──────────────────────────────────────────────────────────
// 0.0 = just started, 1.0 = ready
float FreeCaseReward::cooldownProgress() const
{
    if (isReady()) return 1.f;
    const float remaining = secondsRemaining();
    return std::clamp(1.f - remaining / m_cooldownSecs, 0.f, 1.f);
}

// ── cooldownLabel ─────────────────────────────────────────────────────────────
std::string FreeCaseReward::cooldownLabel() const
{
    if (isReady()) return "FREE CASE READY!";

    const float secs  = secondsRemaining();
    const int   mins  = static_cast<int>(secs) / 60;
    const int   s     = static_cast<int>(secs) % 60;

    char buf[32];
    if (mins >= 60)
    {
        const int hrs = mins / 60;
        const int m   = mins % 60;
        std::snprintf(buf, sizeof(buf), "Free case in %dh %02dm", hrs, m);
    }
    else
    {
        std::snprintf(buf, sizeof(buf), "Free case in %02d:%02d", mins, s);
    }
    return std::string(buf);
}

// ── loadCooldownEnd ───────────────────────────────────────────────────────────
void FreeCaseReward::loadCooldownEnd(double unixTimestamp)
{
    if (unixTimestamp <= 0.0)
    {
        // 0 means "already ready"
        m_cooldownEnd = Clock::now();
        return;
    }
    const auto dur = std::chrono::duration_cast<Clock::duration>(
        std::chrono::duration<double>(unixTimestamp));
    m_cooldownEnd = TimePoint(dur);
}

// ── cooldownEndTimestamp ──────────────────────────────────────────────────────
double FreeCaseReward::cooldownEndTimestamp() const
{
    return std::chrono::duration<double>(m_cooldownEnd.time_since_epoch()).count();
}

// ── claim ─────────────────────────────────────────────────────────────────────
std::optional<Item> FreeCaseReward::claim(const Case& sourceCase)
{
    if (!isReady()) return std::nullopt;

    auto item = sourceCase.open();
    if (!item) return std::nullopt;

    // Reset cooldown from NOW
    m_cooldownEnd = Clock::now()
                  + std::chrono::duration_cast<Clock::duration>(
                        std::chrono::duration<float>(m_cooldownSecs));

    return item;
}
