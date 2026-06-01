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
    , m_resetButton(new QPushButton(QStringLiteral("重置"), this))
    , m_startCombatButton(new QPushButton(QStringLiteral("开始战斗"), this))
    , m_refreshShopButton(new QPushButton(QStringLiteral("刷新商店(2金)"), this))
    , m_upgradePopButton(new QPushButton(QStringLiteral("升级人口"), this))
    , m_saveButton(new QPushButton(QStringLiteral("存档"), this))
    , m_loadButton(new QPushButton(QStringLiteral("读档"), this))
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
{
    m_game->reset();
    refreshPanels();
}

void GameWindow::onStartCombatClicked()
{
    m_game->clearEquipDropMessage();
    m_game->startCombat();
    refreshPanels();
}

void GameWindow::onRefreshShopClicked()
{
    m_game->refreshShop();
    refreshPanels();
}

void GameWindow::onUpgradePopClicked()
{
    m_game->upgradePopulation();
    refreshPanels();
}

void GameWindow::onSaveClicked()
{
    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("保存游戏"), QStringLiteral("synera_save.json"),
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
{
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("读取存档"), QString(), QStringLiteral("JSON (*.json)"));
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
{
    auto* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) {
        return;
    }
    const int slot = btn->property("equipSlot").toInt();
    m_game->setSelectedEquipSlot(slot);
    m_phaseLabel->setText(QStringLiteral("已选中装备栏，请点击要穿戴的我方单位。"));
    refreshPanels();
}

void GameWindow::refreshShopButtons()
{
    const Shop& shop = m_game->shop();
    for (int i = 0; i < Shop::SLOT_COUNT; ++i) {
        QPushButton* btn = m_shopButtons[i];
        if (!btn) {
            continue;
        }
        const QString offer = shop.offerAt(i);
        if (offer.isEmpty()) {
            btn->setText(QStringLiteral("已售"));
            btn->setEnabled(false);
        } else {
            btn->setText(QStringLiteral("%1 (%2金)").arg(offer).arg(shop.costAt(i)));
            btn->setEnabled(m_game->phase() == GamePhase::Prep && m_game->result() == GameResult::Playing);
        }
    }
}

void GameWindow::refreshPanels()
{
    const Player* player = m_game->player();
    m_hpLabel->setText(QStringLiteral("玩家生命: %1 / %2").arg(player->hp()).arg(player->maxHp()));
    m_goldLabel->setText(QStringLiteral("金币: %1").arg(player->gold()));
    const QString st = player->streakText();
    const int interest = player->interestBonus();
    if (!st.isEmpty()) {
        m_goldLabel->setText(m_goldLabel->text() + QStringLiteral(" [%1]").arg(st));
    }
    if (interest > 0) {
        m_goldLabel->setText(m_goldLabel->text() + QStringLiteral(" (利息+%1)").arg(interest));
    }
    m_roundLabel->setText(QStringLiteral("回合: %1 / %2")
                              .arg(player->currentRound())
                              .arg(CombatConst::kMaxRounds));
    m_popLabel->setText(QStringLiteral("场上: %1 / %2 | 升级人口: %3金")
                            .arg(m_game->playerUnitsOnBoard())
                            .arg(player->populationCap())
                            .arg(player->populationUpgradeCost()));

    QString phaseName;
    switch (m_game->phase()) {
    case GamePhase::Prep: phaseName = QStringLiteral("准备"); break;
    case GamePhase::Combat: phaseName = QStringLiteral("战斗"); break;
    case GamePhase::Resolve: phaseName = QStringLiteral("结算"); break;
    }

    QString resultText;
    switch (m_game->result()) {
    case GameResult::Playing: resultText = QString(); break;
    case GameResult::Victory: resultText = QStringLiteral(" | 【通关】"); break;
    case GameResult::Defeat: resultText = QStringLiteral(" | 【失败】"); break;
    }

    m_phaseLabel->setText(QStringLiteral("阶段: %1%2 | %3")
                              .arg(phaseName)
                              .arg(resultText)
                              .arg(m_game->phaseMessage()));
    m_synergyLabel->setText(QStringLiteral("羁绊: %1").arg(m_game->synergySummary()));

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
            btn->setText(QStringLiteral("空"));
            btn->setEnabled(false);
        }
    }

    const QString dropMsg = m_game->equipDropMessage();
    m_equipDropLabel->setText(dropMsg.isEmpty() ? QString() : dropMsg);

    updateButtons();

    const Unit* unit = m_game->selectedUnit();
    if (!unit) {
        m_unitInfoLabel->setText(QStringLiteral("点击商店购买；点击装备再点单位穿戴；拖拽布阵后开战。"));
        return;
    }

    const QString ownerText = unit->owner() == Controller::PlayerCtrl
        ? QStringLiteral("我方") : QStringLiteral("敌方");
    const QString traits = unit->traits().isEmpty()
        ? QStringLiteral("无")
        : QStringList(unit->traits().begin(), unit->traits().end()).join(QStringLiteral(", "));

    m_unitInfoLabel->setText(
        QStringLiteral("%1 [%2] %3 | HP %4/%5 | ATK %6 | 射程 %7 | 法力 %8/%9 | 装备: %10 | 状态: %11 | 羁绊: %12")
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
    shopLayout->addWidget(new QLabel(QStringLiteral("商店:"), this));
    for (int i = 0; i < Shop::SLOT_COUNT; ++i) {
        m_shopButtons[i] = new QPushButton(QStringLiteral("?"), this);
        m_shopButtons[i]->setProperty("shopSlot", i);
        connect(m_shopButtons[i], &QPushButton::clicked, this, &GameWindow::onShopSlotClicked);
        shopLayout->addWidget(m_shopButtons[i]);
    }
    m_mainLayout->addWidget(shopBar);

    QWidget* equipBar = new QWidget(this);
    QHBoxLayout* equipLayout = new QHBoxLayout(equipBar);
    equipLayout->addWidget(new QLabel(QStringLiteral("装备栏:"), this));
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
