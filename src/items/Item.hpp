#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <memory>
#include <optional>

#include "Rarity.hpp"
#include "Wear.hpp"

// ── ItemDef ───────────────────────────────────────────────────────────────────
// Immutable definition of an item type — shared between all instances.
// Think of this as the "template" row in a database.
// ─────────────────────────────────────────────────────────────────────────────
struct ItemDef
{
    std::string id;             // unique key, e.g. "ak47_redline"
    std::string displayName;    // e.g. "AK-47 | Red Line"
    Rarity      rarity;
    float       baseValue;      // USD value at Field-Tested / no wear multiplier

    // Path relative to the binary, e.g. "assets/images/ak47_redline.png"
    std::string imagePath;
};

// ── Item ──────────────────────────────────────────────────────────────────────
// A concrete instance of an item — has wear, a computed value, and
// a lazily-loaded texture that can be drawn to the screen.
// ─────────────────────────────────────────────────────────────────────────────
class Item
{
public:
    // ── Construction ──────────────────────────────────────────────────────────
    // wear is clamped to [0.00, 1.00] automatically.
    Item(const ItemDef& def, float wear);

    // Convenience: create with a random wear value inside a specific tier.
    static Item makeWithRandomWear(const ItemDef& def, WearTier tier);

    // ── Core accessors ────────────────────────────────────────────────────────
    const std::string& id()          const { return m_def.id; }
    const std::string& displayName() const { return m_def.displayName; }
    Rarity             rarity()      const { return m_def.rarity; }
    float              wear()        const { return m_wear; }
    WearTier           wearTier()    const { return m_wearTier; }

    // Final market value = baseValue * rarityMulti * wearMulti
    float value() const { return m_value; }

    // Short condition label, e.g. "(FN)"
    std::string wearLabel()   const;

    // Full description, e.g. "AK-47 | Red Line — Factory New (0.043)"
    std::string description() const;

    // ── Rarity helpers ────────────────────────────────────────────────────────
    sf::Color rarityColor()  const { return ::rarityColor(m_def.rarity); }
    const char* rarityName() const { return ::rarityName(m_def.rarity); }

    // ── Texture / sprite ──────────────────────────────────────────────────────
    // Loads the texture from disk the first time it is called.
    // Returns false if the image file could not be found.
    bool ensureTextureLoaded();

    // Returns true only after a successful ensureTextureLoaded().
    bool hasTexture() const { return m_textureLoaded; }

    // Returns a sprite centred at (0, 0) — caller positions it.
    sf::Sprite makeSprite() const;

    // Raw texture access (e.g. for ImGui or custom drawing).
    const sf::Texture* texture() const;

private:
    void computeValue();

    ItemDef  m_def;
    float    m_wear;
    WearTier m_wearTier;
    float    m_value { 0.f };

    // Texture is shared across copies of the same Item if you copy by value,
    // but typically Items are moved/stored in collections.
    std::shared_ptr<sf::Texture> m_texture;
    bool                         m_textureLoaded { false };
};
