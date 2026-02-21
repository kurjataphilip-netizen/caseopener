#pragma once

#include <string>
#include <array>
#include <stdexcept>

// ── WearTier ──────────────────────────────────────────────────────────────────
// Named condition bands that map to a float range [0.00 – 1.00].
// ─────────────────────────────────────────────────────────────────────────────
enum class WearTier : int
{
    FactoryNew    = 0,   // 0.00 – 0.07
    MinimalWear   = 1,   // 0.07 – 0.15
    FieldTested   = 2,   // 0.15 – 0.38
    WellWorn      = 3,   // 0.38 – 0.45
    BattleScarred = 4    // 0.45 – 1.00
};

// ── WearBand ──────────────────────────────────────────────────────────────────
struct WearBand
{
    const char* name;           // display label
    const char* abbreviation;   // e.g. "FN"
    float       minWear;        // inclusive lower bound
    float       maxWear;        // exclusive upper bound (except last = inclusive 1.0)
    float       valueMulti;     // multiplier applied on top of rarity base value
};

// ── wearBands ─────────────────────────────────────────────────────────────────
// Indexed by static_cast<int>(WearTier::*).
// Value multipliers: Factory New commands a premium; Battle Scarred is discounted.
inline constexpr std::array<WearBand, 5> wearBands
{{
    //  name              abbrev  min     max    valueMulti
    { "Factory New",      "FN",  0.00f,  0.07f,  1.50f },
    { "Minimal Wear",     "MW",  0.07f,  0.15f,  1.15f },
    { "Field-Tested",     "FT",  0.15f,  0.38f,  1.00f },
    { "Well-Worn",        "WW",  0.38f,  0.45f,  0.80f },
    { "Battle-Scarred",   "BS",  0.45f,  1.00f,  0.55f },
}};

// ── classifyWear ──────────────────────────────────────────────────────────────
// Returns the WearTier for a raw float in [0.00, 1.00].
// Clamps values outside [0, 1] to the nearest valid tier.
inline WearTier classifyWear(float wear)
{
    // Iterate in reverse so the last tier (BattleScarred) catches anything >= 0.45.
    for (int i = static_cast<int>(wearBands.size()) - 1; i >= 0; --i)
    {
        if (wear >= wearBands[i].minWear)
            return static_cast<WearTier>(i);
    }
    return WearTier::FactoryNew; // wear <= 0
}

// ── Helper free functions ─────────────────────────────────────────────────────

inline const WearBand& getWearBand(WearTier tier)
{
    return wearBands[static_cast<int>(tier)];
}

inline const WearBand& getWearBand(float wear)
{
    return getWearBand(classifyWear(wear));
}

inline const char* wearTierName(WearTier tier)
{
    return getWearBand(tier).name;
}

inline const char* wearTierAbbrev(WearTier tier)
{
    return getWearBand(tier).abbreviation;
}

inline float wearValueMultiplier(float wear)
{
    return getWearBand(wear).valueMulti;
}

// ── Wear clamping helper ──────────────────────────────────────────────────────
inline float clampWear(float wear)
{
    if (wear < 0.f) return 0.f;
    if (wear > 1.f) return 1.f;
    return wear;
}

// ── Wear display string ───────────────────────────────────────────────────────
// Returns e.g.  "Field-Tested (0.243)"
inline std::string wearDisplayString(float wear)
{
    wear = clampWear(wear);
    const WearBand& band = getWearBand(wear);

    // Build "WearName (0.###)"
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%s (%.3f)", band.name, wear);
    return std::string(buf);
}
