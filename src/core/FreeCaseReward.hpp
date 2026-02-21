#pragma once

#include <chrono>
#include <string>
#include <functional>
#include "../cases/Case.hpp"
#include "../items/Item.hpp"

// ── FreeCaseReward ────────────────────────────────────────────────────────────
// Grants the player a free case opening on a fixed cooldown.
// Cooldown end time is persisted as a Unix timestamp in the save file.
//
// Usage:
//   FreeCaseReward reward;
//   reward.setCooldownSeconds(3600.f);        // 1 hour
//   reward.loadCooldownEnd(savedTimestamp);   // from JSON save
//
//   if (reward.isReady())
//       auto item = reward.claim(someCase);   // returns the free item
//
//   // For UI:
//   float secs = reward.secondsRemaining();
//   std::string label = reward.cooldownLabel(); // "Ready!" or "59:43"
// ─────────────────────────────────────────────────────────────────────────────
class FreeCaseReward
{
public:
    // Default cooldown: 30 minutes (easy to change for testing)
    static constexpr float DEFAULT_COOLDOWN_SECS = 30.f * 60.f;

    explicit FreeCaseReward(float cooldownSeconds = DEFAULT_COOLDOWN_SECS);

    // ── Cooldown state ────────────────────────────────────────────────────────
    bool  isReady()           const;
    float secondsRemaining()  const;   // 0 if ready
    float cooldownProgress()  const;   // 0.0 (just started) → 1.0 (ready)

    // Returns "Ready!" or formatted "MM:SS"
    std::string cooldownLabel() const;

    // ── Persistence ───────────────────────────────────────────────────────────
    // Restore cooldown end from save file (Unix timestamp as double).
    void   loadCooldownEnd(double unixTimestamp);
    double cooldownEndTimestamp() const; // for saving

    // ── Claiming ──────────────────────────────────────────────────────────────
    // Opens sourceCase for free and resets the cooldown.
    // Returns nullopt if not ready or case open fails.
    std::optional<Item> claim(const Case& sourceCase);

    // ── Configuration ─────────────────────────────────────────────────────────
    void  setCooldownSeconds(float secs) { m_cooldownSecs = secs; }
    float cooldownSeconds()       const  { return m_cooldownSecs; }

private:
    using Clock     = std::chrono::system_clock;
    using TimePoint = Clock::time_point;

    TimePoint m_cooldownEnd    { Clock::now() }; // initialise as already ready
    float     m_cooldownSecs;
};
