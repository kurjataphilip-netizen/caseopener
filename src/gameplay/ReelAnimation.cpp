#include "ReelAnimation.hpp"
#include "../items/ItemRegistry.hpp"

#include <random>
#include <cmath>
#include <algorithm>
#include <iostream>

// ── Constructor ───────────────────────────────────────────────────────────────
ReelAnimation::ReelAnimation(sf::Vector2f topLeft, const sf::Font& font)
    : m_topLeft(topLeft)
    , m_font(&font)
{
    // Reel background
    m_reelBg.setSize({ REEL_W, REEL_H });
    m_reelBg.setFillColor(sf::Color(14, 14, 22));
    m_reelBg.setOutlineThickness(2.f);
    m_reelBg.setOutlineColor(sf::Color(55, 55, 80));
    m_reelBg.setPosition(topLeft);

    // Centre marker (vertical line + top/bottom triangles implied by two lines)
    const float cx = topLeft.x + REEL_W / 2.f;
    m_markerLine.setSize({ 3.f, REEL_H });
    m_markerLine.setFillColor(sf::Color(220, 180, 60, 200));
    m_markerLine.setPosition(cx - 1.5f, topLeft.y);

    m_cardShape.setSize({ CARD_W, CARD_H });
}

// ── prepare ───────────────────────────────────────────────────────────────────
void ReelAnimation::prepare(const Case& sourceCase, Item winningItem,
                             float durationSecs)
{
    m_duration = durationSecs;
    m_phase    = Phase::Idle;
    m_elapsed  = 0.f;
    m_revealTimer = 0.f;
    m_glowTimer   = 0.f;

    buildStrip(sourceCase, std::move(winningItem));

    // Target: winner card centre lands exactly on the reel centre marker.
    const float reelCentreLocal = REEL_W / 2.f - CARD_W / 2.f;
    m_targetOffset = static_cast<float>(m_winnerIndex) * CARD_STRIDE - reelCentreLocal;

    // Start the scroll from far left so the reel has to travel a good distance.
    m_startOffset  = m_targetOffset
                   - (CARD_STRIDE * (VISIBLE_COLS * 3.f)); // ~3 screens of travel
    if (m_startOffset < 0.f) m_startOffset = 0.f;

    m_scrollOffset = m_startOffset;
}

// ── buildStrip ────────────────────────────────────────────────────────────────
void ReelAnimation::buildStrip(const Case& sourceCase, Item winningItem)
{
    m_strip.clear();

    static std::mt19937 rng{ std::random_device{}() };
    const int totalCards = PADDING_CARDS * 2 + 1;

    // Build filler items from case pool (just random opens, no texture needed immediately)
    for (int i = 0; i < PADDING_CARDS; ++i)
    {
        auto opt = sourceCase.open();
        if (!opt) opt = winningItem; // fallback
        ReelItem ri { std::move(*opt) };
        ri.item.ensureTextureLoaded();
        ri.textureLoaded = ri.item.hasTexture();
        m_strip.push_back(std::move(ri));
    }

    // Insert winner in the middle
    m_winnerIndex = m_strip.size();
    {
        ReelItem ri { winningItem };
        ri.item.ensureTextureLoaded();
        ri.textureLoaded = ri.item.hasTexture();
        m_strip.push_back(std::move(ri));
    }

    // More filler after winner
    for (int i = 0; i < PADDING_CARDS; ++i)
    {
        auto opt = sourceCase.open();
        if (!opt) opt = winningItem;
        ReelItem ri { std::move(*opt) };
        ri.item.ensureTextureLoaded();
        ri.textureLoaded = ri.item.hasTexture();
        m_strip.push_back(std::move(ri));
    }
}

// ── start ─────────────────────────────────────────────────────────────────────
void ReelAnimation::start()
{
    if (m_phase != Phase::Idle) return;
    m_phase   = Phase::Spinning;
    m_elapsed = 0.f;
}

// ── skip ──────────────────────────────────────────────────────────────────────
void ReelAnimation::skip()
{
    if (m_phase == Phase::Done) return;
    m_scrollOffset = m_targetOffset;
    transitionToDone();
}

// ── easeOutCubic ──────────────────────────────────────────────────────────────
float ReelAnimation::easeOutCubic(float t)
{
    // Classic ease-out cubic: starts fast, decelerates smoothly to 1.
    t = std::clamp(t, 0.f, 1.f);
    float inv = 1.f - t;
    return 1.f - inv * inv * inv;
}

// ── update ────────────────────────────────────────────────────────────────────
void ReelAnimation::update(float dt)
{
    m_glowTimer += dt;

    switch (m_phase)
    {
        case Phase::Idle:
        case Phase::Done:
            break;

        case Phase::Spinning:
        {
            m_elapsed += dt;
            const float t = std::clamp(m_elapsed / m_duration, 0.f, 1.f);
            const float ease = easeOutCubic(t);
            m_scrollOffset = m_startOffset
                           + ease * (m_targetOffset - m_startOffset);

            if (t >= 1.f)
            {
                m_scrollOffset = m_targetOffset;
                m_phase = Phase::Revealing;
                m_revealTimer = 0.f;
            }
            break;
        }

        case Phase::Revealing:
        {
            m_revealTimer += dt;
            if (m_revealTimer >= REVEAL_DURATION)
                transitionToDone();
            break;
        }
    }
}

// ── transitionToDone ──────────────────────────────────────────────────────────
void ReelAnimation::transitionToDone()
{
    m_phase = Phase::Done;
    if (m_onComplete && m_winnerIndex < m_strip.size())
        m_onComplete(m_strip[m_winnerIndex].item);
}

// ── render ────────────────────────────────────────────────────────────────────
void ReelAnimation::render(sf::RenderWindow& window)
{
    // Background
    window.draw(m_reelBg);

    // Clip region approximated via view scissor — draw only visible cards.
    // Determine which card indices are visible.
    const float leftEdge  = m_scrollOffset;
    const float rightEdge = leftEdge + REEL_W;

    int firstCard = std::max(0, static_cast<int>(leftEdge / CARD_STRIDE) - 1);
    int lastCard  = std::min(static_cast<int>(m_strip.size()) - 1,
                             static_cast<int>(rightEdge / CARD_STRIDE) + 1);

    // Reveal alpha for winner glow
    float revealAlpha = 0.f;
    if (m_phase == Phase::Revealing)
        revealAlpha = std::clamp(m_revealTimer / REVEAL_DURATION, 0.f, 1.f);
    if (m_phase == Phase::Done)
        revealAlpha = 1.f;

    // Save and set a scissor view so cards don't bleed outside the reel
    const sf::View origView = window.getView();
    {
        // Create a viewport that covers only the reel rectangle
        const sf::Vector2u winSize = window.getSize();
        sf::View clipView = origView;
        // Pixel-space clip: left/top in pixels, width/height in pixels
        const float px = m_topLeft.x;
        const float py = m_topLeft.y;
        const float pw = REEL_W;
        const float ph = REEL_H;

        clipView.setViewport({
            px / winSize.x,
            py / winSize.y,
            pw / winSize.x,
            ph / winSize.y
        });
        // Adjust the centre so world coords still match
        clipView.setCenter(m_topLeft.x + REEL_W / 2.f,
                           m_topLeft.y + REEL_H / 2.f);
        clipView.setSize(REEL_W, REEL_H);
        window.setView(clipView);
    }

    for (int i = firstCard; i <= lastCard; ++i)
    {
        const float cardScreenX = m_topLeft.x
                                + (static_cast<float>(i) * CARD_STRIDE - m_scrollOffset);
        const float cardScreenY = m_topLeft.y + 12.f;
        const sf::Vector2f centre { cardScreenX + CARD_W / 2.f,
                                    cardScreenY + CARD_H / 2.f };

        drawCard(window, m_strip[i], centre,
                 (i == static_cast<int>(m_winnerIndex)), revealAlpha);
    }

    // Restore view
    window.setView(origView);

    // Draw centre marker on top (outside clip so it spans full height)
    window.draw(m_markerLine);

    // Left/right fade vignette strips
    sf::RectangleShape vignette;
    vignette.setSize({ 60.f, REEL_H });
    // Left fade
    vignette.setFillColor(sf::Color(14, 14, 22, 200));
    vignette.setPosition(m_topLeft);
    window.draw(vignette);
    // Right fade
    vignette.setPosition(m_topLeft.x + REEL_W - 60.f, m_topLeft.y);
    window.draw(vignette);
}

// ── drawCard ──────────────────────────────────────────────────────────────────
void ReelAnimation::drawCard(sf::RenderWindow& window,
                              const ReelItem& ri,
                              sf::Vector2f centre,
                              bool isWinner,
                              float revealAlpha)
{
    const sf::Color rarityCol = ri.item.rarityColor();
    const float leftX = centre.x - CARD_W / 2.f;
    const float topY  = centre.y - CARD_H / 2.f;

    // Card background
    m_cardShape.setPosition(leftX, topY);
    m_cardShape.setFillColor(sf::Color(22, 22, 34));

    if (isWinner && revealAlpha > 0.f)
    {
        // Glow: pulse outline colour
        const float pulse = (std::sin(m_glowTimer * 5.f) + 1.f) / 2.f;
        const sf::Uint8 glowA = static_cast<sf::Uint8>(
            revealAlpha * (180.f + 75.f * pulse));
        m_cardShape.setOutlineThickness(3.f);
        m_cardShape.setOutlineColor(sf::Color(rarityCol.r, rarityCol.g, rarityCol.b, glowA));
    }
    else
    {
        m_cardShape.setOutlineThickness(1.f);
        m_cardShape.setOutlineColor(sf::Color(rarityCol.r, rarityCol.g, rarityCol.b, 80));
    }
    window.draw(m_cardShape);

    // Rarity colour bar at bottom of card
    sf::RectangleShape bar({ CARD_W, 4.f });
    bar.setFillColor(rarityCol);
    bar.setPosition(leftX, topY + CARD_H - 4.f);
    window.draw(bar);

    // Item image or coloured placeholder
    if (ri.item.hasTexture())
    {
        sf::Sprite sprite = ri.item.makeSprite();
        // Scale to fit inside card with 8px padding
        const sf::FloatRect tb = sprite.getLocalBounds();
        const float maxDim     = std::max(tb.width, tb.height);
        const float scale      = (maxDim > 0.f)
                               ? (CARD_W - 16.f) / maxDim
                               : 1.f;
        sprite.setScale(scale, scale);
        sprite.setPosition(centre.x, topY + (CARD_H - 4.f) / 2.f);
        window.draw(sprite);
    }
    else
    {
        // Coloured box placeholder
        sf::RectangleShape ph({ CARD_W - 24.f, CARD_H - 28.f });
        ph.setFillColor(sf::Color(rarityCol.r, rarityCol.g, rarityCol.b, 40));
        ph.setOutlineThickness(1.f);
        ph.setOutlineColor(sf::Color(rarityCol.r, rarityCol.g, rarityCol.b, 100));
        ph.setPosition(leftX + 12.f, topY + 8.f);
        window.draw(ph);
    }

    // Item name label (small, clipped by card width)
    if (m_font)
    {
        sf::Text nameText;
        nameText.setFont(*m_font);
        nameText.setCharacterSize(10);
        nameText.setFillColor(sf::Color(200, 200, 210));

        // Truncate name to fit
        std::string label = ri.item.displayName();
        if (label.size() > 18) label = label.substr(0, 15) + "...";
        nameText.setString(label);

        const sf::FloatRect nb = nameText.getLocalBounds();
        nameText.setOrigin(nb.left + nb.width / 2.f, nb.top);
        nameText.setPosition(centre.x, topY + CARD_H - 20.f);
        window.draw(nameText);
    }
}
