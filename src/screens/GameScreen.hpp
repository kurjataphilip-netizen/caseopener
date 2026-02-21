#pragma once

#include "Screen.hpp"
#include "../ui/Button.hpp"
#include "../ui/InventoryUI.hpp"
#include "../ui/CaseInfoPanel.hpp"
#include "../ui/TradeUpUI.hpp"
#include "../ui/FreeCaseButton.hpp"
#include "../ui/HoverEffect.hpp"
#include "../gameplay/MultiReelManager.hpp"
#include "../gameplay/Inventory.hpp"
#include "../gameplay/TradeUp.hpp"
#include "../cases/CaseRegistry.hpp"
#include "../core/FreeCaseReward.hpp"
#include "../core/JsonSave.hpp"
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <optional>

enum class GameTab { Open, Inventory, TradeUp };

class GameScreen : public Screen
{
public:
    explicit GameScreen(Game& game);
    ~GameScreen();

    void handleEvent(const sf::Event& event, Game& game) override;
    void update     (float dt,               Game& game) override;
    void render     (sf::RenderWindow& window)           override;

private:
    // Build helpers
    void buildHeaderBar       (Game& game);
    void buildTabBar          (Game& game);
    void buildCaseSelector    (Game& game);
    void buildOpenCountButtons(Game& game);
    void buildActionButtons   (Game& game);

    // Render helpers
    void renderOpenTab    (sf::RenderWindow& w, Game& game, float dt);
    void renderResults    (sf::RenderWindow& w);
    void renderCaseSelector(sf::RenderWindow& w);
    void renderHUD        (sf::RenderWindow& w, Game& game);

    // Actions
    void doOpen           (Game& game);
    void doSkip           ();
    void selectCase       (int idx);
    void onAllReelsDone   (std::vector<Item> items, Game& game);
    void claimFreeCase    (Game& game);
    void saveProgress     (Game& game);

    // Toast
    void showToast(const std::string& msg, sf::Color col = sf::Color(220,220,100));

    // Layout constants
    static constexpr float HEADER_H  = 64.f;
    static constexpr float TABBAR_H  = 40.f;
    static constexpr float CONTENT_Y = HEADER_H + TABBAR_H;

    // Header
    sf::RectangleShape m_headerBar, m_headerDivider;
    sf::Text           m_headerLabel;
    Button             m_backButton, m_muteButton, m_saveButton;

    // Tabs
    Button   m_tabOpen, m_tabInventory, m_tabTradeUp;
    GameTab  m_activeTab { GameTab::Open };

    // Case selector
    std::vector<Button> m_caseButtons, m_infoButtons;
    std::vector<HoverEffect> m_caseHovers;
    int                 m_selectedCase { 0 };
    sf::RectangleShape  m_caseSelectorBg;

    // Count buttons
    std::vector<Button>  m_countButtons;
    OpenCount            m_openCount { OpenCount::One };

    // Action buttons
    Button m_openButton, m_skipButton, m_collectButton;

    // Gameplay
    MultiReelManager               m_reelManager;
    Inventory                      m_inventory;
    std::unique_ptr<InventoryUI>   m_inventoryUI;
    std::unique_ptr<TradeUpUI>     m_tradeUpUI;
    CaseInfoPanel                  m_caseInfoPanel;
    FreeCaseReward                 m_freeCaseReward;
    FreeCaseButton                 m_freeCaseButton;
    JsonSave                       m_save;

    // Sparkle effects (for rare reveals)
    std::vector<SparkleEmitter>    m_sparkles;

    enum class OpenState { Idle, Animating, ShowingResults };
    OpenState  m_openState { OpenState::Idle };

    std::vector<Item> m_lastResults;

    // HUD texts
    sf::Text m_balanceText;

    // Toast
    struct Toast { std::string msg; float timer; sf::Color colour; };
    std::vector<Toast> m_toasts;
    static constexpr float TOAST_DURATION = 2.8f;
};
