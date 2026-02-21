#include "TradeUpUI.hpp"
#include "../core/AudioManager.hpp"

#include <cstdio>
#include <cmath>
#include <algorithm>

// ── Constructor ───────────────────────────────────────────────────────────────
TradeUpUI::TradeUpUI(const sf::Font& font, Inventory& inventory,
                     sf::FloatRect bounds)
    : m_font(&font)
    , m_inventory(inventory)
{
    m_slots.resize(SLOTS, std::nullopt);

    m_tradeButton = Button(font, "TRADE UP", {0.f,0.f}, {160.f, 44.f}, 18);
    m_tradeButton.setIdleColor ({ 60, 40, 15 });
    m_tradeButton.setHoverColor({ 100, 70, 20 });
    m_tradeButton.setPressColor({ 40,  25, 10 });
    m_tradeButton.setOutlineColor(sf::Color(220, 160, 40), 2.f);
    m_tradeButton.setOnClick([this]() { doTradeUp(); });

    m_clearButton = Button(font, "Clear", {0.f,0.f}, {80.f, 36.f}, 14);
    m_clearButton.setIdleColor ({ 40, 20, 20 });
    m_clearButton.setHoverColor({ 70, 35, 35 });
    m_clearButton.setOutlineColor(sf::Color(160, 60, 60), 1.f);
    m_clearButton.setOnClick([this]()
    {
        for (auto& s : m_slots) s.reset();
        m_result.reset();
        m_state = State::Selecting;
        revalidate();
    });

    setBounds(bounds);
}

// ── setBounds ─────────────────────────────────────────────────────────────────
void TradeUpUI::setBounds(sf::FloatRect bounds)
{
    m_bounds = bounds;

    m_bgShape.setSize({ bounds.width, bounds.height });
    m_bgShape.setPosition(bounds.left, bounds.top);
    m_bgShape.setFillColor(sf::Color(14, 14, 24));

    // Slot area: upper portion
    m_slotArea  = { bounds.left, bounds.top, bounds.width, bounds.height * 0.52f };
    // Picker area: lower portion
    m_pickerArea = { bounds.left, bounds.top + bounds.height * 0.52f,
                     bounds.width, bounds.height * 0.48f };

    // Position buttons
    const float btnsY = m_slotArea.top + m_slotArea.height - 56.f;
    m_tradeButton.setPosition({ bounds.left + bounds.width - 180.f, btnsY });
    m_clearButton.setPosition({ bounds.left + bounds.width - 270.f, btnsY + 4.f });
}

// ── revalidate ────────────────────────────────────────────────────────────────
void TradeUpUI::revalidate()
{
    // Collect filled slots
    std::vector<std::size_t> filled;
    for (const auto& s : m_slots)
        if (s) filled.push_back(*s);

    if (static_cast<int>(filled.size()) < SLOTS)
    {
        m_validationOk  = false;
        const int needed = SLOTS - static_cast<int>(filled.size());
        m_validationMsg = "Select " + std::to_string(needed) + " more item(s).";
        return;
    }

    auto res = TradeUp::validate(m_inventory, filled);
    m_validationOk  = res.valid;
    m_validationMsg = res.valid
        ? (std::string(rarityName(res.inputRarity)) + " \xe2\x86\x92 "
           + rarityName(res.outputRarity))
        : res.reason;
}

// ── slot helpers ──────────────────────────────────────────────────────────────
bool TradeUpUI::isSlotFilled(int slot) const
{
    return slot < static_cast<int>(m_slots.size()) && m_slots[slot].has_value();
}

void TradeUpUI::removeFromSlot(int slot)
{
    if (slot < static_cast<int>(m_slots.size()))
        m_slots[slot].reset();
    revalidate();
}

bool TradeUpUI::isInSlots(std::size_t viewIdx) const
{
    for (const auto& s : m_slots)
        if (s && *s == viewIdx) return true;
    return false;
}

void TradeUpUI::addToNextSlot(std::size_t viewIdx)
{
    if (isInSlots(viewIdx)) return; // already selected
    for (auto& s : m_slots)
    {
        if (!s)
        {
            s = viewIdx;
            revalidate();
            return;
        }
    }
    // All slots full — replace last
    m_slots[SLOTS - 1] = viewIdx;
    revalidate();
}

// ── doTradeUp ─────────────────────────────────────────────────────────────────
void TradeUpUI::doTradeUp()
{
    if (!m_validationOk || m_state != State::Selecting) return;

    std::vector<std::size_t> filled;
    for (const auto& s : m_slots)
        if (s) filled.push_back(*s);

    // Sort descending so removal doesn't shift earlier indices
    auto result = TradeUp::execute(m_inventory, filled);
    if (!result.success)
    {
        m_validationMsg = result.errorMsg;
        return;
    }

    m_result = std::move(result.item);
    for (auto& s : m_slots) s.reset();

    m_state      = State::Animating;
    m_flashTimer = 0.f;

    AudioManager::instance().play(SoundID::ReelStart);
}

// ── handleEvent ───────────────────────────────────────────────────────────────
void TradeUpUI::handleEvent(const sf::Event& event, const sf::RenderWindow& window)
{
    m_tradeButton.handleEvent(event, window);
    m_clearButton.handleEvent(event, window);

    // Horizontal scroll in picker
    if (event.type == sf::Event::MouseWheelScrolled)
    {
        const sf::Vector2f mf {
            static_cast<float>(sf::Mouse::getPosition(window).x),
            static_cast<float>(sf::Mouse::getPosition(window).y)
        };
        if (m_pickerArea.contains(mf))
            m_scrollVel -= event.mouseWheelScroll.delta * 35.f;
    }

    // Click on input slot → remove item
    if (event.type == sf::Event::MouseButtonReleased &&
        event.mouseButton.button == sf::Mouse::Left)
    {
        const sf::Vector2f mf {
            static_cast<float>(event.mouseButton.x),
            static_cast<float>(event.mouseButton.y)
        };

        // Slot click — calculated same as in render
        const float totalSlotsW = SLOTS * SLOT_W + (SLOTS - 1) * 16.f;
        const float slotsStartX = m_slotArea.left + (m_slotArea.width * 0.35f - totalSlotsW) / 2.f;
        const float slotsY = m_slotArea.top + 60.f;

        for (int i = 0; i < SLOTS; ++i)
        {
            const sf::FloatRect sr {
                slotsStartX + i * (SLOT_W + 16.f), slotsY, SLOT_W, SLOT_H
            };
            if (sr.contains(mf) && isSlotFilled(i))
            {
                removeFromSlot(i);
                AudioManager::instance().play(SoundID::ButtonClick);
                return;
            }
        }

        // Picker click → add to slot
        if (m_pickerArea.contains(mf) && m_state == State::Selecting)
        {
            auto view = m_inventory.sortedView();
            float x = m_pickerArea.left + 8.f - m_scrollX;
            for (std::size_t i = 0; i < view.size(); ++i)
            {
                sf::FloatRect cr { x, m_pickerArea.top + 10.f, MINI_CARD_W, MINI_CARD_H };
                if (cr.contains(mf))
                {
                    addToNextSlot(i);
                    AudioManager::instance().play(SoundID::ButtonClick);
                    return;
                }
                x += MINI_CARD_W + MINI_GAP;
            }
        }
    }
}

// ── update ────────────────────────────────────────────────────────────────────
void TradeUpUI::update(float dt)
{
    m_tradeButton.update(sf::Mouse::getPosition());
    m_clearButton.update(sf::Mouse::getPosition());

    // Scroll momentum
    if (std::abs(m_scrollVel) > 0.5f)
    {
        m_scrollX   += m_scrollVel * dt * 2.5f;
        m_scrollVel *= std::pow(0.85f, dt * 60.f);

        // Clamp scroll
        const float maxScroll = std::max(0.f,
            static_cast<float>(m_inventory.count()) * (MINI_CARD_W + MINI_GAP)
            - m_pickerArea.width + 16.f);
        m_scrollX = std::clamp(m_scrollX, 0.f, maxScroll);
    }
    else
    {
        m_scrollVel = 0.f;
    }

    switch (m_state)
    {
        case State::Animating:
            m_flashTimer += dt;
            if (m_flashTimer >= FLASH_DURATION)
            {
                m_state       = State::ShowingResult;
                m_resultTimer = 0.f;
                if (m_result)
                    AudioManager::instance().playRevealForRarity(
                        static_cast<int>(m_result->rarity()));
            }
            break;

        case State::ShowingResult:
            m_resultTimer += dt;
            if (m_resultTimer >= RESULT_HOLD)
            {
                // Auto-add to inventory and return to selecting
                if (m_result)
                {
                    if (m_onComplete) m_onComplete(std::move(*m_result));
                    m_result.reset();
                }
                m_state = State::Selecting;
                revalidate();
            }
            break;

        default: break;
    }
}

// ── render ────────────────────────────────────────────────────────────────────
void TradeUpUI::render(sf::RenderWindow& window)
{
    window.draw(m_bgShape);

    // ── Title ─────────────────────────────────────────────────────────────────
    sf::Text title;
    title.setFont(*m_font);
    title.setCharacterSize(20);
    title.setFillColor(sf::Color(220, 180, 60));
    title.setStyle(sf::Text::Bold);
    title.setString("TRADE UP CONTRACT");
    title.setPosition(m_bounds.left + 16.f, m_bounds.top + 12.f);
    window.draw(title);

    sf::Text sub;
    sub.setFont(*m_font);
    sub.setCharacterSize(12);
    sub.setFillColor(sf::Color(110, 110, 140));
    sub.setString("Combine " + std::to_string(SLOTS)
                  + " items of the same rarity to get 1 of the next tier");
    sub.setPosition(m_bounds.left + 16.f, m_bounds.top + 38.f);
    window.draw(sub);

    // ── Flash animation overlay ────────────────────────────────────────────────
    if (m_state == State::Animating)
    {
        const float t = m_flashTimer / FLASH_DURATION;
        const float pulse = std::sin(t * 3.14159f * 4.f);
        const sf::Uint8 alpha = static_cast<sf::Uint8>(std::abs(pulse) * 180.f);
        sf::RectangleShape flash({ m_bounds.width, m_bounds.height });
        flash.setPosition(m_bounds.left, m_bounds.top);
        flash.setFillColor(sf::Color(220, 180, 60, alpha));
        window.draw(flash);
    }

    // ── Input slots ───────────────────────────────────────────────────────────
    const float totalSlotsW = SLOTS * SLOT_W + (SLOTS - 1) * 16.f;
    const float slotsStartX = m_slotArea.left + (m_slotArea.width * 0.35f - totalSlotsW) / 2.f;
    const float slotsY      = m_slotArea.top + 60.f;

    for (int i = 0; i < SLOTS; ++i)
        drawInputSlot(window, i, { slotsStartX + i * (SLOT_W + 16.f), slotsY });

    // ── Arrow → result ────────────────────────────────────────────────────────
    const float arrowFromX = slotsStartX + totalSlotsW + 16.f;
    const float arrowToX   = m_slotArea.left + m_slotArea.width * 0.35f
                           + (m_slotArea.width * 0.65f - RESULT_SLOT_W) / 2.f - 20.f;
    const float midY = slotsY + SLOT_H / 2.f;
    drawArrow(window, { arrowFromX, midY }, { arrowToX, midY });

    // ── Result slot ───────────────────────────────────────────────────────────
    const float resultX = arrowToX + 24.f;
    drawResultSlot(window, { resultX, slotsY - 8.f });

    // ── Validation message ────────────────────────────────────────────────────
    sf::Text vmsg;
    vmsg.setFont(*m_font);
    vmsg.setCharacterSize(13);
    vmsg.setFillColor(m_validationOk ? sf::Color(120, 200, 120)
                                     : sf::Color(180, 100, 100));
    vmsg.setString(m_validationMsg);
    vmsg.setPosition(m_bounds.left + 16.f, m_slotArea.top + m_slotArea.height - 48.f);
    window.draw(vmsg);

    // ── Buttons ───────────────────────────────────────────────────────────────
    if (m_state == State::Selecting)
    {
        // Dim trade button when not valid
        if (!m_validationOk)
        {
            m_tradeButton.setIdleColor ({ 35, 25, 10 });
            m_tradeButton.setOutlineColor(sf::Color(100, 80, 20, 120), 1.f);
        }
        else
        {
            m_tradeButton.setIdleColor ({ 60, 40, 15 });
            m_tradeButton.setOutlineColor(sf::Color(220, 160, 40), 2.f);
        }
        window.draw(m_tradeButton);
        window.draw(m_clearButton);
    }

    // ── Inventory picker ──────────────────────────────────────────────────────
    drawPickerArea(window);
}

// ── drawInputSlot ─────────────────────────────────────────────────────────────
void TradeUpUI::drawInputSlot(sf::RenderWindow& window, int slot, sf::Vector2f pos)
{
    m_slotShape.setSize({ SLOT_W, SLOT_H });
    m_slotShape.setPosition(pos);

    const bool filled = isSlotFilled(slot);

    if (filled)
    {
        auto view = m_inventory.sortedView();
        const std::size_t idx = *m_slots[slot];
        if (idx < view.size())
        {
            const Item& item = *view[idx];
            const sf::Color rc = item.rarityColor();
            m_slotShape.setFillColor(sf::Color(rc.r / 5, rc.g / 5, rc.b / 5));
            m_slotShape.setOutlineThickness(2.f);
            m_slotShape.setOutlineColor(rc);
            window.draw(m_slotShape);

            // Item image / coloured block
            sf::RectangleShape imgBox({ SLOT_W - 16.f, SLOT_H - 28.f });
            imgBox.setFillColor(sf::Color(rc.r, rc.g, rc.b, 35));
            imgBox.setPosition(pos.x + 8.f, pos.y + 4.f);
            window.draw(imgBox);

            // Wear + rarity label
            sf::Text lbl;
            lbl.setFont(*m_font);
            lbl.setCharacterSize(10);
            lbl.setFillColor(sf::Color(180, 180, 190));
            char buf[24];
            std::snprintf(buf, sizeof(buf), "%s %s",
                          rarityName(item.rarity()), wearTierAbbrev(item.wearTier()));
            lbl.setString(buf);
            sf::FloatRect lb = lbl.getLocalBounds();
            lbl.setOrigin(lb.left + lb.width / 2.f, lb.top);
            lbl.setPosition(pos.x + SLOT_W / 2.f, pos.y + SLOT_H - 18.f);
            window.draw(lbl);

            // ✕ hint
            sf::Text x;
            x.setFont(*m_font);
            x.setCharacterSize(10);
            x.setFillColor(sf::Color(180, 60, 60, 160));
            x.setString("click to remove");
            sf::FloatRect xb = x.getLocalBounds();
            x.setOrigin(xb.left + xb.width / 2.f, xb.top);
            x.setPosition(pos.x + SLOT_W / 2.f, pos.y + SLOT_H - 7.f);
            window.draw(x);
            return;
        }
    }

    // Empty slot
    m_slotShape.setFillColor(sf::Color(20, 20, 35));
    m_slotShape.setOutlineThickness(1.f);
    m_slotShape.setOutlineColor(sf::Color(55, 55, 85));
    window.draw(m_slotShape);

    sf::Text empty;
    empty.setFont(*m_font);
    empty.setCharacterSize(11);
    empty.setFillColor(sf::Color(60, 60, 90));
    empty.setString("+ item");
    sf::FloatRect eb = empty.getLocalBounds();
    empty.setOrigin(eb.left + eb.width / 2.f, eb.top + eb.height / 2.f);
    empty.setPosition(pos.x + SLOT_W / 2.f, pos.y + SLOT_H / 2.f);
    window.draw(empty);
}

// ── drawResultSlot ────────────────────────────────────────────────────────────
void TradeUpUI::drawResultSlot(sf::RenderWindow& window, sf::Vector2f pos)
{
    sf::RectangleShape rs({ RESULT_SLOT_W, RESULT_SLOT_H });
    rs.setPosition(pos);

    if (m_state == State::ShowingResult && m_result)
    {
        const sf::Color rc = m_result->rarityColor();
        // Pulsing glow
        const float pulse = (std::sin(m_resultTimer * 4.f) + 1.f) / 2.f;
        const sf::Uint8 glowA = static_cast<sf::Uint8>(150.f + 100.f * pulse);
        rs.setFillColor(sf::Color(rc.r / 4, rc.g / 4, rc.b / 4));
        rs.setOutlineThickness(3.f);
        rs.setOutlineColor(sf::Color(rc.r, rc.g, rc.b, glowA));
        window.draw(rs);

        // Rarity colour block
        sf::RectangleShape imgBox({ RESULT_SLOT_W - 16.f, RESULT_SLOT_H - 30.f });
        imgBox.setFillColor(sf::Color(rc.r, rc.g, rc.b, 50));
        imgBox.setPosition(pos.x + 8.f, pos.y + 4.f);
        window.draw(imgBox);

        // Labels
        sf::Text rname;
        rname.setFont(*m_font);
        rname.setCharacterSize(11);
        rname.setFillColor(rc);
        rname.setString(std::string(m_result->rarityName()) + " "
                        + wearTierAbbrev(m_result->wearTier()));
        sf::FloatRect rb = rname.getLocalBounds();
        rname.setOrigin(rb.left + rb.width / 2.f, rb.top);
        rname.setPosition(pos.x + RESULT_SLOT_W / 2.f,
                          pos.y + RESULT_SLOT_H - 22.f);
        window.draw(rname);

        char vbuf[24];
        std::snprintf(vbuf, sizeof(vbuf), "$%.0f", m_result->value());
        sf::Text val;
        val.setFont(*m_font);
        val.setCharacterSize(13);
        val.setFillColor(sf::Color(160, 220, 160));
        val.setString(vbuf);
        sf::FloatRect vb = val.getLocalBounds();
        val.setOrigin(vb.left + vb.width / 2.f, vb.top);
        val.setPosition(pos.x + RESULT_SLOT_W / 2.f,
                        pos.y + RESULT_SLOT_H - 8.f);
        window.draw(val);
    }
    else if (m_state == State::Animating)
    {
        // Spinning question mark
        const float t = m_flashTimer / FLASH_DURATION;
        const sf::Uint8 a = static_cast<sf::Uint8>(100.f + 155.f * t);
        rs.setFillColor(sf::Color(30, 30, 50));
        rs.setOutlineThickness(2.f);
        rs.setOutlineColor(sf::Color(220, 180, 60, a));
        window.draw(rs);

        sf::Text q;
        q.setFont(*m_font);
        q.setCharacterSize(36);
        q.setFillColor(sf::Color(220, 180, 60, a));
        q.setString("?");
        sf::FloatRect qb = q.getLocalBounds();
        q.setOrigin(qb.left + qb.width / 2.f, qb.top + qb.height / 2.f);
        q.setPosition(pos.x + RESULT_SLOT_W / 2.f, pos.y + RESULT_SLOT_H / 2.f);
        window.draw(q);
    }
    else
    {
        // Unknown
        rs.setFillColor(sf::Color(18, 18, 30));
        rs.setOutlineThickness(1.f);
        rs.setOutlineColor(sf::Color(50, 50, 80));
        window.draw(rs);

        sf::Text q;
        q.setFont(*m_font);
        q.setCharacterSize(28);
        q.setFillColor(sf::Color(45, 45, 70));
        q.setString("?");
        sf::FloatRect qb = q.getLocalBounds();
        q.setOrigin(qb.left + qb.width / 2.f, qb.top + qb.height / 2.f);
        q.setPosition(pos.x + RESULT_SLOT_W / 2.f, pos.y + RESULT_SLOT_H / 2.f);
        window.draw(q);
    }
}

// ── drawArrow ─────────────────────────────────────────────────────────────────
void TradeUpUI::drawArrow(sf::RenderWindow& window, sf::Vector2f from, sf::Vector2f to)
{
    const float len = to.x - from.x;
    sf::RectangleShape line({ len - 10.f, 3.f });
    line.setFillColor(sf::Color(120, 120, 160));
    line.setPosition(from.x, from.y - 1.5f);
    window.draw(line);

    // Arrowhead (simple triangle)
    sf::ConvexShape head;
    head.setPointCount(3);
    head.setPoint(0, { 0.f, -6.f });
    head.setPoint(1, { 0.f,  6.f });
    head.setPoint(2, { 10.f, 0.f });
    head.setFillColor(sf::Color(120, 120, 160));
    head.setPosition(to.x - 10.f, to.y);
    window.draw(head);
}

// ── drawPickerArea ────────────────────────────────────────────────────────────
void TradeUpUI::drawPickerArea(sf::RenderWindow& window)
{
    sf::RectangleShape pickerBg({ m_pickerArea.width, m_pickerArea.height });
    pickerBg.setPosition(m_pickerArea.left, m_pickerArea.top);
    pickerBg.setFillColor(sf::Color(12, 12, 20));
    window.draw(pickerBg);

    sf::Text hdr;
    hdr.setFont(*m_font);
    hdr.setCharacterSize(12);
    hdr.setFillColor(sf::Color(90, 90, 120));
    hdr.setString("Click item to add to slot  |  scroll to browse");
    hdr.setPosition(m_pickerArea.left + 8.f, m_pickerArea.top + 4.f);
    window.draw(hdr);

    // Clip picker
    const sf::View origView = window.getView();
    {
        const sf::Vector2u ws = window.getSize();
        sf::View cv = origView;
        const float clipY  = m_pickerArea.top + 24.f;
        const float clipH  = m_pickerArea.height - 24.f;
        cv.setViewport({
            m_pickerArea.left / ws.x,
            clipY / ws.y,
            m_pickerArea.width / ws.x,
            clipH / ws.y
        });
        cv.setCenter(m_pickerArea.left + m_pickerArea.width / 2.f,
                     clipY + clipH / 2.f);
        cv.setSize(m_pickerArea.width, clipH);
        window.setView(cv);
    }

    auto view = m_inventory.sortedView();
    float x = m_pickerArea.left + 8.f - m_scrollX;
    const float y = m_pickerArea.top + 28.f;

    for (std::size_t i = 0; i < view.size(); ++i)
    {
        if (x + MINI_CARD_W < m_pickerArea.left) { x += MINI_CARD_W + MINI_GAP; continue; }
        if (x > m_pickerArea.left + m_pickerArea.width) break;

        sf::FloatRect cr { x, y, MINI_CARD_W, MINI_CARD_H };
        const bool selected = isInSlots(i);
        drawMiniCard(window, *view[i], cr, selected);
        x += MINI_CARD_W + MINI_GAP;
    }

    window.setView(origView);

    if (m_inventory.empty())
    {
        sf::Text empty;
        empty.setFont(*m_font);
        empty.setCharacterSize(14);
        empty.setFillColor(sf::Color(50, 50, 75));
        empty.setString("No items in inventory yet.");
        sf::FloatRect eb = empty.getLocalBounds();
        empty.setOrigin(eb.left + eb.width / 2.f, eb.top + eb.height / 2.f);
        empty.setPosition(m_pickerArea.left + m_pickerArea.width / 2.f,
                          m_pickerArea.top + m_pickerArea.height / 2.f);
        window.draw(empty);
    }
}

// ── drawMiniCard ──────────────────────────────────────────────────────────────
void TradeUpUI::drawMiniCard(sf::RenderWindow& window,
                              const Item& item,
                              sf::FloatRect rect,
                              bool selected)
{
    const sf::Color rc = item.rarityColor();

    sf::RectangleShape card({ rect.width, rect.height });
    card.setPosition(rect.left, rect.top);
    card.setFillColor(sf::Color(20, 20, 34));
    card.setOutlineThickness(selected ? 2.f : 1.f);
    card.setOutlineColor(selected ? sf::Color(220, 180, 60)
                                  : sf::Color(rc.r, rc.g, rc.b, 80));
    window.draw(card);

    // Rarity bar
    sf::RectangleShape bar({ rect.width, 3.f });
    bar.setFillColor(rc);
    bar.setPosition(rect.left, rect.top + rect.height - 3.f);
    window.draw(bar);

    // Selected indicator
    if (selected)
    {
        sf::Text tick;
        tick.setFont(*m_font);
        tick.setCharacterSize(9);
        tick.setFillColor(sf::Color(220, 180, 60));
        tick.setString("SELECTED");
        sf::FloatRect tb = tick.getLocalBounds();
        tick.setOrigin(tb.left + tb.width / 2.f, tb.top);
        tick.setPosition(rect.left + rect.width / 2.f, rect.top + 2.f);
        window.draw(tick);
    }

    // Value
    sf::Text val;
    val.setFont(*m_font);
    val.setCharacterSize(10);
    val.setFillColor(sf::Color(140, 200, 140));
    char buf[16]; std::snprintf(buf, sizeof(buf), "$%.0f", item.value());
    val.setString(buf);
    sf::FloatRect vb = val.getLocalBounds();
    val.setOrigin(vb.left + vb.width / 2.f, vb.top);
    val.setPosition(rect.left + rect.width / 2.f, rect.top + rect.height - 18.f);
    window.draw(val);
}
