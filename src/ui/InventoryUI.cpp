#include "InventoryUI.hpp"
#include <cmath>
#include <cstdio>
#include <algorithm>

// ── Constructor ───────────────────────────────────────────────────────────────
InventoryUI::InventoryUI(sf::FloatRect bounds, const sf::Font& font, Inventory& inventory)
    : m_font(&font)
    , m_inventory(inventory)
{
    m_sellButton  = Button(font, "SELL",  {0,0}, {110.f, 36.f}, 16);
    m_closeButton = Button(font, "CLOSE", {0,0}, {110.f, 36.f}, 16);
    m_sortButton  = Button(font, "Sort: Rarity \xe2\x86\x93", {0,0}, {160.f, 30.f}, 13);

    m_sellButton.setIdleColor ({ 90,  35, 35 });
    m_sellButton.setHoverColor({130,  50, 50 });
    m_sellButton.setPressColor({ 60,  20, 20 });
    m_sellButton.setOutlineColor(sf::Color(200, 80, 80), 1.5f);

    m_closeButton.setIdleColor ({ 35, 35, 55 });
    m_closeButton.setHoverColor({ 60, 60, 90 });

    m_sortButton.setIdleColor ({ 28, 28, 45 });
    m_sortButton.setHoverColor({ 50, 50, 80 });
    m_sortButton.setOutlineColor(sf::Color(80, 80, 130), 1.f);

    setBounds(bounds);

    m_sortButton.setOnClick([this]()
    {
        m_inventory.cycleSort();
        m_sortButton.setLabel(std::string("Sort: ")
            + Inventory::sortModeName(m_inventory.sortMode()));
        m_scrollY = 0.f;
    });

    m_sellButton.setOnClick([this]()
    {
        if (!m_inspectIndex) return;
        const float val = m_inventory.sellItem(*m_inspectIndex);
        if (m_onSell) m_onSell(val);
        m_inspectIndex.reset();
        clampScroll();
    });

    m_closeButton.setOnClick([this]() { m_inspectIndex.reset(); });
}

// ── setBounds ─────────────────────────────────────────────────────────────────
void InventoryUI::setBounds(sf::FloatRect bounds)
{
    m_bounds = bounds;
    m_gridArea = {
        bounds.left,
        bounds.top + HEADER_H,
        bounds.width,
        bounds.height - HEADER_H - FOOTER_H
    };

    m_headerBg.setSize({ bounds.width, HEADER_H });
    m_headerBg.setPosition(bounds.left, bounds.top);
    m_headerBg.setFillColor(sf::Color(22, 22, 36));

    m_gridBg.setSize({ bounds.width, m_gridArea.height });
    m_gridBg.setPosition(bounds.left, bounds.top + HEADER_H);
    m_gridBg.setFillColor(sf::Color(16, 16, 26));

    m_sortButton.setPosition({ bounds.left + 8.f, bounds.top + 7.f });

    const float px = bounds.left + (bounds.width  - PANEL_W) / 2.f;
    const float py = bounds.top  + (bounds.height - PANEL_H) / 2.f;
    m_inspectBg.setSize({ PANEL_W, PANEL_H });
    m_inspectBg.setPosition(px, py);
    m_inspectBg.setFillColor(sf::Color(24, 24, 38));
    m_inspectBg.setOutlineThickness(2.f);
    m_inspectBg.setOutlineColor(sf::Color(80, 80, 130));

    m_sellButton .setPosition({ px + 10.f,            py + PANEL_H - 52.f });
    m_closeButton.setPosition({ px + PANEL_W - 120.f, py + PANEL_H - 52.f });

    m_inspectImageBox.setSize({ PANEL_W - 40.f, 130.f });
    m_inspectImageBox.setPosition(px + 20.f, py + 20.f);
    m_inspectImageBox.setFillColor(sf::Color(30, 30, 48));
    m_inspectImageBox.setOutlineThickness(1.f);
    m_inspectImageBox.setOutlineColor(sf::Color(60, 60, 100));

    clampScroll();
}

// ── colCount ──────────────────────────────────────────────────────────────────
int InventoryUI::colCount() const
{
    return std::max(1, static_cast<int>(
        (m_gridArea.width + CARD_GAP) / (CARD_W + CARD_GAP)));
}

// ── cardRect ──────────────────────────────────────────────────────────────────
sf::FloatRect InventoryUI::cardRect(int col, int row) const
{
    const float x = m_gridArea.left + 8.f + col * (CARD_W + CARD_GAP);
    const float y = m_gridArea.top  + 8.f + row * (CARD_H + CARD_GAP) - m_scrollY;
    return { x, y, CARD_W, CARD_H };
}

// ── maxScrollY ────────────────────────────────────────────────────────────────
float InventoryUI::maxScrollY() const
{
    const int cols  = colCount();
    const int items = static_cast<int>(m_inventory.count());
    if (items == 0) return 0.f;
    const int rows    = (items + cols - 1) / cols;
    const float totalH = rows * (CARD_H + CARD_GAP) + 16.f;
    return std::max(0.f, totalH - m_gridArea.height);
}

// ── clampScroll ───────────────────────────────────────────────────────────────
void InventoryUI::clampScroll()
{
    m_scrollY = std::clamp(m_scrollY, 0.f, maxScrollY());
}

// ── openInspect ───────────────────────────────────────────────────────────────
void InventoryUI::openInspect(std::size_t viewIndex)
{
    m_inspectIndex = viewIndex;

    auto view = m_inventory.sortedView();
    if (viewIndex < view.size())
    {
        const sf::Color rc = view[viewIndex]->rarityColor();
        m_inspectBg.setOutlineColor(rc);
        m_inspectImageBox.setOutlineColor(sf::Color(rc.r, rc.g, rc.b, 140));
    }
}

// ── closeInspect ──────────────────────────────────────────────────────────────
void InventoryUI::closeInspect() { m_inspectIndex.reset(); }

// ── handleEvent ───────────────────────────────────────────────────────────────
void InventoryUI::handleEvent(const sf::Event& event, const sf::RenderWindow& window)
{
    const sf::Vector2i mouse = sf::Mouse::getPosition(window);

    m_sortButton.handleEvent(event, window);

    if (m_inspectIndex)
    {
        m_sellButton .handleEvent(event, window);
        m_closeButton.handleEvent(event, window);
        return;
    }

    // FIX: increased impulse from 40 → 120 so scroll feels responsive
    if (event.type == sf::Event::MouseWheelScrolled)
    {
        const sf::Vector2f mf { static_cast<float>(mouse.x),
                                static_cast<float>(mouse.y) };
        if (m_gridArea.contains(mf))
            m_scrollVel -= event.mouseWheelScroll.delta * 120.f;
    }

    if (event.type == sf::Event::MouseButtonReleased &&
        event.mouseButton.button == sf::Mouse::Left)
    {
        const sf::Vector2f mf { static_cast<float>(mouse.x),
                                static_cast<float>(mouse.y) };
        if (!m_gridArea.contains(mf)) return;

        auto view = m_inventory.sortedView();
        const int cols = colCount();
        for (std::size_t i = 0; i < view.size(); ++i)
        {
            const int col = static_cast<int>(i) % cols;
            const int row = static_cast<int>(i) / cols;
            if (cardRect(col, row).contains(mf))
            {
                openInspect(i);
                return;
            }
        }
    }
}

// ── update ────────────────────────────────────────────────────────────────────
void InventoryUI::update(float dt)
{
    m_sortButton.update(sf::Mouse::getPosition());

    if (m_inspectIndex)
    {
        m_sellButton .update(sf::Mouse::getPosition());
        m_closeButton.update(sf::Mouse::getPosition());
    }

    // FIX: removed * 2.5f multiplier (vel is already in px/s),
    //      tightened friction 0.85 → 0.78 for snappier feel
    if (std::abs(m_scrollVel) > 0.5f)
    {
        m_scrollY   += m_scrollVel * dt;
        m_scrollVel *= std::pow(0.78f, dt * 60.f);
        clampScroll();
    }
    else
    {
        m_scrollVel = 0.f;
    }
}

// ── render ────────────────────────────────────────────────────────────────────
void InventoryUI::render(sf::RenderWindow& window)
{
    drawHeader(window);
    drawGrid(window);
    if (m_inspectIndex) drawInspectPanel(window);
}

// ── drawHeader ────────────────────────────────────────────────────────────────
void InventoryUI::drawHeader(sf::RenderWindow& window)
{
    window.draw(m_headerBg);
    window.draw(m_sortButton);

    char buf[64];
    std::snprintf(buf, sizeof(buf), "Total: $%.2f  (%zu items)",
                  m_inventory.totalValue(), m_inventory.count());

    sf::Text valText;
    valText.setFont(*m_font);
    valText.setCharacterSize(13);
    valText.setFillColor(sf::Color(160, 210, 160));
    valText.setString(buf);

    const sf::FloatRect tb = valText.getLocalBounds();
    valText.setOrigin(tb.left + tb.width, tb.top);
    valText.setPosition(m_bounds.left + m_bounds.width - 10.f,
                        m_bounds.top + (HEADER_H - tb.height) / 2.f);
    window.draw(valText);
}

// ── drawGrid ──────────────────────────────────────────────────────────────────
void InventoryUI::drawGrid(sf::RenderWindow& window)
{
    window.draw(m_gridBg);

    if (m_inventory.empty())
    {
        drawEmptyState(window);
        return;
    }

    const sf::View origView = window.getView();
    {
        const sf::Vector2u ws = window.getSize();
        sf::View cv = origView;
        cv.setViewport({
            m_gridArea.left / ws.x,
            m_gridArea.top  / ws.y,
            m_gridArea.width  / ws.x,
            m_gridArea.height / ws.y
        });
        cv.setCenter(m_gridArea.left + m_gridArea.width  / 2.f,
                     m_gridArea.top  + m_gridArea.height / 2.f);
        cv.setSize(m_gridArea.width, m_gridArea.height);
        window.setView(cv);
    }

    auto view = m_inventory.sortedView();
    const int cols = colCount();

    for (std::size_t i = 0; i < view.size(); ++i)
    {
        const int col = static_cast<int>(i) % cols;
        const int row = static_cast<int>(i) / cols;
        const sf::FloatRect cr = cardRect(col, row);

        if (cr.top + cr.height < m_gridArea.top) continue;
        if (cr.top > m_gridArea.top + m_gridArea.height) break;

        const bool selected = m_inspectIndex && *m_inspectIndex == i;
        drawCard(window, *view[i], cr, selected);
    }

    window.setView(origView);

    // Scrollbar
    const float maxS = maxScrollY();
    if (maxS > 0.f)
    {
        const float barH = m_gridArea.height * (m_gridArea.height / (m_gridArea.height + maxS));
        const float barY = m_gridArea.top + (m_scrollY / maxS) * (m_gridArea.height - barH);
        const float barX = m_bounds.left + m_bounds.width - 6.f;

        sf::RectangleShape bar({ 4.f, barH });
        bar.setFillColor(sf::Color(80, 80, 120, 160));
        bar.setPosition(barX, barY);
        window.draw(bar);
    }

    // Footer hint
    sf::Text hint;
    hint.setFont(*m_font);
    hint.setCharacterSize(11);
    hint.setFillColor(sf::Color(80, 80, 110));
    hint.setString("Scroll to browse  |  Click item to inspect");
    hint.setPosition(m_bounds.left + 8.f,
                     m_bounds.top + m_bounds.height - FOOTER_H + 6.f);
    window.draw(hint);
}

// ── drawCard ──────────────────────────────────────────────────────────────────
void InventoryUI::drawCard(sf::RenderWindow& window,
                            const Item& item,
                            sf::FloatRect cr,
                            bool selected)
{
    const sf::Color rc = item.rarityColor();

    m_cardShape.setSize({ cr.width, cr.height });
    m_cardShape.setPosition(cr.left, cr.top);
    m_cardShape.setFillColor(sf::Color(22, 22, 36));
    m_cardShape.setOutlineThickness(selected ? 2.f : 1.f);
    m_cardShape.setOutlineColor(selected
        ? sf::Color(220, 180, 60)
        : sf::Color(rc.r, rc.g, rc.b, 90));
    window.draw(m_cardShape);

    sf::RectangleShape bar({ cr.width, 3.f });
    bar.setFillColor(rc);
    bar.setPosition(cr.left, cr.top + cr.height - 3.f);
    window.draw(bar);

    if (const_cast<Item&>(item).ensureTextureLoaded() && item.hasTexture())
    {
        sf::Sprite spr = item.makeSprite();
        const sf::FloatRect ib = spr.getLocalBounds();
        const float maxD = std::max(ib.width, ib.height);
        if (maxD > 0.f) spr.setScale((cr.width - 10.f) / maxD,
                                      (cr.width - 10.f) / maxD);
        spr.setPosition(cr.left + cr.width / 2.f,
                        cr.top  + (cr.height - 14.f) / 2.f);
        window.draw(spr);
    }
    else
    {
        sf::RectangleShape ph({ cr.width - 20.f, cr.height - 24.f });
        ph.setFillColor(sf::Color(rc.r, rc.g, rc.b, 30));
        ph.setPosition(cr.left + 10.f, cr.top + 4.f);
        window.draw(ph);
    }

    sf::Text lbl;
    lbl.setFont(*m_font);
    lbl.setCharacterSize(10);
    lbl.setFillColor(sf::Color(180, 180, 190));

    char buf[32];
    std::snprintf(buf, sizeof(buf), "%s  $%.0f",
                  wearTierAbbrev(item.wearTier()), item.value());
    lbl.setString(buf);

    const sf::FloatRect lb = lbl.getLocalBounds();
    lbl.setOrigin(lb.left + lb.width / 2.f, lb.top);
    lbl.setPosition(cr.left + cr.width / 2.f, cr.top + cr.height - 16.f);
    window.draw(lbl);
}

// ── drawInspectPanel ──────────────────────────────────────────────────────────
void InventoryUI::drawInspectPanel(sf::RenderWindow& window)
{
    if (!m_inspectIndex) return;
    auto view = m_inventory.sortedView();
    if (*m_inspectIndex >= view.size()) { m_inspectIndex.reset(); return; }

    const Item& item = *view[*m_inspectIndex];
    const sf::Color rc = item.rarityColor();

    sf::RectangleShape dim({ m_bounds.width, m_bounds.height });
    dim.setPosition(m_bounds.left, m_bounds.top);
    dim.setFillColor(sf::Color(0, 0, 0, 160));
    window.draw(dim);

    window.draw(m_inspectBg);
    window.draw(m_inspectImageBox);
    const sf::FloatRect ib = m_inspectImageBox.getGlobalBounds();

    if (const_cast<Item&>(item).ensureTextureLoaded() && item.hasTexture())
    {
        sf::Sprite spr = item.makeSprite();
        const sf::FloatRect sb = spr.getLocalBounds();
        const float maxD = std::max(sb.width, sb.height);
        if (maxD > 0.f)
        {
            const float scale = std::min((ib.width - 16.f) / maxD,
                                         (ib.height - 16.f) / maxD);
            spr.setScale(scale, scale);
        }
        spr.setPosition(ib.left + ib.width / 2.f, ib.top + ib.height / 2.f);
        window.draw(spr);
    }

    const sf::FloatRect pb = m_inspectBg.getGlobalBounds();
    float textY = pb.top + ib.height + 28.f;

    auto drawLine = [&](const std::string& str, unsigned int size,
                        sf::Color col, bool centred = true)
    {
        sf::Text t;
        t.setFont(*m_font);
        t.setCharacterSize(size);
        t.setFillColor(col);
        t.setString(str);
        const sf::FloatRect tb2 = t.getLocalBounds();
        if (centred)
        {
            t.setOrigin(tb2.left + tb2.width / 2.f, tb2.top);
            t.setPosition(pb.left + pb.width / 2.f, textY);
        }
        else
        {
            t.setOrigin(tb2.left, tb2.top);
            t.setPosition(pb.left + 16.f, textY);
        }
        window.draw(t);
        textY += tb2.height + 6.f;
    };

    drawLine(item.displayName(), 16, sf::Color(220, 220, 230));
    drawLine(std::string(item.rarityName()) + "  " + item.wearLabel(), 13, rc);
    drawLine(wearDisplayString(item.wear()), 12, sf::Color(140, 140, 160));

    char vbuf[32];
    std::snprintf(vbuf, sizeof(vbuf), "$%.2f", item.value());
    drawLine(vbuf, 20, sf::Color(160, 220, 160));

    window.draw(m_sellButton);
    window.draw(m_closeButton);
}

// ── drawEmptyState ────────────────────────────────────────────────────────────
void InventoryUI::drawEmptyState(sf::RenderWindow& window)
{
    sf::Text t;
    t.setFont(*m_font);
    t.setCharacterSize(18);
    t.setFillColor(sf::Color(60, 60, 90));
    t.setString("Your inventory is empty.\nOpen some cases!");

    const sf::FloatRect tb = t.getLocalBounds();
    t.setOrigin(tb.left + tb.width / 2.f, tb.top + tb.height / 2.f);
    t.setPosition(m_gridArea.left + m_gridArea.width  / 2.f,
                  m_gridArea.top  + m_gridArea.height / 2.f);
    window.draw(t);
}
