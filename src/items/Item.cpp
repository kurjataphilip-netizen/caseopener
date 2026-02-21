#include "Item.hpp"

#include <cmath>
#include <cstdio>
#include <iostream>
#include <random>
#include <stdexcept>

// ── Constructor ───────────────────────────────────────────────────────────────
Item::Item(const ItemDef& def, float wear)
    : m_def(def)
    , m_wear(clampWear(wear))
    , m_wearTier(classifyWear(m_wear))
{
    computeValue();
}

// ── makeWithRandomWear ────────────────────────────────────────────────────────
// Generates a uniform-random wear float inside the given WearTier band,
// then constructs an Item with it.
Item Item::makeWithRandomWear(const ItemDef& def, WearTier tier)
{
    const WearBand& band = getWearBand(tier);

    static std::mt19937 rng{ std::random_device{}() };
    std::uniform_real_distribution<float> dist(band.minWear, band.maxWear);

    return Item(def, dist(rng));
}

// ── wearLabel ────────────────────────────────────────────────────────────────
std::string Item::wearLabel() const
{
    char buf[8];
    std::snprintf(buf, sizeof(buf), "(%s)", wearTierAbbrev(m_wearTier));
    return std::string(buf);
}

// ── description ───────────────────────────────────────────────────────────────
std::string Item::description() const
{
    // "AK-47 | Red Line — Factory New (0.043)"
    return m_def.displayName + " \xe2\x80\x94 " + wearDisplayString(m_wear);
    //                          ^^^^ UTF-8 em dash
}

// ── ensureTextureLoaded ───────────────────────────────────────────────────────
bool Item::ensureTextureLoaded()
{
    if (m_textureLoaded) return true;
    if (m_def.imagePath.empty()) return false;

    m_texture = std::make_shared<sf::Texture>();

    if (!m_texture->loadFromFile(m_def.imagePath))
    {
        std::cerr << "[Item] Could not load image: " << m_def.imagePath << "\n";
        m_texture.reset();
        return false;
    }

    m_texture->setSmooth(true);
    m_textureLoaded = true;
    return true;
}

// ── makeSprite ────────────────────────────────────────────────────────────────
sf::Sprite Item::makeSprite() const
{
    if (!m_textureLoaded || !m_texture)
        return sf::Sprite{};

    sf::Sprite sprite(*m_texture);

    // Centre the origin so callers can just setPosition(x, y).
    const sf::FloatRect bounds = sprite.getLocalBounds();
    sprite.setOrigin(bounds.width / 2.f, bounds.height / 2.f);

    return sprite;
}

// ── texture ───────────────────────────────────────────────────────────────────
const sf::Texture* Item::texture() const
{
    return m_texture ? m_texture.get() : nullptr;
}

// ── computeValue ──────────────────────────────────────────────────────────────
// Final value = baseValue × rarityBaseMultiplier × wearMultiplier
//
// Example:
//   Covert AK at baseValue $15, FN wear 0.003:
//   $15 × 250.0 × 1.50 = $5,625.00
void Item::computeValue()
{
    const float rarityMulti = rarityBaseValueMultiplier(m_def.rarity);
    const float wearMulti   = wearValueMultiplier(m_wear);

    m_value = m_def.baseValue * rarityMulti * wearMulti;

    // Round to 2 decimal places for display consistency.
    m_value = std::round(m_value * 100.f) / 100.f;
}
