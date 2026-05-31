#include "core/dragdrophandler.h"
#include "entity/unit.h"
#include "gui/benchslotitem.h"
#include "gui/griditem.h"

DragDropHandler::DragDropHandler(Board& board, Player& player)
    : m_board(board)
    , m_player(player)
    , m_dragActive(false)
    , m_activeUnitId(-1)
    , m_activeBenchSlot(-1)
    , m_sourceGrid(-1, -1)
{
}

void DragDropHandler::beginDrag(int unitId, int benchSlot, const QPoint& sourceGrid)
{
    m_dragActive = true;
    m_activeUnitId = unitId;
    m_activeBenchSlot = benchSlot;
    m_sourceGrid = sourceGrid;
}

void DragDropHandler::endDrag()
{
    m_dragActive = false;
    m_activeUnitId = -1;
    m_activeBenchSlot = -1;
    m_sourceGrid = QPoint(-1, -1);
}

GridItem* DragDropHandler::findGridItem(const std::vector<GridItem*>& items,
                                        const QPoint& gridPos) const
{
    for (GridItem* item : items) {
        if (item && item->gridPos() == gridPos) {
            return item;
        }
    }
    return nullptr;
}

BenchSlotItem* DragDropHandler::findBenchSlot(const std::vector<BenchSlotItem*>& items,
                                              int slot) const
{
    for (BenchSlotItem* item : items) {
        if (item && item->slot() == slot) {
            return item;
        }
    }
    return nullptr;
}

void DragDropHandler::clearGridHighlights(std::vector<GridItem*>& items) const
{
    for (GridItem* item : items) {
        if (item) {
            item->setHoverActive(false);
            item->setDropActive(false);
        }
    }
}

void DragDropHandler::clearBenchHighlights(std::vector<BenchSlotItem*>& items) const
{
    for (BenchSlotItem* item : items) {
        if (item) {
            item->setHoverActive(false);
            item->setDropActive(false);
        }
    }
}

bool DragDropHandler::canDropOnBoard(Unit* unit, const QPoint& source,
                                     const QPoint& target, bool allowSwap,
                                     bool canDrag) const
{
    if (!canDrag || !unit || unit->owner() != Controller::PlayerCtrl) {
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
        int onBoard = 0;
        for (int row = Board::ROWS / 2; row < Board::ROWS; ++row) {
            for (int col = 0; col < Board::COLS; ++col) {
                Unit* u = m_board.getUnitAt(QPoint(col, row));
                if (u && u->owner() == Controller::PlayerCtrl && u->isAlive()) {
                    ++onBoard;
                }
            }
        }
        if (onBoard >= m_player.populationCap()) {
            return false;
        }
        return true;
    }

    return m_board.isValidPosition(source) && m_board.isPlayerHalf(source)
           && m_board.getUnitAt(source) == unit && source != target;
}

bool DragDropHandler::canDropOnBench(Unit* unit, int slot, bool allowSwap,
                                     bool canDrag) const
{
    if (!canDrag || !unit || unit->owner() != Controller::PlayerCtrl
        || !m_player.bench()->isSlotValid(slot)) {
        return false;
    }

    Unit* occupant = m_player.bench()->getUnitAt(slot);
    if (occupant) {
        return allowSwap && occupant != unit;
    }
    return true;
}

void DragDropHandler::applyBoardDrop(Unit* unit, const QPoint& source,
                                     const QPoint& target)
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

void DragDropHandler::applyBenchDrop(Unit* unit, int sourceBenchSlot,
                                     int targetBenchSlot)
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
