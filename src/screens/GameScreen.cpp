#include "GameScreen.hpp"
#include "../Game.hpp"
#include "../items/ItemRegistry.hpp"
#include "../core/AudioManager.hpp"

#include <cstdio>
#include <cmath>
#include <algorithm>

// ═══════════════════════════════════════════════════════════════════════════
// Constructor / Destructor
// ═══════════════════════════════════════════════════════════════════════════
GameScreen::GameScreen(Game& game)
    : m_reelManager(game.font())
    , m_caseInfoPanel(game.font())
    , m_freeCaseReward(30.f * 60.f)
    , m_save("save.json")
{
    buildHeaderBar(game);
    buildTabBar(game);
    buildCaseSelector(game);
    buildOpenCountButtons(game);
    buildActionButtons(game);

    const float W = game.width();
    const float H = game.height();

    m_inventoryUI = std::make_unique<InventoryUI>(
        sf::FloatRect(0.f, CONTENT_Y, W, H - CONTENT_Y),
        game.font(), m_inventory);

    m_inventoryUI->setOnSell([&game](float val)
    {
        game.addBalance(val);
        AudioManager::instance().play(SoundID::ItemSell);
    });

    m_tradeUpUI = std::make_unique<TradeUpUI>(
        game.font(), m_inventory,
        sf::FloatRect(0.f, CONTENT_Y, W, H - CONTENT_Y));

    m_tradeUpUI->setOnTradeComplete([this, &game](Item item)
    {
        m_inventory.addItem(item);
        const int rar = static_cast<int>(item.rarity());
        AudioManager::instance().playRevealForRarity(rar);

        // FIX: Sparkles spawn at screen centre of trade-up panel, not window centre
        if (rar >= static_cast<int>(Rarity::Classified))
        {
            const int   count  = (rar >= 6) ? 80 : 50;
            const float spread = (rar >= 6) ? 250.f : 160.f;
            m_sparkles.emplace_back(
                sf::Vector2f(game.width() / 2.f, game.height() / 2.f),
                item.rarityColor(), count, spread);
        }
        char buf[64];
        std::snprintf(buf, sizeof(buf), "Trade up! Got: %s", item.displayName().c_str());
        showToast(buf, item.rarityColor());
        saveProgress(game);
    });

    // Load save
    {
        JsonSave::SaveData sd;
        m_save.load(sd, m_inventory);
        game.setBalance(sd.balance);
        m_freeCaseReward.loadCooldownEnd(sd.freeCaseCooldownEnd);
    }

    // Free case button
    m_freeCaseButton = FreeCaseButton(game.font(), m_freeCaseReward,
                                      { W - FreeCaseButton::SIZE - 16.f,
                                        H - FreeCaseButton::SIZE - 12.f });
    m_freeCaseButton.setOnClaim([this, &game]() { claimFreeCase(game); });

    m_balanceText.setFont(game.font());
    m_balanceText.setCharacterSize(14);
    m_balanceText.setFillColor(sf::Color(160, 220, 160));

    // FIX: pass window size to prepare() via the callback so layout is correct
    m_reelManager.setOnAllComplete([this, &game](std::vector<Item> items)
    {
        int maxRar = 0;
        for (const auto& i : items)
            maxRar = std::max(maxRar, static_cast<int>(i.rarity()));

        AudioManager::instance().playRevealForRarity(maxRar);

        // FIX: sparkles spawn at each reel's individual winner card position
        if (maxRar >= static_cast<int>(Rarity::Classified))
        {
            for (std::size_t i = 0; i < items.size(); ++i)
            {
                const auto& item = items[i];
                if (static_cast<int>(item.rarity()) < static_cast<int>(Rarity::Classified))
                    continue;

                // Get the screen position of this specific reel's winner card
                sf::Vector2f origin = m_reelManager.winnerCentre(i);
                if (origin.x < 0.f)
                    origin = { game.width() / 2.f, game.height() / 2.f };

                const int   count  = (static_cast<int>(item.rarity()) >= 6) ? 80 : 55;
                const float spread = (static_cast<int>(item.rarity()) >= 6) ? 240.f : 160.f;
                m_sparkles.emplace_back(origin, item.rarityColor(), count, spread);
            }
        }

        onAllReelsDone(std::move(items), game);
    });

    selectCase(0);
}

GameScreen::~GameScreen() {}

// ═══════════════════════════════════════════════════════════════════════════
// Build helpers
// ═══════════════════════════════════════════════════════════════════════════
void GameScreen::buildHeaderBar(Game& game)
{
    const float W = game.width();
    const sf::Font& font = game.font();

    m_headerBar.setSize({ W, HEADER_H });
    m_headerBar.setPosition(0.f, 0.f);
    m_headerBar.setFillColor(sf::Color(16, 16, 28));

    m_headerDivider.setSize({ W, 2.f });
    m_headerDivider.setPosition(0.f, HEADER_H);
    m_headerDivider.setFillColor(sf::Color(45, 45, 75));

    m_headerLabel.setFont(font);
    m_headerLabel.setString("CASE OPENER");
    m_headerLabel.setCharacterSize(28);
    m_headerLabel.setFillColor(sf::Color(220, 180, 60));
    m_headerLabel.setStyle(sf::Text::Bold);
    {
        sf::FloatRect tb = m_headerLabel.getLocalBounds();
        m_headerLabel.setOrigin(tb.left, tb.top + tb.height / 2.f);
        m_headerLabel.setPosition(20.f, HEADER_H / 2.f);
    }

    m_backButton = Button(font, "< MENU", { W - 148.f, 10.f }, { 128.f, 44.f }, 15);
    m_backButton.setIdleColor ({ 26, 26, 44 });
    m_backButton.setHoverColor({ 48, 48, 78 });
    m_backButton.setOutlineColor(sf::Color(75, 75, 125), 1.f);
    m_backButton.setOnClick([&game, this]()
    {
        saveProgress(game);
        game.switchScreen(ScreenID::Menu);
    });

    m_muteButton = Button(font, "[M]", { W - 190.f, 14.f }, { 36.f, 36.f }, 13);
    m_muteButton.setIdleColor ({ 26, 26, 44 });
    m_muteButton.setHoverColor({ 48, 48, 78 });
    m_muteButton.setOutlineColor(sf::Color(70, 70, 110), 1.f);
    m_muteButton.setOnClick([]() { AudioManager::instance().toggleMute(); });

    m_saveButton = Button(font, "Save", { W - 240.f, 14.f }, { 44.f, 36.f }, 12);
    m_saveButton.setIdleColor ({ 22, 40, 22 });
    m_saveButton.setHoverColor({ 36, 66, 36 });
    m_saveButton.setOutlineColor(sf::Color(55, 120, 55), 1.f);
    m_saveButton.setOnClick([&game, this]()
    {
        saveProgress(game);
        showToast("Saved!", sf::Color(120, 220, 120));
    });
}

void GameScreen::buildTabBar(Game& game)
{
    const sf::Font& font = game.font();
    const float tabW = 140.f;
    const float tabH = TABBAR_H - 2.f;

    m_tabOpen      = Button(font, "OPEN",      { 0.f,          HEADER_H + 1.f }, { tabW, tabH }, 13);
    m_tabInventory = Button(font, "INVENTORY", { tabW + 2.f,   HEADER_H + 1.f }, { tabW, tabH }, 13);
    m_tabTradeUp   = Button(font, "TRADE UP",  { tabW*2.f+4.f, HEADER_H + 1.f }, { tabW, tabH }, 13);

    for (auto* btn : { &m_tabOpen, &m_tabInventory, &m_tabTradeUp })
    {
        btn->setIdleColor ({ 20, 20, 36 });
        btn->setHoverColor({ 38, 38, 65 });
    }

    m_tabOpen.setOnClick([this](){ m_activeTab = GameTab::Open; });
    m_tabInventory.setOnClick([this](){ m_activeTab = GameTab::Inventory; });
    m_tabTradeUp.setOnClick([this](){ m_activeTab = GameTab::TradeUp; });
}

void GameScreen::buildCaseSelector(Game& game)
{
    const sf::Font& font  = game.font();
    const auto& cases     = CaseRegistry::instance().sortedByPrice();
    const float W         = game.width();
    const float btnW      = 128.f;
    const float infoW     = 22.f;
    const float btnH      = 52.f;
    const float gap       = 8.f;
    const float groupW    = btnW + infoW + 2.f;
    const float totalW    = static_cast<float>(cases.size()) * (groupW + gap) - gap;
    float x               = (W - totalW) / 2.f;

    m_caseSelectorBg.setSize({ W, btnH + 18.f });
    m_caseSelectorBg.setPosition(0.f, CONTENT_Y);
    m_caseSelectorBg.setFillColor(sf::Color(16, 16, 28));

    m_caseButtons.clear();
    m_infoButtons.clear();
    m_caseHovers.clear();

    for (std::size_t i = 0; i < cases.size(); ++i)
    {
        const Case* c = cases[i];
        char lbl[64];
        std::snprintf(lbl, sizeof(lbl), "%s\n$%.2f", c->displayName().c_str(), c->price());

        Button btn(font, lbl, { x, CONTENT_Y + 7.f }, { btnW, btnH }, 11);
        btn.setIdleColor ({ 24, 24, 42 });
        btn.setHoverColor({ 44, 44, 72 });
        btn.setOutlineColor(sf::Color(65, 65, 105), 1.f);
        const int ci = static_cast<int>(i);
        btn.setOnClick([this, ci]()
        {
            selectCase(ci);
            AudioManager::instance().play(SoundID::ButtonClick);
        });
        m_caseButtons.push_back(std::move(btn));

        Button info(font, "?", { x + btnW + 2.f, CONTENT_Y + 7.f }, { infoW, btnH }, 11);
        info.setIdleColor ({ 18, 18, 36 });
        info.setHoverColor({ 40, 40, 70 });
        info.setOutlineColor(sf::Color(70, 70, 120), 1.f);
        const Case* captCase = c;
        info.setOnClick([this, captCase]()
        {
            m_caseInfoPanel.open(*captCase);
            AudioManager::instance().play(SoundID::ButtonClick);
        });
        m_infoButtons.push_back(std::move(info));
        m_caseHovers.emplace_back(6.f);
        x += groupW + gap;
    }
}

void GameScreen::buildOpenCountButtons(Game& game)
{
    const sf::Font& font = game.font();
    const float W    = game.width();
    const float rowY = CONTENT_Y + 78.f;
    const float btnW = 74.f, btnH = 30.f, gap = 7.f;

    const std::pair<const char*, OpenCount> opts[] = {
        {"x1",OpenCount::One},{"x3",OpenCount::Three},
        {"x5",OpenCount::Five},{"x10",OpenCount::Ten}
    };
    const float totalW = 4*btnW + 3*gap;
    float x = (W - totalW) / 2.f;

    m_countButtons.clear();
    for (auto& [lbl, cnt] : opts)
    {
        Button btn(font, lbl, {x, rowY}, {btnW, btnH}, 14);
        btn.setIdleColor ({ 24, 24, 44 });
        btn.setHoverColor({ 46, 46, 82 });
        btn.setOutlineColor(sf::Color(60, 60, 110), 1.f);
        const OpenCount cc = cnt;
        btn.setOnClick([this, cc]()
        {
            m_openCount = cc;
            AudioManager::instance().play(SoundID::ButtonClick);
        });
        m_countButtons.push_back(std::move(btn));
        x += btnW + gap;
    }
}

void GameScreen::buildActionButtons(Game& game)
{
    const sf::Font& font = game.font();
    const float W    = game.width();
    const float H    = game.height();
    const float btnW = 155.f, btnH = 44.f;
    const float botY = H - btnH - 14.f;

    m_openButton = Button(font, "OPEN CASE", {W/2.f - btnW - 8.f, botY}, {btnW, btnH}, 17);
    m_openButton.setIdleColor ({ 30, 75, 45 });
    m_openButton.setHoverColor({ 45,115, 65 });
    m_openButton.setPressColor({ 18, 50, 28 });
    m_openButton.setOutlineColor(sf::Color(75, 195, 105), 2.f);
    m_openButton.setPulse(true);
    m_openButton.setOnClick([&game, this]() { doOpen(game); });

    m_skipButton = Button(font, "SKIP", {W/2.f + 8.f, botY}, {btnW, btnH}, 17);
    m_skipButton.setIdleColor ({ 46, 46, 24 });
    m_skipButton.setHoverColor({ 76, 76, 38 });
    m_skipButton.setOutlineColor(sf::Color(160, 160, 55), 1.5f);
    m_skipButton.setOnClick([this]()
    {
        doSkip();
        AudioManager::instance().play(SoundID::ButtonClick);
    });

    m_collectButton = Button(font, "COLLECT ALL", {W/2.f - btnW/2.f, botY}, {btnW, btnH}, 17);
    m_collectButton.setIdleColor ({ 28, 55, 85 });
    m_collectButton.setHoverColor({ 42, 85,135 });
    m_collectButton.setOutlineColor(sf::Color(75,145,215), 2.f);
    m_collectButton.setOnClick([this, &game]()
    {
        m_inventory.addItems(m_lastResults);
        saveProgress(game);
        m_lastResults.clear();
        m_openState = OpenState::Idle;
        AudioManager::instance().play(SoundID::ButtonClick);
    });
}

// ═══════════════════════════════════════════════════════════════════════════
// selectCase
// ═══════════════════════════════════════════════════════════════════════════
void GameScreen::selectCase(int idx)
{
    const auto& cases = CaseRegistry::instance().sortedByPrice();
    if (idx < 0 || idx >= static_cast<int>(cases.size())) return;
    m_selectedCase = idx;
    for (int i = 0; i < static_cast<int>(m_caseButtons.size()); ++i)
    {
        if (i == idx)
        {
            m_caseButtons[i].setOutlineColor(sf::Color(220, 180, 60), 2.f);
            m_caseButtons[i].setIdleColor({ 44, 38, 12 });
        }
        else
        {
            m_caseButtons[i].setOutlineColor(sf::Color(65, 65, 105), 1.f);
            m_caseButtons[i].setIdleColor({ 24, 24, 42 });
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// doOpen / doSkip / claimFreeCase / onAllReelsDone
// ═══════════════════════════════════════════════════════════════════════════
void GameScreen::doOpen(Game& game)
{
    if (m_openState != OpenState::Idle) return;

    const auto& cases = CaseRegistry::instance().sortedByPrice();
    if (m_selectedCase >= static_cast<int>(cases.size())) return;

    const Case* c    = cases[m_selectedCase];
    const float cost = c->price() * static_cast<float>(m_openCount);

    if (game.balance() < cost)
    {
        showToast("Not enough funds!", sf::Color(220, 80, 80));
        return;
    }

    game.addBalance(-cost);
    m_lastResults.clear();

    // FIX: pass actual window size so MultiReelManager lays out correctly
    m_reelManager.prepare(*c, m_openCount, game.window().getSize());
    m_reelManager.startAll();
    m_openState = OpenState::Animating;
    AudioManager::instance().play(SoundID::ReelStart);
}

void GameScreen::doSkip()
{
    if (m_openState == OpenState::Animating)
        m_reelManager.skipAll();
}

void GameScreen::claimFreeCase(Game& game)
{
    const auto& cases = CaseRegistry::instance().sortedByPrice();
    if (cases.empty()) return;

    auto item = m_freeCaseReward.claim(*cases[m_selectedCase]);
    if (!item) return;

    m_inventory.addItem(*item);
    saveProgress(game);

    char buf[80];
    std::snprintf(buf, sizeof(buf), "Free case! Got: %s", item->displayName().c_str());
    showToast(buf, item->rarityColor());
    AudioManager::instance().playRevealForRarity(static_cast<int>(item->rarity()));

    if (static_cast<int>(item->rarity()) >= static_cast<int>(Rarity::Classified))
    {
        // Sparkles at the free case button position
        const float W = game.width();
        const float H = game.height();
        m_sparkles.emplace_back(
            sf::Vector2f(W - FreeCaseButton::SIZE / 2.f - 16.f,
                         H - FreeCaseButton::SIZE / 2.f - 12.f),
            item->rarityColor(), 60, 180.f);
    }
}

void GameScreen::onAllReelsDone(std::vector<Item> items, Game& /*game*/)
{
    m_lastResults = std::move(items);
    m_openState   = OpenState::ShowingResults;
}

void GameScreen::saveProgress(Game& game)
{
    JsonSave::SaveData sd;
    sd.balance             = game.balance();
    sd.freeCaseCooldownEnd = m_freeCaseReward.cooldownEndTimestamp();
    m_save.save(sd, m_inventory);
}

void GameScreen::showToast(const std::string& msg, sf::Color col)
{
    m_toasts.push_back({ msg, TOAST_DURATION, col });
    if (m_toasts.size() > 4) m_toasts.erase(m_toasts.begin());
}

// ═══════════════════════════════════════════════════════════════════════════
// handleEvent
// ═══════════════════════════════════════════════════════════════════════════
void GameScreen::handleEvent(const sf::Event& event, Game& game)
{
    if (m_caseInfoPanel.isVisible())
    {
        m_caseInfoPanel.handleEvent(event, game.window());
        return;
    }

    m_backButton.handleEvent(event, game.window());
    m_muteButton.handleEvent(event, game.window());
    m_saveButton.handleEvent(event, game.window());
    m_tabOpen.handleEvent(event, game.window());
    m_tabInventory.handleEvent(event, game.window());
    m_tabTradeUp.handleEvent(event, game.window());

    if (m_activeTab == GameTab::Inventory)
    {
        if (m_inventoryUI) m_inventoryUI->handleEvent(event, game.window());
        return;
    }
    if (m_activeTab == GameTab::TradeUp)
    {
        if (m_tradeUpUI) m_tradeUpUI->handleEvent(event, game.window());
        return;
    }

    // Open tab events
    for (auto& b : m_caseButtons)  b.handleEvent(event, game.window());
    for (auto& b : m_infoButtons)  b.handleEvent(event, game.window());
    for (auto& b : m_countButtons) b.handleEvent(event, game.window());
    m_freeCaseButton.handleEvent(event, game.window());

    if (m_openState == OpenState::Idle)
        m_openButton.handleEvent(event, game.window());
    if (m_openState == OpenState::Animating)
        m_skipButton.handleEvent(event, game.window());
    if (m_openState == OpenState::ShowingResults)
        m_collectButton.handleEvent(event, game.window());
}

// ═══════════════════════════════════════════════════════════════════════════
// update
// ═══════════════════════════════════════════════════════════════════════════
void GameScreen::update(float dt, Game& game)
{
    const sf::Vector2i mouse = sf::Mouse::getPosition();

    m_caseInfoPanel.update(dt);

    m_backButton.update(mouse, dt);
    m_muteButton.update(mouse, dt);
    m_saveButton.update(mouse, dt);
    m_tabOpen.update(mouse, dt);
    m_tabInventory.update(mouse, dt);
    m_tabTradeUp.update(mouse, dt);

    if (m_activeTab == GameTab::Inventory)
    {
        if (m_inventoryUI) m_inventoryUI->update(dt);
    }
    else if (m_activeTab == GameTab::TradeUp)
    {
        if (m_tradeUpUI) m_tradeUpUI->update(dt);
    }
    else
    {
        for (auto& b : m_caseButtons)  b.update(mouse, dt);
        for (auto& b : m_infoButtons)  b.update(mouse, dt);
        for (auto& b : m_countButtons) b.update(mouse, dt);
        m_freeCaseButton.update(dt);

        if (m_openState == OpenState::Idle)
        {
            m_openButton.update(mouse, dt);
            m_openButton.setPulse(true);
        }
        if (m_openState == OpenState::Animating)
        {
            m_skipButton.update(mouse, dt);
            m_reelManager.update(dt);
        }
        if (m_openState == OpenState::ShowingResults)
            m_collectButton.update(mouse, dt);
    }

    // Sparkles
    for (auto& s : m_sparkles) s.update(dt);
    m_sparkles.erase(
        std::remove_if(m_sparkles.begin(), m_sparkles.end(),
            [](const SparkleEmitter& e){ return e.isDead(); }),
        m_sparkles.end());

    // Toasts
    for (auto& t : m_toasts) t.timer -= dt;
    m_toasts.erase(
        std::remove_if(m_toasts.begin(), m_toasts.end(),
            [](const Toast& t){ return t.timer <= 0.f; }),
        m_toasts.end());

    // Balance HUD
    char buf[64];
    std::snprintf(buf, sizeof(buf), "Balance: $%.2f", game.balance());
    m_balanceText.setString(buf);
    {
        sf::FloatRect tb = m_balanceText.getLocalBounds();
        m_balanceText.setOrigin(tb.left + tb.width / 2.f, tb.top);
        m_balanceText.setPosition(game.width() / 2.f, HEADER_H / 2.f - 8.f);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// render
// ═══════════════════════════════════════════════════════════════════════════
void GameScreen::render(sf::RenderWindow& window)
{
    window.draw(m_headerBar);
    window.draw(m_headerLabel);
    window.draw(m_balanceText);
    window.draw(m_backButton);
    window.draw(m_muteButton);
    window.draw(m_saveButton);
    window.draw(m_headerDivider);

    // Tabs with active underline
    auto drawTab = [&](Button& btn, bool active)
    {
        window.draw(btn);
        if (active)
        {
            sf::FloatRect b = btn.bounds();
            sf::RectangleShape ul({ b.width, 3.f });
            ul.setFillColor(sf::Color(220, 180, 60));
            ul.setPosition(b.left, b.top + b.height - 3.f);
            window.draw(ul);
        }
    };
    drawTab(m_tabOpen,      m_activeTab == GameTab::Open);
    drawTab(m_tabInventory, m_activeTab == GameTab::Inventory);
    drawTab(m_tabTradeUp,   m_activeTab == GameTab::TradeUp);

    if (m_activeTab == GameTab::Inventory)
    {
        if (m_inventoryUI) m_inventoryUI->render(window);
    }
    else if (m_activeTab == GameTab::TradeUp)
    {
        if (m_tradeUpUI) m_tradeUpUI->render(window);
    }
    else
    {
        renderOpenTab(window);
    }

    // Sparkles drawn over everything
    for (auto& s : m_sparkles) s.render(window);

    // Toasts
    float toastY = static_cast<float>(window.getSize().y) - 80.f;
    for (auto it = m_toasts.rbegin(); it != m_toasts.rend(); ++it)
    {
        const float alpha = std::min(1.f, it->timer / 0.4f) * std::min(1.f, it->timer);
        sf::Text t;
        t.setFont(*m_headerLabel.getFont());
        t.setCharacterSize(15);
        sf::Color col = it->colour;
        col.a = static_cast<sf::Uint8>(alpha * 230.f);
        t.setFillColor(col);
        t.setString(it->msg);
        sf::FloatRect tb = t.getLocalBounds();
        t.setOrigin(tb.left + tb.width / 2.f, tb.top + tb.height / 2.f);
        t.setPosition(static_cast<float>(window.getSize().x) / 2.f, toastY);
        window.draw(t);
        toastY -= 26.f;
    }

    // F11 hint in bottom corner
    {
        sf::Text hint;
        hint.setFont(*m_headerLabel.getFont());
        hint.setCharacterSize(10);
        hint.setFillColor(sf::Color(45, 45, 68));
        hint.setString("F11 fullscreen");
        hint.setPosition(4.f, static_cast<float>(window.getSize().y) - 14.f);
        window.draw(hint);
    }

    // Modal info panel (always on top)
    m_caseInfoPanel.render(window);
}

void GameScreen::renderOpenTab(sf::RenderWindow& window)
{
    const float W = static_cast<float>(window.getSize().x);
    const float H = static_cast<float>(window.getSize().y);

    // Case selector bar
    window.draw(m_caseSelectorBg);
    for (auto& b : m_caseButtons) window.draw(b);
    for (auto& b : m_infoButtons) window.draw(b);

    // Count buttons with active underline
    const OpenCount counts[] = {OpenCount::One, OpenCount::Three,
                                 OpenCount::Five, OpenCount::Ten};
    for (int i = 0; i < 4; ++i)
    {
        window.draw(m_countButtons[i]);
        if (counts[i] == m_openCount)
        {
            sf::FloatRect b = m_countButtons[i].bounds();
            sf::RectangleShape ul({ b.width, 2.f });
            ul.setFillColor(sf::Color(220, 180, 60));
            ul.setPosition(b.left, b.top + b.height - 2.f);
            window.draw(ul);
        }
    }

    // FIX: Clean two-line cost display (no broken concatenated string)
    {
        const auto& cases = CaseRegistry::instance().sortedByPrice();
        if (m_selectedCase < static_cast<int>(cases.size()))
        {
            const Case* c   = cases[m_selectedCase];
            const int   n   = static_cast<int>(m_openCount);
            const float cost = c->price() * n;
            const float infoY = CONTENT_Y + 118.f;

            // Line 1: case name
            sf::Text name;
            name.setFont(*m_headerLabel.getFont());
            name.setCharacterSize(14);
            name.setFillColor(sf::Color(180, 180, 200));
            name.setString(c->displayName());
            {
                sf::FloatRect nb = name.getLocalBounds();
                name.setOrigin(nb.left + nb.width / 2.f, nb.top);
                name.setPosition(W / 2.f, infoY);
            }
            window.draw(name);

            // Line 2: price formula — properly formatted
            char calcBuf[64];
            if (n == 1)
                std::snprintf(calcBuf, sizeof(calcBuf), "$%.2f", cost);
            else
                std::snprintf(calcBuf, sizeof(calcBuf), "%d \xc3\x97 $%.2f = $%.2f",
                              n, c->price(), cost);  // \xc3\x97 = UTF-8 ×

            sf::Text calc;
            calc.setFont(*m_headerLabel.getFont());
            calc.setCharacterSize(13);
            calc.setFillColor(sf::Color(130, 200, 130));
            calc.setString(calcBuf);
            {
                sf::FloatRect cb = calc.getLocalBounds();
                calc.setOrigin(cb.left + cb.width / 2.f, cb.top);
                calc.setPosition(W / 2.f, infoY + 20.f);
            }
            window.draw(calc);
        }
    }

    // Reel area or idle placeholder
    if (m_openState == OpenState::Animating ||
        m_openState == OpenState::ShowingResults)
    {
        m_reelManager.render(window);
    }
    else
    {
        sf::Text ph;
        ph.setFont(*m_headerLabel.getFont());
        ph.setCharacterSize(17);
        ph.setFillColor(sf::Color(40, 40, 65));
        ph.setString("Select a case and press OPEN CASE");
        sf::FloatRect pb = ph.getLocalBounds();
        ph.setOrigin(pb.left + pb.width / 2.f, pb.top + pb.height / 2.f);
        ph.setPosition(W / 2.f, (CONTENT_Y + 140.f + H - 80.f) / 2.f);
        window.draw(ph);
    }

    // Results strip
    if (m_openState == OpenState::ShowingResults)
    {
        const float stripH = 108.f;
        const float stripY = H - stripH - 62.f;

        sf::RectangleShape strip({ W, stripH });
        strip.setFillColor(sf::Color(10, 10, 20, 238));
        strip.setPosition(0.f, stripY);
        window.draw(strip);

        sf::RectangleShape topLine({ W, 2.f });
        topLine.setFillColor(sf::Color(50, 50, 80));
        topLine.setPosition(0.f, stripY);
        window.draw(topLine);

        const float cW = 88.f, cH = 78.f, cGap = 7.f;
        const int   cnt = static_cast<int>(m_lastResults.size());
        const float totalW2 = cnt * (cW + cGap) - cGap;
        float x = (W - totalW2) / 2.f;
        const float y = stripY + (stripH - cH) / 2.f;

        for (const Item& item : m_lastResults)
        {
            const sf::Color rc = item.rarityColor();

            sf::RectangleShape card({ cW, cH });
            card.setPosition(x, y);
            card.setFillColor(sf::Color(18, 18, 32));
            card.setOutlineThickness(2.f);
            card.setOutlineColor(rc);
            window.draw(card);

            sf::RectangleShape bar({ cW, 3.f });
            bar.setFillColor(rc);
            bar.setPosition(x, y + cH - 3.f);
            window.draw(bar);

            char vb[20];
            std::snprintf(vb, sizeof(vb), "$%.0f", item.value());
            sf::Text vt;
            vt.setFont(*m_headerLabel.getFont());
            vt.setCharacterSize(11);
            vt.setFillColor(sf::Color(150, 210, 150));
            vt.setString(vb);
            sf::FloatRect vtb = vt.getLocalBounds();
            vt.setOrigin(vtb.left + vtb.width / 2.f, vtb.top);
            vt.setPosition(x + cW / 2.f, y + cH - 18.f);
            window.draw(vt);

            x += cW + cGap;
        }

        sf::Text hint;
        hint.setFont(*m_headerLabel.getFont());
        hint.setCharacterSize(12);
        hint.setFillColor(sf::Color(100, 165, 100));
        hint.setString("Press COLLECT ALL to add to inventory");
        sf::FloatRect hb = hint.getLocalBounds();
        hint.setOrigin(hb.left + hb.width / 2.f, hb.top);
        hint.setPosition(W / 2.f, stripY - 20.f);
        window.draw(hint);
    }

    // Action buttons
    if (m_openState == OpenState::Idle)
    {
        window.draw(m_openButton);
        m_freeCaseButton.render(window);
    }
    if (m_openState == OpenState::Animating)
        window.draw(m_skipButton);
    if (m_openState == OpenState::ShowingResults)
        window.draw(m_collectButton);
}
