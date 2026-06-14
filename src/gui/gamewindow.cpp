#include "gui/gamewindow.h"
#include "core/equipment.h"
#include "core/game.h"
#include "core/player.h"
#include "core/shop.h"
#include "entity/combattypes.h"
#include "entity/unit.h"
#include <QFileDialog>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QStringList>
#include <QVBoxLayout>

GameWindow::GameWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_centralWidget(new QWidget(this))
    , m_mainLayout(new QVBoxLayout())
    , m_view(new QGraphicsView(this))
    , m_hpLabel(new QLabel(this))
    , m_goldLabel(new QLabel(this))
    , m_roundLabel(new QLabel(this))
    , m_popLabel(new QLabel(this))
    , m_phaseLabel(new QLabel(this))
    , m_synergyLabel(new QLabel(this))
    , m_unitInfoLabel(new QLabel(this))
    , m_resetButton(new QPushButton(QStringLiteral("\u91CD\u7F6E"), this))
    , m_startCombatButton(new QPushButton(QStringLiteral("\u5F00\u59CB\u6218\u6597"), this))
    , m_refreshShopButton(new QPushButton(QStringLiteral("\u5237\u65B0\u5546\u5E97(2\u91D1)"), this))
    , m_upgradePopButton(new QPushButton(QStringLiteral("\u5347\u7EA7\u4EBA\u53E3"), this))
    , m_saveButton(new QPushButton(QStringLiteral("\u5B58\u6863"), this))
    , m_loadButton(new QPushButton(QStringLiteral("\u8BFB\u6863"), this))
    , m_equipDropLabel(nullptr)
    , m_game(new Game(this))
{
    for (QPushButton*& btn : m_shopButtons) {
        btn = nullptr;
    }
    for (QPushButton*& btn : m_equipButtons) {
        btn = nullptr;
    }
    setupUI();
    m_game->initialize();
    refreshPanels();
}

GameWindow::~GameWindow() = default;

void GameWindow::onResetClicked()
/*重置游戏*/
{
    m_game->reset();
    refreshPanels();
}

void GameWindow::onStartCombatClicked()
/*开始战斗：清除装备掉落提示、启动战斗、刷新面板*/
{
    m_game->clearEquipDropMessage();
    m_game->startCombat();
    refreshPanels();
}

void GameWindow::onRefreshShopClicked()
/*刷新商店*/
{
    m_game->refreshShop();
    refreshPanels();
}

void GameWindow::onUpgradePopClicked()
/*升级人口上限*/
{
    m_game->upgradePopulation();
    refreshPanels();
}

void GameWindow::onSaveClicked()
/*弹出文件保存对话框，保存游戏到JSON*/
{
    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("\u4FDD\u5B58\u6E38\u620F"), QStringLiteral("synera_save.json"),
        QStringLiteral("JSON (*.json)"));
    if (path.isEmpty()) {
        return;
    }
    QString error;
    if (!m_game->saveToFile(path, &error)) {
        m_phaseLabel->setText(error);
    }
    refreshPanels();
}

void GameWindow::onLoadClicked()
/*弹出文件打开对话框，从JSON读取存档*/
{
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("\u8BFB\u53D6\u5B58\u6863"), QString(), QStringLiteral("JSON (*.json)"));
    if (path.isEmpty()) {
        return;
    }
    QString error;
    if (!m_game->loadFromFile(path, &error)) {
        m_phaseLabel->setText(error);
    }
    refreshPanels();
}

void GameWindow::onShopSlotClicked()
/*商店购买：通过按钮属性获取槽位编号，调用Game购买*/
{
    auto* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) {
        return;
    }
    const int slot = btn->property("shopSlot").toInt();
    m_game->buyFromShop(slot);
    refreshPanels();
}

void GameWindow::onEquipSlotClicked()
/*选中装备栏槽位，提示用户点击单位穿戴或卸下装备*/
{
    auto* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) {
        return;
    }
    const int slot = btn->property("equipSlot").toInt();
    m_game->setSelectedEquipSlot(slot);
    m_phaseLabel->setText(QStringLiteral("\u5DF2\u9009\u4E2D\u88C5\u5907\u680F\uFF0C\u8BF7\u70B9\u51FB\u8981\u7A7F\u6234\u7684\u6211\u65B9\u5355\u4F4D\u3002"));
    refreshPanels();
}

void GameWindow::refreshShopButtons()
/*刷新商店按钮：更新名称价格，已售出显示"已售"并禁用*/
{
    const Shop& shop = m_game->shop();
    for (int i = 0; i < Shop::SLOT_COUNT; ++i) {
        QPushButton* btn = m_shopButtons[i];
        if (!btn) {
            continue;
        }
        const QString offer = shop.offerAt(i);
        if (offer.isEmpty()) {
            btn->setText(QStringLiteral("\u5DF2\u552E"));
            btn->setEnabled(false);
        } else {
            btn->setText(QStringLiteral("%1 (%2\u91D1)").arg(offer).arg(shop.costAt(i)));
            btn->setEnabled(m_game->phase() == GamePhase::Prep && m_game->result() == GameResult::Playing);
        }
    }
}

void GameWindow::refreshPanels()
/*刷新全部面板：生命/金币/回合/人口/阶段/羁绊/商店/装备/选中单位信息*/
{
    const Player* player = m_game->player();
    m_hpLabel->setText(QStringLiteral("\u73A9\u5BB6\u751F\u547D: %1 / %2").arg(player->hp()).arg(player->maxHp()));
    m_goldLabel->setText(QStringLiteral("\u91D1\u5E01: %1").arg(player->gold()));
    const QString st = player->streakText();
    const int interest = player->interestBonus();
    if (!st.isEmpty()) {
        m_goldLabel->setText(m_goldLabel->text() + QStringLiteral(" [%1]").arg(st));
    }
    if (interest > 0) {
        m_goldLabel->setText(m_goldLabel->text() + QStringLiteral(" (\u5229\u606F+%1)").arg(interest));
    }
    m_roundLabel->setText(QStringLiteral("\u56DE\u5408: %1 / %2")
                              .arg(player->currentRound())
                              .arg(CombatConst::kMaxRounds));
    m_popLabel->setText(QStringLiteral("\u573A\u4E0A: %1 / %2 | \u5347\u7EA7\u4EBA\u53E3: %3\u91D1")
                            .arg(m_game->playerUnitsOnBoard())
                            .arg(player->populationCap())
                            .arg(player->populationUpgradeCost()));

    QString phaseName;
    switch (m_game->phase()) {
    case GamePhase::Prep: phaseName = QStringLiteral("\u51C6\u5907"); break;
    case GamePhase::Combat: phaseName = QStringLiteral("\u6218\u6597"); break;
    case GamePhase::Resolve: phaseName = QStringLiteral("\u7ED3\u7B97"); break;
    }

    QString resultText;
    switch (m_game->result()) {
    case GameResult::Playing: resultText = QString(); break;
    case GameResult::Victory: resultText = QStringLiteral(" | \u3010\u901A\u5173\u3011"); break;
    case GameResult::Defeat: resultText = QStringLiteral(" | \u3010\u5931\u8D25\u3011"); break;
    }

    m_phaseLabel->setText(QStringLiteral("\u9636\u6BB5: %1%2 | %3")
                              .arg(phaseName)
                              .arg(resultText)
                              .arg(m_game->phaseMessage()));
    m_synergyLabel->setText(QStringLiteral("\u7F81\u7ECA: %1").arg(m_game->synergySummary()));

    refreshShopButtons();
    for (int i = 0; i < 6; ++i) {
        QPushButton* btn = m_equipButtons[i];
        if (!btn) {
            continue;
        }
        if (i < m_game->equipBench().count()) {
            const EquipType t = m_game->equipBench().peek(i);
            btn->setText(Equipment::info(t).name);
            btn->setEnabled(m_game->phase() == GamePhase::Prep);
        } else {
            btn->setText(QStringLiteral("\u7A7A"));
            btn->setEnabled(m_game->phase() == GamePhase::Prep);
        }
    }

    const QString dropMsg = m_game->equipDropMessage();
    m_equipDropLabel->setText(dropMsg.isEmpty() ? QString() : dropMsg);

    updateButtons();

    const Unit* unit = m_game->selectedUnit();
    if (!unit) {
        m_unitInfoLabel->setText(QStringLiteral("\u70B9\u51FB\u5546\u5E97\u8D2D\u4E70\uFF1B\u70B9\u51FB\u88C5\u5907\u518D\u70B9\u5355\u4F4D\u7A7F\u6234\uFF1B\u62D6\u62FD\u5E03\u9635\u540E\u5F00\u6218\u3002"));
        return;
    }

    const QString ownerText = unit->owner() == Controller::PlayerCtrl
        ? QStringLiteral("\u6211\u65B9") : QStringLiteral("\u654C\u65B9");
    const QString traits = unit->traits().isEmpty()
        ? QStringLiteral("\u65E0")
        : QStringList(unit->traits().begin(), unit->traits().end()).join(QStringLiteral(", "));

    m_unitInfoLabel->setText(
        QStringLiteral("%1 [%2] %3 | HP %4/%5 | ATK %6 | \u5C04\u7A0B %7 | \u6CD5\u529B %8/%9 | \u88C5\u5907: %10 | \u72B6\u6001: %11 | \u7F81\u7ECA: %12")
            .arg(unit->displayName())
            .arg(ownerText)
            .arg(unit->heroType())
            .arg(unit->hp())
            .arg(unit->maxHp())
            .arg(unit->atk())
            .arg(unit->range())
            .arg(unit->mana())
            .arg(unit->maxMana())
            .arg(unit->equipmentText())
            .arg(unit->stateText())
            .arg(traits));
}

void GameWindow::updateButtons()
/*根据当前阶段和结果启用/禁用按钮*/
{
    const bool playing = m_game->result() == GameResult::Playing;
    const bool prep = m_game->phase() == GamePhase::Prep;
    m_startCombatButton->setEnabled(playing && prep);
    m_refreshShopButton->setEnabled(playing && prep);
    m_upgradePopButton->setEnabled(playing && prep);
    m_saveButton->setEnabled(true);
    m_loadButton->setEnabled(true);
    m_resetButton->setEnabled(true);
}

void GameWindow::setupUI()
/*构建主窗口布局：顶部状态栏→阶段/羁绊标签→商店栏→装备栏→棋盘视图→底部单位信息*/
{
    setWindowTitle(QStringLiteral("Synera - Starter"));
    resize(1000, 960);

    setCentralWidget(m_centralWidget);
    m_centralWidget->setLayout(m_mainLayout);

    setStyleSheet(R"(
        QMainWindow, QWidget { background-color: #2b2b2b; color: #f0f0f0; }
        QPushButton {
            background-color: #3a3a3a; border: 1px solid #666; border-radius: 4px;
            padding: 5px 10px;
        }
        QPushButton:hover { background-color: #454545; }
        QPushButton:disabled { color: #888; background-color: #2f2f2f; }
        QLabel { font-size: 12px; }
    )");

    QWidget* topBar = new QWidget(this);
    QHBoxLayout* topLayout = new QHBoxLayout(topBar);
    topLayout->addWidget(m_hpLabel);
    topLayout->addWidget(m_goldLabel);
    topLayout->addWidget(m_roundLabel);
    topLayout->addWidget(m_popLabel);
    topLayout->addStretch();
    topLayout->addWidget(m_refreshShopButton);
    topLayout->addWidget(m_upgradePopButton);
    topLayout->addWidget(m_startCombatButton);
    topLayout->addWidget(m_saveButton);
    topLayout->addWidget(m_loadButton);
    topLayout->addWidget(m_resetButton);
    m_mainLayout->addWidget(topBar);

    m_phaseLabel->setWordWrap(true);
    m_mainLayout->addWidget(m_phaseLabel);
    m_synergyLabel->setWordWrap(true);
    m_mainLayout->addWidget(m_synergyLabel);

    QWidget* shopBar = new QWidget(this);
    QHBoxLayout* shopLayout = new QHBoxLayout(shopBar);
    shopLayout->addWidget(new QLabel(QStringLiteral("\u5546\u5E97:"), this));
    for (int i = 0; i < Shop::SLOT_COUNT; ++i) {
        m_shopButtons[i] = new QPushButton(QStringLiteral("?"), this);
        m_shopButtons[i]->setProperty("shopSlot", i);
        connect(m_shopButtons[i], &QPushButton::clicked, this, &GameWindow::onShopSlotClicked);
        shopLayout->addWidget(m_shopButtons[i]);
    }
    m_mainLayout->addWidget(shopBar);

    QWidget* equipBar = new QWidget(this);
    QHBoxLayout* equipLayout = new QHBoxLayout(equipBar);
    equipLayout->addWidget(new QLabel(QStringLiteral("\u88C5\u5907\u680F:"), this));
    for (int i = 0; i < 6; ++i) {
        m_equipButtons[i] = new QPushButton(QStringLiteral("\u7A7A"), this);
        m_equipButtons[i]->setProperty("equipSlot", i);
        connect(m_equipButtons[i], &QPushButton::clicked, this, &GameWindow::onEquipSlotClicked);
        equipLayout->addWidget(m_equipButtons[i]);
    }
    m_equipDropLabel = new QLabel(this);
    QFont dropFont = m_equipDropLabel->font();
    dropFont.setBold(true);
    dropFont.setPointSize(12);
    m_equipDropLabel->setFont(dropFont);
    m_equipDropLabel->setStyleSheet(QStringLiteral("color: #ffcc33;"));
    equipLayout->addWidget(m_equipDropLabel);
    equipLayout->addStretch();
    m_mainLayout->addWidget(equipBar);

    m_view->setRenderHint(QPainter::Antialiasing, true);
    m_view->setDragMode(QGraphicsView::NoDrag);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_view->setScene(m_game->scene());
    m_mainLayout->addWidget(m_view, 1);

    m_unitInfoLabel->setWordWrap(true);
    m_unitInfoLabel->setMinimumHeight(56);
    m_mainLayout->addWidget(m_unitInfoLabel);

    connect(m_resetButton, &QPushButton::clicked, this, &GameWindow::onResetClicked);
    connect(m_startCombatButton, &QPushButton::clicked, this, &GameWindow::onStartCombatClicked);
    connect(m_refreshShopButton, &QPushButton::clicked, this, &GameWindow::onRefreshShopClicked);
    connect(m_upgradePopButton, &QPushButton::clicked, this, &GameWindow::onUpgradePopClicked);
    connect(m_saveButton, &QPushButton::clicked, this, &GameWindow::onSaveClicked);
    connect(m_loadButton, &QPushButton::clicked, this, &GameWindow::onLoadClicked);
    connect(m_game, &Game::stateChanged, this, &GameWindow::refreshPanels);
}
