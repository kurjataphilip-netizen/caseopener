#include "CaseInfoPanel.hpp"
#include "../items/ItemRegistry.hpp"

#include <cstdio>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>

// ── Constructor ───────────────────────────────────────────────────────────────
CaseInfoPanel::CaseInfoPanel(const sf::Font& font)
    : m_font(&font)
{
    m_closeBtn = Button(font, "CLOSE", {0.f, 0.f}, {120.f, 36.f}, 15);
    m_closeBtn.setIdleColor ({ 35, 35, 55 });
    m_closeBtn.setHoverColor({ 60, 60, 90 });
    m_closeBtn.setOutlineColor(sf::Color(90, 90, 140), 1.f);
    m_closeBtn.setOnClick([this]() { close(); });

    m_dimOverlay.setFillColor(sf::Color(0, 0, 0, 0));

    m_bg.setSize({ PANEL_W, PANEL_H });
    m_bg.setFillColor(sf::Color(20, 20, 34));
    m_bg.setOutlineThickness(2.f);
    m_bg.setOutlineColor(sf::Color(70, 70, 110));

    m_titleBar.setSize({ PANEL_W, 48.f });
    m_titleBar.setFillColor(sf::Color(28, 28, 48));
}

// ── open ──────────────────────────────────────────────────────────────────────
void CaseInfoPanel::open(const Case& c)
{
    m_case       = &c;
    m_open       = true;
    m_barRevealT = 0.f;
    buildContent();
}

// ── close ─────────────────────────────────────────────────────────────────────
void CaseInfoPanel::close()
{
    m_open = false;
}

// ── panelPos ──────────────────────────────────────────────────────────────────
sf::Vector2f CaseInfoPanel::panelPos() const
{
    // Centred, with a vertical slide-in from +60px below
    const float cx = (1280.f - PANEL_W) / 2.f;
    const float cy = (720.f  - PANEL_H) / 2.f;
    const float eased = m_slideT * m_slideT * (3.f - 2.f * m_slideT); // smoothstep
    const float yOff  = (1.f - eased) * 60.f;
    return { cx, cy + yOff };
}

// ── buildContent ──────────────────────────────────────────────────────────────
void CaseInfoPanel::buildContent()
{
    if (!m_case) return;

    m_poolLines.clear();
    m_odds.clear();

    // Pool lines grouped by rarity (highest first)
    for (int r = 6; r >= 0; --r)
    {
        const Rarity rar = static_cast<Rarity>(r);
        const sf::Color col = rarityColor(rar);
        bool headerWritten = false;

        for (const CaseEntry& entry : m_case->pool())
        {
            const ItemDef* def = ItemRegistry::instance().find(entry.itemDefId);
            if (!def || def->rarity != rar) continue;

            if (!headerWritten)
            {
                m_poolLines.push_back({ std::string("  ") + rarityName(rar), col, 13 });
                headerWritten = true;
            }
            m_poolLines.push_back({ "      " + def->displayName, sf::Color(190, 190, 200), 12 });
        }
    }

    // Odds entries
    const float total = [&]()
    {
        float s = 0.f;
        for (int i = 0; i < 7; ++i)
            s += m_case->effectiveRarityWeight(static_cast<Rarity>(i));
        return s;
    }();

    for (int i = 6; i >= 0; --i) // rare first
    {
        const Rarity rar = static_cast<Rarity>(i);
        const float  w   = m_case->effectiveRarityWeight(rar);
        if (w <= 0.f) continue;

        const float frac = w / total;
        char pct[16];
        std::snprintf(pct, sizeof(pct), "%.3f%%", frac * 100.f);

        m_odds.push_back({ rarityName(rar), rarityColor(rar), frac, pct });
    }
}

// ── handleEvent ───────────────────────────────────────────────────────────────
void CaseInfoPanel::handleEvent(const sf::Event& event, const sf::RenderWindow& window)
{
    if (!isVisible()) return;

    m_closeBtn.handleEvent(event, window);

    // Click outside panel to close
    if (event.type == sf::Event::MouseButtonReleased &&
        event.mouseButton.button == sf::Mouse::Left)
    {
        const sf::Vector2f mf {
            static_cast<float>(sf::Mouse::getPosition(window).x),
            static_cast<float>(sf::Mouse::getPosition(window).y)
        };
        if (!m_bg.getGlobalBounds().contains(mf))
            close();
    }

    // Escape key
    if (event.type == sf::Event::KeyPressed &&
        event.key.code == sf::Keyboard::Escape)
        close();
}

// ── update ────────────────────────────────────────────────────────────────────
void CaseInfoPanel::update(float dt)
{
    // Slide in / out
    if (m_open)
    {
        m_slideT = std::min(1.f, m_slideT + dt * SLIDE_SPEED);
        m_barRevealT = std::min(1.f, m_barRevealT + dt / BAR_REVEAL_TIME);
    }
    else
    {
        m_slideT = std::max(0.f, m_slideT - dt * SLIDE_SPEED);
    }

    // Dim overlay alpha
    const float dimAlpha = m_slideT * 160.f;
    m_dimOverlay.setFillColor(sf::Color(0, 0, 0, static_cast<sf::Uint8>(dimAlpha)));

    if (!isVisible()) return;

    const sf::Vector2f pos = panelPos();
    m_bg.setPosition(pos);
    m_titleBar.setPosition(pos);
    m_closeBtn.setPosition({ pos.x + PANEL_W - 130.f, pos.y + PANEL_H - 48.f });
    m_closeBtn.update(sf::Mouse::getPosition());
}

// ── render ────────────────────────────────────────────────────────────────────
void CaseInfoPanel::render(sf::RenderWindow& window)
{
    if (!isVisible() || !m_case) return;

    // Set alpha on panel via colour
    const sf::Uint8 alpha = static_cast<sf::Uint8>(m_slideT * 255.f);

    // Dim entire screen
    m_dimOverlay.setSize({ 1280.f, 720.f });
    window.draw(m_dimOverlay);

    // Panel bg
    auto tinted = [&](sf::Color c) {
        c.a = static_cast<sf::Uint8>(c.a * m_slideT);
        return c;
    };
    m_bg.setFillColor(tinted(sf::Color(20, 20, 34, 255)));
    m_bg.setOutlineColor(tinted(sf::Color(70, 70, 110, 255)));
    window.draw(m_bg);

    m_titleBar.setFillColor(tinted(sf::Color(28, 28, 48, 255)));
    window.draw(m_titleBar);

    const sf::Vector2f pos = panelPos();

    // Title
    {
        sf::Text title;
        title.setFont(*m_font);
        title.setCharacterSize(18);
        title.setFillColor(sf::Color(220, 180, 60, alpha));
        title.setStyle(sf::Text::Bold);
        title.setString(m_case->displayName());
        sf::FloatRect tb = title.getLocalBounds();
        title.setOrigin(tb.left, tb.top + tb.height / 2.f);
        title.setPosition(pos.x + 16.f, pos.y + 24.f);
        window.draw(title);

        char pbuf[24];
        std::snprintf(pbuf, sizeof(pbuf), "$%.2f", m_case->price());
        sf::Text price;
        price.setFont(*m_font);
        price.setCharacterSize(16);
        price.setFillColor(sf::Color(160, 220, 160, alpha));
        price.setString(pbuf);
        sf::FloatRect pb = price.getLocalBounds();
        price.setOrigin(pb.left + pb.width, pb.top + pb.height / 2.f);
        price.setPosition(pos.x + PANEL_W - 16.f, pos.y + 24.f);
        window.draw(price);
    }

    float y = pos.y + 58.f;

    // ── Drop Odds section ─────────────────────────────────────────────────────
    {
        sf::Text sec;
        sec.setFont(*m_font);
        sec.setCharacterSize(13);
        sec.setFillColor(sf::Color(130, 130, 160, alpha));
        sec.setString("DROP ODDS");
        sec.setPosition(pos.x + 16.f, y);
        window.draw(sec);
        y += 20.f;
    }

    const float barAreaW = PANEL_W - 32.f;
    const float barH     = 18.f;
    const float barGap   = 6.f;

    for (const auto& oe : m_odds)
    {
        sf::FloatRect rowRect { pos.x + 16.f, y, barAreaW, barH };
        drawOddsBar(window, rowRect,
                    oe.fraction * m_barRevealT,
                    sf::Color(oe.colour.r, oe.colour.g, oe.colour.b, alpha),
                    oe.name, oe.pct);
        y += barH + barGap;
    }

    y += 10.f;

    // ── Pool section ──────────────────────────────────────────────────────────
    {
        sf::Text sec;
        sec.setFont(*m_font);
        sec.setCharacterSize(13);
        sec.setFillColor(sf::Color(130, 130, 160, alpha));
        sec.setString("ITEM POOL");
        sec.setPosition(pos.x + 16.f, y);
        window.draw(sec);
        y += 20.f;
    }

    for (const auto& line : m_poolLines)
    {
        if (y > pos.y + PANEL_H - 60.f) break; // don't overflow panel

        sf::Text t;
        t.setFont(*m_font);
        t.setCharacterSize(line.size);
        sf::Color c = line.colour;
        c.a = alpha;
        t.setFillColor(c);
        t.setString(line.str);
        t.setPosition(pos.x + 16.f, y);
        window.draw(t);
        y += line.size + 4.f;
    }

    // Close button
    window.draw(m_closeBtn);
}

// ── drawOddsBar ───────────────────────────────────────────────────────────────
void CaseInfoPanel::drawOddsBar(sf::RenderWindow& window,
                                 const sf::FloatRect& row,
                                 float fraction,
                                 sf::Color colour,
                                 const std::string& label,
                                 const std::string& pctStr)
{
    const float labelW = 90.f;
    const float pctW   = 64.f;
    const float barW   = row.width - labelW - pctW - 8.f;

    // Label
    sf::Text lbl;
    lbl.setFont(*m_font);
    lbl.setCharacterSize(11);
    lbl.setFillColor(colour);
    lbl.setString(label);
    sf::FloatRect lb = lbl.getLocalBounds();
    lbl.setOrigin(lb.left, lb.top + lb.height / 2.f);
    lbl.setPosition(row.left, row.top + row.height / 2.f);
    window.draw(lbl);

    // Bar background
    sf::RectangleShape barBg({ barW, row.height * 0.6f });
    barBg.setFillColor(sf::Color(35, 35, 55));
    barBg.setPosition(row.left + labelW, row.top + row.height * 0.2f);
    window.draw(barBg);

    // Bar fill (animated)
    if (fraction > 0.f)
    {
        sf::RectangleShape fill({ std::max(2.f, barW * fraction), row.height * 0.6f });
        fill.setFillColor(colour);
        fill.setPosition(row.left + labelW, row.top + row.height * 0.2f);
        window.draw(fill);
    }

    // Percentage
    sf::Text pct;
    pct.setFont(*m_font);
    pct.setCharacterSize(11);
    pct.setFillColor(sf::Color(colour.r, colour.g, colour.b,
                               static_cast<sf::Uint8>(colour.a * 0.8f)));
    pct.setString(pctStr);
    sf::FloatRect pb = pct.getLocalBounds();
    pct.setOrigin(pb.left + pb.width, pb.top + pb.height / 2.f);
    pct.setPosition(row.left + row.width, row.top + row.height / 2.f);
    window.draw(pct);
}
