#ifndef CORE_DRAGDROPHANDLER_H
#define CORE_DRAGDROPHANDLER_H

#include "core/board.h"
#include "core/player.h"
#include <QPoint>
#include <QPointF>
#include <vector>

class Unit;
class GridItem;
class BenchSlotItem;
class QGraphicsScene;

class DragDropHandler
{
public:
    DragDropHandler(Board& board, Player& player);

    bool isActive() const { return m_dragActive; }
    int activeUnitId() const { return m_activeUnitId; }
    int activeBenchSlot() const { return m_activeBenchSlot; }
    QPoint sourceGrid() const { return m_sourceGrid; }

    void beginDrag(int unitId, int benchSlot, const QPoint& sourceGrid);
    void endDrag();

    GridItem* findGridItem(const std::vector<GridItem*>& items, const QPoint& gridPos) const;
    BenchSlotItem* findBenchSlot(const std::vector<BenchSlotItem*>& items, int slot) const;

    void clearGridHighlights(std::vector<GridItem*>& items) const;
    void clearBenchHighlights(std::vector<BenchSlotItem*>& items) const;

    bool canDropOnBoard(Unit* unit, const QPoint& source, const QPoint& target,
                        bool allowSwap, bool canDrag) const;
    bool canDropOnBench(Unit* unit, int slot, bool allowSwap, bool canDrag) const;

    void applyBoardDrop(Unit* unit, const QPoint& source, const QPoint& target);
    void applyBenchDrop(Unit* unit, int sourceBenchSlot, int targetBenchSlot);

private:
    Board& m_board;
    Player& m_player;

    bool m_dragActive;
    int m_activeUnitId;
    int m_activeBenchSlot;
    QPoint m_sourceGrid;
};

#endif // CORE_DRAGDROPHANDLER_H
