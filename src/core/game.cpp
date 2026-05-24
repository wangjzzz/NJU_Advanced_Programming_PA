#include "game.h"
#include <algorithm>
#include "core/gamesave.h"
#include "core/unitfactory.h"
#include "entity/unit.h"
#include "gui/benchslotitem.h"
#include "gui/griditem.h"
#include "gui/unititem.h"
#include <QFile>
#include <QGraphicsScene>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QtMath>

namespace {
constexpr qreal kZGrid = 0.0;
constexpr qreal kZUnit = 1.0;
constexpr qreal kZDraggingUnit = 2.0;
constexpr qreal kBenchSlotSize = 45.0;
}

Game::Game(QObject* parent)
    : QObject(parent)
    , m_scene(new QGraphicsScene(this))
    , m_combatTimer(new QTimer(this))
    , m_phase(GamePhase::Prep)
    , m_result(GameResult::Playing)
    , m_lastCombatWon(false)
    , m_dragActive(false)
    , m_activeUnitId(-1)
    , m_activeBenchSlot(-1)
    , m_sourceGrid(-1, -1)
    , m_selectedUnitId(-1)
    , m_selectedEquipSlot(-1)
    , m_rows(Board::ROWS)
    , m_cols(Board::COLS)
    , m_radius(46.0)
    , m_rowSpacing(69.0)
    , m_benchY(Board::ROWS * 69.0 + 30.0)
    , m_benchSpacing(55.0)
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
        m_phaseMessage = QStringLiteral("请至少将一个单位部署到棋盘下半区后再开始战斗。");
        emit stateChanged();
        return;
    }
    if (!hasEnemiesOnBoard()) {
        spawnEnemiesForRound(m_player.currentRound());
    }

    snapshotPlayerDeployment();
    prepareUnitsForCombat();
    m_phase = GamePhase::Combat;
    m_phaseMessage = QStringLiteral("战斗进行中…");
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

    m_shop.refresh();
    m_synergy.applyToPlayerUnits(m_board, *m_player.bench(), m_allUnits);

    if (m_result == GameResult::Playing) {
        m_phaseMessage = QStringLiteral("准备阶段：购买/布阵/装备，然后「开始战斗」。刷新商店2金，升级人口递增。");
    }
}

void Game::beginResolvePhase(bool playerWon)
{
    m_combatTimer->stop();
    m_phase = GamePhase::Resolve;
    m_lastCombatWon = playerWon;

    const int round = m_player.currentRound();
    if (playerWon) {
        m_player.addGold(CombatConst::kGoldWinBase + round * 2);
        m_phaseMessage = QStringLiteral("战斗胜利！获得金币。");
    } else {
        m_player.takeDamage(CombatConst::kPlayerDamageOnLoss + (round - 1) * 2);
        m_player.addGold(CombatConst::kGoldLoseBase);
        m_phaseMessage = QStringLiteral("战斗失败，玩家受到伤害。");
    }

    cleanupAfterCombat();
    tryDropEquipment(playerWon);
    m_synergy.applyToPlayerUnits(m_board, *m_player.bench(), m_allUnits);

    if (!m_player.isAlive()) {
        m_result = GameResult::Defeat;
        m_phaseMessage = QStringLiteral("游戏失败：玩家生命值归零。");
        emit stateChanged();
        return;
    }

    if (playerWon && round >= CombatConst::kMaxRounds) {
        m_result = GameResult::Victory;
        m_phaseMessage = QStringLiteral("恭喜通关！你击败了全部 %1 轮敌人。").arg(CombatConst::kMaxRounds);
        emit stateChanged();
        return;
    }

    if (playerWon) {
        m_player.advanceRound();
    }

    clearEnemyUnits();
    beginPrepPhase();

    if (playerWon) {
        m_phaseMessage = QStringLiteral("上轮胜利！进入第 %1 轮准备阶段。").arg(m_player.currentRound());
    } else {
        m_phaseMessage = QStringLiteral("上轮失败，重新部署后继续第 %1 轮。").arg(m_player.currentRound());
    }
    emit stateChanged();
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
    if (unit->name() == QStringLiteral("战士")) {
        unit->setMaxHp(450);
        unit->setAtk(40);
        unit->setRange(1);
        unit->setMaxMana(60);
    } else if (unit->name() == QStringLiteral("弓手")) {
        unit->setMaxHp(320);
        unit->setAtk(50);
        unit->setRange(3);
        unit->setMaxMana(60);
    } else if (unit->name() == QStringLiteral("法师")) {
        unit->setMaxHp(280);
        unit->setAtk(70);
        unit->setRange(2);
        unit->setMaxMana(40);
    } else if (unit->name() == QStringLiteral("牧师")) {
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
    struct SpawnInfo {
        QString name;
        QString trait;
        QPoint pos;
        bool fromRound2;
    };

    const QVector<SpawnInfo> table = {
        {QStringLiteral("骷髅"), QStringLiteral("亡灵"), QPoint(3, 1), false},
        {QStringLiteral("幽灵"), QStringLiteral("法师"), QPoint(4, 2), false},
        {QStringLiteral("恶魔"), QStringLiteral("战士"), QPoint(2, 0), true},
    };

    for (const SpawnInfo& info : table) {
        if (info.fromRound2 && round < 2) {
            continue;
        }
        if (round == 1 && info.name == QStringLiteral("幽灵")) {
            continue;
        }
        if (m_board.hasUnitAt(info.pos)) {
            continue;
        }

        Unit* enemy = UnitFactory::createEnemy(info.name, info.trait, round);
        registerUnit(enemy);
        m_board.addUnit(enemy, info.pos);
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

GridItem* Game::findGridItem(const QPoint& gridPos) const
{
    for (GridItem* item : m_gridItems) {
        if (item && item->gridPos() == gridPos) {
            return item;
        }
    }
    return nullptr;
}

UnitItem* Game::findUnitItem(int unitId) const
{
    auto it = m_unitItemById.find(unitId);
    return it == m_unitItemById.end() ? nullptr : it->second;
}

BenchSlotItem* Game::findBenchSlot(int slot) const
{
    for (BenchSlotItem* item : m_benchItems) {
        if (item && item->slot() == slot) {
            return item;
        }
    }
    return nullptr;
}

void Game::clearGridHighlights()
{
    for (GridItem* item : m_gridItems) {
        if (item) {
            item->setHoverActive(false);
            item->setDropActive(false);
        }
    }
}

void Game::clearBenchHighlights()
{
    for (BenchSlotItem* item : m_benchItems) {
        if (item) {
            item->setHoverActive(false);
            item->setDropActive(false);
        }
    }
}

bool Game::isBenchScenePos(const QPointF& scenePos) const
{
    return scenePos.y() >= m_benchY && scenePos.y() < m_benchY + kBenchSlotSize;
}

int Game::benchSlotAt(const QPointF& scenePos) const
{
    if (!isBenchScenePos(scenePos)) {
        return -1;
    }
    return qBound(0, static_cast<int>(scenePos.x() / m_benchSpacing), Bench::BENCH_SIZE - 1);
}

bool Game::canDropOnBoard(Unit* unit, const QPoint& source, const QPoint& target, bool allowSwap) const
{
    if (!canDragUnits() || !unit || unit->owner() != Controller::PlayerCtrl) {
        return false;
    }
    if (!m_board.isValidPosition(target) || !m_board.isPlayerHalf(target)) {
        return false;
    }

    Unit* occupant = m_board.getUnitAt(target);
    if (occupant) {
        return allowSwap && occupant != unit && occupant->owner() == Controller::PlayerCtrl
               && source != QPoint(-1, -1) && m_board.getUnitAt(source) == unit;
    }

    if (source == QPoint(-1, -1)) {
        if (m_player.bench()->findUnitSlot(unit) < 0) {
            return false;
        }
        if (countPlayerUnitsOnBoard() >= m_player.populationCap()) {
            return false;
        }
        return true;
    }

    return m_board.isValidPosition(source) && m_board.isPlayerHalf(source)
           && m_board.getUnitAt(source) == unit && source != target;
}

bool Game::canDropOnBench(Unit* unit, int slot, bool allowSwap) const
{
    if (!canDragUnits() || !unit || unit->owner() != Controller::PlayerCtrl
        || !m_player.bench()->isSlotValid(slot)) {
        return false;
    }

    Unit* occupant = m_player.bench()->getUnitAt(slot);
    if (occupant) {
        return allowSwap && occupant != unit;
    }
    return true;
}

void Game::applyBoardDrop(Unit* unit, const QPoint& source, const QPoint& target)
{
    Unit* occupant = m_board.getUnitAt(target);
    if (occupant && occupant != unit) {
        m_board.removeUnit(unit);
        m_board.removeUnit(occupant);
        m_board.addUnit(unit, target);
        m_board.addUnit(occupant, source);
        return;
    }

    if (source == QPoint(-1, -1)) {
        const int benchSlot = m_player.bench()->findUnitSlot(unit);
        if (benchSlot >= 0) {
            m_player.bench()->removeUnit(benchSlot);
        }
    } else {
        m_board.removeUnit(unit);
    }
    m_board.addUnit(unit, target);
}

void Game::applyBenchDrop(Unit* unit, int sourceBenchSlot, int targetBenchSlot)
{
    Unit* occupant = m_player.bench()->getUnitAt(targetBenchSlot);
    if (occupant && occupant != unit) {
        m_player.bench()->swapUnits(sourceBenchSlot, targetBenchSlot);
        return;
    }

    if (sourceBenchSlot >= 0) {
        m_player.bench()->removeUnit(sourceBenchSlot);
    } else {
        m_board.removeUnit(unit);
    }
    m_player.bench()->addUnitToSlot(unit, targetBenchSlot);
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
            const QPolygonF poly = cellHexPolygon(row, col);
            GridItem* gridItem = new GridItem(row, col, poly);
            gridItem->setZValue(kZGrid);
            gridItem->setBaseColor(row < Board::ROWS / 2 ? QColor(90, 55, 55) : QColor(55, 55, 90));

            m_scene->addItem(gridItem);
            m_gridItems.push_back(gridItem);

            const QRectF bounds = gridItem->boundingRect();
            totalBounds = first ? bounds : totalBounds.united(bounds);
            first = false;
        }
    }

    for (int slot = 0; slot < Bench::BENCH_SIZE; ++slot) {
        const QRectF slotRect(0, 0, kBenchSlotSize, kBenchSlotSize);
        BenchSlotItem* benchItem = new BenchSlotItem(slot, slotRect);
        benchItem->setPos(benchSlotCenter(slot) - QPointF(kBenchSlotSize / 2, kBenchSlotSize / 2));
        benchItem->setZValue(kZGrid);
        m_scene->addItem(benchItem);
        m_benchItems.push_back(benchItem);
        totalBounds = totalBounds.united(benchItem->sceneBoundingRect());
    }

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
        item->setPos(gridToWorld(pos.y(), pos.x()));
        item->setZValue(item->unitId() == m_activeUnitId ? kZDraggingUnit : kZUnit);
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
        item->setPos(benchSlotCenter(slot));
        item->setZValue(item->unitId() == m_activeUnitId ? kZDraggingUnit : kZUnit);
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

void Game::handleDragStarted(int unitId, const QPoint& sourceGrid, const QPointF& scenePos)
{
    if (!canDragUnits()) {
        return;
    }

    Unit* unit = findUnitById(unitId);
    if (!unit || unit->owner() != Controller::PlayerCtrl) {
        return;
    }

    m_dragActive = true;
    m_activeUnitId = unitId;
    m_activeBenchSlot = m_player.bench()->findUnitSlot(unit);
    m_sourceGrid = m_activeBenchSlot >= 0 ? QPoint(-1, -1) : sourceGrid;

    UnitItem* item = findUnitItem(unitId);
    if (item) {
        item->setZValue(kZDraggingUnit);
        item->setPos(scenePos);
    }
}

void Game::handleDragMoved(int unitId, const QPoint&, const QPointF& scenePos)
{
    if (!m_dragActive) {
        return;
    }

    UnitItem* item = findUnitItem(unitId);
    if (item) {
        item->setPos(scenePos);
    }

    Unit* unit = findUnitById(unitId);
    clearGridHighlights();
    clearBenchHighlights();

    const QPoint target = worldToGrid(scenePos);
    GridItem* targetItem = findGridItem(target);
    if (targetItem && unit && canDropOnBoard(unit, m_sourceGrid, target, true)) {
        targetItem->setHoverActive(true);
        targetItem->setDropActive(true);
    }

    const int benchSlot = benchSlotAt(scenePos);
    if (benchSlot >= 0) {
        BenchSlotItem* slotItem = findBenchSlot(benchSlot);
        if (slotItem && unit && canDropOnBench(unit, benchSlot, true)) {
            slotItem->setHoverActive(true);
            slotItem->setDropActive(true);
        }
    }
}

void Game::handleDropOnBoard(int unitId, const QPoint& source, const QPointF& scenePos)
{
    if (!m_dragActive) {
        return;
    }

    Unit* unit = findUnitById(unitId);
    clearGridHighlights();
    clearBenchHighlights();

    if (unit && canDragUnits()) {
        const QPoint boardTarget = worldToGrid(scenePos);
        const bool fromBench = m_activeBenchSlot >= 0;
        if (canDropOnBoard(unit, fromBench ? QPoint(-1, -1) : source, boardTarget, !fromBench)) {
            applyBoardDrop(unit, fromBench ? QPoint(-1, -1) : source, boardTarget);
        } else {
            const int benchSlot = benchSlotAt(scenePos);
            if (benchSlot >= 0 && canDropOnBench(unit, benchSlot, true)) {
                applyBenchDrop(unit, m_activeBenchSlot, benchSlot);
            }
        }
    }

    m_activeBenchSlot = -1;

    UnitItem* item = findUnitItem(m_activeUnitId);
    if (item) {
        item->setZValue(kZUnit);
    }

    m_dragActive = false;
    m_activeUnitId = -1;
    m_sourceGrid = QPoint(-1, -1);

    syncFromBoard();
    syncFromBench();
    emit stateChanged();
}

QPointF Game::gridToWorld(int row, int col) const
{
    const qreal colSpacing = m_radius * qSqrt(3.0);
    const qreal xOffset = (row % 2 == 0) ? colSpacing * 0.5 : 0.0;
    return QPointF(xOffset + col * colSpacing, row * m_rowSpacing);
}

QPoint Game::worldToGrid(const QPointF& world) const
{
    QPoint best(-1, -1);
    qreal bestDist = 1e18;

    for (int row = 0; row < m_rows; ++row) {
        for (int col = 0; col < m_cols; ++col) {
            const QPointF center = gridToWorld(row, col);
            const qreal dx = world.x() - center.x();
            const qreal dy = world.y() - center.y();
            const qreal d2 = dx * dx + dy * dy;
            if (d2 < bestDist) {
                bestDist = d2;
                best = QPoint(col, row);
            }
        }
    }
    return best;
}

QPolygonF Game::cellHexPolygon(int row, int col) const
{
    const QPointF center = gridToWorld(row, col);
    QPolygonF poly;
    poly.reserve(6);

    for (int i = 0; i < 6; ++i) {
        const qreal angleDeg = 60.0 * i - 90.0;
        const qreal angleRad = qDegreesToRadians(angleDeg);
        poly.append(QPointF(
            center.x() + m_radius * qCos(angleRad),
            center.y() + m_radius * qSin(angleRad)));
    }
    return poly;
}

QPointF Game::benchSlotCenter(int slot) const
{
    return QPointF(slot * m_benchSpacing + m_benchSpacing / 2, m_benchY + kBenchSlotSize / 2);
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
        m_phaseMessage = QStringLiteral("备战区已满，无法购买。");
        return false;
    }

    const int cost = m_shop.costAt(slot);
    if (!m_player.spendGold(cost)) {
        m_phaseMessage = QStringLiteral("金币不足。");
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
    m_phaseMessage = QStringLiteral("购买成功：%1").arg(unit->displayName());
    emit stateChanged();
    return true;
}

bool Game::refreshShop()
{
    if (m_phase != GamePhase::Prep || m_result != GameResult::Playing) {
        return false;
    }
    if (!m_player.spendGold(2)) {
        m_phaseMessage = QStringLiteral("刷新商店需要 2 金币。");
        return false;
    }
    m_shop.refresh();
    m_phaseMessage = QStringLiteral("商店已刷新。");
    emit stateChanged();
    return true;
}

bool Game::upgradePopulation()
{
    if (m_phase != GamePhase::Prep || m_result != GameResult::Playing) {
        return false;
    }
    if (!m_player.upgradePopulationCap()) {
        m_phaseMessage = QStringLiteral("无法升级人口（金币不足或已达上限）。");
        return false;
    }
    m_phaseMessage = QStringLiteral("人口上限提升至 %1。").arg(m_player.populationCap());
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
        m_phaseMessage = QStringLiteral("该单位无法穿戴更多装备。");
        return false;
    }
    m_selectedEquipSlot = -1;
    m_phaseMessage = QStringLiteral("已为 %1 装备 %2").arg(unit->displayName(), Equipment::info(type).name);
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
    m_phaseMessage = QStringLiteral("升星成功：%1").arg(keeper->displayName());
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
        m_phaseMessage += QStringLiteral(" 获得装备掉落！");
    }
}

static QJsonObject unitToJson(const Unit* unit, const QPoint& boardPos, int benchSlot)
{
    QJsonObject o;
    o[QStringLiteral("name")] = unit->name();
    o[QStringLiteral("star")] = unit->starLevel();
    o[QStringLiteral("hp")] = unit->hp();
    o[QStringLiteral("equip")] = static_cast<int>(unit->equipment());
    o[QStringLiteral("traits")] = QJsonArray::fromStringList(QStringList(unit->traits().begin(), unit->traits().end()));
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
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("无法写入存档文件。");
        }
        return false;
    }
    const QJsonDocument doc(toJson());
    file.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

bool Game::loadFromFile(const QString& path, QString* errorMessage)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("无法读取存档文件。");
        }
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("存档格式错误。");
        }
        return false;
    }

    const QJsonObject root = doc.object();

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
        Unit* unit = UnitFactory::createHero(o.value(QStringLiteral("name")).toString(), Controller::PlayerCtrl);
        setupHeroStats(unit);
        unit->setStarLevel(o.value(QStringLiteral("star")).toInt(1));
        unit->recalculateStats();
        unit->setHp(o.value(QStringLiteral("hp")).toInt(unit->maxHp()));
        unit->equipItem(static_cast<EquipType>(o.value(QStringLiteral("equip")).toInt(0)));

        registerUnit(unit);
        if (o.contains(QStringLiteral("bench"))) {
            m_player.bench()->addUnitToSlot(unit, o.value(QStringLiteral("bench")).toInt());
        } else if (o.contains(QStringLiteral("bx"))) {
            const QPoint pos(o.value(QStringLiteral("bx")).toInt(), o.value(QStringLiteral("by")).toInt());
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
            m_shop.setSlot(i, shopOffers.at(i).toString(), shopCosts.at(i).toInt(3));
        } else {
            m_shop.clearSlot(i);
        }
    }

    m_result = static_cast<GameResult>(root.value(QStringLiteral("result")).toInt(0));
    clearEnemyUnits();
    beginPrepPhase();
    m_phaseMessage = QStringLiteral("存档已加载。");
    emit stateChanged();
    return true;
}
