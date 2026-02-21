#include "ReelAnimation.hpp"
#include "../items/ItemRegistry.hpp"

#include <random>
#include <cmath>
#include <algorithm>
#include <iostream>

// ── Constructor ───────────────────────────────────────────────────────────────
ReelAnimation::ReelAnimation(sf::Vector2f topLeft, const sf::Font& font,
                               LayoutScale scale)
    : m_topLeft(topLeft)
    , m_font(&font)
    , m_scale(scale)
{
    // If default scale, compute from constants
    if (m_scale.reelW == 0.f)
    {
        m_scale.cardW      = CARD_W;
        m_scale.cardH      = CARD_H;
        m_scale.cardGap    = CARD_GAP;
        m_scale.cardStride = CARD_STRIDE;
        m_scale.reelW      = REEL_W;
        m_scale.reelH      = REEL_H;
    }

    const float W = m_scale.reelW;
    const float H = m_scale.reelH;

    m_reelBg.setSize({ W, H });
    m_reelBg.setFillColor(sf::Color(14, 14, 22));
    m_reelBg.setOutlineThickness(1.f);
    m_reelBg.setOutlineColor(sf::Color(40, 40, 65));
    m_reelBg.setPosition(topLeft);

    // Centre marker
    const float cx = topLeft.x + W / 2.f;
    m_markerLine.setSize({ 2.f, H });
    m_markerLine.setFillColor(sf::Color(220, 180, 60, 180));
    m_markerLine.setPosition(cx - 1.f, topLeft.y);

    m_cardShape.setSize({ m_scale.cardW, m_scale.cardH });
}

// ── prepare ───────────────────────────────────────────────────────────────────
void ReelAnimation::prepare(const Case& sourceCase, Item winningItem,
                             float durationSecs)
{
    m_duration    = durationSecs;
    m_phase       = Phase::Idle;
    m_elapsed     = 0.f;
    m_revealTimer = 0.f;
    m_glowTimer   = 0.f;

    buildStrip(sourceCase, std::move(winningItem));

    const float cW = m_scale.cardW;
    const float cS = m_scale.cardStride;
    const float rW = m_scale.reelW;

    // Target: winner card centred on the marker
    const float reelCentreLocal = rW / 2.f - cW / 2.f;
    m_targetOffset = static_cast<float>(m_winnerIndex) * cS - reelCentreLocal;

    // Start point: several screens of travel before winner
    m_startOffset = m_targetOffset - (cS * (VISIBLE_COLS * 3.5f));
    if (m_startOffset < 0.f) m_startOffset = 0.f;

    m_scrollOffset = m_startOffset;
}

// ── buildStrip ────────────────────────────────────────────────────────────────
void ReelAnimation::buildStrip(const Case& sourceCase, Item winningItem)
{
    m_strip.clear();

    for (int i = 0; i < PADDING_CARDS; ++i)
    {
        auto opt = sourceCase.open();
        if (!opt) opt = winningItem;
        ReelItem ri { std::move(*opt) };
        ri.item.ensureTextureLoaded();
        ri.textureLoaded = ri.item.hasTexture();
        m_strip.push_back(std::move(ri));
    }

    m_winnerIndex = m_strip.size();
    {
        ReelItem ri { winningItem };
        ri.item.ensureTextureLoaded();
        ri.textureLoaded = ri.item.hasTexture();
        m_strip.push_back(std::move(ri));
    }

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

// ── easeOutQuint ──────────────────────────────────────────────────────────────
// Faster initial movement, very smooth and satisfying stop.
float ReelAnimation::easeOutQuint(float t)
{
    t = std::clamp(t, 0.f, 1.f);
    const float inv = 1.f - t;
    return 1.f - inv * inv * inv * inv * inv;
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
            const float t    = std::clamp(m_elapsed / m_duration, 0.f, 1.f);
            const float ease = easeOutQuint(t);
            m_scrollOffset   = m_startOffset + ease * (m_targetOffset - m_startOffset);

            if (t >= 1.f)
            {
                m_scrollOffset = m_targetOffset;
                m_phase        = Phase::Revealing;
                m_revealTimer  = 0.f;
            }
            break;
        }

        case Phase::Revealing:
            m_revealTimer += dt;
            if (m_revealTimer >= REVEAL_DURATION)
                transitionToDone();
            break;
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
    const float cS = m_scale.cardStride;
    const float rW = m_scale.reelW;
    const float rH = m_scale.reelH;

    window.draw(m_reelBg);

    const float leftEdge  = m_scrollOffset;
    const float rightEdge = leftEdge + rW;

    int firstCard = std::max(0, static_cast<int>(leftEdge / cS) - 1);
    int lastCard  = std::min(static_cast<int>(m_strip.size()) - 1,
                             static_cast<int>(rightEdge / cS) + 1);

    float revealAlpha = 0.f;
    if (m_phase == Phase::Revealing)
        revealAlpha = std::clamp(m_revealTimer / REVEAL_DURATION, 0.f, 1.f);
    if (m_phase == Phase::Done)
        revealAlpha = 1.f;

    // Viewport scissor so cards don't bleed outside reel bounds
    const sf::View origView = window.getView();
    {
        const sf::Vector2u winSize = window.getSize();
        sf::View clipView = origView;
        clipView.setViewport({
            m_topLeft.x / winSize.x,
            m_topLeft.y / winSize.y,
            rW / winSize.x,
            rH / winSize.y
        });
        clipView.setCenter(m_topLeft.x + rW / 2.f,
                           m_topLeft.y + rH / 2.f);
        clipView.setSize(rW, rH);
        window.setView(clipView);
    }

    for (int i = firstCard; i <= lastCard; ++i)
    {
        const float cardScreenX = m_topLeft.x
            + (static_cast<float>(i) * cS - m_scrollOffset);
        const float cardScreenY = m_topLeft.y + 8.f;

        // Centre of card in world space — used for sprite centering
        const sf::Vector2f centre {
            cardScreenX + m_scale.cardW / 2.f,
            cardScreenY + m_scale.cardH / 2.f
        };

        drawCard(window, m_strip[i], centre,
                 (i == static_cast<int>(m_winnerIndex)), revealAlpha);
    }

    window.setView(origView);

    // Marker line and vignette drawn after restoring view
    window.draw(m_markerLine);

    // Left/right vignette fade
    sf::RectangleShape vignette;
    const float vigW = std::min(55.f, rW * 0.12f);
    vignette.setSize({ vigW, rH });
    vignette.setFillColor(sf::Color(14, 14, 22, 210));
    vignette.setPosition(m_topLeft);
    window.draw(vignette);
    vignette.setPosition(m_topLeft.x + rW - vigW, m_topLeft.y);
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
    const float cW = m_scale.cardW;
    const float cH = m_scale.cardH;
    const float leftX = centre.x - cW / 2.f;
    const float topY  = centre.y - cH / 2.f;

    // Card background
    m_cardShape.setSize({ cW, cH });
    m_cardShape.setPosition(leftX, topY);
    m_cardShape.setFillColor(sf::Color(20, 20, 32));

    if (isWinner && revealAlpha > 0.f)
    {
        const float pulse = (std::sin(m_glowTimer * 5.f) + 1.f) / 2.f;
        const sf::Uint8 glowA = static_cast<sf::Uint8>(
            revealAlpha * (160.f + 95.f * pulse));
        m_cardShape.setOutlineThickness(3.f);
        m_cardShape.setOutlineColor(sf::Color(rarityCol.r, rarityCol.g, rarityCol.b, glowA));
    }
    else
    {
        m_cardShape.setOutlineThickness(1.f);
        m_cardShape.setOutlineColor(sf::Color(rarityCol.r, rarityCol.g, rarityCol.b, 70));
    }
    window.draw(m_cardShape);

    // Rarity colour bar at bottom
    sf::RectangleShape bar({ cW, 4.f });
    bar.setFillColor(rarityCol);
    bar.setPosition(leftX, topY + cH - 4.f);
    window.draw(bar);

    // ── Item image ────────────────────────────────────────────────────────────
    // BUG FIX: sprite.setOrigin() was called but the position calculation
    // assumed a top-left origin. Now both consistently use centred origin.
    const float imgPad    = 10.f;
    const float imgAreaW  = cW - imgPad * 2.f;
    const float imgAreaH  = cH - imgPad * 2.f - 16.f; // leave room for label

    if (ri.item.hasTexture())
    {
        sf::Sprite sprite = ri.item.makeSprite(); // makeSprite() sets origin to centre
        const sf::FloatRect tb = sprite.getLocalBounds();
        const float maxDim     = std::max(tb.width, tb.height);
        const float scale      = (maxDim > 0.f)
                               ? std::min(imgAreaW, imgAreaH) / maxDim
                               : 1.f;
        sprite.setScale(scale, scale);
        // Position at centre of image area (not card centre — shift up for label)
        sprite.setPosition(centre.x, topY + imgPad + imgAreaH / 2.f);
        window.draw(sprite);
    }
    else
    {
        sf::RectangleShape ph({ imgAreaW, imgAreaH });
        ph.setFillColor(sf::Color(rarityCol.r, rarityCol.g, rarityCol.b, 35));
        ph.setOutlineThickness(1.f);
        ph.setOutlineColor(sf::Color(rarityCol.r, rarityCol.g, rarityCol.b, 90));
        ph.setPosition(leftX + imgPad, topY + imgPad);
        window.draw(ph);
    }

    // Item name label
    if (m_font)
    {
        sf::Text nameText;
        nameText.setFont(*m_font);
        nameText.setCharacterSize(static_cast<unsigned>(std::max(8.f, cW * 0.07f)));
        nameText.setFillColor(sf::Color(195, 195, 210));

        std::string label = ri.item.displayName();
        const std::size_t maxChars = static_cast<std::size_t>(cW / 7.5f);
        if (label.size() > maxChars)
            label = label.substr(0, maxChars - 2) + "..";
        nameText.setString(label);

        const sf::FloatRect nb = nameText.getLocalBounds();
        nameText.setOrigin(nb.left + nb.width / 2.f, nb.top);
        nameText.setPosition(centre.x, topY + cH - 18.f);
        window.draw(nameText);
    }
}
