#include "game.h"
#include <algorithm>
#include <array>
#include <stdexcept>
#include "core/gamesave.h"
#include "core/unitfactory.h"
#include "entity/unit.h"
#include "gui/benchslotitem.h"
#include "gui/griditem.h"
#include "gui/overlayitem.h"
#include "gui/unititem.h"
#include <QFile>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QtMath>

namespace {
constexpr qreal kZGrid = 0.0;
constexpr qreal kZUnit = 1.0;
constexpr qreal kZDraggingUnit = 2.0;
}

Game::Game(QObject* parent)
    : QObject(parent)
    , m_scene(new QGraphicsScene(this))
    , m_sellZoneItem(nullptr)
    , m_overlayItem(nullptr)
    , m_combatTimer(new QTimer(this))
    , m_geometry(Board::ROWS, Board::COLS, 46.0, 69.0,
                 Board::ROWS * 69.0 + 30.0, 55.0)
    , m_dragDrop(m_board, m_player)
    , m_phase(GamePhase::Prep)
    , m_result(GameResult::Playing)
    , m_lastCombatWon(false)
    , m_selectedUnitId(-1)
    , m_selectedEquipSlot(-1)
{
    m_combatTimer->setInterval(1000 / CombatConst::kFramesPerSecond);
    connect(m_combatTimer, &QTimer::timeout, this, &Game::onCombatTick);
}

Game::~Game()
{
    qDeleteAll(m_allUnits);
    m_allUnits.clear();
}

void Game::initialize()
{
    buildScene();
    reset();
}

void Game::reset()
{
    m_combatTimer->stop();
    m_board.clear();

    QList<Unit*> toDelete = m_allUnits;
    for (Unit* unit : toDelete) {
        m_board.removeUnit(unit);
        auto it = m_unitItemById.find(unit->id());
        if (it != m_unitItemById.end()) {
            m_scene->removeItem(it->second);
            delete it->second;
            m_unitItemById.erase(it);
        }
        m_allUnits.removeOne(unit);
        delete unit;
    }
    m_unitItems.clear();

    m_player.reset();
    m_equipBench.clear();
    m_shop.refresh();
    m_result = GameResult::Playing;
    m_phaseMessage.clear();
    m_selectedEquipSlot = -1;

    beginPrepPhase();
    emit stateChanged();
}

void Game::startCombat()
{
    if (m_phase != GamePhase::Prep || m_result != GameResult::Playing) {
        return;
    }
    if (countPlayerUnitsOnBoard() <= 0) {
        m_phaseMessage = QStringLiteral("\u8BF7\u81F3\u5C11\u5C06\u4E00\u4E2A\u5355\u4F4D\u90E8\u7F72\u5230\u68CB\u76D8\u4E0B\u534A\u533A\u540E\u518D\u5F00\u59CB\u6218\u6597\u3002");
        emit stateChanged();
        return;
    }
    if (!hasEnemiesOnBoard()) {
        spawnEnemiesForRound(m_player.currentRound());
    }

    snapshotPlayerDeployment();
    prepareUnitsForCombat();
    m_phase = GamePhase::Combat;
    m_phaseMessage = QStringLiteral("\u6218\u6597\u8FDB\u884C\u4E2D\u2026");
    m_combatTimer->start();
    emit stateChanged();
}

void Game::beginPrepPhase()
{
    m_phase = GamePhase::Prep;

    spawnEnemiesForRound(m_player.currentRound());
    ensureUnitItems();
    syncFromBoard();
    syncFromBench();
    refreshUnitVisuals();

    const int interest = m_player.interestBonus();
    const int streak = m_player.streakBonus();
    if (interest > 0 || streak > 0) {
        m_player.addGold(interest + streak);
    }

    m_shop.refresh();
    m_synergy.applyToPlayerUnits(m_board, *m_player.bench(), m_allUnits);

    if (m_result == GameResult::Playing) {
        QString msg = QStringLiteral("\u51C6\u5907\u9636\u6BB5\uFF1A\u8D2D\u4E70/\u5E03\u9635/\u88C5\u5907\uFF0C\u7136\u540E\u300E\u5F00\u59CB\u6218\u6597\u300F\u3002");
        if (interest > 0) {
            msg += QStringLiteral(" \u5229\u606F+%1\u91D1").arg(interest);
        }
        const QString st = m_player.streakText();
        if (!st.isEmpty()) {
            msg += QStringLiteral(" %1\u5956\u52B1+%2\u91D1").arg(st).arg(streak);
        }
        m_phaseMessage = msg;
    }
}

void Game::beginResolvePhase(bool playerWon)
{
    m_combatTimer->stop();
    clearAttackLines();
    m_phase = GamePhase::Resolve;
    m_lastCombatWon = playerWon;

    const int round = m_player.currentRound();
    if (playerWon) {
        m_player.onCombatWin();
        m_player.addGold(CombatConst::kGoldWinBase + round * 2);
        m_phaseMessage = QStringLiteral("\u6218\u6597\u80DC\u5229\uFF01\u83B7\u5F97\u91D1\u5E01\u3002");
    } else {
        m_player.onCombatLose();
        m_player.takeDamage(CombatConst::kPlayerDamageOnLoss + (round - 1) * 2);
        m_player.addGold(CombatConst::kGoldLoseBase);
        m_phaseMessage = QStringLiteral("\u6218\u6597\u5931\u8D25\uFF0C\u73A9\u5BB6\u53D7\u5230\u4F24\u5BB3\u3002");
    }

    tryDropEquipment(playerWon);
    cleanupAfterCombat();
    m_synergy.applyToPlayerUnits(m_board, *m_player.bench(), m_allUnits);

    if (!m_player.isAlive()) {
        m_result = GameResult::Defeat;
        m_phaseMessage = QStringLiteral("\u6E38\u620F\u5931\u8D25\uFF1A\u73A9\u5BB6\u751F\u547D\u503C\u5F52\u96F6\u3002");
        emit stateChanged();
        showBattleOverlay(false, true);
        return;
    }

    if (playerWon && round >= CombatConst::kMaxRounds) {
        m_result = GameResult::Victory;
        m_phaseMessage = QStringLiteral("\u606D\u559C\u901A\u5173\uFF01\u4F60\u51FB\u8D25\u4E86\u5168\u90E8 %1 \u8F6E\u654C\u4EBA\u3002").arg(CombatConst::kMaxRounds);
        emit stateChanged();
        showBattleOverlay(true, true);
        return;
    }

    if (playerWon) {
        m_player.advanceRound();
    }

    clearEnemyUnits();
    beginPrepPhase();

    if (playerWon) {
        m_phaseMessage = QStringLiteral("\u4E0A\u8F6E\u80DC\u5229\uFF01\u8FDB\u5165\u7B2C %1 \u8F6E\u51C6\u5907\u9636\u6BB5\u3002").arg(m_player.currentRound());
    } else {
        m_phaseMessage = QStringLiteral("\u4E0A\u8F6E\u5931\u8D25\uFF0C\u91CD\u65B0\u90E8\u7F72\u540E\u7EE7\u7EED\u7B2C %1 \u8F6E\u3002").arg(m_player.currentRound());
    }
    emit stateChanged();
    showBattleOverlay(playerWon, false);
}

void Game::showBattleOverlay(bool won, bool isGameOver)
{
    hideBattleOverlay();

    const QRectF viewRect = m_scene->sceneRect();
    const qreal w = qMin(viewRect.width() * 0.55, 500.0);
    const qreal h = qMin(viewRect.height() * 0.35, 220.0);
    const QRectF overlayRect(
        viewRect.x() + (viewRect.width() - w) / 2,
        viewRect.y() + (viewRect.height() - h) / 2,
        w, h);

    QString title;
    QString subtext;
    if (isGameOver) {
        title = won ? QStringLiteral("\u901A\u5173\uFF01")
                    : QStringLiteral("\u5931\u8D25\u2026");
        subtext = won
            ? QStringLiteral("\u606D\u559C\u4F60\u51FB\u8D25\u4E86\u6240\u6709\u654C\u4EBA\uFF01")
            : QStringLiteral("\u4F60\u7684\u961F\u4F0D\u88AB\u6D88\u706D\u4E86\u3002");
    } else {
        title = won ? QStringLiteral("\u80DC\u5229\uFF01")
                    : QStringLiteral("\u5931\u8D25");
        subtext = won
            ? QStringLiteral("\u8FD9\u4E00\u8F6E\u4F60\u8D62\u4E86\uFF01")
            : QStringLiteral("\u8FD9\u4E00\u8F6E\u4F60\u8F93\u4E86\u3002");
    }

    m_overlayItem = new OverlayItem(overlayRect, title, subtext);
    m_scene->addItem(m_overlayItem);
    connect(m_overlayItem, &OverlayItem::dismissed, this, &Game::hideBattleOverlay);
}

void Game::hideBattleOverlay()
{
    if (!m_overlayItem) {
        return;
    }
    m_scene->removeItem(m_overlayItem);
    delete m_overlayItem;
    m_overlayItem = nullptr;
}

void Game::prepareUnitsForCombat()
{
    m_synergy.applyToPlayerUnits(m_board, *m_player.bench(), m_allUnits);
    for (Unit* unit : m_allUnits) {
        if (!unit) {
            continue;
        }
        unit->resetCombatState();
        if (unit->isAlive() && m_board.getUnitAt(unit->position()) == unit) {
            unit->setMana(0);
        }
    }
}

void Game::cleanupAfterCombat()
{
    QList<Unit*> deadUnits;
    for (Unit* unit : m_allUnits) {
        if (!unit || unit->isAlive()) {
            continue;
        }
        m_board.removeUnit(unit);
        if (unit->owner() == Controller::PlayerCtrl) {
            deadUnits.append(unit);
        }
    }

    for (Unit* unit : deadUnits) {
        const int slot = m_player.bench()->findUnitSlot(unit);
        if (slot >= 0) {
            m_player.bench()->removeUnit(slot);
        }
        auto it = m_unitItemById.find(unit->id());
        if (it != m_unitItemById.end()) {
            m_scene->removeItem(it->second);
            m_unitItems.erase(std::remove(m_unitItems.begin(), m_unitItems.end(), it->second),
                              m_unitItems.end());
            delete it->second;
            m_unitItemById.erase(it);
        }
        m_allUnits.removeOne(unit);
        delete unit;
    }

    restorePlayerDeployment();
    clearEnemyUnits();
    syncFromBoard();
    syncFromBench();
    refreshUnitVisuals();
}

void Game::snapshotPlayerDeployment()
{
    m_preCombatBoardPositions.clear();
    m_preCombatBenchSlots.clear();

    for (Unit* unit : m_allUnits) {
        if (!unit || unit->owner() != Controller::PlayerCtrl) {
            continue;
        }

        const int benchSlot = m_player.bench()->findUnitSlot(unit);
        if (benchSlot >= 0) {
            m_preCombatBenchSlots.insert(unit->id(), benchSlot);
        } else if (m_board.getUnitAt(unit->position()) == unit) {
            m_preCombatBoardPositions.insert(unit->id(), unit->position());
        }
    }
}

void Game::restorePlayerDeployment()
{
    QList<Unit*> survivors;
    for (Unit* unit : m_allUnits) {
        if (unit && unit->owner() == Controller::PlayerCtrl && unit->isAlive()) {
            survivors.append(unit);
        }
    }

    for (Unit* unit : survivors) {
        m_board.removeUnit(unit);
        const int benchSlot = m_player.bench()->findUnitSlot(unit);
        if (benchSlot >= 0) {
            m_player.bench()->removeUnit(benchSlot);
        }
        unit->setHp(unit->maxHp());
        unit->setMana(0);
        unit->resetCombatState();
    }

    for (Unit* unit : survivors) {
        const auto boardIt = m_preCombatBoardPositions.find(unit->id());
        if (boardIt == m_preCombatBoardPositions.end()) {
            continue;
        }

        const QPoint pos = boardIt.value();
        if (m_board.isValidPosition(pos) && m_board.isPlayerHalf(pos) && !m_board.hasUnitAt(pos)) {
            m_board.addUnit(unit, pos);
        } else {
            const QPoint fallback = findFirstEmptyPlayerCell();
            if (fallback.x() >= 0) {
                m_board.addUnit(unit, fallback);
            }
        }
    }

    for (Unit* unit : survivors) {
        if (m_board.getUnitAt(unit->position()) == unit) {
            continue;
        }

        const auto benchIt = m_preCombatBenchSlots.find(unit->id());
        if (benchIt != m_preCombatBenchSlots.end() && !m_player.bench()->hasUnitAt(benchIt.value())) {
            m_player.bench()->addUnitToSlot(unit, benchIt.value());
        } else {
            m_player.bench()->addUnit(unit);
        }
    }
}

QPoint Game::findFirstEmptyPlayerCell() const
{
    for (int row = Board::ROWS / 2; row < Board::ROWS; ++row) {
        for (int col = 0; col < Board::COLS; ++col) {
            const QPoint pos(col, row);
            if (!m_board.hasUnitAt(pos)) {
                return pos;
            }
        }
    }
    return QPoint(-1, -1);
}

void Game::onCombatTick()
{
    if (m_phase != GamePhase::Combat) {
        return;
    }

    m_combat.tick(m_board, m_allUnits);
    drawAttackLines();
    syncFromBoard();
    refreshUnitVisuals();

    bool playerWon = false;
    if (m_combat.checkBattleEnd(m_board, playerWon)) {
        beginResolvePhase(playerWon);
    }

    emit stateChanged();
}

Unit* Game::selectedUnit() const
{
    return findUnitById(m_selectedUnitId);
}

int Game::countPlayerUnitsOnBoard() const
{
    int count = 0;
    for (int row = Board::ROWS / 2; row < Board::ROWS; ++row) {
        for (int col = 0; col < Board::COLS; ++col) {
            Unit* unit = m_board.getUnitAt(QPoint(col, row));
            if (unit && unit->owner() == Controller::PlayerCtrl && unit->isAlive()) {
                ++count;
            }
        }
    }
    return count;
}

bool Game::hasEnemiesOnBoard() const
{
    for (int row = 0; row < Board::ROWS / 2; ++row) {
        for (int col = 0; col < Board::COLS; ++col) {
            Unit* unit = m_board.getUnitAt(QPoint(col, row));
            if (unit && unit->owner() == Controller::EnemyCtrl && unit->isAlive()) {
                return true;
            }
        }
    }
    return false;
}

void Game::setupHeroStats(Unit* unit)
{
    if (!unit) {
        return;
    }
    if (unit->name() == QStringLiteral("\u6218\u58EB")) {
        unit->setMaxHp(450);
        unit->setAtk(40);
        unit->setRange(1);
        unit->setMaxMana(60);
    } else if (unit->name() == QStringLiteral("\u5F13\u624B")) {
        unit->setMaxHp(320);
        unit->setAtk(50);
        unit->setRange(3);
        unit->setMaxMana(60);
    } else if (unit->name() == QStringLiteral("\u6CD5\u5E08")) {
        unit->setMaxHp(280);
        unit->setAtk(70);
        unit->setRange(2);
        unit->setMaxMana(40);
    } else if (unit->name() == QStringLiteral("\u7267\u5E08")) {
        unit->setMaxHp(300);
        unit->setAtk(25);
        unit->setRange(2);
        unit->setMaxMana(50);
    }
    unit->captureBaseStats();
    unit->recalculateStats();
    unit->setHp(unit->maxHp());
    unit->setMana(0);
}

Unit* Game::registerUnit(Unit* unit)
{
    if (unit) {
        m_allUnits.append(unit);
    }
    return unit;
}

void Game::clearEnemyUnits()
{
    QList<Unit*> toRemove;
    for (Unit* unit : m_allUnits) {
        if (unit->owner() == Controller::EnemyCtrl) {
            toRemove.append(unit);
        }
    }

    for (Unit* unit : toRemove) {
        m_board.removeUnit(unit);
        auto it = m_unitItemById.find(unit->id());
        if (it != m_unitItemById.end()) {
            m_scene->removeItem(it->second);
            m_unitItems.erase(std::remove(m_unitItems.begin(), m_unitItems.end(), it->second),
                              m_unitItems.end());
            delete it->second;
            m_unitItemById.erase(it);
        }
        m_allUnits.removeOne(unit);
        delete unit;
    }
}

void Game::spawnEnemiesForRound(int round)
{
    struct EnemyType {
        QString name;
        QString trait;
    };

    const QVector<EnemyType> enemyPool = {
        {QStringLiteral("\u9AB7\u9AC5"), QStringLiteral("\u4EA1\u7075")},
        {QStringLiteral("\u5E7D\u7075"), QStringLiteral("\u6CD5\u5E08")},
        {QStringLiteral("\u6076\u9B54"), QStringLiteral("\u6218\u58EB")},
    };

    int enemyCount = 1;
    if (round >= 10)      enemyCount = 6;
    else if (round >= 8)  enemyCount = 5;
    else if (round >= 6)  enemyCount = 4;
    else if (round >= 4)  enemyCount = 3;
    else if (round >= 2)  enemyCount = 2;

    QVector<QPoint> available;
    for (int y = 0; y < Board::ROWS / 2; ++y) {
        for (int x = 0; x < Board::COLS; ++x) {
            QPoint pos(x, y);
            if (!m_board.hasUnitAt(pos)) {
                available.append(pos);
            }
        }
    }

    auto& rng = *QRandomGenerator::global();
    for (int i = available.size() - 1; i > 0; --i) {
        available.swapItemsAt(i, rng.bounded(i + 1));
    }

    for (int i = 0; i < enemyCount && i < available.size(); ++i) {
        int typeIdx = rng.bounded(enemyPool.size());
        const EnemyType& type = enemyPool[typeIdx];
        Unit* enemy = UnitFactory::createEnemy(type.name, type.trait, round);
        registerUnit(enemy);
        m_board.addUnit(enemy, available[i]);
    }

    if (round == 10 && enemyCount + 1 < available.size()) {
        const QString bossName = QStringLiteral("\u6076\u9B54");
        const QString bossTrait = QStringLiteral("\u6218\u58EB");
        Unit* boss1 = UnitFactory::createEnemy(bossName, bossTrait, round, true);
        registerUnit(boss1);
        m_board.addUnit(boss1, available[enemyCount]);
        Unit* boss2 = UnitFactory::createEnemy(bossName, bossTrait, round, true);
        registerUnit(boss2);
        m_board.addUnit(boss2, available[enemyCount + 1]);
    } else if ((round == 4 || round == 7) && enemyCount < available.size()) {
        const QString bossName = QStringLiteral("\u6076\u9B54");
        const QString bossTrait = QStringLiteral("\u6218\u58EB");
        Unit* boss = UnitFactory::createEnemy(bossName, bossTrait, round, true);
        registerUnit(boss);
        m_board.addUnit(boss, available[enemyCount]);
    }
}

Unit* Game::findUnitById(int unitId) const
{
    for (Unit* unit : m_allUnits) {
        if (unit && unit->id() == unitId) {
            return unit;
        }
    }
    return nullptr;
}

UnitItem* Game::findUnitItem(int unitId) const
{
    auto it = m_unitItemById.find(unitId);
    return it == m_unitItemById.end() ? nullptr : it->second;
}

void Game::buildScene()
{
    m_scene->clear();
    m_gridItems.clear();
    m_benchItems.clear();
    m_unitItems.clear();
    m_unitItemById.clear();

    QRectF totalBounds;
    bool first = true;

    for (int row = 0; row < Board::ROWS; ++row) {
        for (int col = 0; col < Board::COLS; ++col) {
            const QPolygonF poly = m_geometry.cellHexPolygon(row, col);
            GridItem* gridItem = new GridItem(row, col, poly);
            gridItem->setZValue(kZGrid);
            gridItem->setBaseColor(row < Board::ROWS / 2
                ? QColor(90, 55, 55) : QColor(55, 55, 90));

            m_scene->addItem(gridItem);
            m_gridItems.push_back(gridItem);

            const QRectF bounds = gridItem->boundingRect();
            totalBounds = first ? bounds : totalBounds.united(bounds);
            first = false;
        }
    }

    constexpr qreal kSlotSize = BoardGeometry::kBenchSlotSize;
    for (int slot = 0; slot < Bench::BENCH_SIZE; ++slot) {
        const QRectF slotRect(0, 0, kSlotSize, kSlotSize);
        BenchSlotItem* benchItem = new BenchSlotItem(slot, slotRect);
        const QPointF center = m_geometry.benchSlotCenter(slot);
        benchItem->setPos(center - QPointF(kSlotSize / 2, kSlotSize / 2));
        benchItem->setZValue(kZGrid);
        m_scene->addItem(benchItem);
        m_benchItems.push_back(benchItem);
        totalBounds = totalBounds.united(benchItem->sceneBoundingRect());
    }

    const QRectF szRect = m_geometry.sellZoneRect();
    m_sellZoneItem = new QGraphicsRectItem(szRect);
    m_sellZoneItem->setPen(QPen(QColor(200, 60, 60), 2));
    m_sellZoneItem->setBrush(QColor(80, 35, 35));
    m_sellZoneItem->setZValue(kZGrid);
    m_scene->addItem(m_sellZoneItem);

    auto* sellLabel = new QGraphicsSimpleTextItem(QStringLiteral("\u51FA\u552E"));
    sellLabel->setBrush(QColor(220, 80, 80));
    sellLabel->setPos(szRect.center() - QPointF(14, 8));
    sellLabel->setZValue(kZGrid + 0.1);
    m_scene->addItem(sellLabel);

    totalBounds = totalBounds.united(m_sellZoneItem->sceneBoundingRect());

    ensureUnitItems();
    m_scene->setSceneRect(totalBounds.adjusted(-40, -40, 40, 80));
}

void Game::ensureUnitItems()
{
    for (Unit* unit : m_allUnits) {
        if (m_unitItemById.find(unit->id()) != m_unitItemById.end()) {
            continue;
        }

        UnitItem* unitItem = new UnitItem(unit);
        unitItem->setZValue(kZUnit);
        m_scene->addItem(unitItem);
        m_unitItems.push_back(unitItem);
        m_unitItemById[unit->id()] = unitItem;

        connect(unitItem, &UnitItem::dragStarted, this, &Game::handleDragStarted);
        connect(unitItem, &UnitItem::dragMoved, this, &Game::handleDragMoved);
        connect(unitItem, &UnitItem::dragDropped, this, &Game::handleDropOnBoard);
        connect(unitItem, &UnitItem::clicked, this, &Game::handleUnitClicked);
        connect(unitItem, &UnitItem::rightClicked, this, &Game::sellUnit);
    }
}

void Game::refreshUnitVisuals()
{
    for (UnitItem* item : m_unitItems) {
        if (item) {
            item->update();
        }
    }
}

void Game::clearAttackLines()
{
    for (QGraphicsLineItem* line : m_attackLineItems) {
        m_scene->removeItem(line);
        delete line;
    }
    m_attackLineItems.clear();
}

void Game::drawAttackLines()
{
    clearAttackLines();

    for (const auto& rec : m_combat.lastAttackLines()) {
        const QPointF fromWorld = m_geometry.gridToWorld(rec.from.y(), rec.from.x());
        const QPointF toWorld = m_geometry.gridToWorld(rec.to.y(), rec.to.x());

        const qreal dx = toWorld.x() - fromWorld.x();
        const qreal dy = toWorld.y() - fromWorld.y();
        const qreal len = qSqrt(dx * dx + dy * dy);
        if (len < 0.01) {
            continue;
        }
        const qreal ux = dx / len;
        const qreal uy = dy / len;
        const qreal offset = 28.0;
        const QPointF fromEdge(fromWorld.x() + ux * offset, fromWorld.y() + uy * offset);
        const QPointF toEdge(toWorld.x() - ux * offset, toWorld.y() - uy * offset);

        auto* line = new QGraphicsLineItem(
            QLineF(fromEdge, toEdge));
        const QColor lineColor = rec.isHeal
            ? QColor(100, 255, 100, 200)
            : QColor(255, 200, 60, 200);
        line->setPen(QPen(lineColor, 2.5));
        line->setZValue(3.0);
        m_scene->addItem(line);
        m_attackLineItems.push_back(line);
    }
}

void Game::syncFromBoard()
{
    for (UnitItem* item : m_unitItems) {
        if (!item || !item->unit()) {
            continue;
        }

        Unit* unit = item->unit();
        const QPoint pos = unit->position();
        if (!m_board.isValidPosition(pos) || m_board.getUnitAt(pos) != unit) {
            if (m_player.bench()->findUnitSlot(unit) < 0) {
                item->setVisible(!unit->isDead());
            }
            continue;
        }

        item->setVisible(true);
        item->setGridPos(pos);
        item->setBenchSlot(-1);
        item->setPos(m_geometry.gridToWorld(pos.y(), pos.x()));
        item->setZValue(item->unitId() == m_dragDrop.activeUnitId() ? kZDraggingUnit : kZUnit);
        item->update();
    }
}

void Game::syncFromBench()
{
    for (UnitItem* item : m_unitItems) {
        if (!item || !item->unit()) {
            continue;
        }

        Unit* unit = item->unit();
        const int slot = m_player.bench()->findUnitSlot(unit);
        if (slot < 0) {
            if (m_board.getUnitAt(unit->position()) != unit) {
                item->setVisible(!unit->isDead());
            }
            continue;
        }

        item->setVisible(true);
        item->setBenchSlot(slot);
        item->setPos(m_geometry.benchSlotCenter(slot));
        item->setZValue(item->unitId() == m_dragDrop.activeUnitId() ? kZDraggingUnit : kZUnit);
        item->update();
    }
}

void Game::handleUnitClicked(int unitId)
{
    m_selectedUnitId = unitId;
    if (m_selectedEquipSlot >= 0) {
        equipFromBench(m_selectedEquipSlot, unitId);
    }
    emit stateChanged();
}

void Game::sellUnit(int unitId)
{
    if (m_phase != GamePhase::Prep || m_result != GameResult::Playing) {
        return;
    }

    Unit* unit = findUnitById(unitId);
    if (!unit || unit->owner() != Controller::PlayerCtrl || !unit->isAlive()) {
        return;
    }

    int basePrice = 3;
    const QString name = unit->name();
    if (name == QStringLiteral("\u5F13\u624B") || name == QStringLiteral("\u7267\u5E08")) basePrice = 4;
    else if (name == QStringLiteral("\u6CD5\u5E08")) basePrice = 5;

    const int refund = unit->starLevel() == 1 ? basePrice : basePrice * 3;
    m_player.addGold(refund);

    m_board.removeUnit(unit);
    const int benchSlot = m_player.bench()->findUnitSlot(unit);
    if (benchSlot >= 0) {
        m_player.bench()->removeUnit(benchSlot);
    }

    auto it = m_unitItemById.find(unit->id());
    if (it != m_unitItemById.end()) {
        m_scene->removeItem(it->second);
        m_unitItems.erase(std::remove(m_unitItems.begin(), m_unitItems.end(), it->second),
                          m_unitItems.end());
        delete it->second;
        m_unitItemById.erase(it);
    }
    m_allUnits.removeOne(unit);
    delete unit;

    if (m_selectedUnitId == unitId) {
        m_selectedUnitId = -1;
    }

    m_phaseMessage = QStringLiteral("\u5DF2\u51FA\u552E %1\uFF0C\u8FD4\u8FD8 %2 \u91D1\u5E01\u3002").arg(name).arg(refund);
    m_synergy.applyToPlayerUnits(m_board, *m_player.bench(), m_allUnits);
    syncFromBoard();
    syncFromBench();
    emit stateChanged();
}

void Game::handleDragStarted(int unitId, const QPoint& sourceGrid, const QPointF& scenePos)
{
    if (!canDragUnits()) {
        return;
    }

    Unit* unit = findUnitById(unitId);
    if (!unit || unit->owner() != Controller::PlayerCtrl) {
        return;
    }

    const int benchSlot = m_player.bench()->findUnitSlot(unit);
    const QPoint effectiveSource = benchSlot >= 0 ? QPoint(-1, -1) : sourceGrid;
    m_dragDrop.beginDrag(unitId, benchSlot, effectiveSource);

    UnitItem* item = findUnitItem(unitId);
    if (item) {
        item->setZValue(kZDraggingUnit);
        item->setPos(scenePos);
    }
}

void Game::handleDragMoved(int unitId, const QPoint&, const QPointF& scenePos)
{
    if (!m_dragDrop.isActive()) {
        return;
    }

    UnitItem* item = findUnitItem(unitId);
    if (item) {
        item->setPos(scenePos);
    }

    Unit* unit = findUnitById(unitId);
    m_dragDrop.clearGridHighlights(m_gridItems);
    m_dragDrop.clearBenchHighlights(m_benchItems);

    if (m_sellZoneItem) {
        m_sellZoneItem->setBrush(QColor(80, 35, 35));
    }

    if (m_geometry.isSellScenePos(scenePos)) {
        if (m_sellZoneItem && unit && unit->owner() == Controller::PlayerCtrl) {
            m_sellZoneItem->setBrush(QColor(200, 50, 50));
        }
        return;
    }

    if (m_geometry.isBenchScenePos(scenePos)) {
        const int benchSlot = m_geometry.benchSlotAt(scenePos);
        if (benchSlot >= 0) {
            BenchSlotItem* slotItem = m_dragDrop.findBenchSlot(m_benchItems, benchSlot);
            if (slotItem && unit && m_dragDrop.canDropOnBench(unit, benchSlot, true, canDragUnits())) {
                slotItem->setHoverActive(true);
                slotItem->setDropActive(true);
            }
        }
    } else {
        const QPoint target = m_geometry.worldToGrid(scenePos);
        GridItem* targetItem = m_dragDrop.findGridItem(m_gridItems, target);
        if (targetItem && unit) {
            const bool canDrag = canDragUnits();
            if (m_dragDrop.canDropOnBoard(unit, m_dragDrop.sourceGrid(), target, true, canDrag)) {
                targetItem->setHoverActive(true);
                targetItem->setDropActive(true);
            }
        }
    }
}

void Game::handleDropOnBoard(int unitId, const QPoint& source, const QPointF& scenePos)
{
    if (!m_dragDrop.isActive()) {
        return;
    }

    Unit* unit = findUnitById(unitId);
    m_dragDrop.clearGridHighlights(m_gridItems);
    m_dragDrop.clearBenchHighlights(m_benchItems);

    if (m_sellZoneItem) {
        m_sellZoneItem->setBrush(QColor(80, 35, 35));
    }

    if (unit && canDragUnits() && m_geometry.isSellScenePos(scenePos)) {
        sellUnit(unitId);
    } else if (unit && canDragUnits()) {
        const bool fromBench = m_dragDrop.activeBenchSlot() >= 0;
        const QPoint effectiveSource = fromBench ? QPoint(-1, -1) : source;
        bool handled = false;

        if (m_geometry.isBenchScenePos(scenePos)) {
            const int benchSlot = m_geometry.benchSlotAt(scenePos);
            if (benchSlot >= 0) {
                const int srcBench = m_dragDrop.activeBenchSlot();
                if (m_dragDrop.canDropOnBench(unit, benchSlot, true, true)) {
                    m_dragDrop.applyBenchDrop(unit, srcBench, benchSlot);
                    handled = true;
                }
            }
            if (!handled) {
                const QPoint boardTarget = m_geometry.worldToGrid(scenePos);
                if (m_dragDrop.canDropOnBoard(unit, effectiveSource, boardTarget, !fromBench, true)) {
                    m_dragDrop.applyBoardDrop(unit, effectiveSource, boardTarget);
                }
            }
        } else {
            const QPoint boardTarget = m_geometry.worldToGrid(scenePos);
            if (m_dragDrop.canDropOnBoard(unit, effectiveSource, boardTarget, !fromBench, true)) {
                m_dragDrop.applyBoardDrop(unit, effectiveSource, boardTarget);
                handled = true;
            }
            if (!handled) {
                const int benchSlot = m_geometry.benchSlotAt(scenePos);
                if (benchSlot >= 0) {
                    const int srcBench = m_dragDrop.activeBenchSlot();
                    if (m_dragDrop.canDropOnBench(unit, benchSlot, true, true)) {
                        m_dragDrop.applyBenchDrop(unit, srcBench, benchSlot);
                    }
                }
            }
        }
    }

    UnitItem* item = findUnitItem(m_dragDrop.activeUnitId());
    if (item) {
        item->setZValue(kZUnit);
    }

    m_dragDrop.endDrag();

    syncFromBoard();
    syncFromBench();
    emit stateChanged();
}

QString Game::synergySummary() const
{
    return m_synergy.summary(m_board, *m_player.bench(), m_allUnits);
}

bool Game::buyFromShop(int slot)
{
    if (m_phase != GamePhase::Prep || m_result != GameResult::Playing) {
        return false;
    }
    if (!m_shop.isSlotValid(slot) || m_shop.offerAt(slot).isEmpty()) {
        return false;
    }
    if (m_player.bench()->isFull()) {
        m_phaseMessage = QStringLiteral("\u5907\u6218\u533A\u5DF2\u6EE1\uFF0C\u65E0\u6CD5\u8D2D\u4E70\u3002");
        return false;
    }

    const int cost = m_shop.costAt(slot);
    if (!m_player.spendGold(cost)) {
        m_phaseMessage = QStringLiteral("\u91D1\u5E01\u4E0D\u8DB3\u3002");
        return false;
    }

    Unit* unit = UnitFactory::createHero(m_shop.offerAt(slot), Controller::PlayerCtrl);
    setupHeroStats(unit);
    registerUnit(unit);
    m_player.bench()->addUnit(unit);
    m_shop.clearSlot(slot);
    ensureUnitItems();
    tryAutoMerge(unit);
    m_synergy.applyToPlayerUnits(m_board, *m_player.bench(), m_allUnits);
    syncFromBench();
    m_phaseMessage = QStringLiteral("\u8D2D\u4E70\u6210\u529F\uFF1A%1").arg(unit->displayName());
    emit stateChanged();
    return true;
}

bool Game::refreshShop()
{
    if (m_phase != GamePhase::Prep || m_result != GameResult::Playing) {
        return false;
    }
    if (!m_player.spendGold(2)) {
        m_phaseMessage = QStringLiteral("\u5237\u65B0\u5546\u5E97\u9700\u8981 2 \u91D1\u5E01\u3002");
        return false;
    }
    m_shop.refresh();
    m_phaseMessage = QStringLiteral("\u5546\u5E97\u5DF2\u5237\u65B0\u3002");
    emit stateChanged();
    return true;
}

bool Game::upgradePopulation()
{
    if (m_phase != GamePhase::Prep || m_result != GameResult::Playing) {
        return false;
    }
    if (!m_player.upgradePopulationCap()) {
        m_phaseMessage = QStringLiteral("\u65E0\u6CD5\u5347\u7EA7\u4EBA\u53E3\uFF08\u91D1\u5E01\u4E0D\u8DB3\u6216\u5DF2\u8FBE\u4E0A\u9650\uFF09\u3002");
        return false;
    }
    m_phaseMessage = QStringLiteral("\u4EBA\u53E3\u4E0A\u9650\u63D0\u5347\u81F3 %1\u3002").arg(m_player.populationCap());
    emit stateChanged();
    return true;
}

bool Game::equipFromBench(int equipSlot, int unitId)
{
    if (m_phase != GamePhase::Prep) {
        return false;
    }
    Unit* unit = findUnitById(unitId);
    if (!unit || unit->owner() != Controller::PlayerCtrl) {
        return false;
    }

    const EquipType type = m_equipBench.take(equipSlot);
    if (type == EquipType::None) {
        return false;
    }
    if (!unit->equipItem(type)) {
        m_equipBench.add(type);
        m_phaseMessage = QStringLiteral("\u8BE5\u5355\u4F4D\u65E0\u6CD5\u7A7F\u6234\u66F4\u591A\u88C5\u5907\u3002");
        return false;
    }
    m_selectedEquipSlot = -1;
    m_phaseMessage = QStringLiteral("\u5DF2\u4E3A %1 \u88C5\u5907 %2").arg(unit->displayName(), Equipment::info(type).name);
    emit stateChanged();
    return true;
}

void Game::tryAutoMerge(Unit* acquired)
{
    if (!acquired || acquired->starLevel() != 1) {
        return;
    }

    QList<Unit*> matches;
    for (Unit* unit : m_allUnits) {
        if (unit && unit->owner() == Controller::PlayerCtrl && unit->starLevel() == 1
            && unit->name() == acquired->name()) {
            matches.append(unit);
        }
    }

    if (matches.size() < 3) {
        return;
    }

    Unit* keeper = acquired;
    int removed = 0;
    for (Unit* unit : matches) {
        if (unit == keeper) {
            continue;
        }
        m_board.removeUnit(unit);
        const int slot = m_player.bench()->findUnitSlot(unit);
        if (slot >= 0) {
            m_player.bench()->removeUnit(slot);
        }
        auto it = m_unitItemById.find(unit->id());
        if (it != m_unitItemById.end()) {
            m_scene->removeItem(it->second);
            m_unitItems.erase(std::remove(m_unitItems.begin(), m_unitItems.end(), it->second),
                              m_unitItems.end());
            delete it->second;
            m_unitItemById.erase(it);
        }
        m_allUnits.removeOne(unit);
        delete unit;
        if (++removed >= 2) {
            break;
        }
    }

    keeper->upgradeToTwoStar();
    m_phaseMessage = QStringLiteral("\u5347\u661F\u6210\u529F\uFF1A%1").arg(keeper->displayName());
}

void Game::tryDropEquipment(bool playerWon)
{
    if (!playerWon) {
        return;
    }
    if (QRandomGenerator::global()->bounded(100) >= 45) {
        return;
    }
    if (m_equipBench.add(Equipment::randomDrop())) {
        m_phaseMessage += QStringLiteral(" \u83B7\u5F97\u88C5\u5907\u6389\u843D\uFF01");
        m_equipDropMessage = QStringLiteral("\u6389\u843D\u88C5\u5907!");
    }
}

static QJsonObject unitToJson(const Unit* unit, const QPoint& boardPos, int benchSlot)
{
    QJsonObject o;
    o[QStringLiteral("name")] = unit->name();
    o[QStringLiteral("star")] = unit->starLevel();
    o[QStringLiteral("hp")] = unit->hp();
    o[QStringLiteral("equip")] = static_cast<int>(unit->equipment());
    o[QStringLiteral("traits")] = QJsonArray::fromStringList(
        QStringList(unit->traits().begin(), unit->traits().end()));
    if (boardPos.x() >= 0) {
        o[QStringLiteral("bx")] = boardPos.x();
        o[QStringLiteral("by")] = boardPos.y();
    }
    if (benchSlot >= 0) {
        o[QStringLiteral("bench")] = benchSlot;
    }
    return o;
}

QJsonObject Game::toJson() const
{
    QJsonObject root;
    root[QStringLiteral("hp")] = m_player.hp();
    root[QStringLiteral("gold")] = m_player.gold();
    root[QStringLiteral("round")] = m_player.currentRound();
    root[QStringLiteral("popUpgrades")] = m_player.populationUpgradeCount();
    root[QStringLiteral("popCap")] = m_player.populationCap();
    root[QStringLiteral("result")] = static_cast<int>(m_result);

    QJsonArray units;
    for (Unit* unit : m_allUnits) {
        if (!unit || unit->owner() != Controller::PlayerCtrl) {
            continue;
        }
        const int benchSlot = m_player.bench()->findUnitSlot(unit);
        QPoint pos(-1, -1);
        if (m_board.getUnitAt(unit->position()) == unit) {
            pos = unit->position();
        }
        units.append(unitToJson(unit, pos, benchSlot));
    }
    root[QStringLiteral("units")] = units;

    QJsonArray equips;
    for (EquipType t : m_equipBench.all()) {
        equips.append(static_cast<int>(t));
    }
    root[QStringLiteral("equips")] = equips;

    QJsonArray shopOffers;
    QJsonArray shopCosts;
    for (int i = 0; i < Shop::SLOT_COUNT; ++i) {
        shopOffers.append(m_shop.offerAt(i));
        shopCosts.append(m_shop.costAt(i));
    }
    root[QStringLiteral("shopOffers")] = shopOffers;
    root[QStringLiteral("shopCosts")] = shopCosts;
    return root;
}

bool Game::saveToFile(const QString& path, QString* errorMessage) const
{
    try {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("\u65E0\u6CD5\u5199\u5165\u5B58\u6863\u6587\u4EF6\u3002");
            }
            return false;
        }
        const QJsonDocument doc(toJson());
        file.write(doc.toJson(QJsonDocument::Indented));
        return true;
    } catch (const std::exception& e) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("\u5B58\u6863\u5F02\u5E38: %1").arg(e.what());
        }
        return false;
    } catch (...) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("\u5B58\u6863\u51FA\u73B0\u672A\u77E5\u5F02\u5E38\u3002");
        }
        return false;
    }
}

bool Game::loadFromFile(const QString& path, QString* errorMessage)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("\u65E0\u6CD5\u8BFB\u53D6\u5B58\u6863\u6587\u4EF6\u3002");
        }
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("\u5B58\u6863\u683C\u5F0F\u9519\u8BEF\u3002");
        }
        return false;
    }

    const QJsonObject root = doc.object();

    try {
        m_combatTimer->stop();
        m_board.clear();
        for (Unit* u : m_allUnits) {
            auto it = m_unitItemById.find(u->id());
            if (it != m_unitItemById.end()) {
                m_scene->removeItem(it->second);
                delete it->second;
            }
            delete u;
        }
        m_allUnits.clear();
        m_unitItems.clear();
        m_unitItemById.clear();
        m_equipBench.clear();
        m_player.bench()->clear();

        m_player.reset();
        m_player.setHp(root.value(QStringLiteral("hp")).toInt(m_player.maxHp()));
        m_player.setGold(root.value(QStringLiteral("gold")).toInt(15));
        m_player.setPopulationCap(root.value(QStringLiteral("popCap")).toInt(4));
        m_player.setCurrentRound(root.value(QStringLiteral("round")).toInt(1));
        m_player.setPopulationUpgradeCount(root.value(QStringLiteral("popUpgrades")).toInt(0));

        const QJsonArray units = root.value(QStringLiteral("units")).toArray();
        for (const QJsonValue& val : units) {
            const QJsonObject o = val.toObject();
            Unit* unit = UnitFactory::createHero(
                o.value(QStringLiteral("name")).toString(), Controller::PlayerCtrl);
            setupHeroStats(unit);
            unit->setStarLevel(o.value(QStringLiteral("star")).toInt(1));
            unit->recalculateStats();
            unit->setHp(o.value(QStringLiteral("hp")).toInt(unit->maxHp()));
            unit->equipItem(static_cast<EquipType>(o.value(QStringLiteral("equip")).toInt(0)));

            registerUnit(unit);
            if (o.contains(QStringLiteral("bench"))) {
                m_player.bench()->addUnitToSlot(unit, o.value(QStringLiteral("bench")).toInt());
            } else if (o.contains(QStringLiteral("bx"))) {
                const QPoint pos(o.value(QStringLiteral("bx")).toInt(),
                                 o.value(QStringLiteral("by")).toInt());
                m_board.addUnit(unit, pos);
            } else {
                m_player.bench()->addUnit(unit);
            }
        }

        const QJsonArray equips = root.value(QStringLiteral("equips")).toArray();
        for (const QJsonValue& e : equips) {
            m_equipBench.add(static_cast<EquipType>(e.toInt()));
        }

        const QJsonArray shopOffers = root.value(QStringLiteral("shopOffers")).toArray();
        const QJsonArray shopCosts = root.value(QStringLiteral("shopCosts")).toArray();
        for (int i = 0; i < Shop::SLOT_COUNT; ++i) {
            if (i < shopOffers.size() && !shopOffers.at(i).toString().isEmpty()) {
                m_shop.setSlot(i, shopOffers.at(i).toString(),
                               shopCosts.at(i).toInt(3));
            } else {
                m_shop.clearSlot(i);
            }
        }

        m_result = static_cast<GameResult>(root.value(QStringLiteral("result")).toInt(0));
        clearEnemyUnits();
        beginPrepPhase();
        m_phaseMessage = QStringLiteral("\u5B58\u6863\u5DF2\u52A0\u8F7D\u3002");
        emit stateChanged();
        return true;
    } catch (const std::exception& e) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("\u8BFB\u6863\u5F02\u5E38: %1").arg(e.what());
        }
        reset();
        return false;
    } catch (...) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("\u8BFB\u6863\u51FA\u73B0\u672A\u77E5\u5F02\u5E38\u3002");
        }
        reset();
        return false;
    }
}
