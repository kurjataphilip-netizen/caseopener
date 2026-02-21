#pragma once

#include <SFML/Graphics/Color.hpp>
#include <string>
#include <array>

// ── Rarity ────────────────────────────────────────────────────────────────────
enum class Rarity : int
{
    Consumer    = 0,
    Industrial  = 1,
    MilSpec     = 2,
    Restricted  = 3,
    Classified  = 4,
    Covert      = 5,
    Contraband  = 6
};

// ── RarityMeta ────────────────────────────────────────────────────────────────
// Uses plain RGB ints instead of sf::Color so this can be a simple const array.
// (sf::Color is not a literal type in GCC 15, breaking constexpr)
struct RarityMeta
{
    const char* name;
    int         r, g, b;
    float       dropWeight;
    float       baseValueMulti;

    sf::Color color() const { return sf::Color(r, g, b); }
};

// ── rarityData ────────────────────────────────────────────────────────────────
inline const std::array<RarityMeta, 7> rarityData
{{
    { "Consumer",    176, 195, 217,  79.92f,   1.0f  },
    { "Industrial",   94, 152, 217,  15.98f,   2.5f  },
    { "Mil-Spec",     75, 105, 255,   3.20f,   8.0f  },
    { "Restricted",  136,  71, 255,   0.64f,  25.0f  },
    { "Classified",  211,  44, 230,   0.128f, 80.0f  },
    { "Covert",      235,  75,  75,   0.026f, 250.0f },
    { "Contraband",  228, 174,  57,   0.004f, 800.0f },
}};

// ── Helper free functions ─────────────────────────────────────────────────────

inline const RarityMeta& getRarityMeta(Rarity r)
{
    return rarityData[static_cast<int>(r)];
}

inline const char* rarityName(Rarity r)
{
    return getRarityMeta(r).name;
}

inline sf::Color rarityColor(Rarity r)
{
    return getRarityMeta(r).color();
}

inline float rarityDropWeight(Rarity r)
{
    return getRarityMeta(r).dropWeight;
}

inline float rarityBaseValueMultiplier(Rarity r)
{
    return getRarityMeta(r).baseValueMulti;
}

inline float totalRarityWeight()
{
    float sum = 0.f;
    for (const auto& m : rarityData) sum += m.dropWeight;
    return sum;
}
